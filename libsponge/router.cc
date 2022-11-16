#include "router.hh"
#include <iostream>

using namespace std;

// 向路由表添加一个路由
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    _router_table.push_back({route_prefix, prefix_length, next_hop, interface_num});
}

// 从路由器合适的接口发送数据报给下一跳 dgram 将被路由的数据报
void Router::route_one_datagram(InternetDatagram &dgram) {
    // 如果数据报TTL为0或递减后为0，丢弃数据报
    if(dgram.header().ttl <= 1){
        return;
    }
    // 递减数据报的TTL
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

// 浏览路由器的所有的接口，并将每个传入的IPv4数据报路由到其适当的传出接口
void Router::route() {
    // 遍历路由器的网络接口
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        // 如果网络接口收到了IPv4数据包，则将发送给合适的下一跳接口
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
