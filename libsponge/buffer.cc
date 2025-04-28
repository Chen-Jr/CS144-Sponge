#include "buffer.hh"

namespace sponge {

void Buffer::remove_prefix(const size_t size) {
    if (_prefix_pointer + size >= _storage->size()) {
        _prefix_pointer = _storage->size() - 1;
    } else {
        _prefix_pointer += size;
    }
    return;
}

std::string_view Buffer::get_string() {
    if (!_storage) {
        return {};
    }
    return {_storage->data() + _prefix_pointer, _storage->size() - _prefix_pointer};
}

size_t Buffer::size() const {
    if (!_storage) {
        return 0;
    }
    return _storage->size() - _prefix_pointer;
}

size_t BufferList::size() const {
    size_t result = 0;
    for (const auto &buffer_item : _bufferList) {
        result += buffer_item.size();
    }
    return result;
}

const std::string BufferList::peek_output(const size_t size) const {
    std::string str = std::string();
    str.reserve(size);
    size_t remain_size = size;
    for (auto item : _bufferList) {
        if (item.size() < remain_size) {
            str += item.get_string();
            remain_size -= item.size();
        } else {
            str += item.get_string().substr(0, remain_size);
            remain_size = 0;
            break;
        }
    }
    if (remain_size != 0) {
        str.reserve(size - remain_size);
    }
    return str;
}

void BufferList::remove_prefix(size_t &size) {
    while (size > 0) {
        if (_bufferList.empty()) {
            throw std::out_of_range("BufferLIst::remove_prefix(Nothing to remove)");
        }
        if (_bufferList.front().size() <= size) {
            size -= _bufferList.front().size();
            _bufferList.pop_front();
        } else {
            _bufferList.front().remove_prefix(size);
            size = 0;
        }
    }
    return;
}

}  // namespace sponge