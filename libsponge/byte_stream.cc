#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

using namespace std;

size_t ByteStream::write(const string &data) {
    // if data is empty, just skip it don't push it input the buffer.
    if (data.empty()) {
        return 0;
    }
    size_t written_size = 0;
    size_t capacity = remaining_capacity();
    written_size = data.size() > capacity ? capacity : data.size();
    _bytes_wirtten += written_size;
    _buffers.push_back(data.substr(0, written_size));
    return written_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const { return _buffers.peek_output(len); }

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t len_left = len;
    _buffers.remove_prefix(len_left);
    _bytes_read += len - len_left;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string result = string(_buffers.peek_output(len));
    _bytes_read += result.size();
    size_t len_left = len;
    _buffers.remove_prefix(len_left);
    return result;
}

void ByteStream::end_input() { is_input_end = true; }

bool ByteStream::input_ended() const { return is_input_end; }

size_t ByteStream::buffer_size() const { return _buffers.size(); }

bool ByteStream::buffer_empty() const { return _buffers.empty(); }

bool ByteStream::eof() const { return is_input_end && _buffers.empty(); }

size_t ByteStream::bytes_written() const { return _bytes_wirtten; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buffers.size(); }
