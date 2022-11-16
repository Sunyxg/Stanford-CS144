#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.


using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
// 将ab_seqno转换为seqno
// 其中n为待转换的ab_seqno，isn为初始序列号
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // 将64位ab_seqno与0xFFFFFFFF向与，相当于对2^32取余
    uint32_t i = n & 0xFFFFFFFF;
    // 将其与isn的32位值相加，溢出则进入下一轮循环
    uint32_t r = i+isn.raw_value();
    // 封装为WrappingInt32
    return WrappingInt32{r};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
// 将seqno转换为ab_seqno  
// 其中n为待转换的seqno，isn为初始序列号，checkpoint为此次转换的参考点
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // 将checkpoint转换为其对应的参考seqno
    WrappingInt32 m = wrap(checkpoint, isn);
    // 计算seqno到参考seqno的偏移offset
    uint32_t mm = m.raw_value();
    uint32_t nn = n.raw_value();
    uint32_t offset = nn-mm;
    uint64_t ret;
    // 寻找并返回满足条件的n对应的最接近checkpoint的值
    // 当offset小于2^31或从checkpoint开始向下的偏移量不足时返回checkpoint+offset
    if(offset < (1u<<31) || (checkpoint < ((1ul<<32) - offset))){
        ret = checkpoint+offset;
    }else{
        // 当offset大于等于2^31且从checkpoint开始向下的偏移量足够时返回checkpoint-(1ul<<32) + offset
        ret = checkpoint-(1ul<<32) + offset;
    }
    return ret;
}
