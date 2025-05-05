#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <stack>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _byted_in_flight; }

void TCPSender::fill_window() {
    // First check the window size. If there is no more window to store. Just skip it.
    size_t remaining_size = _window_size > _byted_in_flight ? _window_size - _byted_in_flight : 0;
    if (remaining_size == 0) {
        return;
    }

    TCPSegment seg = TCPSegment();
    // Set up syn tag. Only set fin when syn has never been push into the queue.
    if (!_is_syn_received) {
        if (!_seg_queue.empty() && _seg_queue.front().seg_ref()->header().syn == 1) {
            return;
        } else if (!_seg_queue.empty() && _seg_queue.front().seg_ref()->header().syn != 1) {
            throw std::runtime_error("When syn is not ready, the front should have syn tag");
        }
        seg.header().syn = 1;
    }

    // Try read from the buffer stream with readable size.
    size_t readable_size = std::min(remaining_size, _stream.buffer_size());
    std::string buffer_str = std::move(_stream.read(readable_size));

    // Set up fin tag if and only if we have read all the data in _stream. And we should set fin when fin- segment has
    // Never been push into the queue.
    if (_stream.eof() && !_is_fin_received) {
        if (!_seg_queue.empty() && _seg_queue.back().seg_ref()->header().fin == 1) {
            return;
        }
        if (remaining_size >= readable_size + 1) {
            seg.header().fin = 1;
        }
    }

    // If nothing to read while current seg is not syn nor fin, then we should skip current segment.
    if (readable_size == 0 && !(seg.header().syn | seg.header().fin)) {
        return;
    }
    size_t index = 0;
    // The payload size should under TCPConfig::MAX_PAYLOAD_SIZE. Otherwise, we should split it into different part.
    bool is_fin = seg.header().fin;
    do {
        // We should set fin the the last payload!!!
        if (is_fin && readable_size <= TCPConfig::MAX_PAYLOAD_SIZE) {
            seg.header().fin = 1;
        } else if (is_fin && readable_size > TCPConfig::MAX_PAYLOAD_SIZE) {
            seg.header().fin = 0;
        }
        // readable_size > TCPConfig::MAX_PAYLOAD_SIZE
        Buffer buffer = Buffer(std::move(buffer_str.substr(index++ * TCPConfig::MAX_PAYLOAD_SIZE,
                                                           std::min(TCPConfig::MAX_PAYLOAD_SIZE, readable_size))));
        // Set up payload if need.
        seg.payload() = buffer;

        // Set up _next_seqno.
        seg.header().seqno = wrap(_next_seqno, _isn);

        // Set up next_seqno.
        _next_seqno += seg.length_in_sequence_space();
        _byted_in_flight += seg.length_in_sequence_space();

        _segments_out.push(seg);
        TCPSenderSegment sender_seg =
            TCPSenderSegment(std::move(seg), _initial_retransmission_timeout, _last_tick_time);
        _seg_queue.push_back(sender_seg);

        // Update readable_size.
        readable_size = readable_size > TCPConfig::MAX_PAYLOAD_SIZE ? readable_size - TCPConfig::MAX_PAYLOAD_SIZE : 0;
    } while (readable_size > 0);

    return;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ack_seqno = unwrap(ackno, _isn, _next_seqno);
    // Illegal situation, since the ackno should never grater thant _next_seqno.
    if (ack_seqno > _next_seqno) {
        return;
    }
    // If window size if equal to 0, we should set our window size to 1 in order to keep connection to the connector.
    _window_size = window_size;
    if (window_size == 0) {
        _window_size = 1;
        _is_window_zero = 1;
    } else {
        _window_size = window_size;
        _is_window_zero = 0;
    }
    bool should_restart = 0;
    while (!_seg_queue.empty() && (unwrap(_seg_queue.front().seqno(), _isn, _next_seqno) +
                                       _seg_queue.front().seg_ref()->length_in_sequence_space() <=
                                   ack_seqno)) {
        _consecutive_retrans = 0;
        auto &queue_front = _seg_queue.front();
        // Set up syn_received tag
        if (queue_front.seg_ref()->header().syn && !_is_syn_received) {
            _is_syn_received = 1;
        }

        // Set up fin_received tag.
        if (queue_front.seg_ref()->header().fin && !_is_fin_received) {
            _is_fin_received = 1;
        }

        // calculate _byted_in_flight.
        _byted_in_flight -= queue_front.seg_ref()->length_in_sequence_space();
        _seg_queue.pop_front();
        should_restart = 1;
    }
    if (!_seg_queue.empty() && should_restart) {
        auto &front = _seg_queue.front();
        front.rot() = _initial_retransmission_timeout;
        front.time() = _last_tick_time;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _last_tick_time += ms_since_last_tick;
    // Find and resend data.
    // Double the time only when syn is completed.
    // Note in CS144, after the RTO time, we only retransmit the top segment.
    if (!_seg_queue.empty()) {
        TCPSenderSegment &sender_seg = _seg_queue.front();
        if (sender_seg.time() + sender_seg.rot() <= _last_tick_time) {
            // Only double the current timeout once.
            if (!_is_window_zero) {
                sender_seg.rot() = sender_seg.rot() * 2;
            }
            sender_seg.time() = _last_tick_time;
            _consecutive_retrans++;
            _segments_out.push(sender_seg.seg());
        }
    }
    return;
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retrans; }

void TCPSender::send_empty_segment() {
    TCPSegment seg = TCPSegment();
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
