#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"


//! \brief A complete endpoint of a TCP connection
class TCPConnection {
  private:
    TCPConfig _cfg;
    TCPReceiver _receiver{_cfg.recv_capacity};
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};
    // TCPConnection希望发送的段的出站队列
    std::queue<TCPSegment> _segments_out{};
    // 用于判断是主动关闭还是被动关闭
    bool _linger_after_streams_finish{true};
    // 标志TCPConnection是否处于激活状态
    bool _active{true};
    // 自从接收到最后一个子串经历的毫秒数 
    uint64_t _linger_time{0};


  public:
    //! \name "Input" interface for the writer
    //!@{

    //! \brief Initiate a connection by sending a SYN segment
    // 通过发送SYN段来启动一个连接
    void connect();
    // 通过四次挥手断开连接
    void clean_close();
    // 直接发送RST断开连接
    void unclean_close();
    
    //! \brief Write data to the outbound byte stream, and send it over TCP if possible
    //! \returns the number of bytes from `data` that were actually written.
    // 向出站流写入数据并通过TCP发送，返回实际写入的字节数
    size_t write(const std::string &data);

    //! \returns the number of `bytes` that can be written right now.
    // 返回TCPSender现在可以写入的字节数量
    size_t remaining_outbound_capacity() const;

    //! \brief Shut down the outbound byte stream (still allows reading incoming data)
    // 关闭出站的字节流（仍然允许读取进入的数据）
    void end_input_stream();
    //!@}

    //! \name "Output" interface for the reader
    //!@{

    //! \brief The inbound byte stream received from the peer
    // 从另一端收到的入站字节流
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
    //!@}

    //! \name Accessors used for testing

    //!@{
    //! \brief number of bytes sent and not yet acknowledged, counting SYN/FIN each as one byte
    // 已发送但尚未确认的字节数，SYN/FIN各算作一个字节
    size_t bytes_in_flight() const;
    //! \brief number of bytes not yet reassembled
    // 尚未重新组装的字节数
    size_t unassembled_bytes() const;
    //! \brief Number of milliseconds since the last segment was received
    // 收到最后一个片段后的毫秒数
    size_t time_since_last_segment_received() const;
    //!< \brief summarize the state of the sender, receiver, and the connection
    // 总结发送方、接收方和连接的状态
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    //!@}

    //! \name Methods for the owner or operating system to call
    //!@{

    //! Called when a new segment has been received from the network
    // 当从网络中收到一个新的段时被调用
    void segment_received(const TCPSegment &seg);

    void send_segment();

    // 计时
    void tick(const size_t ms_since_last_tick);

    //! \brief TCPSegments that the TCPConnection has enqueued for transmission.
    //! \note The owner or operating system will dequeue these and
    //! put each one into the payload of a lower-layer datagram (usually Internet datagrams (IP),
    //! but could also be user datagrams (UDP) or any other kind).
    // TCPconnection已经排队等待传输的TCPsegment
    // 将其放在下层数据报的有效载荷中
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief Is the connection still alive in any way?
    //! \returns `true` if either stream is still running or if the TCPConnection is lingering
    //! after both streams have finished (e.g. to ACK retransmissions from the peer)
    // 判断TCPconnection是否还处于激活状态
    bool active() const;
    //!@}

    //! Construct a new connection from a configuration
    // 构造函数
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    //! \name construction and destruction
    //! moving is allowed; copying is disallowed; default construction not possible

    //!@{
    ~TCPConnection();  //!< destructor sends a RST if the connection is still open
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH
