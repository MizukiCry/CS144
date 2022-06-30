#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader Head = seg.header();
    if(Head.syn && !SYN)
    {
        SYN = true;
        ISN = Head.seqno;
        if(Head.fin)FIN = true;
        _reassembler.push_substring(seg.payload().copy(), 0, Head.fin);
        return;
    }
    if(!SYN)return;
    if(Head.fin)FIN = true;
    _reassembler.push_substring(seg.payload().copy(), unwrap(Head.seqno, ISN, _reassembler.GetPos()) - 1, Head.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(!SYN)return nullopt;
    return wrap(_reassembler.GetPos() + 1 + (_reassembler.empty() && FIN), ISN);
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
