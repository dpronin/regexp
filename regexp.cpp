#include <cerrno>
#include <cstdint>
#include <cstring>

#include <iostream>

#include "regexplib.hpp"

int main(int argc, char const* argv[])
{
    if (argc < 2) {
        std::cerr << "no pattern given\n";
        return EINVAL;
    }

    for (std::string line; std::cin >> line;) {
        if (regexp::does_match(line, argv[1]))
            std::cout << line << std::endl;
    }

    return 0;
}
