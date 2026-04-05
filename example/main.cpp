// QObjectLeakTracker example
//
// Demonstrates how the leak tracker detects QObjects that are not deleted
// before the application exits.
//
// For details, run this program with debug logging enabled:
//      QT_LOGGING_RULES="LeakTracker.debug=true" ./LeakTrackerExample

#include <QCoreApplication>
#include <QDebug>

#include "QObjectLeakTracker.h"

// Minimal QObject subclass example #1
class DatabaseConnection : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseConnection(QObject *parent = nullptr) : QObject(parent) {}
};

// Minimal QObject subclass example #2
class HttpRequest : public QObject
{
    Q_OBJECT
public:
    explicit HttpRequest(QObject *parent = nullptr) : QObject(parent) {}
};

int main(int argc, char *argv[])
{
    // Install leak tracker before creating QObjects.
    QObjectLeakTracker::install();

    // Filter on our own classes (see above definitions.)
    QObjectLeakTracker::addPrefix("Database");
    QObjectLeakTracker::addPrefix("Http");

    // Place the application in a scope
    {
        QCoreApplication app(argc, argv);

        qDebug() << "Creating objects...";

        // This object will be deleted with the app via the QObject hierarchy.
        // (Note how we pass the app object to the constructo here.)
        auto *request = new HttpRequest(&app);
        Q_UNUSED(request);
        qDebug() << "Created HttpRequest (parented to app)";

        // This object is leaked (no parent, never deleted).
        auto *leak = new DatabaseConnection();
        Q_UNUSED(leak);
        qDebug() << "Created DatabaseConnection (no parent, will leak)";

        // Print out state before exiting scope, which will destroy "app."
        qDebug() << Qt::endl
                 << "Remaining objects before exiting scope:"
                 << QObjectLeakTracker::remainingCount()
                 << Qt::endl;
    }

    // Demonstrates a leak: our HttpRequest object was deleted but the
    // DatabaseConnection was not.
    QObjectLeakTracker::printRemaining();

    return 0;
}

#include "main.moc"
