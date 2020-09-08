#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    WrappingInt32 seqno=seg.header().seqno;
    bool eof=seg.header().fin;
    //segment with syn or fin may have payload also
    if(!_syn_received){
        if(seg.header().syn){
            _syn_received=true;
            _isn=seqno;
            if(seg.header().fin&&seg.length_in_sequence_space()==2){//一个只有fin和syn的空报文
                _reassembler.stream_out().end_input();
                return true;
            }
        }
        else return false;
    }
    else if(seg.header().syn) {return false;}//duplicate
    //absolute index 数据报文的起始字节编号和终止字节编号，包含fin和syn
    auto seqno_start=unwrap(seg.header().seqno,_isn,_checkpoint);
    auto seqno_end=seqno_start+max(seg.length_in_sequence_space()-1,static_cast<size_t>(0));
    auto window_start=unwrap(_ack,_isn,_checkpoint); 
    //window size==0的时候需要发送方持续发送数据，直到窗口size不等于0
    auto window_end=window_start+max(window_size()-1,static_cast<size_t>(0));//if window size==0?
    //determine if it's out of range
    bool fall_in_window=(seqno_start<=window_end&&seqno_start>=window_start)||(seqno_end<=window_end&&seqno_end>=window_start);
    if(!fall_in_window) return false;
    //write data
    _reassembler.push_substring(seg.payload().copy(),seqno_start-1+(seg.header().syn),eof);//if seq_start==0?
    //update _checkpoint and ack
    _checkpoint=_reassembler.stream_out().bytes_written();
    bool end_input=_reassembler.stream_out().input_ended();
    int offset=(end_input?1:0)+(_syn_received?1:0);
    _ack=wrap(_reassembler.stream_out().bytes_written()+offset,_isn);
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!_syn_received) return {};
    else{
        return _ack;
    }
}

size_t TCPReceiver::window_size() const {
    return _capacity-_reassembler.stream_out().buffer_size();
}
