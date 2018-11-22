#ifndef ENDIAN_H
#define ENDIAN_H

#include <climits>
#include <utility>
#include <type_traits>
#include <cstdint>

// compile-time endianness swap based on http://stackoverflow.com/a/36937049
// usage:
// bswap<std::uint16_t>(0x1234u)                 ->   0x3412u
// bswap<std::uint64_t>(0x0123456789abcdefULL)   ->   0xefcdab8967452301ULL
namespace detail
{
template<typename T, std::size_t... N>
static constexpr T bswap_impl(T i, std::index_sequence<N...>)
{
    return (((i >> N*CHAR_BIT & std::uint8_t(-1)) << (sizeof(T)-1-N)*CHAR_BIT) | ...);
}
}

template<typename T, typename U = std::make_unsigned_t<T>>
static constexpr U bswap(T i)
{
    return detail::bswap_impl<U>(i, std::make_index_sequence<sizeof(T)>{});
}


#endif // ENDIAN_H
