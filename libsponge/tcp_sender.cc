#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <random>
using namespace std;

TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _stream(capacity) 
    , _initial_retransmission_timeout{retx_timeout}
    , RTO{retx_timeout}{}


// 根据接收端返回信息，发送相应的TCPSegment
void TCPSender::fill_window() {     
    // 如果FIN已发送，则不会发送任何新的TCPSegment
    if(fin_send){
        return;
    }

    // 根据接收方确认号和窗口大小，若窗口大小为0视为窗口大小为1
    // 窗口为0时，发送携带一字节数据的零窗口探测报文
    uint64_t end = _window ? (ack_s + _window) : (ack_s + 1);
    // 循环发送落在窗口内的TCPSegment，直至窗口内数据已被发完，或已发送了FIN
    while(_next_seqno < end){
        TCPSegment seg = TCPSegment(); 
        // 当下一个要发送字节的绝对序列号为0，意味着SYN尚未发送
        if(_next_seqno == 0){
            seg.header().syn = true;
        } 
        // 设置TCPSegment的seqno   
        seg.header().seqno = wrap(_next_seqno , _isn);
        // 判断并计算TCPSegment最大可装载的字节数
        uint64_t w_size = end - _next_seqno; // 窗口中剩余未发送的字节数
        uint64_t data_size = (TCPConfig::MAX_PAYLOAD_SIZE <= w_size) ? TCPConfig::MAX_PAYLOAD_SIZE : w_size;
        // 向TCPSegment中装载数据   
        seg.payload() = string(stream_in().read(data_size));  

        // 如果FIN还未发送，且ByteStream已完全写入，且窗口中仍有空间供FIN写入
        if((!fin_send) && stream_in().eof() && ((_next_seqno + seg.length_in_sequence_space()) < end)){
            seg.header().fin = true;
            fin_send = true;
        }
        
        // 没数据没SYN没FIN就退出循环
        if(seg.length_in_sequence_space() > 0){
            // 跟新_next_seqno的位置，并将TCPSegment推入_segment_out与_segments_noack
            _next_seqno += seg.length_in_sequence_space();
            _segments_out.push(seg);
            start = true;
            _segments_noack.insert({_next_seqno, seg});
        }else{
            break;
        }       
    }
}

// 接收TCPReceiver返回的ackno和window_size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    
    // 计算收到的ackno对应的stream index
    uint64_t new_ack = unwrap(ackno,_isn,ack_s);
    // 接收到不合法的ackno(字节流尚未发送)，直接丢弃
    if(new_ack > _next_seqno){
        return ;
    }
    // 判断接收到的ackno是否是新的ackno
    if(new_ack > ack_s){
        // 接收到新的ackno，重置重传计时器
        RTO = _initial_retransmission_timeout;
        retransnum = 0;
        time_counter = 0;
        // 更新ackno
        ack_s = new_ack;  
        // 剔除_segment_noack中已被确认的Segment
        for(auto iter = _segments_noack.begin();iter != _segments_noack.end();){
            if(iter->first <= ack_s ){
                iter = _segments_noack.erase(iter);
            }else{
                iter ++;
            }
        }
    }    

    // 更新TCPReceiver的窗口大小
    _window = window_size;
    // 如果已发送的数据已全部确认，关闭重传计时器
    if(ack_s == _next_seqno){
        start = false;
    }
    return ;
}

// 计时函数，参数为为自上次调用此方法经过的毫秒数
void TCPSender::tick(const size_t ms_since_last_tick) { 
    // 如果重传计时器未启动，则直接退出
    if(!start){
        return;
    }
    // 判断是否超时
    if((time_counter + ms_since_last_tick) >= RTO){
        // 重传计时器超时
        retransnum ++;
        // 当窗口为0时，将重传时间置为初始重传时间，此时重传的是零窗口探测报文
        RTO = (!_window) ? _initial_retransmission_timeout : (_initial_retransmission_timeout*(1u << retransnum));        
        // 重传_segments_noack中保存的index最小的一个TCPSegment
        time_counter = 0;
        _segments_out.push(_segments_noack.begin()->second);
    }else{
        // 重传计时器未超时，进行计时
        time_counter += ms_since_last_tick;
    }
}

// 返回已发送但还未被确认接收的字节数量
uint64_t TCPSender::bytes_in_flight() const { return {_next_seqno - ack_s}; }

// 返回重传计时器连续重传次数
unsigned int TCPSender::consecutive_retransmissions() const { return {retransnum}; }

// 生成并发送一个在序列空间中长度为零的TCPSegment
void TCPSender::send_empty_segment(){
    TCPSegment seg = TCPSegment();
    seg.header().seqno = wrap(_next_seqno , _isn);
    _segments_out.push(seg);
}


