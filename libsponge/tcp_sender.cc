#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.


using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _rto_base{retx_timeout}
    , _stream(capacity) 
    , RTO{retx_timeout}{}

uint64_t TCPSender::bytes_in_flight() const { return {_next_seqno - ack_s}; }

void TCPSender::fill_window() {     
    // 已发送FIN
    if(fin_send){
        return;
    }
    uint64_t end = ack_s + _window;
    while(_next_seqno < end){
        TCPSegment seg = TCPSegment(); 
        // SYN尚未发送
        if(_next_seqno == 0){
            seg.header().syn = true;
        } 
        // 设置seqno序号   
        seg.header().seqno = wrap(_next_seqno , _isn);
        // 判断最大装载数据量并装载数据
        uint64_t w_size = end - _next_seqno;
        uint64_t data_size = (TCPConfig::MAX_PAYLOAD_SIZE <= w_size) ? TCPConfig::MAX_PAYLOAD_SIZE : w_size;   
        seg.payload() = string(stream_in().read(data_size));  

        if((!fin_send) && stream_in().eof() && ((_next_seqno + seg.length_in_sequence_space()) < end)){
            seg.header().fin = true;
            fin_send = true;
        }
        
        // 没数据没SYN没FIN就退出循环
        if(seg.length_in_sequence_space() > 0){
            _next_seqno += seg.length_in_sequence_space();
            _segments_out.push(seg);
            start = true;
            _segments_noack.insert({_next_seqno, seg});
        }else{
            break;
        }       
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    // ack无效判断
    uint64_t new_ack = unwrap(ackno,_isn,_next_seqno);
    if(new_ack > _next_seqno){
        return ;
    }
    // 超时重传操作
    if(new_ack > ack_s){
        RTO = _initial_retransmission_timeout;
        retransnum = 0;
        ack_s = new_ack;  
        //剔除已被确认的Segment
        for(auto iter = _segments_noack.begin();iter != _segments_noack.end();){
            if(iter->first <= ack_s ){
                iter = _segments_noack.erase(iter);
            }else{
                iter ++;
            }
        }
    }    
    // 修正_ack和窗口大小
    if(window_size != 0){
        _window = window_size;
    }else{
        window_zero = true;
        _window = 1;
    }
    
    if(ack_s == _next_seqno){
        start = false;
    }

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if(!start){
        return;
    }
    if(RTO <= ms_since_last_tick){
        if(retransnum == 0){
            _rto_base = _initial_retransmission_timeout*2;
        }else{
            _rto_base = _rto_base*2;
        }
        RTO = window_zero ? _initial_retransmission_timeout:_rto_base;
        retransnum ++;

        //重传数据
        auto iter = _segments_noack.begin();
        if(iter != _segments_noack.end()){
            _segments_out.push(iter->second);
            start = true;
        }else{
            printf("重传为空！！！\n");
        }
    }else{
        RTO -= ms_since_last_tick;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return {retransnum}; }

void TCPSender::send_empty_segment(){
    TCPSegment seg = TCPSegment();
    seg.header().seqno = wrap(_next_seqno , _isn);
    _segments_out.push(seg);
}


