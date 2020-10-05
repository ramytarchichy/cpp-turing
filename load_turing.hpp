#pragma once

#include "turing.hpp"
#include <string>

namespace turing
{
    machine<std::string, char> import_txt(const std::string& text);
}
