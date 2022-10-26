#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.


using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t i = n & 0xFFFFFFFF;
    uint32_t r = i+isn.raw_value();
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
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    WrappingInt32 m = wrap(checkpoint, isn);
    uint32_t mm = m.raw_value();
    uint32_t nn = n.raw_value();
    uint32_t offset = nn-mm;
    uint64_t ret;
    if(offset < (1u<<31) || (checkpoint < ((1ul<<32) - offset))){
        ret = checkpoint+offset;
    }else{
        ret = checkpoint-(1ul<<32) + offset;
    }
    
    return ret;
}
