#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}
#include <vector>

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    std::string final_data = data;
    size_t final_index = index;
    if (index < _top_pointer) {
        size_t data_right_index = index + data.size();
        if (data_right_index < _top_pointer) {
            // all the string is duplicate, just drop it all.
            return;
        }
        // remove duplicate sub-string, then push the remaining string.
        final_data = data.substr(_top_pointer, data.size() - (_top_pointer - index + 1));
        final_index = _top_pointer;
    }

    _buffer_list.push_back(final_data, final_index);
    size_t remain_capacity = _output.remaining_capacity();

    if (_top_pointer != _buffer_list.top_index()) {
        return;
    }

    std::string buffer_string = _buffer_list.pop_up_buffer(remain_capacity);
    _output.write(buffer_string);
    if (eof) {
        _output.end_input();
    }

    // TODO: add eof function.
}

size_t StreamReassembler::unassembled_bytes() const { return _buffer_list.size(); }

bool StreamReassembler::empty() const { return _buffer_list.empty(); }
