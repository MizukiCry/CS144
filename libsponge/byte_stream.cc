#include <algorithm>
#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): Stream(), Capacity(capacity) { }

size_t ByteStream::write(const string &data) {
    const size_t WriteNum = min(data.length(), Capacity - Stream.size());
    for(size_t i = 0; i < WriteNum; ++i)Stream.push_back(data[i]);
    WriteCnt += WriteNum;
    return WriteNum;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    const size_t PeekLen = min(len, Stream.size());
    string Res;
    auto p = Stream.begin();
    for(size_t i = 0; i < PeekLen; ++i)
        Res.push_back(*p++);
    return Res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if(Stream.size() < len)
        return set_error();
    for(size_t i = 0; i < len; ++i)
        Stream.pop_front();
    ReadCnt += len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    if(Stream.size() < len)
    {
        set_error();
        return {};
    }
    const string Res = peek_output(len);
    pop_output(len);
    return Res;
}

void ByteStream::end_input() {
    EndInput = true;
}

bool ByteStream::input_ended() const {
    return EndInput;
}

size_t ByteStream::buffer_size() const {
    return Stream.size();
}

bool ByteStream::buffer_empty() const {
    return Stream.empty();
}

bool ByteStream::eof() const {
    return Stream.empty() && EndInput;
}

size_t ByteStream::bytes_written() const {
    return WriteCnt;
}

size_t ByteStream::bytes_read() const {
    return ReadCnt;
}

size_t ByteStream::remaining_capacity() const {
    return Capacity - Stream.size();
}
