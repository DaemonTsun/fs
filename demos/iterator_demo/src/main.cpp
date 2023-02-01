
#include <stdio.h>

#include "fs/path.hpp"
#include "fs/iterator.hpp"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        puts("no input directory");
        return 1;
    }

    fs::path pth(argv[1]);

    if (!fs::is_directory(&pth))
    {
        printf("%s is not a directory\n", argv[1]);
        return 1;
    }

    printf("contents of directory %s:\n", pth.c_str());

    for (const fs::path *p : fs::iterate(&pth))
        printf("[%s]\n", p->c_str());

    return 0;
}
