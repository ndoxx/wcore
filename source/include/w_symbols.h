#ifndef W_SYMBOLS_H
#define W_SYMBOLS_H

#include <cstdint>

namespace wcore
{

enum class ORDER: uint32_t
{
    IRRELEVANT,
    FRONT_TO_BACK,
    BACK_TO_FRONT
};

enum class MODEL_CATEGORY: uint32_t
{
    IRRELEVANT,
    OPAQUE,
    TRANSPARENT
};

enum class NEIGHBOR: uint32_t
{
    WEST,
    NORTH,
    EAST,
    SOUTH
};

}

#endif // W_SYMBOLS_H
