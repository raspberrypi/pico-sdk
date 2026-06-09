#include <version>
#include <cstdarg>

#include "pico.h"

namespace std {
    inline namespace __2 {
        [[__noreturn__]] _LIBCPP_AVAILABILITY_VERBOSE_ABORT _LIBCPP_OVERRIDABLE_FUNC_VIS _LIBCPP_ATTRIBUTE_FORMAT(
        __printf__, 1, 2) void __libcpp_verbose_abort(const char* __format, ...) _NOEXCEPT {
            panic("c++ abort: %s", __format);
        }
    }
}
