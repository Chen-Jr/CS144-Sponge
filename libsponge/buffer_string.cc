#include "buffer_string.hh"

#include <algorithm>

namespace sponge {

BufferString::BufferString(const uint64_t index, std::string &&target, bool eof) : _eof(eof) {
    _index = std::make_pair(index, index + target.size());
    _target = std::make_shared<std::string>(std::move(target));
}

void BufferStringList::push_back(const std::string &target, const uint64_t index, const bool eof) {
    bool final_eof = eof;
    std::string target_ref = target;
    BufferString buffer = BufferString(index, std::move(target_ref), final_eof);
    auto lower_left = std::lower_bound(
        _list.begin(), _list.end(), buffer.get_index().first, [](const BufferString &buff, const uint64_t key) {
            return key > buff.get_index().second;
        });

    auto upper_right = std::upper_bound(
        _list.begin(), _list.end(), buffer.get_index().second, [](const uint64_t key, const BufferString &buff) {
            return key < buff.get_index().first;
        });

    if (lower_left == upper_right) {
        _list.insert(lower_left, buffer);
        return;
    }

    // lower_left can't be list.end(), if lower_left is equal to list.end() then upper_right must be list.end(), which
    // means we should insert it to the back.
    bool is_list_end = upper_right == _list.end();
    upper_right = !is_list_end ? upper_right : std::prev(upper_right);

    if (std::distance(lower_left, upper_right) < 0) {
        throw std::runtime_error("BufferStringList::push_back lower_left is larger than upper_right");
    }

    // try merged.
    size_t left_index = std::min(lower_left->get_index().first, buffer.get_index().first);

    // For upper_right, if original upper_right is equal to list.end(), which means the list doesn't have any left_index
    // greater than current_right. In this case, current_right is greater than upper_right's left. So we should select
    // the maximum of upper_right's right and current_right as the maximum index.
    // Example: upper_right: [5, 10], current_index [3, 9], 9 is greater than 5, so the right_index should be max(10, 9)
    // = 10.
    size_t right_index =
        !is_list_end ? buffer.get_index().second : std::max(upper_right->get_index().second, buffer.get_index().second);

    // If current range is totally inside the lower_left range, there is no need to do the following operation, just
    // skip it.
    if (lower_left->get_index().first <= buffer.get_index().first &&
        lower_left->get_index().second >= buffer.get_index().second) {
        return;
    }

    std::string new_string = std::string();
    new_string.reserve(right_index - left_index);

    // if current index is greater than lower_left (which means the left_index is less than current_index.left), then we
    // should add the remaining part of lower_left's sub-string to buffer.
    // Example: lower_left = [0, 2], current_index is [1, 3], we should add [0, 1] to new_string.
    if (buffer.get_index().first > left_index) {
        new_string += lower_left->get_string().substr(0, (buffer.get_index().first - left_index));
    }

    new_string += buffer.get_string();
    // Because the final right_index is greater than the following node's left index, we can erase those nodes directly,
    // and replace them with target string. But the final right_index is not always greater than the node's right index.
    // Instead, we should extend the new_string to the right nodes and update the right_index.
    // Example: [0, 3] [4, 7] [11, 14], while the input is [3, 4]. the final result should be [0, 7] instead of [0, 4].
    for (auto it = lower_left; it != upper_right;) {
        if (it->get_index().second > right_index) {
            new_string +=
                it->get_string().substr(right_index - it->get_index().first, it->get_index().second - right_index);
        }
        auto next_it = std::next(it);
        it = next_it;
    }
    // When the upper_right hits the list.end(), we will meet two situations. (1). current_right is larger than
    // upper_right.right. Example: upper_right: [5, 10], current_index: [1, 11], and in this case we do nothing, just
    // append current string to the end. (2). current_right is less than upper_right's right, then we should add the
    // remaining part of upper_right's sub-string to buffer. Example: upper_right = [5, 10], current_index is [1, 9], we
    // should add [9, 10] to new_string. We should always try to merge upper_right's eof since the upper_right is the
    // last node in the list.
    if (is_list_end) {
        if (buffer.get_index().second < right_index) {
            new_string += upper_right->get_string().substr(buffer.get_index().second - upper_right->get_index().first,
                                                           right_index - buffer.get_index().second);
        }
        final_eof |= upper_right->eof();
        auto upper_index = upper_right - _list.begin();
        auto lower_index = lower_left - _list.begin();
        _list[upper_index] = BufferString(left_index, std::move(new_string), final_eof);
        _list.erase((_list.begin() + lower_index), _list.begin() + upper_index);
        return;
    }
    auto lower_index = lower_left - _list.begin();
    _list[lower_index] = BufferString(left_index, std::move(new_string), final_eof);
    _list.erase((_list.begin() + lower_index + 1), upper_right);
    return;
}

size_t BufferStringList::size() const {
    size_t size = {};
    for (auto it : _list) {
        size += it.get_index().second - it.get_index().first;
    }
    return size;
}

uint64_t BufferStringList::top_index() const {
    if (_list.empty()) {
        return 0;
    }
    return _list.front().get_index().first;
}

std::string BufferStringList::get_buffer(const size_t size, const bool should_pop) {
    if (_list.empty()) {
        return {};
    }
    std::string_view result_view = _list.front().get_string();
    if (size >= result_view.size()) {
        std::string result = std::string(result_view);
        if (should_pop) {
            _list.pop_front();
        }
        return result;
    } else {
        std::string result = std::string(result_view.substr(0, size));
        if (should_pop) {
            std::string remaining = std::string(result_view.substr(size, result_view.size() - size));
            BufferString remaining_buffer = BufferString(_list.front().get_index().first + size, std::move(remaining));
            _list.pop_front();
            _list.push_back(remaining_buffer);
        }
        return result;
    }
    return {};
}

BufferString BufferStringList::get_buffer_string(const size_t size, const bool should_pop) {
    if (_list.empty()) {
        return {};
    }
    BufferString buffer = _list.front();
    if (size >= buffer.size()) {
        if (should_pop) {
            _list.pop_front();
        }
        return buffer;
    } else {
        BufferString result =
            BufferString(buffer.get_index().first, std::string(buffer.get_string().substr(0, size)), 0);

        if (should_pop) {
            BufferString remaining = BufferString(buffer.get_index().first + size,
                                                  std::string(buffer.get_string().substr(size, buffer.size() - size)),
                                                  buffer.eof());
            _list.pop_front();
            _list.push_back(remaining);
        }
        return result;
    }
    return {};
}

std::string BufferStringList::pop_up_buffer(const size_t size) { return get_buffer(size, 1); }

std::string BufferStringList::peek_buffer(const size_t size) { return get_buffer(size); }

BufferString BufferStringList::pop_up_buffer_string(const size_t size) { return get_buffer_string(size, 1); }

BufferString BufferStringList::peek_buffer_string(const size_t size) { return get_buffer_string(size); }

}  // namespace sponge