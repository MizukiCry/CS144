#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
    Buf.resize(capacity);
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {

    if(eof)EPos = index + data.length();
    size_t l = std::max(Pos, index), r = std::min(Pos + _output.remaining_capacity(), index + data.length());
    for(size_t p = l % _capacity, i = 0; l + i < r; ++i)
    {
        if(!Buf[p].has_value())
        {
            ++Cnt;
            Buf[p] = data[l + i - index];
        }
        if(++p == _capacity)p = 0;
    }
    string Tmp;
    for(size_t p = Pos % _capacity; Buf[p].has_value(); --Cnt, ++Pos)
    {
        Tmp += Buf[p].value();
        Buf[p].reset();
        if(++p == _capacity)p = 0;
    }
    if(!Tmp.empty())_output.write(Tmp);
    if(Pos == EPos)_output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return Cnt; }

bool StreamReassembler::empty() const { return Cnt == 0; }
