#include "network_interface.hh"
#include "arp_message.hh"
#include "ethernet_frame.hh"
#include <iostream>

using namespace std;

//! \param[in] ethernet_address  物理地址
//! \param[in] ip_address  IP地址
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

// 将IP数据包封装在以太网帧中发送给其要发送的IP地址接口
// dgram: 要发送的ip数据包  next_hop: 要发送接口的IP地址
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // 将下一跳的ip地址转换为原始的32位表示法
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    // 在ARP表中查找next_hop_ip
    optional<EthernetAddress> ethadd = find_addr(next_hop_ip);
    
    // 将待发送的IP数据报封装为以太网帧
    EthernetFrame frame;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_IPv4;
    frame.payload() = dgram.serialize(); 
    // 如果ARP表中包含了要发送接口的IP地址
    if(ethadd.has_value()){
        // 填写以太网帧首部的目的物理地址，并将封装好的以太网帧放入待发送队列
        frame.header().dst = ethadd.value();
        frames_out().push(frame);
    }else{
        // 如果ARP表中不包含要发送接口的IP地址，则应发送ARP请求报文
        // 在已发送ARP请求报文的队列中查找发送接口的IP地址
        auto iter = arp_time.find(next_hop_ip);
        // 如果5s内没有发送过该IP地址的ARP请求报文，则发送ARP请求报文，并记录发送的时间
        if(iter == arp_time.end()){
            send_arp(next_hop_ip,true);
            arp_time.insert({next_hop_ip,timer});
        }
        // 将封装好的以太网帧放入等待队列
        _frames_wait.insert({next_hop_ip,frame});
    }

}

// 接收一个以太网帧并作出回应  frame: 以太网帧的物理地址来源
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // 只接收以太网帧首部中目的地址为本接口或广播地址的帧
    if(frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST){
        return {};
    }

    // 如果以太网帧类型为IPv4数据报，则返回数据报
    if(frame.header().type == EthernetHeader::TYPE_IPv4 ){
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) != ParseResult::NoError) {
            return {} ;
        }
        return dgram;
    }

    // 如果以太网帧类型为ARP数据报，则应更新网络接口的ARP表，并判断是否需要响应
    if(frame.header().type == EthernetHeader::TYPE_ARP){
        // 判断是否能成功解析出ARP报文
        ARPMessage arpm;
        if (arpm.parse(frame.payload()) != ParseResult::NoError) {
            return {} ;
        }
        // 更新网络接口的ARP表(更新ip_addr对应的物理地址和更新时间)
        arp_map[arpm.sender_ip_address].ethaddr = arpm.sender_ethernet_address;
        arp_map[arpm.sender_ip_address].s_time = timer;

        // 若处于等待的IP数据报获得其对应的物理地址，则发送他们
        if(!_frames_wait.empty()){
            EthernetFrame fra;
            for(auto iter = _frames_wait.begin(); iter != _frames_wait.end(); ){
                if(iter->first == arpm.sender_ip_address){
                    fra = iter->second;
                    fra.header().dst = arpm.sender_ethernet_address;
                    frames_out().push(fra);
                    iter = _frames_wait.erase(iter);
                }else{
                    iter ++;
                }
            }
        }
        
        // 如果接收到的是ARP请求，且请求的目的IP地址为当前接口的IP地址，则发送ARP响应报文
        if(arpm.opcode == ARPMessage::OPCODE_REQUEST && arpm.target_ip_address == _ip_address.ipv4_numeric()){
            // 向来源地址发送ARPreply报文
            send_arp(arpm.sender_ip_address,false);
        }
    }
    
    return {};
}

// 向ip_addr发送一个ARP报文，flag用于分辨是请求报文还是回复报文
void NetworkInterface::send_arp(const uint32_t ip_addr , const bool flag){
    // 创建ARP报文的以太网帧
    EthernetFrame frame;
    // 设置以太网帧的源地址为当前物理地址
    frame.header().src = _ethernet_address;
    // 设置以太网帧的类型为ARP报文类型
    frame.header().type = EthernetHeader::TYPE_ARP;
    // 创建ARP报文
    ARPMessage arpm;
    // 设置发送端的IP地址和物理地址为当前接口的地址
    arpm.sender_ethernet_address = _ethernet_address;
    arpm.sender_ip_address = _ip_address.ipv4_numeric();
    // 设置目的地址为传入的ip_addr
    arpm.target_ip_address = ip_addr;

    // 发送ARP请求报文
    if(flag){
        // 设置ARP请求报文以太网首部的目的物理地址为广播地址(向所有网络接口发送广播)
        frame.header().dst = ETHERNET_BROADCAST;
        // 标记为请求报文
        arpm.opcode = 1;
        // 设置ARP请求报文中的目的物理地址为0(请求报文还未知ip_addr对应的物理地址)
        arpm.target_ethernet_address = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    }else{
        // 发送ARP响应报文
        // 网络接口在自己的ARP表中查找ip_addr对应的物理地址
        optional<EthernetAddress> ethadd = find_addr(ip_addr);
        // 设置ARP响应报文以太网首部的目的物理地址为ip_addr对应的物理地址
        frame.header().dst = ethadd.value();
        // 标记为响应报文
        arpm.opcode = 2;
        // 设置ARP响应报文中的目的物理地址为ip_addr对应的物理地址
        arpm.target_ethernet_address = ethadd.value();
    }

    // 将ARP报文封装进以太网帧中并发送
    frame.payload() = arpm.serialize(); 
    frames_out().push(frame);
}

// 在当前网络接口的ARP表中查找ip_addr对应的物理地址
optional<EthernetAddress> NetworkInterface::find_addr(const uint32_t ip_addr){    
    EthernetAddress ethadd;
    for(auto iter = arp_map.begin(); iter != arp_map.end(); iter++){
        if(iter->first == ip_addr){
            ethadd = iter->second.ethaddr;
            return ethadd;
        }
    }
    return{};
}

//! 计时函数，参数为为自上次调用此方法经过的毫秒数
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    // 计时
    timer += ms_since_last_tick;

    // 清除ARP表中的过期内容，每条记录仅保持30s
    for(auto iter = arp_map.begin(); iter != arp_map.end(); ){
        if((timer - iter->second.s_time) >= 30000){
            iter = arp_map.erase(iter);
        }else{
            iter++;
        }
    }

    // 清除ARP请求队列中的过期内容，每条记录仅保持5s
    for(auto iter = arp_time.begin(); iter != arp_time.end(); ){
        if((timer - iter->second) >= 5000){
            iter = arp_time.erase(iter);
        }else{
            iter++;
        }
    }

}
