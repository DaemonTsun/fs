
#include <stdio.h>
#include "shl/error.hpp"
// #include "fs/filesystem_watcher.hpp"

/*
void callback(const char *path, fs::watcher_event_type event)
{
    printf("%d %s\n", value(event), path);
}
*/

int main(int argc, char **argv)
{
    return 0;
}
#if 0
try
{
    fs::filesystem_watcher *watcher;
    fs::create_filesystem_watcher(&watcher, callback);

    for (int i = 1; i < argc; ++i)
        fs::watch_file(watcher, argv[i]);

    fs::start_filesystem_watcher(watcher);

    char c = '\0';

    while (true)
    {
        c = getchar();

        if (c == 'q' || c < 0)
            break;

        // (un)watching the same file(s) multiple times does nothing
        if (c == 'w')
            for (int i = 1; i < argc; ++i)
                fs::watch_file(watcher, argv[i]);

        if (c == 'u')
            for (int i = 1; i < argc; ++i)
                fs::unwatch_file(watcher, argv[i]);
    }

    fs::destroy_filesystem_watcher(watcher);

    return 0;
}
catch (error &e)
{
    fprintf(stderr, "%s\n", e.what);
}
#endif
