# QObjectLeakTracker

A debug-only QObject leak detector for Qt 6 applications. Logs remaining objects when needed, for example at application shutdown. Useful for detecting certain types of leaks during development.

Originally developed as part of [Fang](https://github.com/MrEricSir/Fang)

## Features

- Tracks QObject creation/destruction with minimal code changes
- Logs objects still in memory class name
- Whitelist by prefix to filter on specific class names
- Snapshot API for unit tests
- Does not interfere with release builds

## Quick Start

Install the tracker early in `main()`, before creating any QObject-derived classes. Print the remaining objects before exiting.

Build the application in debug mode and run it to see the output.

```cpp
#include "QObjectLeakTracker.h"

int main(int argc, char *argv[])
{
    QObjectLeakTracker::install();

    //
    // Your application code goes here.
    //

    QObjectLeakTracker::printRemaining();
    return 0;
}
```

### Prefix Whitelist

By default, `printRemaining()` reports all QObjects, including Qt internals (QNetworkReply, QTimer, etc.) Add prefixes to limit reports to your own classes:

```cpp
QObjectLeakTracker::install();
QObjectLeakTracker::addPrefix("MyApp");
QObjectLeakTracker::addPrefix("MyObjectType");
```

Now only classes whose names start with "MyApp" or "MyObjectType" will appear in the report.

### Scope Helper

Assert that a code block does not allocate any QObjects. 

Example:

```cpp
QLEAK_NO_ALLOC_SCOPE();
// Code that should not create QObjects.
QLEAK_ASSERT_NO_ALLOC();
```

### Snapshot API

Inspect the tracking state programmatically. Uuseful in unit tests.

```cpp
QMap<QString, int> snap = QObjectLeakTracker::snapshot();
```

## How To Include

Add QObjectLeakTracker as a git submodule:

```bash
git submodule add https://github.com/MrEricSir/QObjectLeakTracker.git external/QObjectLeakTracker
git add .gitmodules
git commit -m "Add QObjectLeakTracker submodule"
```

Add it to your `CMakeLists.txt`:

```cmake
add_subdirectory(external/QObjectLeakTracker)
target_link_libraries(YourApp PRIVATE QObjectLeakTracker)
```

## Requirements

- Qt 6 (Core module, CorePrivate for hook access)
- C++17
- CMake 3.16+

## License

MIT License. See [LICENSE](LICENSE) for details.
