
#include <stdio.h>

#include "fs/path.hpp"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        puts("no input directory");
        return 1;
    }

    /*
    fs::path pth(argv[1]);

    if (!fs::is_directory(&pth))
    {
        printf("%s is not a directory\n", argv[1]);
        return 1;
    }

    printf("contents of directory %s:\n", argv[1]);

    iterate_path(p, &pth, true)
        printf("[%s]\n", p->c_str());

    */

    return 0;
}
