#if DISABLE_VECTOR_TEST
#include "kitchen_sink.c"
#else
#include <vector>

#define EXTRA_FUNC test_vector
#include "kitchen_sink.c"

std::vector<const char *> sv({"foo", "bar", "humbug"});

void test_vector() {
    static std::vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    for (const auto &x : sv) {
        puts(x);
    }
    // false positive
GCC_Pragma("GCC diagnostic push")
GCC_Pragma("GCC diagnostic ignored \"-Wanalyzer-possible-null-dereference\"")
    for (const auto &x : vec) {
        printf("Number %d\n", x);
    }
GCC_Pragma("GCC diagnostic pop")
}

#endif
