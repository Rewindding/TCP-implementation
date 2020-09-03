#include "stream_reassembler.hh"
#include <algorithm>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),rcv_base(0),next_seq(0),unassembled_cnt(0),last_byte_num(-16),window(_capacity,0),received(_capacity,false){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t data_end_index=index+data.size()-1;
    if(eof) last_byte_num=data_end_index;
    if(index-rcv_base>=_capacity) return;//out of window bound;
    else if(data_end_index<next_seq) return;//completely duplicate
    else if(index<=next_seq){//in order
        size_t start=next_seq-rcv_base;//first byte index of window start to write
        size_t len=min(_capacity-start,data.size());//max bytes that could be writed
        string ordered_seq;
        for(size_t i=0;i<start;++i){
            ordered_seq+=window[i];
        }
        for(size_t i=start,k=0;k<len;++i,++k){
            if(!received[i]){
                window[i]=data[k];
                received[i]=true;
                ++unassembled_cnt;
            }
        }
        ordered_seq+=data.substr(0,len);
        size_t end=start+len-1;
        while(end+1<_capacity&&received[end+1])  {
            ordered_seq+=window[++end];
        }
        size_t writed = _output.write(ordered_seq);
        for(size_t i=0;i<writed;++i){//shrink window
            window.pop_front();
            window.push_back(' ');
            received.pop_front();
            received.push_back(false);
        }
        unassembled_cnt-=writed;
        rcv_base+=writed;
        next_seq+=len;
    }
    else{//out of order
        size_t len=min(_capacity-index,data.size());
        for(size_t i=index,k=0;k<len;++i,++k){
            if(!received[i]){
                window[i]=data[k];
                received[i]=true;
                ++unassembled_cnt;
            }
        }
    }
}
//这个的意思到底要返回个啥？是当前窗口中byte的总数，还是有序byte的总数？ 还是无序byte的总数？
size_t StreamReassembler::unassembled_bytes() const { return unassembled_cnt; }

bool StreamReassembler::empty() const {return unassembled_cnt==0; }
