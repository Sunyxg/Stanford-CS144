#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface 物理地址
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface ip地址
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
// dgram 要发送的ip数据包
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
// next_hop 要发送接口的ip地址
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    // 将下一跳的ip地址转换为原始的32位表示法
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    // 判断next_hop_ip是否在arp表中
    optional<EthernetAddress> ethadd = find_addr(next_hop_ip);
    
    EthernetFrame frame;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_IPv4;
    frame.payload() = dgram.serialize(); 
    // 找到了ip地址对应的物理地址
    if(ethadd.has_value()){
        frame.header().dst = ethadd.value();
        frames_out().push(frame);
    }else{
        // 使用ARP协议查询下一跳物理地址
        auto iter = arp_time.find(next_hop_ip);
        if(iter == arp_time.end()){
            send_arp(next_hop_ip,true);
            arp_time.insert({next_hop_ip,timer});
        }
        _frames_wait.insert({next_hop_ip,frame});
    }

}

//! \param[in] frame the incoming Ethernet frame
// frame 以太网帧的物理地址来源
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // 只接收目的地址为本接口或为广播地址的帧
    if(frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST){
        return {};
    }

    // 如果帧类型为IPv4，则返回数据报
    if(frame.header().type == EthernetHeader::TYPE_IPv4 ){
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) != ParseResult::NoError) {
            return {} ;
        }
        return dgram;
    }

    if(frame.header().type == EthernetHeader::TYPE_ARP){
        // 更新网络接口的ARP表
        ARPMessage arpm;
        if (arpm.parse(frame.payload()) != ParseResult::NoError) {
            return {} ;
        }
        
        // 修改arp_map
        arp_map[arpm.sender_ip_address].ethaddr = arpm.sender_ethernet_address;
        arp_map[arpm.sender_ip_address].s_time = timer;

        // 若等待的ip数据报获得其对应的物理地址，则发送他们
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
        

        // 判断是否为ARP请求并回复 // 是否需要进一步判断ip地址（代理ARP）
        if(arpm.opcode == ARPMessage::OPCODE_REQUEST && arpm.target_ip_address == _ip_address.ipv4_numeric()){
            // 向来源地址发送ARPreply报文
            send_arp(arpm.sender_ip_address,false);
        }
    }
    
    return {};
}



// ARP报文发送，flag用于分辨是请求报文还是回复报文
void NetworkInterface::send_arp(const uint32_t ip_addr , const bool flag){
    EthernetFrame frame;
    frame.header().src = _ethernet_address;
    frame.header().type = EthernetHeader::TYPE_ARP;
    ARPMessage arpm;
    arpm.sender_ethernet_address = _ethernet_address;
    arpm.sender_ip_address = _ip_address.ipv4_numeric();
    arpm.target_ip_address = ip_addr;

    // ARP请求报文
    if(flag){
        frame.header().dst = ETHERNET_BROADCAST;
        arpm.opcode = 1;
        arpm.target_ethernet_address = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    }else{
        optional<EthernetAddress> ethadd = find_addr(ip_addr);
        frame.header().dst = ethadd.value();
        arpm.opcode = 2;
        arpm.target_ethernet_address = ethadd.value();
    }

    // 拼接并发送frame
    frame.payload() = arpm.serialize(); 
    frames_out().push(frame);

}

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


//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    // 计时
    timer += ms_since_last_tick;

    // 清除arp_map过期内容
    for(auto iter = arp_map.begin(); iter != arp_map.end(); ){
        if((timer - iter->second.s_time) >= 30000){
            iter = arp_map.erase(iter);
        }else{
            iter++;
        }
    }

    // 清除arp_time过期内容
    for(auto iter = arp_time.begin(); iter != arp_time.end(); ){
        if((timer - iter->second) >= 5000){
            iter = arp_time.erase(iter);
        }else{
            iter++;
        }
    }

}
