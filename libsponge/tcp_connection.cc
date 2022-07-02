#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

using namespace std;

void TCPConnection::set_rst() {
    _sender.send_empty_segment();
    auto seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    set_ack(seg);
    seg.header().rst = true;
    _segments_out.push(seg);
}

void TCPConnection::set_ack(TCPSegment& seg) const {
    const auto ackno = _receiver.ackno();
    if(ackno.has_value())
    {
        seg.header().ack = true;
        seg.header().ackno = ackno.value();
    }
    seg.header().win = _receiver.window_size();
}

bool TCPConnection::send() {
    if(_sender.segments_out().empty())return false;
    while(!_sender.segments_out().empty())
    {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        set_ack(seg);
        _segments_out.push(seg);
    }
    return true;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_since_last_segment_received = 0;
    if(seg.header().rst)
    {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active = false;
        return;
    }
    _receiver.segment_received(seg);
    if(in_end() && !_sender.stream_in().eof())_linger_after_streams_finish = false;
    if(seg.header().ack)
    {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        send();
    }
    if(seg.length_in_sequence_space())
    {
        _sender.fill_window();
        if(!send())
        {
            _sender.send_empty_segment();
            auto _seg = _sender.segments_out().front();
            _sender.segments_out().pop();
            set_ack(_seg);
            _segments_out.push(_seg);
        }
    }
}

size_t TCPConnection::write(const string &data) {
    if(data.empty())return 0;
    size_t len = _sender.stream_in().write(data);
    _sender.fill_window();
    send();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if(!_sender.segments_out().empty())
    {
        auto seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        set_ack(seg);
        if(_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS)
        {
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            seg.header().rst = true;
            _active = false;
        }
        _segments_out.push(seg);
    }
    if(in_end() && out_end() && (!_linger_after_streams_finish || _time_since_last_segment_received >= 10 * _cfg.rt_timeout))_active = false;
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send();
}

void TCPConnection::connect() {
    _sender.fill_window();
    send();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            set_rst();
            _active = false;
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}