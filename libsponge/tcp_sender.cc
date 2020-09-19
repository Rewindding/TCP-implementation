#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>
#include <cassert>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}
using namespace std;
//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity),_RTO(_initial_retransmission_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno-_send_base; }

void TCPSender::fill_window() {
    _rcv_window_size=max(_rcv_window_size,static_cast<size_t>(1));
    //window size occupied=next_seq-send_base
    while(_next_seqno-_send_base<_rcv_window_size&&(!_stream.buffer_empty()||_next_seqno==0||(!_FIN_SET&&_stream.eof()))){
        size_t remain_size=_rcv_window_size-(_next_seqno-_send_base);
        size_t seg_len=min(TCPConfig::MAX_PAYLOAD_SIZE,remain_size);
        TCPSegment seg{};
        seg.header().seqno=wrap(_next_seqno,_isn);
        seg.header().syn=(_next_seqno==0);
        seg_len-=(seg.header().syn?1:0);
        string payload=_stream.read(seg_len);
        seg.payload()=Buffer(move(payload));
        seg_len-=payload.size();
        if(seg_len>0) seg.header().fin=_stream.eof();
        _FIN_SET=seg.header().fin;
        _segments_out.push(seg);
        _outstanding_segs.push(seg);
        _next_seqno+=seg.length_in_sequence_space();
        if(!_timer_start){
            _timer_start=true;
            _timer=_time_passed;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    auto ab_ack=unwrap(ackno,_isn,_next_seqno);
    if(ab_ack>_next_seqno) return false;
    else if(ab_ack>=_send_base){
        //update send base and window size
        _send_base=ab_ack;
        _rcv_window_size=window_size;
        //reset rto and consecutive times
        _RTO = _initial_retransmission_timeout;
        _consecutive_retransmission_time=0;
        while(!_outstanding_segs.empty()){
            auto& last_send_seg=_outstanding_segs.front();
            //last index of the segment
            auto last_index=unwrap(last_send_seg.header().seqno,_isn,_next_seqno)+last_send_seg.length_in_sequence_space()-1;
            if(last_index<ab_ack){
                //_bytes_in_flight-=last_send_seg.length_in_sequence_space();
                _outstanding_segs.pop();
            }
            else break;
        }
        if(!_segments_out.empty()) {
            _timer_start=false;//restart timer;
        }
    }
    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _time_passed+=ms_since_last_tick;
    // here judge timeout
    if(!_timer_start) return;
    size_t duration=_time_passed-_timer;
    if(duration>=_RTO){//time out
        assert(!_outstanding_segs.empty());
        TCPSegment& retran_seg=_outstanding_segs.front();
        _segments_out.push(retran_seg);
        if(_rcv_window_size>0){
            _timer=_time_passed;//restart_timer
            _RTO*=2;//报告里面说窗口size>0 才double???
        }
        _consecutive_retransmission_time+=1;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission_time; }

void TCPSender::send_empty_segment() {
    TCPSegment seg{};
    seg.header().seqno=wrap(_next_seqno,_isn);
    _segments_out.push(seg);
    if(!_timer_start){
        _timer_start=true;
        _timer=_time_passed;
    }
}
