
#include <stdio.h> // getchar
#include "shl/print.hpp"
#include "fs/filesystem_watcher.hpp"

void callback(fs::const_fs_string path, fs::watcher_event_type event)
{
    tprint("% %\n", value(event), path);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        tprint("error: expecting command line argument files to watch\n");
        return 1;
    }

    fs::filesystem_watcher *watcher;
    error err{};
    watcher = fs::filesystem_watcher_create(callback, &err);

    if (err.error_code != 0)
    {
        tprint("error: %\n", err.what);
        return 1;
    }

    for (int i = 1; i < argc; ++i)
        fs::filesystem_watcher_watch_file(watcher, to_const_string(argv[i]));

    char c = '\0';

    tprint("Watcher started. Press 'u' to unwatch all files, 'w' to watch them again, 'q' to quit\n"
           "or press any other key to display new events.");

    while (true)
    {
        c = getchar();

        if (c == 'q' || c < 0)
            break;

        // (un)watching the same file(s) multiple times does nothing
        if (c == 'w')
        {
            for (int i = 1; i < argc; ++i)
            if (!fs::filesystem_watcher_watch_file(watcher, to_const_string(argv[i]), &err))
            {
                tprint("error: %\n", err.what);
                break;
            }

            continue;
        }

        if (c == 'u')
        {
            for (int i = 1; i < argc; ++i)
            if (!fs::filesystem_watcher_unwatch_file(watcher, to_const_string(argv[i]), &err))
            {
                tprint("error: %\n", err.what);
                break;
            }

            continue;
        }

        if (fs::filesystem_watcher_has_events(watcher, &err))
            fs::filesystem_watcher_process_events(watcher, &err);

        if (err.error_code != 0)
        {
            tprint("error: %\n", err.what);
            break;
        }
    }

    fs::filesystem_watcher_destroy(watcher, &err);

    if (err.error_code != 0)
    {
        tprint("error: %\n", err.what);
        return 1;
    }

    return 0;
}
