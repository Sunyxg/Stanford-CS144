#include "tcp_receiver.hh"
using namespace std;

// 接收端接收TCPSegment实现
void TCPReceiver::segment_received(const TCPSegment &seg) {    
    // 根据TCPReceiver状态，判断是否接收当前TCPSegment
    if(!_syn){
        // 处于LISTEN状态时，只等待接收SYN包，若在SYN包到来前就有其他数据到来，则必须丢弃
        if(seg.header().syn){
            // 接收到SYN包，则将状态转变为SYN_RECV状态
            _syn = true;
            _isn = seg.header().seqno.raw_value();
        }else{
            return;
        }
    }else{
        // 处于SYN_RECV状态时，可接收普通数据包和FIN包，若接收到其他SYN包，则必须丢弃
        if(seg.header().syn){
            return;
        }
    }

    // 接收TCPSegment中字节流的部分
    // 计算字节流的stream index
    size_t ab_seqno = unwrap(seg.header().seqno , WrappingInt32(_isn), _checkp);
    size_t index = ab_seqno - 1 + seg.header().syn;
    // 将符合条件的字符串子串写入重排窗口
    _reassembler.push_substring(seg.payload().copy(),index,seg.header().fin);   
    // 返回写入结束后第一个有待重组的字节索引  
    _checkp = stream_out().bytes_written();

    // 如果接收端已完成字节流的重排和写入工作，将TCPReceiver状态转换为FIN_RECV
    if(stream_out().input_ended()){
        // 返回接收端当前第一个未组装（重排）的字节序号(字节流接收完毕，FIN也占用一个seqno)
        ack = wrap((_checkp+2),WrappingInt32(_isn));
        _syn = false; 
        _checkp = 0;     
    }else{
        // 返回接收端当前第一个未组装（重排）的字节序号
        ack = wrap((_checkp+1),WrappingInt32(_isn));
    }
    return ;
}

//返回接收端当前第一个未组装（重排）的字节序号
optional<WrappingInt32> TCPReceiver::ackno() const { return ack; }
//返回接收端还可以组装的字节序列长度
size_t TCPReceiver::window_size() const { return {stream_out().remaining_capacity()}; }
