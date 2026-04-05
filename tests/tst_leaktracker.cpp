#include <QtTest>
#include <QDebug>
#include "QObjectLeakTracker.h"

// Used to capture log output.
static QStringList capturedMessages;
static QtMessageHandler originalHandler = nullptr;

void testMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);
    capturedMessages.append(msg);
}

class TestHelper : public QObject
{
    Q_OBJECT
public:
    explicit TestHelper(QObject *parent = nullptr) : QObject(parent) {}
};

class TestLeakTracker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Core tracking tests
    void testInstallAndTrack();
    void testStackAllocation();
    void testParentChildCascade();
    void testMultiLevelHierarchy();

    // Snapshot and reporting
    void testSnapshot();
    void testPrintRemaining_empty();
    void testPrintRemaining_withObjects();
    void testPrintRemaining_showsClassNames();

    // Scope tests
    void testScope_created();
    void testScope_destroyed();
    void testScope_noAlloc();

    // Whitelist tests
    void testWhitelistFilters();
};

void TestLeakTracker::initTestCase()
{
#ifdef QT_DEBUG
    QObjectLeakTracker::install();
#endif
}

void TestLeakTracker::testInstallAndTrack()
{
#ifdef QT_DEBUG
    int initialCount = QObjectLeakTracker::remainingCount();

    QObject* obj = new QObject();
    QCOMPARE(QObjectLeakTracker::remainingCount(), initialCount + 1);

    delete obj;
    QCOMPARE(QObjectLeakTracker::remainingCount(), initialCount);
#endif
}

void TestLeakTracker::testStackAllocation()
{
#ifdef QT_DEBUG
    int initialCount = QObjectLeakTracker::remainingCount();

    {
        QObject stackObj;
        QVERIFY(stackObj.parent() == nullptr);
        QCOMPARE(QObjectLeakTracker::remainingCount(), initialCount + 1);
    }

    QCOMPARE(QObjectLeakTracker::remainingCount(), initialCount);
#endif
}

void TestLeakTracker::testParentChildCascade()
{
#ifdef QT_DEBUG
    int initialCount = QObjectLeakTracker::remainingCount();

    QObject* parent = new QObject();
    QObject* child1 = new QObject(parent);
    QObject* child2 = new QObject(parent);
    QObject* child3 = new QObject(parent);

    Q_UNUSED(child1);
    Q_UNUSED(child2);
    Q_UNUSED(child3);

    QCOMPARE(QObjectLeakTracker::remainingCount(), initialCount + 4);

    delete parent;

    QCOMPARE(QObjectLeakTracker::remainingCount(), initialCount);
#endif
}

void TestLeakTracker::testMultiLevelHierarchy()
{
#ifdef QT_DEBUG
    int initialCount = QObjectLeakTracker::remainingCount();

    QObject* parent = new QObject();
    QObject* child1 = new QObject(parent);
    QObject* child2 = new QObject(parent);
    QObject* grandchild1 = new QObject(child1);
    QObject* grandchild2 = new QObject(child1);
    QObject* grandchild3 = new QObject(child2);

    Q_UNUSED(grandchild1);
    Q_UNUSED(grandchild2);
    Q_UNUSED(grandchild3);

    QCOMPARE(QObjectLeakTracker::remainingCount(), initialCount + 6);

    delete parent;

    QCOMPARE(QObjectLeakTracker::remainingCount(), initialCount);
#endif
}

void TestLeakTracker::testSnapshot()
{
#ifdef QT_DEBUG
    // Create objects of known type
    QObject* obj1 = new QObject();
    QObject* obj2 = new QObject();

    QMap<QString, int> snap = QObjectLeakTracker::snapshot();
    QVERIFY(snap.contains("QObject"));
    QVERIFY(snap["QObject"] >= 2);

    delete obj1;
    delete obj2;
#endif
}

void TestLeakTracker::testPrintRemaining_empty()
{
#ifdef QT_DEBUG
    int initialCount = QObjectLeakTracker::remainingCount();

    capturedMessages.clear();
    originalHandler = qInstallMessageHandler(testMessageHandler);

    QObjectLeakTracker::printRemaining();

    qInstallMessageHandler(originalHandler);

    bool foundHeader = false;
    bool foundCount = false;
    for (const QString& msg : capturedMessages) {
        if (msg.contains("QObject leak check")) {
            foundHeader = true;
        }
        if (msg.contains("Remaining objects:")) {
            foundCount = true;
            QVERIFY(msg.contains(QString::number(initialCount)));
        }
    }

    QVERIFY(foundHeader);
    QVERIFY(foundCount);
#endif
}

void TestLeakTracker::testPrintRemaining_withObjects()
{
#ifdef QT_DEBUG
    int initialCount = QObjectLeakTracker::remainingCount();

    QObject* obj1 = new QObject();
    QObject* obj2 = new QObject();

    capturedMessages.clear();
    originalHandler = qInstallMessageHandler(testMessageHandler);

    QObjectLeakTracker::printRemaining();

    qInstallMessageHandler(originalHandler);

    bool foundCorrectCount = false;
    for (const QString& msg : capturedMessages) {
        if (msg.contains("Remaining objects:") &&
            msg.contains(QString::number(initialCount + 2))) {
            foundCorrectCount = true;
        }
    }

    QVERIFY(foundCorrectCount);

    delete obj1;
    delete obj2;
#endif
}

void TestLeakTracker::testPrintRemaining_showsClassNames()
{
#ifdef QT_DEBUG
    QObject* obj = new QObject();

    capturedMessages.clear();
    originalHandler = qInstallMessageHandler(testMessageHandler);

    QObjectLeakTracker::printRemaining();

    qInstallMessageHandler(originalHandler);

    bool foundQObject = false;
    for (const QString& msg : capturedMessages) {
        if (msg.contains("QObject")) {
            foundQObject = true;
        }
    }

    QVERIFY(foundQObject);

    delete obj;
#endif
}

void TestLeakTracker::testScope_created()
{
#ifdef QT_DEBUG
    QObjectLeakTracker::Scope scope;

    QObject* obj1 = new QObject();
    QObject* obj2 = new QObject();

    QCOMPARE(scope.created(), 2);

    delete obj1;
    delete obj2;
#endif
}

void TestLeakTracker::testScope_destroyed()
{
#ifdef QT_DEBUG
    QObject* obj1 = new QObject();
    QObject* obj2 = new QObject();

    QObjectLeakTracker::Scope scope;

    delete obj1;
    delete obj2;

    QCOMPARE(scope.destroyed(), 2);
#endif
}

void TestLeakTracker::testScope_noAlloc()
{
#ifdef QT_DEBUG
    QObjectLeakTracker::Scope scope;

    // Do nothing - no allocations

    QCOMPARE(scope.created(), 0);
    QCOMPARE(scope.destroyed(), 0);
#endif
}

void TestLeakTracker::testWhitelistFilters()
{
#ifdef QT_DEBUG
    // Register "Test" prefix - only TestHelper should pass the filter.
    QObjectLeakTracker::addPrefix("Test");

    QObject *plain = new QObject();
    TestHelper *helper = new TestHelper();

    // remainingCount should only count the TestHelper (filtered).
    QCOMPARE(QObjectLeakTracker::remainingCount(), 1);

    // snapshot should contain TestHelper but not QObject.
    QMap<QString, int> snap = QObjectLeakTracker::snapshot();
    QVERIFY(snap.contains("TestHelper"));
    QVERIFY(!snap.contains("QObject"));

    delete helper;
    delete plain;
#endif
}

QTEST_MAIN(TestLeakTracker)
#include "tst_leaktracker.moc"
