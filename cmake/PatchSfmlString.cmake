if(NOT DEFINED INPUT_FILE)
    message(FATAL_ERROR "INPUT_FILE is required")
endif()

file(READ "${INPUT_FILE}" CONTENTS)

string(FIND "${CONTENTS}" "struct char_traits<unsigned int>" PATCH_ALREADY_APPLIED)
if(NOT PATCH_ALREADY_APPLIED EQUAL -1)
    return()
endif()

set(PATCH_BLOCK [=[
#include <algorithm>
#include <ios>

namespace std
{
namespace
{
template<class CharType, class IntType>
struct char_traits_base
{
    using char_type  = CharType;
    using int_type   = IntType;
    using off_type   = streamoff;
    using pos_type   = fpos<mbstate_t>;
    using state_type = mbstate_t;

    static inline constexpr void assign(char_type& c1, const char_type& c2) noexcept { c1 = c2; }
    static inline constexpr bool eq(char_type c1, char_type c2) noexcept { return c1 == c2; }
    static inline constexpr bool lt(char_type c1, char_type c2) noexcept { return c1 < c2; }

    static constexpr int compare(const char_type* lhs, const char_type* rhs, size_t count) noexcept
    {
        for (; count; --count, ++lhs, ++rhs)
        {
            if (lt(*lhs, *rhs))
                return -1;
            if (lt(*rhs, *lhs))
                return 1;
        }
        return 0;
    }

    static inline constexpr size_t length(const char_type* s) noexcept
    {
        size_t i = 0;
        for (; s[i] != 0; ++i)
        {
        }
        return i;
    }

    static constexpr const char_type* find(const char_type* s, size_t n, const char_type& a) noexcept
    {
        for (; n; --n)
        {
            if (*s == a)
                return s;
            ++s;
        }
        return nullptr;
    }

    static inline char_type* move(char_type* s1, const char_type* s2, size_t n) noexcept
    {
        return reinterpret_cast<char_type*>(__builtin_memmove(s1, s2, n * sizeof(char_type)));
    }

    static inline char_type* copy(char_type* s1, const char_type* s2, size_t n) noexcept
    {
        __builtin_memmove(s1, s2, n * sizeof(char_type));
        return s1;
    }

    static inline char_type* assign(char_type* s, size_t n, char_type a) noexcept
    {
        std::fill_n(s, n, a);
        return s;
    }

    static inline constexpr int_type not_eof(int_type c) noexcept { return eq_int_type(c, eof()) ? ~eof() : c; }
    static inline constexpr char_type to_char_type(int_type c) noexcept { return static_cast<char_type>(c); }
    static inline constexpr int_type to_int_type(char_type c) noexcept { return static_cast<int_type>(c); }
    static inline constexpr bool eq_int_type(int_type c1, int_type c2) noexcept { return c1 == c2; }
    static inline constexpr int_type eof() noexcept { return static_cast<int_type>(EOF); }
};
}

template<>
struct char_traits<unsigned char> : char_traits_base<unsigned char, unsigned int>
{
};

template<>
struct char_traits<unsigned short> : char_traits_base<unsigned short, unsigned int>
{
};

template<>
struct char_traits<unsigned int> : char_traits_base<unsigned int, unsigned int>
{
};
}

]=])

string(REPLACE "#include <string>\n" "#include <string>\n${PATCH_BLOCK}\n" UPDATED_CONTENTS "${CONTENTS}")
file(WRITE "${INPUT_FILE}" "${UPDATED_CONTENTS}")
