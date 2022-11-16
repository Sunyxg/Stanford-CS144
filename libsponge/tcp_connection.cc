#include "tcp_connection.hh"
#include <iostream>

using namespace std;

// 返回TCPSender现在可以写入的字节数量
size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity();  }
// 返回TCPSender已发送但还未被确认接收的字节数量
size_t TCPConnection::bytes_in_flight() const { return {_sender.bytes_in_flight()}; }
// 返回TCPReceiver中已被接收但还未重组的字节数量
size_t TCPConnection::unassembled_bytes() const { return {_receiver.unassembled_bytes()}; }
// 返回自从接收到最后一个子串经历的毫秒数 
size_t TCPConnection::time_since_last_segment_received() const { return _linger_time; }
// 判断TCPconnection是否还处于激活状态
bool TCPConnection::active() const { return _active; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    // 如果TCPConnection未激活则不接收任何TCPSegment
    if(!active()){
        return;
    } 

    // 如果接收到RST数据包直接断开TCPConnection
    if(seg.header().rst){
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        _active = false;
        return;
    }
    
    // 将数据包传递给TCPReceiver
    _receiver.segment_received(seg);
    // 如果ackno没值，说明未收到SYN
    if(!_receiver.ackno().has_value()){
        return;
    }
    // 每次接收到新TCPSegment重置_linger_time
    _linger_time = 0;
    // 如果ACK为真,将ackno和window_size传递给TCPSender
    if(seg.header().ack){
        _sender.ack_received(seg.header().ackno,seg.header().win);        
    }
    // TCPSender填充处于接收方窗口内的字节流
    _sender.fill_window();

    // 如果收到了包含数据(有SYN,FIN或payload)的Segment且TCPSender没有待发送数据
    // 要发送空数据包用于返回ACK和window_size
    if(seg.length_in_sequence_space() > 0){
        if(_sender.segments_out().empty()){
            _sender.send_empty_segment();
        }
    }
    // 给“keep-alive” Segment回复消息
    if((seg.length_in_sequence_space() == 0) && (seg.header().seqno == (_receiver.ackno().value() - 1))){
        _sender.send_empty_segment();
    }
    // 发送TCPSender中待发送的数据包
    send_segment();
}

// TCPConnection发送TCPSegment
void TCPConnection::send_segment(){
    TCPSegment s;
    // 当TCPSender中待发送Segment不为空时
    while(!_sender.segments_out().empty()){
        // 将取出TCPSender中待发送的TCPSegment
        s = _sender.segments_out().front();
        // 如果TCPReceiver的ackno有值
        if(_receiver.ackno().has_value()){
            // 写入TCPReceiver当前的ackno和window_size，并设置其ACK标志
            s.header().ack = true;
            s.header().ackno = _receiver.ackno().value();
            s.header().win = _receiver.window_size();
        }
        // 将TCPSegment放入TCPConnection的发送队列        
        _segments_out.push(s); 
        _sender.segments_out().pop();
    }
    // 判断是否应当断开TCPConnection
    clean_close();
}

// 向TCPConnection的输出字节流写入数据并发送，返回实际写入的字节数
size_t TCPConnection::write(const string &data) {
    size_t num = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segment();
    return num;
}

// 计时函数，参数为为自上次调用此方法经过的毫秒数
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // 只有当TCPConnection处于激活状态才进行计时
    if(!active()){
        return;
    } 
    // 通知TCPSender计时
    _sender.tick(ms_since_last_tick);
    // 更新_linger_time
    _linger_time += ms_since_last_tick;
    // 如果重传次数大于最大重传次数，RST断开TCP连接
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        unclean_close();
        return;
    }
    // 重传待发送字符串    
    send_segment();    
}

// 关闭TCPSender的出站字节流(TCPReceiver仍能进行读取)
void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segment();
}

// 通过发送SYN段来建立一个TCP连接
void TCPConnection::connect() {
    _sender.fill_window();
    send_segment();
}

// 发送RST断开连接
void TCPConnection::unclean_close(){
    // 将TCPConnection的输入输出字节流设置为error状态
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    // 向对方发送RST通知其关闭TCPConnection
    TCPSegment s ;
    s.header().rst = true;
    _segments_out.push(s);
    // 关闭自己的TCPConnection
    _active = false;
}

// 正常四次挥手断开连接
void TCPConnection::clean_close(){
    // 条件一：接收端输入ByteStream字节流已接收完毕
    bool condition1 = _receiver.stream_out().eof();
    // 条件二：发送端待发送字节流已完全发送
    bool condition2 = _sender.fin_sent();
    // 条件三：发送端发出的字节流已被对方接收完毕
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

// 析构函数，如果TCPConnection处于活跃状态则断开连接
TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            unclean_close();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
