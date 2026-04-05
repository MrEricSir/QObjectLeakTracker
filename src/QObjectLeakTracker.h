// MIT License

// Copyright (c) 2026 Eric Gregory <mrericsir@gmail.com>

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef QOBJECTLEAKTRACKER_H
#define QOBJECTLEAKTRACKER_H

#include <QMap>
#include <QString>
#include <QtGlobal>

#ifdef Q_QDOC

/*!
    \module QObjectLeakTracker
    \title QObjectLeakTracker C++ Classes
    \brief Debug-only leak tracker for QObject-derived classes.
*/

/*!
    \class QObjectLeakTracker
    \inmodule QObjectLeakTracker
    \brief The QObjectLeakTracker class tracks leaked QObjects in Qt applications.
    shutdown.

    Object tracking is performed via Qt's private API and may not be stable
    between versions.

    Only used for debug builds. In release builds, all functions are no-ops.

    \section1 Basic Usage

    Call install() in \c main() before creating any QObjects:

    \code
    int main(int argc, char *argv[])
    {
        QObjectLeakTracker::install();

        //
        // Your application code goes here.
        //

        QObjectLeakTracker::printRemaining();
        return 0;
    }
    \endcode

    \section1 Prefix whitelist

    By default, printRemaining() reports all QObjects, including Qt internals
    (QNetworkReply, QTimer, etc.) Add prefixes to limit reports to your own
    classes:

    \code
    QObjectLeakTracker::addPrefix("MyApp");
    QObjectLeakTracker::addPrefix("MyObjectType");
    \endcode

    Only classes whose names start with a registered prefix will appear in
    printRemaining(), snapshot(), and remainingCount().

    \section1 Scope Helper

    Use the convenience macros to assert that a code block does not allocate
    any QObjects:

    \code
    QLEAK_NO_ALLOC_SCOPE();
    // ... code that should not create QObjects ...
    QLEAK_ASSERT_NO_ALLOC();
    \endcode

    \sa QObjectLeakTracker::Scope
*/
class QObjectLeakTracker
{
public:

    /*!
        Installs the leak tracker by hooking into Qt's private API.

        Call this at the very beginning of \c main(), before constructing a
        QApplication or any other QObject.
    */
    static void install();

    /*!
        Prints a summary of all remaining (not yet destroyed) QObjects.

        The output includes a count of remaining objects and their type,
        depending on the log verbosity level.

        Output depends on whitelist.

        \sa snapshot(), remainingCount(), addPrefix()
    */
    static void printRemaining();

    /*!
        Returns the number of tracked QObjects that have not been destroyed.

        Output depends on whitelist and build type (returns 0 in release
        builds.)

        \sa snapshot(), printRemaining()
    */
    static int remainingCount();

    /*!
        Returns a map from class name to instance count for all QObjects that
        have not been destroyed.

        Output depends on whitelist.

        \sa remainingCount(), printRemaining()
    */
    static QMap<QString, int> snapshot();

    /*!
        Registers \a prefix for class name whitelisting used in reporting.

        When at least one prefix has been registered, printRemaining(),
        snapshot(), and remainingCount() will only consider objects whose
        \c{metaObject()->className()} starts with one of the registered
        prefixes.

        By default, the whitelist is empty and all QObject-derived classes
        will be reported, including Qt internal classes.

        \code
        QObjectLeakTracker::addPrefix("MyApp");
        \endcode

        \sa printRemaining(), snapshot(), remainingCount()
    */
    static void addPrefix(const QString &prefix);

    /*!
        Returns the total number of QObjects created. Note that this number
        does not take the whitelist into account.

        \sa totalDestroyed()
    */
    static qint64 totalCreated();

    /*!
        Returns the total number of QObjects destroyed. Note that this number
        does not take the whitelist into account.

        \sa totalCreated()
    */
    static qint64 totalDestroyed();

    /*!
        \class QObjectLeakTracker::Scope
        \inmodule QObjectLeakTracker
        \brief Records QObject creation and destruction counts within a
        code block.

        Construct a Scope on the stack to capture a baseline. Then call
        created() and destroyed() to find out how many QObjects were
        allocated or freed since the Scope was constructed.

        \code
        {
            QObjectLeakTracker::Scope scope;
            auto *obj = new QObject;
            // scope.created() == 1
            delete obj;
            // scope.destroyed() == 1
        }
        \endcode

        \sa QObjectLeakTracker, QLEAK_NO_ALLOC_SCOPE(), QLEAK_ASSERT_NO_ALLOC()
    */
    class Scope
    {
    public:
        /*!
            Constructs a Scope, capturing the current cumulative created and
            destroyed counts as a baseline.
        */
        Scope();

        /*!
            Returns the number of QObjects created since this Scope was
            constructed.
        */
        qint64 created() const;

        /*!
            Returns the number of QObjects destroyed since this Scope was
            constructed.
        */
        qint64 destroyed() const;
    };
};

/*!
    \macro QLEAK_NO_ALLOC_SCOPE()
    \relates QObjectLeakTracker

    Declares a QObjectLeakTracker::Scope variable for the current block.
    Use together with QLEAK_ASSERT_NO_ALLOC() to verify that no QObjects
    are created within the block.

    \sa QLEAK_ASSERT_NO_ALLOC(), QObjectLeakTracker::Scope
*/

/*!
    \macro QLEAK_ASSERT_NO_ALLOC()
    \relates QObjectLeakTracker

    Asserts that no QObjects have been created since the most recent
    QLEAK_NO_ALLOC_SCOPE() in the same block. Triggers \c Q_ASSERT on
    failure.

    \sa QLEAK_NO_ALLOC_SCOPE(), QObjectLeakTracker::Scope
*/

#elif defined(QT_DEBUG) // Debug builds

class QObjectLeakTracker
{
public:
    static void install();
    static void printRemaining();
    static int remainingCount();
    static QMap<QString, int> snapshot();
    static void addPrefix(const QString &prefix);
    static qint64 totalCreated();
    static qint64 totalDestroyed();

    class Scope
    {
    public:
        Scope();
        qint64 created() const;
        qint64 destroyed() const;

    private:
        qint64 createdAtStart;
        qint64 destroyedAtStart;
    };
};

#define QLEAK_NO_ALLOC_SCOPE() QObjectLeakTracker::Scope _leakTrackerAllocScope
#define QLEAK_ASSERT_NO_ALLOC() Q_ASSERT(_leakTrackerAllocScope.created() == 0)

#else // Release builds

class QObjectLeakTracker
{
public:
    static inline void install() {}
    static inline void printRemaining() {}
    static inline int remainingCount() { return 0; }
    static inline QMap<QString, int> snapshot() { return {}; }
    static inline void addPrefix(const QString &) {}
    static inline qint64 totalCreated() { return 0; }
    static inline qint64 totalDestroyed() { return 0; }

    class Scope
    {
    public:
        Scope() = default;
        qint64 created() const { return 0; }
        qint64 destroyed() const { return 0; }
    };
};

#define QLEAK_NO_ALLOC_SCOPE() do {} while (0)
#define QLEAK_ASSERT_NO_ALLOC() do {} while (0)

#endif // QT_DEBUG

#endif // QOBJECTLEAKTRACKER_H
