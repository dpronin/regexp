#include <cerrno>

#include "regexplib.hpp"

int main(int argc, char const* argv[])
{
    if (argc < 3)
        return EINVAL;
    return regexp::does_match(argv[1], argv[2]) ? 0 : 1;
}
