#ifndef UTILS_H
#define UTILS_H


#include <sstream>
#include <algorithm>
#include <iterator>

namespace wcore
{

// ----- constexpr string hash facilities -----
// NOTE: hashing algorithm used is FNV-1a

namespace detail
{
    // FNV-1a constants
    static constexpr unsigned long long basis = 14695981039346656037ULL;
    static constexpr unsigned long long prime = 1099511628211ULL;

    // compile-time hash helper function
    extern constexpr unsigned long long hash_one(char c, const char* remain, unsigned long long value)
    {
        return c == 0 ? value : hash_one(remain[0], remain + 1, (value ^ c) * prime);
    }
}


typedef unsigned long long hash_t;
// compile-time hash
extern constexpr hash_t H_(const char* str)
{
    return detail::hash_one(str[0], str + 1, detail::basis);
}

template <class Container>
void split_string(const std::string& str, Container& cont, char delim = ' ')
{
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delim)) {
        cont.push_back(token);
    }
}

}

#ifndef __IS_TOOL__
#include "include/intern_string.h" // for HRESOLVE
#endif

#endif
