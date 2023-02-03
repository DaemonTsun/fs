# fs
filesystem library, mostly a wrapper around std::filesystem with additional features and a better API.

## features
- `fs::path` with little to no header dependencies
- `fs::iterator to iterate directories
- `fs::filesystem_watcher` to watch over directories and files. requires pthread

## building

make sure git submodules are properly initialized with `git submodule update --init --recursive`, then run the following:

```sh
$ mkdir bin
$ cd bin
$ cmake ..
$ make
```

## installation

`sudo make install` inside the build directory.
