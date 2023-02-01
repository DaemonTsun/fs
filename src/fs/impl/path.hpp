
#pragma once

#include <filesystem>

#include "fs/path.hpp"

struct fs::path::_path
{
    std::filesystem::path data;
};
