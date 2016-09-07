#ifndef __COMMON_H__
#define __COMMON_H__

#include <iostream>
#include <string>
#include <sys/stat.h>

#include <bitstream.h>
#include <adaptive_huffman.h>

typedef BitStream<std::ostream &>   BitOut;
typedef BitStream<std::istream &>   BitIn;
typedef AdaptiveHuffman<BitOut>     HuffOut;
typedef AdaptiveHuffman<BitIn>      HuffIn;
typedef BitStream<HuffOut>          BitHuffOut;
typedef BitStream<HuffIn>           BitHuffIn;

template<>
inline
void BitHuffIn::flush(void)
{
    // Nothing to flush when used for reading
}

template<>
inline
void BitIn::flush(void)
{
    // Nothing to flush when used for reading
}

inline
bool file_exists(const std::string &name)
{
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

inline
bool has_suffix(const std::string &str, const std::string &suffix)
{
    return  str.size() >= suffix.size() &&
            str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

#endif // __COMMON_H__
