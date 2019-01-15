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


    inline void hash_combine(std::size_t& seed) { }

    template <typename T, typename... Rest>
    inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        hash_combine(seed, rest...);
    }
}

#define MAKE_HASHABLE(type, ...) \
    namespace std {\
        template<> struct hash<type> {\
            std::size_t operator()(const type &t) const {\
                std::size_t ret = 0;\
                wcore::detail::hash_combine(ret, __VA_ARGS__);\
                return ret;\
            }\
        };\
    }

typedef unsigned long long hash_t;
// compile-time hash
extern constexpr hash_t H_(const char* str)
{
    return detail::hash_one(str[0], str + 1, detail::basis);
}

inline hash_t HCOMBINE_(hash_t first, hash_t second)
{
    return (first ^ second) * detail::prime;
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
