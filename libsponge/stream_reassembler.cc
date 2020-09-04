#include "stream_reassembler.hh"
#include <algorithm>
#include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),rcv_base(0),unassembled_cnt(0),last_byte_num(-16),window(_capacity,' '),received(_capacity,false){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t last=index+data.size();
    if(_capacity==65123){
        cout<<"called,data: "<<data<<" index:"<<index<<" ,eof: "<<eof<<"\n";
    }
    if(eof){
        last_byte_num=last;
        if(rcv_base==last_byte_num){
            _output.end_input();
        }//最后一个报文，可能是空的
    }
    //size_t没有负数，减法运算得到负数的时候直接溢出！！！
    if(last-1<rcv_base||index>=_capacity+rcv_base) {
        return;
    }
    
    size_t pos=max(index,rcv_base);
    size_t border=min(data.size()+index-1,rcv_base+_capacity-1);
    while(pos<=border){//capacity and data
        int p=pos%_capacity;
        if(!received[p]){
            window[p]=data[pos-index];
            received[p]=true;
            ++unassembled_cnt;
        }
        pos++;
    }
    trans_data();
    if(rcv_base==last_byte_num){
        _output.end_input();
    }   
}
void StreamReassembler::trans_data(){
    size_t pos_d=rcv_base;
    size_t border_d=rcv_base+_capacity-1;
    string str="";
    while(pos_d<=border_d){
        int p=pos_d%_capacity;
        if(received[p]){
            str+=window[p];
            received[p]=false;
        }
        else break;
        ++pos_d;
    }
    size_t writed=_output.write(str);
    for(size_t i=rcv_base+writed;i<rcv_base+str.size();++i){
        //cout<<"here1\n";
        int p=i%_capacity;
        received[p]=true;
    }
    unassembled_cnt-=writed;
    rcv_base+=writed;
}
//返回当前窗口中收到的字节总数
size_t StreamReassembler::unassembled_bytes() const { return unassembled_cnt; }

bool StreamReassembler::empty() const {return unassembled_cnt==0; }
