// Rather than auto-generating as part of the build, this header
// is checked in directly.
//
// You can change what is included by configuring these `label_flag`s:
//   --@pico-sdk//bazel/config:PICO_CONFIG_EXTRA_HEADER=//my_proj:my_custom_headers
//   --@pico-sdk//bazel/config:PICO_CONFIG_PLATFORM_HEADER=//my_proj:my_custom_headers

// This header must be provided by //bazel/config:PICO_CONFIG_EXTRA_HEADER:
#include "pico_config_extra_headers.h"

// This header must be provided by //bazel/config:PICO_CONFIG_PLATFORM_HEADER:
#include "pico_config_platform_headers.h"
