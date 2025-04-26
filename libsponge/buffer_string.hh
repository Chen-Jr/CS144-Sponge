#include <list>
#include <memory>
#include <string>
#include <string_view>

namespace sponge {
class BufferString {
  private:
    std::pair<size_t, size_t> _index{};
    std::shared_ptr<std::string> _target{};

  public:
    BufferString() = default;
    BufferString(const size_t index, std::string &&target);
    std::string_view get_string() const { return {_target->data(), _target->size()}; }
    std::pair<size_t, size_t> get_index() const { return _index; }
};

class BufferStringList {
  private:
    std::list<BufferString> _list{};
    std::string get_buffer(const size_t size, const bool should_pop = 0);

  public:
    BufferStringList() = default;
    void push_back(const std::string &target, const size_t index);
    size_t size() const;
    bool empty() const { return _list.empty(); }
    size_t top_index() const;
    std::string pop_up_buffer(const size_t size = std::numeric_limits<size_t>::max());
    std::string peek_buffer(const size_t size = std::numeric_limits<size_t>::max());
};

}  // namespace sponge