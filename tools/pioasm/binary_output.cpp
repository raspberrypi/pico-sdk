/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "output_format.h"
#include <iostream>

struct binary_output : public output_format {
    struct factory {
        factory() {
            output_format::add(new binary_output());
        }
    };

    binary_output() : output_format("binary") {}

    std::string get_description() {
        return "Raw binary output (only valid for single program inputs)";
    }

    virtual int output(std::string destination, std::vector<std::string> output_options,
                       const compiled_source &source) {
        FILE *out = open_single_output(destination);
        if (!out) return 1;

        if (source.programs.size() > 1) {
            // todo don't have locations any more!
            std::cerr << "error: binary output only supports a single program input\n";
            return 1;
        }
        for (const auto &i : source.programs[0].instructions) {
            fputc(i & 0xFF, out);
            fputc(i >> 8, out);
        }
        if (out != stdout) { fclose(out); }
        return 0;
    }
};

static binary_output::factory creator;
