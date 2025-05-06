#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"

#include <iostream>
#include <stdexcept>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

void NetworkInterface::_try_set_cache_if_need(const size_t &ip_address, EthernetAddress &ether_address) {
    if (to_string(ether_address) != to_string(ETHERNET_BROADCAST) &&
        to_string(ether_address) != to_string(EthernetAddress{})) {
        _ip_mac_map.set_cache(ip_address, ether_address, _ms_since_last_tick, _expire_time);
    }
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    auto mac_cache = _ip_mac_map.get_cache(next_hop_ip);
    EthernetFrame ether_frame = {};
    ether_frame.header().src = _ethernet_address;
    if (mac_cache.has_value()) {
        // Already have next_hop_ip's MAC address's cache. Just send the entrie datagram directly
        ether_frame.header().type = EthernetHeader::TYPE_IPv4;
        ether_frame.header().dst = *mac_cache.value();
        ether_frame.payload() = dgram.serialize().concatenate();
    } else {
        // Next_hop_ip's MAC address's cache not exist, we should send ARP broadcast.
        // If last arp request is within 5 second, just ignore it.
        if (_arp_ip_queue.find(_ip_address.ipv4_numeric()) != _arp_ip_queue.end() &&
            _arp_ip_queue[_ip_address.ipv4_numeric()].first >= _ms_since_last_tick) {
            return;
        }
        ARPMessage arp_meg = {};
        arp_meg.sender_ip_address = _ip_address.ipv4_numeric();
        arp_meg.sender_ethernet_address = _ethernet_address;
        arp_meg.target_ip_address = next_hop.ipv4_numeric();
        arp_meg.opcode = ARPMessage::OPCODE_REQUEST;

        ether_frame.payload() = arp_meg.serialize();
        ether_frame.header().dst = ETHERNET_BROADCAST;
        ether_frame.header().type = EthernetHeader::TYPE_ARP;
        // Keep current ip datagram to cache queue.
        _arp_ip_queue[_ip_address.ipv4_numeric()] =
            std::make_pair(_ms_since_last_tick + (5 * 1000), std::make_shared<InternetDatagram>(std::move(dgram)));
    }
    _frames_out.push(ether_frame);
    return;
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    std::optional<InternetDatagram> result = {};
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        // If frame's destination is not the current node, ignore it.
        if (to_string(frame.header().dst) != to_string(_ethernet_address)) {
            return result;
        }
        // If current frame is already ipv4, just parse the ip datagram and pass it over.
        InternetDatagram ip_datagram = {};
        if (ip_datagram.parse(frame.payload().concatenate()) != ParseResult::NoError) {
            throw std::runtime_error("ip datagram parse error.");
        }
        result.emplace(ip_datagram);
    } else if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage get_msg = {};
        if (get_msg.parse(frame.payload().concatenate()) != ParseResult::NoError) {
            throw std::runtime_error("ARP Message parse error.");
        }
        EthernetFrame ether_frame = {};
        if (get_msg.opcode == ARPMessage::OPCODE_REQUEST) {
            auto mac_cache = _ip_mac_map.get_cache(get_msg.target_ip_address);
            // if cache is empty, but the target ip address is current node, just return it.
            if (!mac_cache.has_value() && get_msg.target_ip_address == _ip_address.ipv4_numeric()) {
                EthernetAddress target_add = _ethernet_address;
                mac_cache.emplace(std::make_shared<EthernetAddress>(std::move(target_add)));
            }

            // Set up ARP reply to the original sender.
            // Note that the orignal sender is the target right now.
            if (mac_cache.has_value()) {
                ARPMessage final_arp = {};
                final_arp.target_ip_address = get_msg.sender_ip_address;
                final_arp.target_ethernet_address = get_msg.sender_ethernet_address;
                final_arp.sender_ethernet_address = *mac_cache.value();
                final_arp.sender_ip_address = get_msg.target_ip_address;
                final_arp.opcode = ARPMessage::OPCODE_REPLY;
                ether_frame.header().dst = get_msg.sender_ethernet_address;
                ether_frame.header().src = _ethernet_address;
                ether_frame.header().type = EthernetHeader::TYPE_ARP;
                ether_frame.payload() = final_arp.serialize();
                _frames_out.push(ether_frame);
            }

            // We should try to cache both sender and target's ethernet address if those address are valid.
            _try_set_cache_if_need(get_msg.sender_ip_address, get_msg.sender_ethernet_address);
            _try_set_cache_if_need(get_msg.target_ip_address, get_msg.target_ethernet_address);
        } else if (get_msg.opcode == ARPMessage::OPCODE_REPLY) {
            // If previously we already have not sent ip datagram, we should send it right now.
            if (_arp_ip_queue.find(get_msg.target_ip_address) != _arp_ip_queue.end()) {
                auto ip_datagram = _arp_ip_queue[get_msg.target_ip_address];
                ether_frame.header().dst = get_msg.sender_ethernet_address;
                ether_frame.header().src = get_msg.target_ethernet_address;
                ether_frame.header().type = EthernetHeader::TYPE_IPv4;
                ether_frame.payload() = ip_datagram.second->serialize().concatenate();
                _frames_out.push(ether_frame);

                _arp_ip_queue.erase(get_msg.target_ip_address);
            }
            // We should try to cache both sender and target's ethernet address if those address are valid.
            _try_set_cache_if_need(get_msg.sender_ip_address, get_msg.sender_ethernet_address);
            _try_set_cache_if_need(get_msg.target_ip_address, get_msg.target_ethernet_address);
        }
    }
    return result;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    _ms_since_last_tick += ms_since_last_tick;
    _ip_mac_map.try_clean_expire_cache(_ms_since_last_tick);
    return;
}

void IP2EthernetCache::set_cache(const uint32_t ip_address,
                                 EthernetAddress &ether_adderss,
                                 const size_t ms_since_last_tick,
                                 const size_t expire_time) {
    if (_ip_mac_cache.find(ip_address) != _ip_mac_cache.end()) {
        for (auto it = _cach_expire_queue.begin(); it < _cach_expire_queue.end(); it++) {
            if (it->second == ip_address) {
                _cach_expire_queue.erase(it);
                break;
            }
        }
    }
    _ip_mac_cache[ip_address] = std::make_shared<EthernetAddress>(std::move(ether_adderss));
    _cach_expire_queue.push_back(std::make_pair(ms_since_last_tick + expire_time, ip_address));
}

std::optional<std::shared_ptr<EthernetAddress>> IP2EthernetCache::get_cache(const uint32_t ip_address) {
    std::optional<std::shared_ptr<EthernetAddress>> result = {};
    if (_ip_mac_cache.find(ip_address) != _ip_mac_cache.end()) {
        result.emplace(_ip_mac_cache[ip_address]);
    }
    return result;
}

void IP2EthernetCache::try_clean_expire_cache(size_t current_time) {
    while (!_cach_expire_queue.empty()) {
        auto &front = _cach_expire_queue.front();
        if (front.first < current_time) {
            _ip_mac_cache.erase(front.second);
            _cach_expire_queue.pop_front();
        } else {
            return;
        }
    }
    return;
}