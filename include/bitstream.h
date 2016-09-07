#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#include <algorithm>
#include <cassert>
#include <iostream>

typedef unsigned char uchar_t;

template<typename STREAM, typename ACCUMULATOR=uint64_t>
class BitStream
{
    STREAM              stream;
    ACCUMULATOR         accumulator;
    static const size_t BITSPACE = sizeof(accumulator) * 8;
    size_t              accumulator_size;
    size_t              last_count;

public:
    BitStream(STREAM _stream);
    ~BitStream(void);

    void flush(void);
    bool good(void) const;

    bool write_bits(uchar_t *buffer, size_t bits);
    bool read_bits(uchar_t *buffer, size_t bits);

    void write(char *buffer, size_t bytes);
    void read(char *buffer, size_t bytes);

    size_t gcount(void) const;
}; // class BitStream

template<typename STREAM, typename ACCUMULATOR>
inline
BitStream<STREAM, ACCUMULATOR>::BitStream(STREAM _stream)
:stream{_stream}
,accumulator{0}
,accumulator_size{0}
,last_count{0}
{
}

template<typename STREAM, typename ACCUMULATOR>
inline
BitStream<STREAM, ACCUMULATOR>::~BitStream(void)
{
    flush();
}

template<typename STREAM, typename ACCUMULATOR>
inline
void BitStream<STREAM, ACCUMULATOR>::flush(void)
{
    if(accumulator_size)
        stream.write((char *) &accumulator, (accumulator_size + 7) / 8);
}

template<typename STREAM, typename ACCUMULATOR>
inline
bool BitStream<STREAM, ACCUMULATOR>::good(void) const
{
    return stream.good();
}

template<typename STREAM, typename ACCUMULATOR>
inline
bool BitStream<STREAM, ACCUMULATOR>::write_bits(uchar_t *buffer, size_t bits)
{
    last_count = 0;
    if(!accumulator_size && bits >= 8)
    {
        stream.write((char*) buffer, bits / 8);
        last_count += bits / 8;
        bits %= 8;
        buffer += last_count;
    }

    assert(accumulator_size < BITSPACE);
    while(bits >= BITSPACE && stream.good())
    {
        accumulator |= (*(ACCUMULATOR *) buffer) << accumulator_size;
        stream.write((char *) &accumulator, sizeof(accumulator));
        last_count += sizeof(accumulator);
        accumulator = (*(ACCUMULATOR *) buffer) >> (BITSPACE - accumulator_size);
        bits -= BITSPACE;
        buffer += sizeof(accumulator);
    }

    assert(bits < BITSPACE);
    while(bits >= 8 && accumulator_size + 8 < BITSPACE)
    {
        accumulator |= ((ACCUMULATOR) *buffer) << accumulator_size;
        accumulator_size += 8;
        bits -= 8;
        ++ buffer;
    }

    assert(accumulator_size <= BITSPACE);
    if(accumulator_size + bits >= BITSPACE)
    {
        accumulator |= ((ACCUMULATOR) *buffer) << accumulator_size;
        stream.write((char *) &accumulator, sizeof(accumulator));
        last_count += sizeof(accumulator);
        accumulator = ((ACCUMULATOR) *buffer) >> (BITSPACE - accumulator_size);
        accumulator_size = accumulator_size + std::min((size_t) 8, bits) - BITSPACE;
        assert(accumulator_size <= 8);
        bits -= std::min((size_t) 8, bits);
        ++ buffer;
    }

    while(bits)
    {
        accumulator |= ((ACCUMULATOR) *buffer) << accumulator_size;
        accumulator_size += std::min((size_t) 8, bits);
        bits -= std::min((size_t) 8, bits);
        ++ last_count;
        ++ buffer;
        assert(accumulator_size < BITSPACE);
    }

    assert(accumulator_size < BITSPACE);
    accumulator &= ((ACCUMULATOR) 1 << accumulator_size) - 1;
    return last_count;
}

template<typename STREAM, typename ACCUMULATOR>
inline
bool BitStream<STREAM, ACCUMULATOR>::read_bits(uchar_t *buffer, size_t bits)
{
    last_count = 0;
    if(!accumulator_size && bits >= 8)
    {
        stream.read((char *) buffer, bits / 8);
        assert((size_t) stream.gcount() <= bits / 8);
        last_count += stream.gcount();
        bits -= stream.gcount() * 8;
        buffer += last_count;
    }

    while(bits >= BITSPACE && stream.good())
    {
        ACCUMULATOR input = 0;
        stream.read((char *) &input, sizeof(input));
        if(stream.good())
        {
            last_count += stream.gcount();
            *(ACCUMULATOR *&) buffer = accumulator | (input << accumulator_size);
            accumulator = input >> (BITSPACE - accumulator_size);
            bits -= BITSPACE;
            buffer += sizeof(accumulator);
        }
    }

    assert(bits < BITSPACE || !stream.good());
    while(accumulator_size < bits && accumulator_size + 8 <= BITSPACE && stream.good())
    {
        uchar_t input = 0;
        stream.read((char *) &input, sizeof(input));
        if(stream.good())
        {
            accumulator |= ((ACCUMULATOR) input) << accumulator_size;
            accumulator_size += 8;
        }
    }

    assert(BITSPACE - accumulator_size < 8 || accumulator_size >= bits || !stream.good());
    if(bits > accumulator_size && stream.good())
    {
        assert(BITSPACE - accumulator_size < 8);
        uchar_t input = 0;
        stream.read((char *) &input, sizeof(input));
        if(stream.good())
        {
            *(ACCUMULATOR *&) buffer = (accumulator | (((ACCUMULATOR) input) << accumulator_size)) & (((ACCUMULATOR) 1 << bits) - 1);
            last_count += (bits + 7) / 8;
            accumulator = input >> (BITSPACE - accumulator_size);
            accumulator_size = accumulator_size + 8 - BITSPACE;
            assert(accumulator_size < 8);
            buffer += sizeof(accumulator);
            bits = 0;
        }
    }

    bits = std::min(bits, accumulator_size);
    while(bits)
    {
        *buffer ++ = accumulator & (((ACCUMULATOR) 1 << std::min((size_t) 8, bits)) - 1);
        accumulator >>= std::min((size_t) 8, bits);
        accumulator_size -= std::min((size_t) 8, bits);
        bits -= std::min((size_t) 8, bits);
        ++ last_count;
    }

    accumulator &= ((ACCUMULATOR) 1 << accumulator_size) - 1;
    return last_count;
}

template<typename STREAM, typename ACCUMULATOR>
inline
void BitStream<STREAM, ACCUMULATOR>::write(char *buffer, size_t bytes)
{
    if(accumulator_size)
    {
        write_bits((uchar_t*) buffer, bytes * 8);
        return;
    }

    stream.write(buffer, bytes);
    last_count = bytes;
}

template<typename STREAM, typename ACCUMULATOR>
inline
void BitStream<STREAM, ACCUMULATOR>::read(char *buffer, size_t bytes)
{
    if(accumulator_size)
    {
        read_bits((uchar_t*) buffer, bytes * 8);
        return;
    }

    stream.read(buffer, bytes);
    last_count = stream.gcount();
}

template<typename STREAM, typename ACCUMULATOR>
inline
size_t BitStream<STREAM, ACCUMULATOR>::gcount(void) const
{
    return last_count;
}

#endif // __BITSTREAM_H__
