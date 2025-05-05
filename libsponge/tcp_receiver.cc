#include "tcp_receiver.hh"

#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

namespace tcp_receiver {
void push_back_wrap_string(const TCPSegment &seg,
                           const WrappingInt32 isn,
                           std::string &&data,
                           StreamReassembler &reassembler,
                           bool eof = 0) {
    uint64_t index = unwrap(seg.header().seqno, isn, reassembler.top_pointer());
    if (seg.header().syn && data.size() == 0) {
        // If SYN package doesn't carry any data, we should manually move the index pointer to the next.
        reassembler.push_substring(data, index, eof);
        reassembler.top_pointer() = index + 1;
    } else {
        reassembler.push_substring(data, index, eof);
    }
    return;
}
}  // namespace tcp_receiver

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // If 'isn' is not set, it means the connection not yet establish. Skip the segment.
    if (!_isn.has_value() && !seg.header().syn) {
        return;
    }
    // Set up `isn`
    if (seg.header().syn && !_isn.has_value()) {
        _isn.emplace(seg.header().seqno);
    }

    // Set up for `fsn`
    if (seg.header().fin && !_fsn.has_value()) {
        _fsn.emplace(seg.header().seqno);
    }

    // We are under `ESTABLISHED` state. Try reassemble data.
    if (_isn.has_value() && !_fsn.has_value()) {
        tcp_receiver::push_back_wrap_string(seg, _isn.value(), std::string(seg.payload().str()), _reassembler);
    } else if (_isn.has_value() && seg.header().fin) {
        // We are under `CLOSE WAIT2` state, current segment contain the remaining data. We should set eof to the pushed
        // data.
        tcp_receiver::push_back_wrap_string(seg, _isn.value(), std::string(seg.payload().str()), _reassembler, 1);
    } else if (_isn.has_value() && _fsn.has_value()) {
        // We are under `CLOSE WAIT` state, previous segment contain the data. We should push the data.
        tcp_receiver::push_back_wrap_string(seg, _isn.value(), std::string(seg.payload().str()), _reassembler);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    std::optional<WrappingInt32> result = {};
    // Assumed SYN don't carry data.
    if (_isn.has_value()) {
        result.emplace(wrap(_reassembler.stream_out().bytes_written(), _isn.value()) + 1);
        // `SYN INIT` State, no data written yet,
        if (_reassembler.stream_out().input_ended()) {
            result.emplace(result.value() + 1);
        }
    }
    return result;
}

size_t TCPReceiver::window_size() const { return _reassembler.current_capactiy(); }
