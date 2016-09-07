#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

typedef unsigned char uchar_t;

struct Element
{
    uchar_t     byte;
    uint32_t    prev;
    uint32_t    next;
    uint32_t    left;
    uint32_t    right;
}; // struct Element

struct Memory
{
    std::vector<uchar_t>    byte;
    std::vector<uint32_t>   prev; // prefix
    std::vector<uint32_t>   next; // first suffix
    std::vector<uint32_t>   left; // tree with same prefix
    std::vector<uint32_t>   right; // tree with same prefix

    void reserve(size_t size)
    {
        byte.reserve(size);
        prev.reserve(size);
        next.reserve(size);
        left.reserve(size);
        right.reserve(size);
    }

    size_t size(void) const
    {
        assert(byte.size() == prev.size());
        assert(prev.size() == next.size());
        assert(next.size() == left.size());
        assert(left.size() == right.size());
        return byte.size();
    }

    size_t capacity(void) const
    {
        assert(byte.capacity() == prev.capacity());
        assert(prev.capacity() == next.capacity());
        assert(next.capacity() == left.capacity());
        assert(left.capacity() == right.capacity());
        return byte.capacity();
    }

    bool empty(void) const
    {
        assert(byte.empty() == prev.empty());
        assert(prev.empty() == next.empty());
        assert(next.empty() == left.empty());
        assert(left.empty() == right.empty());
        return byte.empty();
    }

    void clear(void)
    {
        byte.clear();
        prev.clear();
        next.clear();
        left.clear();
        right.clear();
    }

    void resize(size_t size)
    {
        byte.resize(size);
        prev.resize(size);
        next.resize(size);
        left.resize(size);
        right.resize(size);
    }

    void emplace_back(uchar_t _byte, uint32_t _prev, uint32_t _next=0, uint32_t _left=0, uint32_t _right=0)
    {
        byte.emplace_back(_byte);
        prev.emplace_back(_prev);
        next.emplace_back(_next);
        left.emplace_back(_left);
        right.emplace_back(_right);
    }
}; // struct Memory

class Dictionary
{
protected:
    Memory memory;

    size_t size_limit;
    uint32_t current;

public:
    Dictionary(size_t _size_limit);
    bool step(uchar_t byte, size_t &id);
    void step_back(uchar_t &byte, size_t &id);
    std::vector<uchar_t> jump(size_t id);
    void add_suffix(uchar_t byte);
    virtual void clear(void);
    size_t size(void) const;
    bool empty(void) const;

protected:
    bool is_valid(uint32_t id) const
    {
        return 0 < id && id <= memory.size();
    }

}; // class Dictionary

template<int VALUES>
class PrepopulatedDictionary: public Dictionary
{
public:
    PrepopulatedDictionary(size_t _size_limit);

    void clear(void) override;
    bool empty(void) const;
}; // class PrepopulatedDictionary

inline
Dictionary::Dictionary(size_t _size_limit)
:memory{}
,size_limit{_size_limit}
,current{0}
{
    memory.reserve(size_limit / sizeof(Element));
    assert(memory.capacity() >= size_limit / sizeof(Element));
}

inline
bool Dictionary::step(uchar_t byte, size_t &id)
{
    if(memory.empty())
    {
        id = 0;
        return false;
    }

    size_t search = 1;
    if(is_valid(current))
        search = memory.next[current - 1];

    while(is_valid(search))
    {
        uchar_t &elbyte = memory.byte[search - 1];
        if(elbyte == byte)
        {
            id = current = search;
            return true;
        }

        else if(elbyte < byte)
            search = memory.right[search - 1];

        else
            search = memory.left[search - 1];
    }

    return false;
}

inline
void Dictionary::step_back(uchar_t &byte, size_t &id)
{
    assert(is_valid(current));
    byte = memory.byte[current - 1];
    id = current = memory.prev[current - 1];
}

inline
std::vector<uchar_t> Dictionary::jump(size_t id)
{
    if(!id)
        return {};

    std::vector<uchar_t> result;
    current = id;
    while(is_valid(current))
    {
        result.push_back(memory.byte[current - 1]);
        current = memory.prev[current - 1];
    }

    current = id;
    std::reverse(begin(result), end(result));
    return result;
}

inline
void Dictionary::add_suffix(uchar_t byte)
{
    if((memory.size() + 1) * sizeof(Element) > size_limit)
    {
        clear();
        return;
    }

    if(memory.empty())
    {
        current = 0;
        memory.emplace_back(byte, current);
        return;
    }

    uint32_t search = 1;
    if(is_valid(current))
    {
        uint32_t &elnext = memory.next[current - 1];
        if(!is_valid(elnext))
        {
            elnext = memory.size() + 1;
            memory.emplace_back(byte, current);
            current = 0;
            return;
        }

        search = elnext;
    }

    while(is_valid(search))
    {
        uchar_t elbyte = memory.byte[search - 1];
        if(elbyte < byte)
        {
            uint32_t &elright = memory.right[search - 1];
            search = elright;
            if(!is_valid(search))
                elright = memory.size() + 1;
        }

        else if(elbyte > byte)
        {
            uint32_t &elleft = memory.left[search - 1];
            search = elleft;
            if(!is_valid(search))
                elleft = memory.size() + 1;
        }

        else // ==
            return;
    }

    memory.emplace_back(byte, current);
    current = 0;
}

inline
void Dictionary::clear(void)
{
    current = 0;
    memory.clear();
}

inline
size_t Dictionary::size(void) const
{
    return memory.size();
}

inline
bool Dictionary::empty(void) const
{
    return !size();
}

template<int VALUES>
inline
PrepopulatedDictionary<VALUES>::PrepopulatedDictionary(size_t _size_limit)
:Dictionary{_size_limit}
{
    for(size_t value = VALUES / 2; value > 0; value /= 2)
        for(size_t current =  value; current < VALUES; current += value * 2)
            this->add_suffix(current);

    this->add_suffix(0);
    assert(this->size() == 256);
}

template<int VALUES>
inline
void PrepopulatedDictionary<VALUES>::clear(void)
{
    current = 0;
    memory.resize(VALUES);
    bzero(&memory.next[0], sizeof(uint32_t) * VALUES);
}

template<int VALUES>
inline
bool PrepopulatedDictionary<VALUES>::empty(void) const
{
    return size() == VALUES;
}

#endif // __DICTIONARY_H__
