#include <list>
#include <memory>
#include <string>
#include <string_view>

namespace sponge {
class BufferString {
  private:
    std::pair<size_t, size_t> _index{};
    bool _eof = 0;
    std::shared_ptr<std::string> _target{};

  public:
    BufferString() = default;
    BufferString(const size_t index, std::string &&target, const bool eof = 0);
    std::string_view get_string() const { return {_target->data(), _target->size()}; }
    std::pair<size_t, size_t> get_index() const { return _index; }
    size_t size() const { return _target->size(); };
    bool eof() const { return _eof; }
};

class BufferStringList {
  private:
    std::list<BufferString> _list{};
    std::string get_buffer(const size_t size, const bool should_pop = 0);
    BufferString get_buffer_string(const size_t size, const bool should_pop = 0);

  public:
    BufferStringList() = default;
    void push_back(const std::string &target, const size_t index, const bool eof = 0);
    size_t size() const;
    bool empty() const { return _list.empty(); }
    size_t top_index() const;
    std::string pop_up_buffer(const size_t size = std::numeric_limits<size_t>::max());
    std::string peek_buffer(const size_t size = std::numeric_limits<size_t>::max());
    BufferString pop_up_buffer_string(const size_t size = std::numeric_limits<size_t>::max());
    BufferString peek_buffer_string(const size_t size = std::numeric_limits<size_t>::max());
};

}  // namespace sponge