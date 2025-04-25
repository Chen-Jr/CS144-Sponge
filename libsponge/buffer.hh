#include <deque>
#include <memory>
#include <string>
#include <string_view>

namespace sponge {
class Buffer {
  private:
    std::shared_ptr<std::string> _storage{};
    size_t _prefix_pointer{};

  public:
    Buffer() = default;
    Buffer(std::string &&str) : _storage(std::make_shared<std::string>(std::move(str))){};
    void remove_prefix(const size_t size);
    size_t size() const;
    std::string_view get_string();
};

class BufferList {
  private:
    std::deque<Buffer> _bufferList{};

  public:
    BufferList() = default;
    BufferList(Buffer buffer) : _bufferList{buffer} {};
    size_t size() const;
    void push_back(const Buffer &buffer) { _bufferList.push_back(buffer); }
    void push_back(std::string &&buffer_string) { push_back(Buffer(std::move(buffer_string))); }

    const std::string peek_output(const size_t size) const;

    void remove_prefix(size_t &size);
    bool empty() const { return _bufferList.empty(); }
};
}  // namespace sponge