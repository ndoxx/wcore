#ifndef KEYMAP_H
#define KEYMAP_H

#include <map>
#include <string>
#include <cstdint>

#include "wtypes.h"

namespace wcore
{
namespace keymap
{

typedef std::map<hash_t, uint16_t> KeyNamesMap;

extern KeyNamesMap NAMES;

} // namespace keymap
} // namespace wcore

#endif // KEYMAP_H
