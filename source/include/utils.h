#ifndef UTILS_H
#define UTILS_H


#include <sstream>
#include <algorithm>
#include <iterator>

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

// If __PRESERVE_STRS__ defined H_ is a function that hashes strings
// else it's a dummy macro that leaves the string intact.
#ifdef __PRESERVE_STRS__
    #define H_(X) (std::string(X))
    typedef std::string hash_t;
#else
    typedef unsigned long long hash_t;
    // compile-time hash
    extern constexpr hash_t H_(const char* str)
    {
        return detail::hash_one(str[0], str + 1, detail::basis);
    }
#endif

typedef unsigned long long hashstr_t;
// compile-time hash
extern constexpr hashstr_t HS_(const char* str)
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

#endif
