#ifndef KEYMAP_H
#define KEYMAP_H

#include <map>
#include <string>
#include <cstdint>

#include "utils.h"

namespace keymap
{

typedef std::map<hash_t, uint16_t> KeyNamesMap;

extern KeyNamesMap NAMES;

}


#endif // KEYMAP_H
