#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const uint64_t index, const bool eof) {
    std::string data_ref = data;
    uint64_t index_ref = index;
    bool eof_ref = eof;
    // check duplicate.
    if (index_ref < _top_pointer) {
        if (index_ref + data_ref.size() < _top_pointer) {
            // all the string is duplicate, just drop it all.
            return;
        }
        data_ref = data_ref.substr(_top_pointer - index_ref, data_ref.size() - (_top_pointer - index_ref));
        index_ref = _top_pointer;
    }

    // check overall capacity.
    if (data_ref.size() > current_capactiy()) {
        if (eof_ref > current_capactiy()) {
            return;
        }
        // assin to string_ref;
        data_ref = data_ref.substr(0, current_capactiy());
        eof_ref = 0;
    }

    _buffer_list.push_back(data_ref, index_ref, eof_ref);

    if (_top_pointer != _buffer_list.top_index()) {
        return;
    }

    size_t remain_capacity = _output.remaining_capacity();
    auto buffer_string = _buffer_list.pop_up_buffer_string(remain_capacity);
    _top_pointer += buffer_string.size();
    _output.write(std::string(buffer_string.get_string()));
    if (buffer_string.eof()) {
        _output.end_input();
    }
    return;
}

size_t StreamReassembler::unassembled_bytes() const { return _buffer_list.size(); }

bool StreamReassembler::empty() const { return _buffer_list.empty(); }

size_t StreamReassembler::current_capactiy() const { return _capacity - _output.buffer_size(); }