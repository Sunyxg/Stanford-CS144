#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.


using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity();  }

size_t TCPConnection::bytes_in_flight() const { return {_sender.bytes_in_flight()}; }

size_t TCPConnection::unassembled_bytes() const { return {_receiver.unassembled_bytes()}; }

size_t TCPConnection::time_since_last_segment_received() const { return _linger_time; }

void TCPConnection::segment_received(const TCPSegment &seg) { 

    if(!active()){
        return;
    } 

    //判断RST位
    if(seg.header().rst){
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        _active = false;
        return;
    }
    
    //将seg传递给TCPreceiver
    _receiver.segment_received(seg);
    //如果ackno没值，说明未收到SYN，不理会
    if(!_receiver.ackno().has_value()){
        return;
    }

    _linger_time = 0;

    //如果ACK为真,传递给TCPsender
    if(seg.header().ack){
        _sender.ack_received(seg.header().ackno,seg.header().win);        
    }
    
    _sender.fill_window();

    // 如果收到seg且sender没有待发送数据，要发送ACK
    if(seg.length_in_sequence_space() > 0){
        if(_sender.segments_out().empty()){
            _sender.send_empty_segment();
        }
    }

    if ((seg.length_in_sequence_space() == 0) && (seg.header().seqno == (_receiver.ackno().value() - 1) )) {
        _sender.send_empty_segment();
    }
    send_segment();
}

// 发送segment
void TCPConnection::send_segment(){
    // 将sender中segment填入TCPConnection中
    TCPSegment s;
    while(!_sender.segments_out().empty()){
         s = _sender.segments_out().front();

        // 根据receiver更改ackno和wz
        if(_receiver.ackno().has_value()){
            s.header().ack = true;
            s.header().ackno = _receiver.ackno().value();
            s.header().win = _receiver.window_size();
        }        
        _segments_out.push(s); 
        _sender.segments_out().pop();
    }
    clean_close();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t num = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segment();
    return num;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if(!active()){
        return;
    } 

    _sender.tick(ms_since_last_tick); 
    _linger_time += ms_since_last_tick;

    //如果重传次数大于最大重传次数，RST
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        unclean_close();
        return;
    }
    
    send_segment();
    
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segment();
}



void TCPConnection::connect() {
    // 是否要清空byte_stream?
    _sender.fill_window();
    send_segment();
}

// RST断开连接
void TCPConnection::unclean_close(){
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    TCPSegment s ;
    s.header().rst = true;
    _segments_out.push(s);
    _active = false;
}

// 正常四次挥手断开连接
void TCPConnection::clean_close(){
    // 三个条件
    bool condition1 = _receiver.stream_out().eof();
    bool condition2 = _sender.fin_sent();
    bool condition3 = _sender.bytes_in_flight() == 0;

    // 判断主动还是被动断开连接
    if(condition1 && (!condition2) ){
        _linger_after_streams_finish = false;
    }

    // 根据断开连接方式断开连接
    if(condition1 && condition2 && condition3){
        if(_linger_after_streams_finish){
            if(_linger_time >= (10 * _cfg.rt_timeout)){
                _active = false;
            }
        }else{
            _active = false;
        }
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            // cerr << "Warning: Unclean shutdown of TCPConnection\n";
            // Your code here: need to send a RST segment to the peer
            unclean_close();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
