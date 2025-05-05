#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _last_received_time; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    // Rect last received time
    _last_received_time = 0;
    // When rst is set, ignore every thing.
    if (seg.header().rst) {
        _set_rst();
        return;
    }

    // When the connection is in-active, then we should ignore the remaining segment.
    if (!active()) {
        return;
    }

    // We have an edge case here. If local peer send the fin first, then we have to wait for some time to check if the
    // final ack message successfully sent to the remote peer. If during the wait time, another fin ack is sent, it
    // means local peer's final ack is lost. Then we have to resend another ack message. Otherwise, we should ignore
    // remaining data since the receiver has already end.
    if (_sender.is_fin_received() && _receiver.stream_out().eof()) {
        if (_send_fin_first && !seg.header().fin) {
            return;
        }
    }
    // If ack exist, we should notice the sender.
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    // Let the receiver handle the segment
    // try send to receiver anyway.
    _receiver.segment_received(seg);

    // Check if remote peer first trigger fin segment.
    if (_receiver.fsn().has_value() && !_sender.stream_in().eof()) {
        _send_fin_first = 0;
    }

    // If the receiver has received all the data, and the sender has sended all the data, and if local peer send the fin
    // first, then we meet active-close situation. Otherwise the passive-close.
    // If remote peer send the fin first, then we meet passive-close, we can close the connection after sending the
    // last directly.
    if (_sender.is_syn_received() && _receiver.stream_out().eof() && !_send_fin_first) {
        _linger_after_streams_finish = false;
    }

    // Keep-Alive:
    if (_receiver.ackno().has_value() && seg.length_in_sequence_space() == 0 &&
        (_receiver.ackno().value() - 1 == seg.header().seqno)) {
        _sender.send_empty_segment();
        _send_ack_if_need();
        return;
    }

    // Try to get if current connection have something to send.
    // We should only send the data if and only if:
    // 1. The sender already received syn. (Local  peer send the data first.)
    // 2. Current segment contain a syn.   (Remote peer send the data first.)
    // We should not fill window in other illegal situation.
    if (_sender.is_syn_received() || seg.header().syn) {
        _sender.fill_window();
    }
    bool should_send_empty_ack = 0;
    // We have to should send ack (even sender has nothing to send) when:
    // 1. Segment carries datas, we should give remote peer an ack.
    // 2. Segment not carries datas, but segment contain either syn or fin, even though the local may send an ack for it
    // before, but the previous ack for 'syn' and 'fin' may lost! For safty reason, we should response those segment
    // directly to prevent potential connection lost.
    if (seg.payload().size() != 0) {
        should_send_empty_ack = 1;
    } else if (seg.payload().size() == 0 && seg.length_in_sequence_space() >= 1) {
        should_send_empty_ack = 1;
    }

    if (_sender.segments_out().empty() && should_send_empty_ack) {
        _sender.send_empty_segment();
    }
    _send_ack_if_need();
    return;
}

bool TCPConnection::active() const {
    if (_is_rst) {
        return false;
    }

    // If receiver have received all the data, and sender has sended all the data, then the connection is clean close.
    if (_sender.is_fin_received() && _receiver.stream_out().eof() && !_linger_after_streams_finish) {
        return false;
    }
    return true;
}

size_t TCPConnection::write(const string &data) {
    size_t result = _sender.stream_in().write(data);
    _sender.fill_window();
    _send_ack_if_need();
    return result;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (_sender.consecutive_retransmissions() >= _cfg.MAX_RETX_ATTEMPTS) {
        // Try to set RST tag in seg.
        _send_rst_to_peer();
        return;
    }
    // If both sender and receiver are done, then we are under active-close, we should check if the last_received time
    // is greater then rt_timout.
    if (_sender.is_syn_received() && _receiver.stream_out().eof() && _linger_after_streams_finish) {
        if (_last_received_time + ms_since_last_tick >= 10 * _cfg.rt_timeout) {
            _linger_after_streams_finish = false;
        }
    }
    _sender.tick(ms_since_last_tick);
    _last_received_time += ms_since_last_tick;

    _send_ack_if_need();
    return;
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    _send_ack_if_need();
    return;
}

void TCPConnection::connect() {
    _sender.fill_window();
    _send_ack_if_need();
    return;
}

void TCPConnection::_send_rst_to_peer() {
    // If rst is set, then we should fist clear sender's segments.
    while (!_sender.segments_out().empty()) {
        _sender.segments_out().pop();
    }
    // Then we set rst to segment directly after, and the peer consider the connection is lost.
    TCPSegment seg = TCPSegment();
    seg.header().seqno = _sender.next_seqno();
    seg.header().rst = true;
    _segments_out.push(seg);
    _set_rst();
    return;
}

void TCPConnection::_set_rst() {
    // Terminate both sender and receiver.
    _is_rst = 1;
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    return;
}

void TCPConnection::_send_ack_if_need() {
    while (!_sender.segments_out().empty()) {
        auto &seg = _sender.segments_out().front();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = 1;
            seg.header().ackno = _receiver.ackno().value();
        }
        seg.header().win = std::min(static_cast<size_t>(numeric_limits<uint16_t>::max()), _receiver.window_size());
        _sender.segments_out().pop();
        _segments_out.push(seg);
    }
    return;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _send_rst_to_peer();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
    return;
}
