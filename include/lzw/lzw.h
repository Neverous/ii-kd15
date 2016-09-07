#ifndef __LZW_H__
#define __LZW_H__

#include "bitstream.h"
#include "code.h"

#include <vector>

template<typename LOG, typename DICTIONARY, typename OUTPUT>
class LZW
{
    LOG         log;
    DICTIONARY  dictionary;
    OUTPUT      output;

    size_t      previous_id{0};
    size_t      current_id{0};
    bool        simulation{false};
    bool        error{false};

public:
    LZW(LOG _log, DICTIONARY _dictionary, OUTPUT _output);
    ~LZW(void);

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
    auto &decompress_code(const LZWCode<BITS> &code);

    void write_current_code(void);
}; // class LZW

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
LZW<LOG, DICTIONARY, OUTPUT>::LZW(LOG _log, DICTIONARY _dictionary, OUTPUT _output)
:log{_log}
,dictionary{_dictionary}
,output{_output}
{
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
LZW<LOG, DICTIONARY, OUTPUT>::~LZW(void)
{
    flush();
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
void LZW<LOG, DICTIONARY, OUTPUT>::flush(void)
{
    if(!good())
        return;

    if(current_id)
    {
        log(log.DEBUG) << "Flushing last code";
        write_current_code();
    }
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
void LZW<LOG, DICTIONARY, OUTPUT>::simulate(void)
{
    simulation = true;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
bool LZW<LOG, DICTIONARY, OUTPUT>::good(void)
{
    return !error;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
template<typename INPUT>
inline
auto &LZW<LOG, DICTIONARY, OUTPUT>::compress(INPUT input)
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
auto &LZW<LOG, DICTIONARY, OUTPUT>::compress_bytes(uchar_t *byte, size_t size)
{
    while(good() && size --)
        compress_byte(*byte ++);

    return *this;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
auto &LZW<LOG, DICTIONARY, OUTPUT>::compress_byte(uchar_t byte)
{
    if(!dictionary.step(byte, current_id))
    {
        write_current_code();
        dictionary.add_suffix(byte);
        dictionary.step(byte, current_id);
    }

    return *this;
}

#define SWITCH_SIZE_OPT_BODY    \
    SWITCH_SIZE_OPT             \
        CASE_SIZE_OPT(1)        \
        CASE_SIZE_OPT(2)        \
        CASE_SIZE_OPT(3)        \
        CASE_SIZE_OPT(4)        \
        CASE_SIZE_OPT(5)        \
        CASE_SIZE_OPT(6)        \
        CASE_SIZE_OPT(7)        \
        CASE_SIZE_OPT(8)        \
        CASE_SIZE_OPT(9)        \
        CASE_SIZE_OPT(10)       \
        CASE_SIZE_OPT(11)       \
        CASE_SIZE_OPT(12)       \
        CASE_SIZE_OPT(13)       \
        CASE_SIZE_OPT(14)       \
        CASE_SIZE_OPT(15)       \
        CASE_SIZE_OPT(16)       \
        CASE_SIZE_OPT(17)       \
        CASE_SIZE_OPT(18)       \
        CASE_SIZE_OPT(19)       \
        CASE_SIZE_OPT(20)       \
        CASE_SIZE_OPT(21)       \
        CASE_SIZE_OPT(22)       \
        CASE_SIZE_OPT(23)       \
        CASE_SIZE_OPT(24)       \
        CASE_SIZE_OPT(25)       \
        CASE_SIZE_OPT(26)       \
        CASE_SIZE_OPT(27)       \
        CASE_SIZE_OPT(28)       \
        CASE_SIZE_OPT(29)       \
        CASE_SIZE_OPT(30)       \
        CASE_SIZE_OPT(31)       \
    END_SWITCH_SIZE_OPT

template<typename LOG, typename DICTIONARY, typename OUTPUT>
template<typename INPUT>
inline
auto &LZW<LOG, DICTIONARY, OUTPUT>::decompress(INPUT input)
{
#define SWITCH_SIZE_OPT     if(false) {} else
#define END_SWITCH_SIZE_OPT {}
#define CASE_SIZE_OPT(power)                                \
    if(dictionary.size() < (1U << power) - 1)               \
    {                                                       \
        LZWCode<power> code{1};                             \
        input.read_bits((uchar_t *) &code, code.bitsize()); \
        if(input.good())                                    \
            decompress_code(code);                          \
    } else

    while(good() && input.good())
    {
        SWITCH_SIZE_OPT_BODY
    }

#undef SWITCH_SIZE_OPT
#undef END_SWITCH_SIZE_OPT
#undef CASE_SIZE_OPT
    return *this;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
template<int BITS>
inline
auto &LZW<LOG, DICTIONARY, OUTPUT>::decompress_code(const LZWCode<BITS> &code)
{
    log(log.DEBUG)  << "Decompressing " << code;
    log(log.DEBUG)  << "Current dictionary size=" << dictionary.size()
                    << " previous_id=" << previous_id;
    assert(code.get_id());
    std::vector<uchar_t> data = dictionary.jump(code.get_id());
    if(data.empty())
    {
        assert(code.get_id() == dictionary.size() + 1);
        log(log.DEBUG) << "Empty?";
        data = dictionary.jump(previous_id);
        assert(data.size() > 0);
        if(!simulation)
        {
            output.write((char *) &data[0], data.size() * sizeof(uchar_t));
            output.write((char *) &data[0], sizeof(uchar_t));
        }

        dictionary.add_suffix(data[0]);
        if(dictionary.empty())
            previous_id = 0;

        else
            previous_id = code.get_id();

        return *this;
    }

    if(!simulation)
        output.write((char *) &data[0], data.size() * sizeof(uchar_t));

    if(previous_id)
    {
        std::vector<uchar_t> test = dictionary.jump(previous_id);
        assert(test.size() > 0);
        dictionary.add_suffix(data[0]);

        if(dictionary.empty())
            previous_id = 0;

        else
            previous_id = code.get_id();

        return *this;
    }

    previous_id = code.get_id();
    return *this;
}

template<typename LOG, typename DICTIONARY, typename OUTPUT>
inline
void LZW<LOG, DICTIONARY, OUTPUT>::write_current_code(void)
{
#define SWITCH_SIZE_OPT     if(false) {} else
#define END_SWITCH_SIZE_OPT {}
#define CASE_SIZE_OPT(power)                                        \
    if(dictionary.size() < (1U << power))                           \
    {                                                               \
        LZWCode<power> code{current_id};                            \
        log(log.DEBUG) << "Current dictionary size=" << dictionary.size(); \
        log(log.DEBUG) << "Part compressed into " << code;          \
        if(!simulation)                                             \
            output.write_bits((uchar_t*) &code, code.bitsize());    \
    } else

    SWITCH_SIZE_OPT_BODY

#undef SWITCH_SIZE_OPT
#undef END_SWITCH_SIZE_OPT
#undef CASE_SIZE_OPT

    current_id = 0;
}

#endif // __LZW_H__
