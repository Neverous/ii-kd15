#ifndef __CODE_H__
#define __CODE_H__

#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>

typedef unsigned char uchar_t;

#pragma pack(push, 1)
template<int BITS>
class LZ78Code
{
    static_assert(BITS < (1 << sizeof(size_t)), "BITS too big");

    bool        kind:1;
    uchar_t     byte:8;
    size_t      id  :BITS;

public:

    LZ78Code(uchar_t _byte=0, size_t _id=0);

    bool is_long_code(void) const;
    bool is_short_code(void) const;

    size_t get_id(void) const;
    uchar_t get_byte(void) const;
    size_t bitsize(uint32_t maxbits) const;

    template<int SBITS>
    operator LZ78Code<SBITS>&(void);

    template<int SBITS>
    LZ78Code(const LZ78Code<SBITS> &src);
}; // class LZ78Code
#pragma pack(pop)

template<int BITS>
inline
LZ78Code<BITS>::LZ78Code(uchar_t _byte, size_t _id)
:kind{!!_id}
,byte{_byte}
,id{}
{
    static_assert(sizeof(LZ78Code<BITS>) == (BITS + 16) / 8, "Invalid code size");
    assert(_id < (1U << BITS) && "ID out of range");
    if(_id)
        id = _id;
}

template<int BITS>
inline
bool LZ78Code<BITS>::is_long_code(void) const
{
    return kind;
}

template<int BITS>
inline
bool LZ78Code<BITS>::is_short_code(void) const
{
    return !is_long_code();
}

template<int BITS>
inline
size_t LZ78Code<BITS>::get_id(void) const
{
    return id;
}

template<int BITS>
inline
uchar_t LZ78Code<BITS>::get_byte(void) const
{
    return byte;
}

template<int BITS>
inline
size_t LZ78Code<BITS>::bitsize(uint32_t maxbits) const
{
    return 9 + (is_long_code() ? maxbits : 0);
}

template<int BITS>
template<int SBITS>
inline
LZ78Code<BITS>::operator LZ78Code<SBITS>&(void)
{
    static_assert(SBITS <= BITS, "Invalid size cast");
    return *(LZ78Code<SBITS>*) this;
}

template<int BITS>
template<int SBITS>
inline
LZ78Code<BITS>::LZ78Code(const LZ78Code<SBITS> &src)
:kind{src.kind}
,byte{src.byte}
,id{src.id}
{
    static_assert(SBITS <= BITS, "Invalid size cast");
}

// OPERATORS
template<int BITS>
inline
std::ostream &operator<<(std::ostream &stream, LZ78Code<BITS> &code)
{
    stream << "LZ78Code<" << BITS << ">(";
    if(code.is_long_code())
        stream << "id=" << code.get_id() << ", ";

    return stream   << "byte=0x" << std::hex << (size_t) code.get_byte() << std::dec
                    << ", bitsize=" << code.bitsize(BITS) << ")";
}

#endif // __CODE_H__
