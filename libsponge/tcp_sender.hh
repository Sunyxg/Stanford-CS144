#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>
#include <map>
//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    // 随机生成的初始序列号ISN
    WrappingInt32 _isn;
    // 发送方已确认的ackno
    uint64_t ack_s = 0;
    // TCPSender要发送的Segment队列
    std::queue<TCPSegment> _segments_out{};
    // 用于记录TCPSender已发送但还未被TCPReceiver确认的TCPSegment
    // <Segment发送后的next_seqno, TCPSegment>
    std::map<uint64_t,TCPSegment> _segments_noack{};
    // TCPSender待发送字节流的缓冲区
    ByteStream _stream;
    // 下一个要发送的字节的绝对序列号
    uint64_t _next_seqno{0};
    // 接收方当前可接收窗口的大小
    uint16_t _window = 1;
    // FIN已发送标志
    bool fin_send = false;
    // 重传计时器RTO的初始值 1000
    unsigned int _initial_retransmission_timeout;
    // 重传计时器
    unsigned int time_counter = 0;
    // 重传计时器启动标志
    bool start = false;
    // 重传计时器连续重传次数
    unsigned int retransnum  = 0;
    // 当前超时重传时间
    unsigned int RTO;
  
  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    // 从接收方收到一个确认信息，
    // 包括窗口的左边缘（= ackno）和右边缘（= ackno + window size）
    // 查看其未完成的段的集合，并删除任何现在已被完全确认的段
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    // 生成并发送一个在序列空间中长度为零的TCPSegment
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    // 从ByteStream读取并TCPSegments的形式发送尽可能多的字节
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    // 将检查重传计时器是否已过期，如果是，则以最低的序列号重传未发送的段
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    // 有多少个字节已发送但尚未确认
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    // 返回连续重传的次数
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    // TCPSender中排队等待传输的TCPSegments
    // 需要匹配TCPReceiver的ack和窗口大小
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{
    // 返回下一个要发送字节的绝对序列号
    uint64_t next_seqno_absolute() const { return _next_seqno; }
    // 返回下一个要发送字节的相对序列号
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    // 返回FIN是否已经发送
    bool fin_sent() const {return fin_send;}
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
