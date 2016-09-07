#ifndef __ADAPTIVE_HUFFMAN_H__
#define __ADAPTIVE_HUFFMAN_H__

#include <algorithm>
#include <cstdint>
#include <vector>

typedef unsigned char uchar_t;

struct Node
{
    uchar_t     byte;
    uint16_t    number;
    uint64_t    weight;

    uint16_t    parent;
    uint16_t    left;
    uint16_t    right;

    Node(uchar_t _byte, uint16_t _number, uint64_t _weight=0, uint16_t _parent=0, uint16_t _left=0, uint16_t _right=0)
        :byte{_byte}, number{_number}, weight{_weight}, parent{_parent}, left{_left}, right{_right}
    {
    }
}; // struct Node

template<typename BITSTREAM>
class AdaptiveHuffman
{
    BITSTREAM           stream;
    std::vector<Node>   memory;

    uint16_t    null;
    uint16_t    root;
    std::vector<uint16_t>   byte2node;
    std::vector<uint16_t>   number2node;

    size_t      last_count;

public:
    AdaptiveHuffman(BITSTREAM _stream);

    bool good(void) const;

    void write(char *buffer, size_t bytes);
    void read(char *buffer, size_t bytes);

    bool put(uchar_t byte);
    bool get(char &byte);

    size_t gcount(void) const;

private:
    void get_code(uint16_t current, uchar_t *code, size_t &size);
    void add_new_byte(uchar_t byte);
    void update_tree(uint16_t current);
#ifndef NDEBUG
    bool validate_tree(uint16_t current);
#endif
    uint16_t highest_node(uint16_t current);
    void exchange(uint16_t a, uint16_t b);
}; // class AdaptiveHuffman

template<typename BITSTREAM>
inline
AdaptiveHuffman<BITSTREAM>::AdaptiveHuffman(BITSTREAM _stream)
:stream{_stream}
,memory{}
,null{1}
,root{null}
,byte2node{}
,number2node{}
{
    memory.reserve(513);
    assert(memory.capacity() >= 513);
    memory.emplace_back(0, 512);
    byte2node.resize(256);
    number2node.resize(513);

    number2node[512] = root;
}

template<typename BITSTREAM>
inline
bool AdaptiveHuffman<BITSTREAM>::good(void) const
{
    return stream.good();
}

template<typename BITSTREAM>
inline
void AdaptiveHuffman<BITSTREAM>::write(char *buffer, size_t bytes)
{
    last_count = 0;
    size_t count = bytes;
    while(bytes --)
        if(!put(*buffer ++))
            return;

    last_count = count;
}

template<typename BITSTREAM>
inline
void AdaptiveHuffman<BITSTREAM>::read(char *buffer, size_t bytes)
{
    last_count = 0;
    size_t count = bytes;
    while(bytes --)
        if(!get(*buffer ++))
            return;

    last_count = count;
}

template<typename BITSTREAM>
inline
bool AdaptiveHuffman<BITSTREAM>::put(uchar_t byte)
{
    last_count = 0;
    uchar_t code[64];
    size_t size = 0;
    if(byte2node.at(byte))
    {
        get_code(byte2node.at(byte), code, size);
        assert(memory.at(byte2node.at(byte) - 1).byte == byte);
        update_tree(byte2node.at(byte));
        stream.write_bits(code, size);
    }

    else
    {
        get_code(null, code, size);
        stream.write_bits(code, size);
        stream.write((char*) &byte, 1);
        add_new_byte(byte);
    }

    last_count = 1;
    return true;
}

template<typename BITSTREAM>
inline
bool AdaptiveHuffman<BITSTREAM>::get(char &byte)
{
    last_count = 0;
    uint16_t current = root;
    while(current)
    {
        if(current == null)
        {
            stream.read((char*) &byte, 1);
            last_count = stream.gcount();
            if(!last_count)
                return false;

            add_new_byte(byte);
            return true;
        }

        Node &node = memory.at(current - 1);
        if(!node.left)
        {
            assert(!node.right);
            byte = node.byte;
            update_tree(current);
            last_count = 1;
            return true;
        }

        uchar_t bit = 0;
        stream.read_bits(&bit, 1);
        current = bit ? node.right : node.left;
    }

    assert(false && "Something went wrong");
    return false;
}

template<typename BITSTREAM>
inline
size_t AdaptiveHuffman<BITSTREAM>::gcount(void) const
{
    return last_count;
}

