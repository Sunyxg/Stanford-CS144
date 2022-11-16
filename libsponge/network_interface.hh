#ifndef SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
#define SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH

#include "ethernet_frame.hh"
#include "tcp_over_ip.hh"
#include "tun.hh"

#include <map>
#include <optional>
#include <queue>


//! \brief A "network interface" that connects IP (the internet layer, or network layer)
//! with Ethernet (the network access layer, or link layer).

//! This module is the lowest layer of a TCP/IP stack
//! (connecting IP with the lower-layer network protocol,
//! e.g. Ethernet). But the same module is also used repeatedly
//! as part of a router: a router generally has many network
//! interfaces, and the router's job is to route Internet datagrams
//! between the different interfaces.

//! The network interface translates datagrams (coming from the
//! "customer," e.g. a TCP/IP stack or router) into Ethernet
//! frames. To fill in the Ethernet destination address, it looks up
//! the Ethernet address of the next IP hop of each datagram, making
//! requests with the [Address Resolution Protocol](\ref rfc::rfc826).
//! In the opposite direction, the network interface accepts Ethernet
//! frames, checks if they are intended for it, and if so, processes
//! the the payload depending on its type. If it's an IPv4 datagram,
//! the network interface passes it up the stack. If it's an ARP
//! request or reply, the network interface processes the frame
//! and learns or replies as necessary.
class NetworkInterface {
  private:
    // 网络接口的物理地址
    EthernetAddress _ethernet_address;
    // 网络接口的IP地址
    Address _ip_address;
    // 网络接口的计时器
    size_t timer = 0;
    // 封装物理地址和ARP表记录的创建时间
    struct Ethaddr_time{
      EthernetAddress ethaddr;
      size_t s_time;
    };
    // 网络接口待发送以太网帧的序列
    std::queue<EthernetFrame> _frames_out{};
    // 等待队列用于存放等待ARP响应报文的IP报文
    std::multimap<uint32_t, EthernetFrame> _frames_wait{};
    // 网络接口维护的地址解析协议(ARP)表< ip_addr , ethernet_addr , s_time>
    // 记录了网络接口已知的ip_addr对应的ethernet_add以及此记录被创建的初始时间
    std::map<uint32_t, Ethaddr_time> arp_map{};
    // 存放5s内已经发送了ARP请求报文的IP地址和发送ARP请求的时间
    std::map<uint32_t, size_t> arp_time{};
  
  public:
    //! \brief Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer) addresses
    // 通过给定的物理地址和ip地址构建一个网络接口
    NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address);

    //! \brief Access queue of Ethernet frames awaiting transmission
    // 等待传输的以太网帧队列
    std::queue<EthernetFrame> &frames_out() { return _frames_out; }

    //! \brief Sends an IPv4 datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination address).
    // 发送一个IPv4数据报，封装在一个以太网帧中（如果它知道以太网目标地址）
    //! Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address for the next hop
    //! ("Sending" is accomplished by pushing the frame onto the frames_out queue.)
    // 将需要使用ARP协议来查询下一跳的以太网目的地址（发送时通过将帧推入frames_out来实现的）
    void send_datagram(const InternetDatagram &dgram, const Address &next_hop);

    void send_arp(const uint32_t ip_addr , const bool flag);
    //! \brief Receives an Ethernet frame and responds appropriately.
    // 接收一个以太网帧并作出回应
    //! If type is IPv4, returns the datagram.
    // 如果类型是IPv4，返回数据报
    //! If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
    // 如果类型是ARP请求，从sender学习一个映射，并发送ARP回复
    //! If type is ARP reply, learn a mapping from the "sender" fields.
    // 如果类型是ARP回复，从sender学习一个映射
    std::optional<InternetDatagram> recv_frame(const EthernetFrame &frame);

    std::optional<EthernetAddress> find_addr( const uint32_t ip_addr);
    //! \brief Called periodically when time elapses
    void tick(const size_t ms_since_last_tick);
};

#endif  // SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
