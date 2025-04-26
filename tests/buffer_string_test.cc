#include <buffer_string.hh>
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
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // ab
    buffer_list.push_back(str.substr(0, 2), 0);
    IS_TRUE(buffer_list.peek_buffer(), "ab");
    IS_TRUE(buffer_list.size(), 2u);

    // cdefghijklmno
    buffer_list.push_back(str.substr(5, 10), 5);
    IS_TRUE(buffer_list.size(), 12u);
    IS_TRUE(buffer_list.peek_buffer(), "ab");

    // a
    buffer_list.push_back(str.substr(0, 1), 0);
    IS_TRUE(buffer_list.size(), 12u);
    IS_TRUE(buffer_list.peek_buffer(), "ab");

    // bcdef
    buffer_list.push_back(str.substr(1, 5), 1);
    IS_TRUE(buffer_list.size(), 15u);
    IS_TRUE(buffer_list.peek_buffer(), "abcdefghijklmno");

    // pop_up buffer
    IS_TRUE(buffer_list.pop_up_buffer().size(), 15u);
    IS_TRUE(buffer_list.empty(), true);
    IS_TRUE(buffer_list.peek_buffer().size(), 0u);
    return;
}

void test_merge_one_by_one() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";

    size_t size = 1;
    for (size_t i = 0; i < 10; i++) {
        buffer_list.push_back(str.substr(i, i + 1), i);
        IS_TRUE(buffer_list.peek_buffer().size(), size);
        IS_TRUE(buffer_list.peek_buffer(), str.substr(0, size));
        size += 2;
    }
}

void test_merge_one_by_one2() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    for (size_t i = 0; i < str.length(); i++) {
        buffer_list.push_back(str.substr(i, 1), i);
        IS_TRUE(buffer_list.peek_buffer(), str.substr(0, i + 1));
        IS_TRUE(buffer_list.peek_buffer().size(), i + 1);
    }
}

void test_multi_same() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    buffer_list.push_back(str.substr(0, 10), 0);
    buffer_list.push_back(str.substr(0, 10), 0);
    buffer_list.push_back(str.substr(0, 10), 0);
    buffer_list.push_back(str.substr(0, 10), 0);
    buffer_list.push_back(str.substr(0, 10), 0);
    buffer_list.push_back(str.substr(0, 10), 0);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 10));
    IS_TRUE(buffer_list.size(), 10u);
}

void test_merge() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "ab" [0, 2]
    buffer_list.push_back(str.substr(0, 2), 0);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 2));
    IS_TRUE(buffer_list.size(), 2u);

    // insert "ef" [4, 6]
    buffer_list.push_back(str.substr(4, 2), 4);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 2));
    IS_TRUE(buffer_list.size(), 4u);

    // insert "gh" [6, 8]
    buffer_list.push_back(str.substr(6, 2), 6);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 2));
    IS_TRUE(buffer_list.size(), 6u);

    // insert "bc" [1, 3]
    buffer_list.push_back(str.substr(1, 2), 1);
    IS_TRUE(buffer_list.peek_buffer(), "abc");
    IS_TRUE(buffer_list.size(), 7u);

    // insert "abc" [0, 3]
    buffer_list.push_back(str.substr(0, 3), 0);
    IS_TRUE(buffer_list.peek_buffer(), "abc");
    IS_TRUE(buffer_list.size(), 7u);

    // insert "c" [3, 3]
    buffer_list.push_back(str.substr(2, 1), 0);
    IS_TRUE(buffer_list.peek_buffer(), "abc");
    IS_TRUE(buffer_list.size(), 7u);

    // insert "d" [4, 5]
    buffer_list.push_back(str.substr(3, 1), 3);
    IS_TRUE(buffer_list.peek_buffer(), "abcdefgh");
    IS_TRUE(buffer_list.size(), 8u);
}

void test_merge_several() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "ab" [0, 2]
    buffer_list.push_back(str.substr(0, 2), 0);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 2));
    IS_TRUE(buffer_list.size(), 2u);

    // insert "ef" [4, 6]
    buffer_list.push_back(str.substr(4, 2), 4);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 2));
    IS_TRUE(buffer_list.size(), 4u);

    // insert "ij" [8, 10]
    buffer_list.push_back(str.substr(8, 2), 8);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 2));
    IS_TRUE(buffer_list.size(), 6u);

    // insert "mn" [12, 14]
    buffer_list.push_back(str.substr(12, 2), 12);
    IS_TRUE(buffer_list.peek_buffer(), "ab");
    IS_TRUE(buffer_list.size(), 8u);

    // insert "abcdefghi" [0, 9]
    buffer_list.push_back(str.substr(0, 9), 0);
    IS_TRUE(buffer_list.peek_buffer(), "abcdefghij");
}

