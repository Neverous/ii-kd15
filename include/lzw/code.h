#ifndef __CODE_H__
#define __CODE_H__

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>

typedef unsigned char uchar_t;

#pragma pack(push, 1)
template<int BITS>
class LZWCode
{
    static_assert(BITS < (1 << sizeof(size_t)), "BITS too big");

    size_t      id  :BITS;

public:
    LZWCode(size_t _id=0);

    size_t get_id(void) const;
    size_t bitsize(void) const;

    template<int SBITS>
    operator LZWCode<SBITS>&(void);

    template<int SBITS>
    LZWCode(const LZWCode<SBITS> &src);
}; // class LZ78Code
#pragma pack(pop)

// OPERATORS
template<int BITS>
inline
LZWCode<BITS>::LZWCode(size_t _id)
:id{_id}
{
    static_assert(sizeof(LZWCode<BITS>) == (BITS + 7) / 8, "Invalid code size");
    assert(_id < (1U << BITS) && "ID out of range");
    assert(0 < _id && "Invalid ID");
}

template<int BITS>
inline
size_t LZWCode<BITS>::get_id(void) const
{
    return id;
}

template<int BITS>
inline
size_t LZWCode<BITS>::bitsize(void) const
{
    return BITS;
}

template<int BITS>
template<int SBITS>
inline
LZWCode<BITS>::operator LZWCode<SBITS>&(void)
{
    static_assert(SBITS <= BITS, "Invalid size cast");
    return *(LZWCode<SBITS>*) this;
}

template<int BITS>
template<int SBITS>
inline
LZWCode<BITS>::LZWCode(const LZWCode<SBITS> &src)
:id{src.id}
{
    static_assert(SBITS <= BITS, "Invalid size cast");
}

// OPERATORS
template<int BITS>
inline
std::ostream &operator<<(std::ostream &stream, LZWCode<BITS> &code)
{
    return stream   << "LZWCode<" << BITS << ">("
                    << "id=" << code.get_id() << ", "
                    << "bitsize=" << code.bitsize() << ")";
}

#endif // __CODE_H__
