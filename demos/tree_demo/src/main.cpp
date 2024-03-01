
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
