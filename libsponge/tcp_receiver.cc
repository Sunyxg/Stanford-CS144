#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

// 未考虑超出窗口情况！！！！
void TCPReceiver::segment_received(const TCPSegment &seg) {
    
    // 
    if(seg.header().syn){
        if(_syn){
            return ;
        }
        _syn = true;
        _isn = seg.header().seqno.raw_value();
        _begin = seg.header().seqno.raw_value();
        
    }else{
        _begin = seg.header().seqno.raw_value() - 1;
    }

    if((!_syn)){
        return ;
    }

    size_t len = seg.length_in_sequence_space() - seg.header().syn - seg.header().fin;
    size_t index = unwrap(WrappingInt32(_begin) , WrappingInt32(_isn), _checkp);
    if(((index + len) < _checkp) || (index) >= (_checkp + window_size())){
        return ;
    }
    _reassembler.push_substring(seg.payload().copy(),index,seg.header().fin);      
    _checkp = stream_out().bytes_written();

    if(stream_out().input_ended()){
        ack = wrap((_checkp+2),WrappingInt32(_isn));
        _syn = false; 
        _checkp = 0;  
        _begin = 0;     
    }else{
        ack = wrap((_checkp+1),WrappingInt32(_isn));
    }
    return ;
}

optional<WrappingInt32> TCPReceiver::ackno() const { return ack; }

size_t TCPReceiver::window_size() const { return {stream_out().remaining_capacity()}; }
