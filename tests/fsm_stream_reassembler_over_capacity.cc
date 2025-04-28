#include "stream_reassembler.hh"

#include <iostream>
#include <stdexcept>

using namespace std;
using namespace sponge;

#define IS_TRUE(target, expect)                                                                                        \
    do {                                                                                                               \
        auto __target = (target);                                                                                      \
        auto __expect = (expect);                                                                                      \
        if (__target != __expect) {                                                                                    \
            std::cerr << __FUNCTION__ << " failed on line " << __LINE__ << std::endl;                                  \
            std::cerr << "  Expect: " << __expect << std::endl;                                                        \
            std::cerr << "  Target: " << __target << std::endl;                                                        \
            throw std::runtime_error("IS_TRUE assertion failed");                                                      \
        }                                                                                                              \
    } while (0)

void test_basic() {
    StreamReassembler reassembler = StreamReassembler(5);
    string str = "abcdefghijklmnopqrstuvwxyz";
    reassembler.push_substring("bcd", 1, 0);
    reassembler.push_substring("efghi", 4, 1);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 0ul);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);

    reassembler.push_substring("a", 0, 0);
    IS_TRUE(reassembler.stream_out().bytes_written(), 5ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);

    str = reassembler.stream_out().read(5);
    IS_TRUE(str, "abcde");
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 5ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 5ul);

    reassembler.push_substring("efghi", 4, 1ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 9ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 5ul);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);

    str = reassembler.stream_out().read(4);
    IS_TRUE(str, "fghi");
    IS_TRUE(reassembler.stream_out().bytes_written(), 9ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 9ul);
    IS_TRUE(reassembler.stream_out().eof(), 1ul);
}

void test_dynamic_eof() {
    StreamReassembler reassembler = StreamReassembler(5);
    string str = "abcdefghijklmnopqrstuvwxyz";
    reassembler.push_substring("ijklm", 8, 1);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.unassembled_bytes(), 5ul);

    reassembler.push_substring("bc", 1, 0);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.unassembled_bytes(), 5ul);

    reassembler.push_substring("a", 0, 0);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 3ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.unassembled_bytes(), 2ul);

    reassembler.push_substring("ijklm", 8, 1);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 3ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.unassembled_bytes(), 2ul);

    IS_TRUE(reassembler.stream_out().read(3), "abc");
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 3ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 3ul);
    IS_TRUE(reassembler.unassembled_bytes(), 2ul);

    reassembler.push_substring("defgh", 3, 0);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 8ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 3ul);
    IS_TRUE(reassembler.unassembled_bytes(), 0ul);

    reassembler.push_substring("ijklm", 8, 1);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 8ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 3ul);
    IS_TRUE(reassembler.unassembled_bytes(), 0ul);

    IS_TRUE(reassembler.stream_out().read(5), "defgh");
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 8ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 8ul);
    IS_TRUE(reassembler.unassembled_bytes(), 0ul);

    reassembler.push_substring("ijklm", 8, 1);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 13ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 8ul);
    IS_TRUE(reassembler.unassembled_bytes(), 0ul);

    IS_TRUE(reassembler.stream_out().read(5), "ijklm");
    IS_TRUE(reassembler.stream_out().eof(), 1ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 13ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 13ul);
    IS_TRUE(reassembler.unassembled_bytes(), 0ul);
}

void test_overlap() {
    StreamReassembler reassembler = StreamReassembler(5);
    string str = "abcdefghijklmnopqrstuvwxyz";
    reassembler.push_substring("ijklm", 8, 1);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.unassembled_bytes(), 5ul);

    reassembler.push_substring("a", 0, 0);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 1ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.unassembled_bytes(), 4ul);

    // assembler: ijkl
    reassembler.push_substring("ijklm", 8, 1);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 1ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.unassembled_bytes(), 4ul);

    // assmembler: deij
    reassembler.push_substring("de", 3, 0);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 1ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.unassembled_bytes(), 4ul);

    // assmembler: bcde
    reassembler.push_substring("bc", 1, 0);
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 5ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 0ul);
    IS_TRUE(reassembler.unassembled_bytes(), 0ul);

    // try read.
    IS_TRUE(reassembler.stream_out().read(5), "abcde");
    IS_TRUE(reassembler.stream_out().eof(), 0ul);
    IS_TRUE(reassembler.stream_out().bytes_written(), 5ul);
    IS_TRUE(reassembler.stream_out().bytes_read(), 5ul);
    IS_TRUE(reassembler.unassembled_bytes(), 0ul);
}

int main() {
    try {
        test_basic();
        test_dynamic_eof();
        test_overlap();
        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}