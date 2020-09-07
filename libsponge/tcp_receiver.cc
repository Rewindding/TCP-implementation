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
    if(!_syn_received){
        if(seg.header().syn){
            _syn_received=true;
            _isn=seqno;
            return true;
        }
        else return false;
    }
    else if(seg.header().syn) {return false;}//duplicate
    uint64_t absolute_seqno=unwrap(seqno,_isn,_checkpoint);
    size_t stream_index=absolute_seqno-1;
    size_t rcv_base=_reassembler.get_rcv_base();
    
    if(rcv_base+_capacity<=stream_index){
        //overflow!
        return false;
    }
    //write data
    string data=seg.payload().copy();
    _reassembler.push_substring(data,stream_index,eof);
    //update checkpoint
    _checkpoint=max(_checkpoint,absolute_seqno);
    //update next_seq how?
    _next_seq=max(_next_seq,_reassembler.get_rcv_base());
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!_syn_received) return {};
    else return wrap(_next_seq,_isn); 
}

size_t TCPReceiver::window_size() const { return {}; }