void test_merge_several2() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "abc" [0, 3]
    buffer_list.push_back(str.substr(0, 3), 0);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 3));
    IS_TRUE(buffer_list.size(), 3u);

    // insert "ghi" [6, 9]
    buffer_list.push_back(str.substr(6, 3), 6);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 3));
    IS_TRUE(buffer_list.size(), 6u);

    // insert "mno" [12, 15]
    buffer_list.push_back(str.substr(12, 3), 12);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 3));
    IS_TRUE(buffer_list.size(), 9u);

    // insert "rst" [18, 21]
    buffer_list.push_back(str.substr(18, 3), 18);
    IS_TRUE(buffer_list.peek_buffer(), "abc");
    IS_TRUE(buffer_list.size(), 12u);

    // insert "abcdefghijklmn" [0, 13]
    buffer_list.push_back(str.substr(0, 14), 0);
    IS_TRUE(buffer_list.peek_buffer(), "abcdefghijklmno");
}

void test_merge_several3() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "abc" [0, 3]
    buffer_list.push_back(str.substr(0, 3), 0);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 3));
    IS_TRUE(buffer_list.size(), 3u);

    // insert "efg" [4, 7]
    buffer_list.push_back(str.substr(4, 3), 4);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(0, 3));
    IS_TRUE(buffer_list.size(), 6u);

    // insert "rst" [18, 21]
    buffer_list.push_back(str.substr(18, 3), 18);
    IS_TRUE(buffer_list.peek_buffer(), "abc");
    IS_TRUE(buffer_list.size(), 9u);

    // // insert "d" [3, 4]
    buffer_list.push_back(str.substr(3, 1), 3);
    IS_TRUE(buffer_list.peek_buffer(), "abcdefg");
}

void test_merge_several4() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "bc" [1, 3]
    buffer_list.push_back(str.substr(1, 2), 1);
    IS_TRUE(buffer_list.peek_buffer(), str.substr(1, 2));
    IS_TRUE(buffer_list.size(), 2u);

    // insert "cdef" [2, 6]
    buffer_list.push_back(str.substr(2, 4), 2);
    IS_TRUE(buffer_list.peek_buffer(), "bcdef");
    IS_TRUE(buffer_list.size(), 5u);

    // insert "rst" [18, 21]
    buffer_list.push_back(str.substr(18, 3), 18);
    IS_TRUE(buffer_list.peek_buffer(), "bcdef");
    IS_TRUE(buffer_list.size(), 8u);

    // insert "jkl" [18, 21]
    buffer_list.push_back("jkl", 9);
    IS_TRUE(buffer_list.peek_buffer(), "bcdef");
    IS_TRUE(buffer_list.size(), 11u);

    // // insert "abcdefghijklmnopqrstuvw" [0, 22]
    buffer_list.push_back(str.substr(0, 22), 0);
    IS_TRUE(buffer_list.peek_buffer(), "abcdefghijklmnopqrstuv");
    IS_TRUE(buffer_list.size(), 22u);
}

void test_merge_all_time() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "abc" [0, 3]
    buffer_list.push_back(str.substr(0, 3), 0);
    IS_TRUE(buffer_list.peek_buffer(), "abc");
    IS_TRUE(buffer_list.size(), 3u);

    // insert "abcdef" [0, 6]
    buffer_list.push_back(str.substr(0, 6), 0);
    IS_TRUE(buffer_list.peek_buffer(), "abcdef");
    IS_TRUE(buffer_list.size(), 6u);

    // insert "abcdefghijklmnopqrstuvwxyz" [0, 24]
    buffer_list.push_back(str.substr(0, 24), 0);
    IS_TRUE(buffer_list.peek_buffer(), "abcdefghijklmnopqrstuvwxyz");
    IS_TRUE(buffer_list.size(), 24u);
}

void test_merge_internal() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "ab" [0, 2]
    buffer_list.push_back(str.substr(0, 2), 0);
    IS_TRUE(buffer_list.peek_buffer(), "ab");
    IS_TRUE(buffer_list.size(), 2u);

    // insert "hi" [7, 9]
    buffer_list.push_back(str.substr(7, 2), 7);
    IS_TRUE(buffer_list.peek_buffer(), "ab");
    IS_TRUE(buffer_list.size(), 4u);

    // insert "efghijk" [4, 11]
    buffer_list.push_back(str.substr(4, 7), 4);
    IS_TRUE(buffer_list.peek_buffer(), "ab");
    IS_TRUE(buffer_list.size(), 9u);
}

