#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _stream(capacity)
    , InitialRTO{retx_timeout}
    , RTO{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return FlightBytes; }

void TCPSender::fill_window() {
    TCPSegment seg;
    if(!SYN)
    {
        SYN = true;
        seg.header().syn = true;
        send_segment(seg);
        return;
    }

    if(!_segments_outstanding.empty() && _segments_outstanding.front().header().syn)return;
    if(!_stream.buffer_size() && !_stream.eof())return;
    if(FIN)return;

    if(WSize)
        while(Space)
        {
            const size_t l = min({_stream.buffer_size(), Space, TCPConfig::MAX_PAYLOAD_SIZE});
            seg.payload() = Buffer(move(_stream.read(l)));
            if(_stream.eof() && Space > l)seg.header().fin = FIN = true;
            send_segment(seg);
            if(_stream.buffer_empty())break;
        }
    else if(Space == 0)
    {
        if(_stream.eof())seg.header().fin = FIN = true;
        else if(!_stream.buffer_empty())seg.payload() = Buffer(move(_stream.read(1)));
        send_segment(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    const uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    if(abs_ackno > _next_seqno)return;
    if(!_segments_outstanding.empty() && abs_ackno < unwrap(_segments_outstanding.front().header().seqno, _isn, _next_seqno))return;

    WSize = Space = window_size;
    while(!_segments_outstanding.empty())
    {
        const auto seg = _segments_outstanding.front();
        if(unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() > abs_ackno)break;
        FlightBytes -= seg.length_in_sequence_space();
        _segments_outstanding.pop();
        CTime = RetryTime = 0;
        RTO = InitialRTO;
    }

    if(!_segments_outstanding.empty())Space = abs_ackno + window_size - unwrap(_segments_outstanding.front().header().seqno, _isn, _next_seqno) - FlightBytes;
    if(!FlightBytes)Timer = false;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(!Timer)return;
    CTime += ms_since_last_tick;
    if(CTime >= RTO)
    {
        _segments_out.push(_segments_outstanding.front());
        if(WSize || _segments_outstanding.front().header().syn)++RetryTime, RTO <<= 1;
        CTime = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return RetryTime; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

void TCPSender::send_segment(TCPSegment &seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);
    _next_seqno += seg.length_in_sequence_space();
    FlightBytes += seg.length_in_sequence_space();
    if(SYN)Space -= seg.length_in_sequence_space();
    _segments_out.push(seg);
    _segments_outstanding.push(seg);
    if(!Timer)Timer = true, CTime = 0;
}