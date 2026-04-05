#include "QObjectLeakTracker.h"

#ifdef QT_DEBUG

#include <private/qhooks_p.h>

#include <QObject>
#include <QSet>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QMap>
#include <QByteArrayView>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logLeakTracker, "LeakTracker")

// These are intentionally leaked as part of the detection process. They
// must outlive all QObjects in the application.
static QMutex &trackerMutex()
{
    static QMutex *mutex = new QMutex;
    return *mutex;
}

static QSet<QObject *> &trackedObjects()
{
    static QSet<QObject *> *set = new QSet<QObject *>;
    return *set;
}

static QSet<QByteArray> &prefixWhitelist()
{
    static QSet<QByteArray> *set = new QSet<QByteArray>;
    return *set;
}

// className pointers from metaObject() are unique to the class metaobject,
// so we can cache the lookup result keyed on the pointer rather than the string.
static QHash<const char *, bool> &whitelistCache()
{
    static QHash<const char *, bool> *cache = new QHash<const char *, bool>;
    return *cache;
}

static bool matchesWhitelist(const char *className)
{
    if (prefixWhitelist().isEmpty()) {
        return true;
    }

    auto it = whitelistCache().constFind(className);
    if (it != whitelistCache().constEnd()) {
        return it.value();
    }

    bool matched = false;
    const QByteArrayView name(className);
    for (const QByteArray &prefix : prefixWhitelist()) {
        if (name.startsWith(prefix)) {
            matched = true;
            break;
        }
    }
    whitelistCache().insert(className, matched);
    return matched;
}

static std::atomic<qint64> cumulativeCreated{0};
static std::atomic<qint64> cumulativeDestroyed{0};

static void onAddQObject(QObject *object)
{
    QMutexLocker locker(&trackerMutex());
    trackedObjects().insert(object);
    cumulativeCreated.fetch_add(1, std::memory_order_relaxed);
}

static void onRemoveQObject(QObject *object)
{
    QMutexLocker locker(&trackerMutex());
    trackedObjects().remove(object);
    cumulativeDestroyed.fetch_add(1, std::memory_order_relaxed);
}

void QObjectLeakTracker::install()
{
    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&onAddQObject);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&onRemoveQObject);
}

void QObjectLeakTracker::addPrefix(const QString &prefix)
{
    prefixWhitelist().insert(prefix.toLatin1());
    whitelistCache().clear();
}

void QObjectLeakTracker::printRemaining()
{
    QMutexLocker locker(&trackerMutex());

    QMap<QString, int> instanceCount;

    for (QObject *object : trackedObjects()) {
        const char *className = object->metaObject()->className();
        if (matchesWhitelist(className)) {
            instanceCount[className]++;
        }
    }

    int filteredCount = 0;
    for (int count : instanceCount) {
        filteredCount += count;
    }

    qCInfo(logLeakTracker);
    qCInfo(logLeakTracker) << "========== QObject leak check ==========";
    qCInfo(logLeakTracker) << "Remaining objects:" << filteredCount;
    qCInfo(logLeakTracker);

    for (const QString &className : instanceCount.keys()) {
        qCDebug(logLeakTracker) << QString("%1 : %2")
                               .arg(instanceCount[className], 4)
                               .arg(className);
    }

    qCInfo(logLeakTracker) << "=========================================";
    qCInfo(logLeakTracker);
}

int QObjectLeakTracker::remainingCount()
{
    QMutexLocker locker(&trackerMutex());

    if (prefixWhitelist().isEmpty()) {
        return trackedObjects().size();
    }

    int count = 0;
    for (QObject *object : trackedObjects()) {
        if (matchesWhitelist(object->metaObject()->className())) {
            count++;
        }
    }
    return count;
}

QMap<QString, int> QObjectLeakTracker::snapshot()
{
    QMutexLocker locker(&trackerMutex());
    QMap<QString, int> result;

    for (QObject *object : trackedObjects()) {
        const char *className = object->metaObject()->className();
        if (matchesWhitelist(className)) {
            result[className]++;
        }
    }

    return result;
}

qint64 QObjectLeakTracker::totalCreated()
{
    return cumulativeCreated.load(std::memory_order_relaxed);
}

qint64 QObjectLeakTracker::totalDestroyed()
{
    return cumulativeDestroyed.load(std::memory_order_relaxed);
}

QObjectLeakTracker::Scope::Scope()
    : createdAtStart(cumulativeCreated.load(std::memory_order_relaxed))
    , destroyedAtStart(cumulativeDestroyed.load(std::memory_order_relaxed))
{
}

qint64 QObjectLeakTracker::Scope::created() const
{
    return cumulativeCreated.load(std::memory_order_relaxed) - createdAtStart;
}

qint64 QObjectLeakTracker::Scope::destroyed() const
{
    return cumulativeDestroyed.load(std::memory_order_relaxed) - destroyedAtStart;
}

#endif // QT_DEBUG
