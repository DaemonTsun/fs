# fs
A replacement library for the standard filesystem library, because STDs are no good. Currently all features are in the [`src/fs/path.hpp`](src/fs/path.hpp) header, so only this header needs to be included.

## Features

- [`fs::path`](src/fs/path.hpp): lightweight struct to represent paths.
- `fs::filename(path)`, `fs::append_path(path, string)`, ...: Path functions to obtain information about paths or modify paths.
- `fs::get_home_path(*out)`, `fs::get_executable_path(*out)`, ...: Functions to obtain special filesystem paths.
- `fs::copy(From, To, Options)`, `fs::create_directory(...)`, `fs::touch(Path)`: Filesystem manipulation functions.
- `for_path(it, path)`, `for_recursive_path(it, path)`: no-nonsense filesystem iterators.

See [`path.hpp`](src/fs/path.hpp) for details and documentation.

### Planned Features

- `filesystem_watcher`: broken as of version 0.8

## Tree Demo

This example code is a complete [`tree`](https://en.wikipedia.org/wiki/Tree_(command)) program, recursively listing the contents of the given directory (Code from [`demos/tree_demo/src/main.cpp`](demos/tree_demo/src/main.cpp)).

```cpp
#include "shl/print.hpp"
#include "shl/defer.hpp"
#include "fs/path.hpp"

int main(int argc, char **argv)
{
    fs::path target{};
    defer { fs::free(&target); };

    if (argc < 2)
        fs::set_path(&target, ".");
    else
        fs::set_path(&target, argv[1]);

    u64 file_count = 0;
    u64 dir_count  = 0;

    for_recursive_path(it, target)
    {
        for (u32 i = 0; i < it->depth; ++i)
            put("  ");

        tprint("%\n", fs::filename(it->path));

        if (it->type == fs::filesystem_type::File)
            file_count += 1;
        else if (it->type == fs::filesystem_type::Directory)
            dir_count  += 1;
    }

    tprint("\n% directories, % files\n", dir_count, file_count);

    return 0;
}
```

## Usage
Either clone the repostory and simply include it in CMake like so:

```cmake
add_subdirectory(path/to/fs)
target_link_libraries(your-target PRIVATE fs-0.8)
target_include_directories(your-target PRIVATE ${fs-0.8_SOURCES_DIR})
```
OR follow the next steps to build and install it and manually link and include it.

### Building (optional)

If fs is not included in CMake and should be installed instead, run the following from the root of the repository:

```sh
$ cmake <dir with CMakeLists.txt> -B <output dir>
$ cmake --build <output dir>
```

### Installation (optional)

`cmake --install <output dir>` inside the build directory.

## Tests (optional)

Tests can be built by specifying the `-DTests=1` command line flag in the `cmake` command and the tests can be run using `cmake --build <output dir> --target runtests` or `ctest --test-dir <output dir>`.

