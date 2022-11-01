#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`


//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// route_prefix 用于匹配数据报目标地址的“至多32位”IPv4前缀
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
// prefix_length 需要匹配数据报目标地址的前缀位数
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
// next_hop 下一跳的IP地址。如果网络直接连接到路由器，下一跳地址应该是数据报的最终目的地
//! \param[in] interface_num The index of the interface to send the datagram out on.
// interface_num 发送数据报的接口的索引
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    _router_table.push_back({route_prefix, prefix_length, next_hop, interface_num});
}

//! \param[in] dgram The datagram to be routed
// dgram 将被路由的数据报
void Router::route_one_datagram(InternetDatagram &dgram) {
    // 如果数据报ttl为0或递减后为0，丢弃数据报
    if(dgram.header().ttl <= 1){
        return;
    }
    dgram.header().ttl-- ;

    // 根据最长前缀匹配原则为数据报匹配合适的路由
    int index = -1;
    uint8_t max_prelength = 0;
    for(size_t i = 0; i < _router_table.size(); i++){
        // 1.根据prefix_length生成掩码
        uint32_t mask  = (_router_table[i].prefix_length == 0) ? 0 : 0xffffffff << (32 - _router_table[i].prefix_length);

        // 2.判断路由是否匹配以及是否是最长匹配
        if(((dgram.header().dst & mask) == _router_table[i].route_prefix) && (_router_table[i].prefix_length >= max_prelength)){
            index = i;
            max_prelength = _router_table[i].prefix_length;
        }
    }

    // 如果没有匹配路由，丢弃数据报
    if(index == -1){
        return;
    }

    // 根据匹配结果转发数据报
    if (!_router_table[index].next_hop.has_value()) {
        // 如果next_hop为空，直接发给目的ip地址
        interface(_router_table[index].interface_num).send_datagram(dgram, Address::from_ipv4_numeric(dgram.header().dst));
    } else {
        // 如果next_hop不为空，发给next_hop
        interface(_router_table[index].interface_num).send_datagram(dgram, _router_table[index].next_hop.value());
    }

}


void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    // 浏览所有的接口，并将每个传入的数据报路由到其适当的传出接口
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
