#ifndef __LZ78_H__
#define __LZ78_H__

#include "bitstream.h"
#include "code.h"

#include <vector>

template<typename LOG, typename DICTIONARY, typename OUTPUT>
class LZ78
{
    LOG         log;
    DICTIONARY  dictionary;
    OUTPUT      output;

    size_t      current_id{0};
    bool        simulation{false};
    bool        error{false};

public:
    LZ78(LOG _log, DICTIONARY _dictionary, OUTPUT _output);
    ~LZ78(void);

    void flush(void);

    void simulate(void);
    bool good(void);

    template<typename INPUT>
    auto &compress(INPUT input);

    template<typename INPUT>
    auto &decompress(INPUT input);

private:
    auto &compress_bytes(uchar_t *byte, size_t size);
    auto &compress_byte(uchar_t byte);

    template<int BITS>
    auto &decompress_code(const LZ78Code<BITS> &code);

    void write_current_code(uchar_t byte);
}; // class LZ78

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
LZ78<LOG, DICTIONARY, OUTPUT>::LZ78(LOG _log, DICTIONARY _dictionary, OUTPUT _output)
:log{_log}
,dictionary{_dictionary}
,output{_output}
{
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
LZ78<LOG, DICTIONARY, OUTPUT>::~LZ78(void)
{
    flush();
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
void LZ78<LOG, DICTIONARY, OUTPUT>::flush(void)
{
    if(!good())
        return;

    if(current_id)
    {
        uchar_t last;
        dictionary.step_back(last, current_id);
        log(log.DEBUG) << "Flushing last code";
        write_current_code(last);
    }
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
void LZ78<LOG, DICTIONARY, OUTPUT>::simulate(void)
{
    simulation = true;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
bool LZ78<LOG, DICTIONARY, OUTPUT>::good(void)
{
    return !error;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
template<typename INPUT>
inline
auto &LZ78<LOG, DICTIONARY, OUTPUT>::compress(INPUT input)
{
    uchar_t buffer[16384];
    while(good() && input.good())
    {
        input.read((char*) buffer, 16384);
        compress_bytes(buffer, input.gcount());
    }

    return *this;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
auto &LZ78<LOG, DICTIONARY, OUTPUT>::compress_bytes(uchar_t *byte, size_t size)
{
    while(good() && size --)
        compress_byte(*byte ++);

    return *this;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
auto &LZ78<LOG, DICTIONARY, OUTPUT>::compress_byte(uchar_t byte)
{
    if(!dictionary.step(byte, current_id))
    {
        write_current_code(byte);
        dictionary.add_suffix(byte);
    }

    return *this;
}

inline
uint32_t nearest2pow(uint32_t value)
{
    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    ++ value;
    value += value == 0;
    return __builtin_ctz(value);
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
template<typename INPUT>
inline
auto &LZ78<LOG, DICTIONARY, OUTPUT>::decompress(INPUT input)
{
    while(good() && input.good())
    {
        LZ78Code<31> code;
        input.read_bits((uchar_t *) &code, 8);
        input.read_bits(((uchar_t *) &code) + 1, code.bitsize(nearest2pow(dictionary.size() + 1)) - 8);
        if(input.good())
            decompress_code(code);
    }

    return *this;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
template<int BITS>
inline
auto &LZ78<LOG, DICTIONARY, OUTPUT>::decompress_code(const LZ78Code<BITS> &code)
{
    log(log.DEBUG) << "Decompressing " << code << " realsize=" << code.bitsize(nearest2pow(dictionary.size() + 1));
    if(code.get_id())
    {
        std::vector<uchar_t> data = dictionary.jump(code.get_id());
        if(!simulation)
            output.write((char *) &data[0], data.size() * sizeof(uchar_t));
    }

    if(!simulation)
    {
        char byte = code.get_byte();
        output.write(&byte, 1);
    }

    dictionary.add_suffix(code.get_byte());
    return *this;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
void LZ78<LOG, DICTIONARY, OUTPUT>::write_current_code(uchar_t byte)
{
    LZ78Code<31> code{byte, current_id};
    log(log.DEBUG) << "Part compressed into " << code << " realsize=" << code.bitsize(nearest2pow(dictionary.size() + 1));
    if(!simulation)
        output.write_bits((uchar_t*) &code, code.bitsize(nearest2pow(dictionary.size() + 1)));

    current_id = 0;
}

#endif // __LZ78_H__