template<typename BITSTREAM>
inline
void AdaptiveHuffman<BITSTREAM>::get_code(uint16_t current, uchar_t *code, size_t &size)
{
    bool bit[257];
    size_t bits = 0;
    while(current && current != root)
    {
        assert(bits < 257);
        Node &node = memory.at(current - 1);
        Node &parent = memory.at(node.parent - 1);
        bit[bits ++] = (parent.right == current);
        current = node.parent;
    }

    size = bits;
    assert(current == null || bits > 0);
    size_t c = 0;
    size_t b = 8;
    while(bits --)
    {
        assert(c < 64);
        if(b == 8)
        {
            code[c ++] = 0;
            b = 0;
        }

        code[c - 1] |= (bit[bits] << b);
        ++ b;
    }

}

template<typename BITSTREAM>
inline
void AdaptiveHuffman<BITSTREAM>::add_new_byte(uchar_t byte)
{
    assert(!byte2node.at(byte));
    assert(null);
    memory.emplace_back(0, memory.at(null - 1).number - 2, 0, null);
    memory.emplace_back(byte, memory.at(null - 1).number - 1, 0, null);

    Node &escape = memory.at(null - 1);
    escape.left = memory.size() - 1;
    escape.right = memory.size();

    byte2node.at(byte) = escape.right;
    number2node.at(escape.number - 2) = escape.left;
    number2node.at(escape.number - 1) = escape.right;
    null = escape.left;
    assert(memory.at(byte2node.at(byte) - 1).byte == byte);
    update_tree(escape.right);
    assert(memory.at(byte2node.at(byte) - 1).byte == byte);
}

template<typename BITSTREAM>
inline
void AdaptiveHuffman<BITSTREAM>::update_tree(uint16_t current)
{
    assert(validate_tree(root));
    while(current)
    {
        uint16_t highest = highest_node(current);
        exchange(current, highest);
        Node &node = memory.at(current - 1);
        ++ node.weight;
        current = node.parent;
        assert(validate_tree(root));
    }

}

template<typename BITSTREAM>
inline
uint16_t AdaptiveHuffman<BITSTREAM>::highest_node(uint16_t current)
{
    assert(current);
    Node &node = memory.at(current - 1);
    uint16_t number = node.number;
    while(number + 1U < number2node.size() && memory.at(number2node.at(number + 1) - 1).weight == node.weight)
        ++ number;

    return number2node.at(number);
}

template<typename BITSTREAM>
inline
void AdaptiveHuffman<BITSTREAM>::exchange(uint16_t a, uint16_t b)
{
    if(!a || !b || a == root || b == root || a == b || memory.at(a-1).parent == b || memory.at(b-1).parent == a)
        return;

    Node &_a = memory.at(a - 1);
    Node &_b = memory.at(b - 1);
    Node &A = memory.at(_a.parent - 1);
    Node &B = memory.at(_b.parent - 1);;
    if(A.left == a && B.left == b)
        std::swap(A.left, B.left);

    else if(A.left == a && B.right == b)
        std::swap(A.left, B.right);

    else if(A.right == a && B.left == b)
        std::swap(A.right, B.left);

    else if(A.right == a && B.right == b)
        std::swap(A.right, B.right);

    std::swap(number2node.at(_a.number), number2node.at(_b.number));
    std::swap(_a.number, _b.number);
    std::swap(_a.parent, _b.parent);
}

#ifndef NDEBUG
template<typename BITSTREAM>
inline
bool AdaptiveHuffman<BITSTREAM>::validate_tree(uint16_t node)
{
    assert(node);
    Node &current = memory.at(node - 1);
    if(current.left)
    {
        Node &left = memory.at(current.left - 1);
        assert(left.parent == node);
        assert(left.number < current.number);
        validate_tree(current.left);
    }

    if(current.right)
    {
        Node &right = memory.at(current.right - 1);
        assert(right.parent == node);
        assert(right.number < current.number);
        validate_tree(current.right);
    }

    assert(!current.left == !current.right);
    if(current.left && current.right)
    {
        Node &left = memory.at(current.left - 1);
        Node &right = memory.at(current.right - 1);
        assert(current.number > right.number && right.number > left.number);
    }

    return true;
}
#endif // NDEBUG

#endif // __ADAPTIVE_HUFFMAN_H__
