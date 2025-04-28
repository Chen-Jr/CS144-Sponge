#include "wrapping_integers.hh"

#include <iostream>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return WrappingInt32(static_cast<uint32_t>(isn.raw_value() + n)); }

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t lower32 = static_cast<uint32_t>(n - isn);
    uint32_t upper32_1 = static_cast<uint32_t>((checkpoint + (1 << 31)) >> 32);
    uint32_t upper32_2 = static_cast<uint32_t>((checkpoint - (1 << 31)) >> 32);

    uint64_t result1 = (1ul * upper32_1 << 32) + lower32;
    uint64_t result2 = (1ul * upper32_2 << 32) + lower32;
    if (std::max(result1, checkpoint) - std::min(result1, checkpoint) <
        std::max(result2, checkpoint) - std::min(result2, checkpoint)) {
        return result1;
    }
    return result2;
}