void test_eof_test() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "ab" [0, 2]
    buffer_list.push_back(str.substr(0, 2), 0);
    IS_TRUE(buffer_list.peek_buffer(), "ab");
    IS_TRUE(buffer_list.peek_buffer_string().eof(), 0);
    IS_TRUE(buffer_list.size(), 2u);

    // insert "hi" [7, 9]
    buffer_list.push_back(str.substr(7, 2), 7, 1);
    IS_TRUE(buffer_list.peek_buffer(), "ab");
    IS_TRUE(buffer_list.peek_buffer_string().eof(), 0);
    IS_TRUE(buffer_list.size(), 4u);

    // insert "efghijk" [4, 9]
    buffer_list.push_back(str.substr(4, 7), 4);
    IS_TRUE(buffer_list.peek_buffer(), "ab");
    IS_TRUE(buffer_list.peek_buffer_string().eof(), 0);
    IS_TRUE(buffer_list.size(), 9u);

    // insert "cd"
    buffer_list.push_back(str.substr(2, 2), 2);
    IS_TRUE(buffer_list.peek_buffer(), "abcdefghijk");
    IS_TRUE(buffer_list.peek_buffer_string().eof(), 1);
    for (int i = 0; i < 11; i++) {
        IS_TRUE(buffer_list.peek_buffer_string(i).eof(), false);
    }
    IS_TRUE(buffer_list.peek_buffer_string(11).eof(), true);
    IS_TRUE(buffer_list.size(), 11u);
    IS_TRUE(buffer_list.empty(), false);

    // test pop_string
    for (int i = 0; i < 10; i++) {
        BufferString buffer = buffer_list.pop_up_buffer_string(1);
        IS_TRUE(buffer.eof(), false);
        IS_TRUE(buffer.get_string(), str.substr(i, 1));
        IS_TRUE(buffer_list.empty(), false);
    }
    BufferString buffer = buffer_list.pop_up_buffer_string(1);
    IS_TRUE(buffer.eof(), true);
    IS_TRUE(buffer.get_string(), str.substr(10, 1));
    IS_TRUE(buffer_list.empty(), true);
}

void test_push_front() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "stu" [18, 21]
    buffer_list.push_back(str.substr(18, 3), 18);
    IS_TRUE(buffer_list.peek_buffer(), "stu");
    IS_TRUE(buffer_list.size(), 3u);

    // insert "mno" [12, 15]
    buffer_list.push_back(str.substr(12, 3), 12);
    IS_TRUE(buffer_list.peek_buffer(), "mno");
    IS_TRUE(buffer_list.size(), 6u);

    // insert "ghi" [6, 9]
    buffer_list.push_back(str.substr(6, 3), 6);
    IS_TRUE(buffer_list.peek_buffer(), "ghi");
    IS_TRUE(buffer_list.size(), 9u);

    // insert "abc" [0, 3]
    buffer_list.push_back(str.substr(0, 3), 0);
    IS_TRUE(buffer_list.peek_buffer(), "abc");
    IS_TRUE(buffer_list.size(), 12u);
}

void test_popup_size() {
    BufferStringList buffer_list = BufferStringList();
    string str = "abcdefghijklmnopqrstuvwxyz";
    // insert "abc" [0, 3]
    buffer_list.push_back(str.substr(0, 3), 1);
    IS_TRUE(buffer_list.peek_buffer(), "abc");
    IS_TRUE(buffer_list.size(), 3u);

    // insert "abcdef" [0, 6]
    buffer_list.push_back(str.substr(0, 6), 1);
    IS_TRUE(buffer_list.peek_buffer(), "abcdef");
    IS_TRUE(buffer_list.size(), 6u);
    std::string pop_up_string = buffer_list.pop_up_buffer(2);
    IS_TRUE(pop_up_string, "ab");
    IS_TRUE(buffer_list.size(), 4u);
    pop_up_string = buffer_list.pop_up_buffer(1);
    IS_TRUE(pop_up_string, "c");
    IS_TRUE(buffer_list.size(), 3u);
    pop_up_string = buffer_list.pop_up_buffer();
    IS_TRUE(pop_up_string, "def");
    IS_TRUE(buffer_list.size(), 0u);
    IS_TRUE(buffer_list.empty(), true);
}

int main() {
    try {
        test_basic();
        test_merge_one_by_one();
        test_merge_one_by_one2();
        test_multi_same();
        test_merge();
        test_merge_several();
        test_merge_several2();
        test_merge_several3();
        test_merge_several4();
        test_popup_size();
        test_merge_internal();
        test_eof_test();
        test_push_front();
        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
    }

    return 0;
}