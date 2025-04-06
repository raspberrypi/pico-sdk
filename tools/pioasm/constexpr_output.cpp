/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "output_format.h"
#include "pio_disassembler.h"

struct constexpr_output : public output_format {
    struct factory {
        factory() { output_format::add(new constexpr_output()); }
    };

    constexpr_output() : output_format("constexpr") {}

    std::string get_description() {
        return "c++ constexpr array output (only raw program are output) ";
    }

    virtual int output(std::string destination,
                       std::vector<std::string> output_options,
                       const compiled_source &source) {
        FILE *out = open_single_output(destination);
        if (!out)
            return 1;

        fprintf(out, "#pragma once\n\n");
        fprintf(out, "#include <array>\n");
        fprintf(out, "#include <cstdint>\n\n");

        fprintf(out, "namespace Pioasm {\n\n");

        for (auto &p : source.programs) {
            fprintf(out, "inline constexpr std::array<uint16_t, %lu> %s = {\n",
                    p.instructions.size(), p.name.c_str());
            for (auto i = 0u; i < p.instructions.size(); ++i) {
                const auto &inst = p.instructions[i];
                if (i == p.wrap_target) {
                    fprintf(out, "            //     .wrap_target\n");
                }
                fprintf(out, "    0x%04x, // %2d: %s\n", inst, i,
                        disassemble(inst, p.sideset_bits_including_opt.get(),
                                    p.sideset_opt)
                            .c_str());
                if (i == p.wrap) {
                    fprintf(out, "            //     .wrap\n");
                }
            }
            fprintf(out, "};\n\n");
        }

        fprintf(out, "}");

        if (out != stdout) {
            fclose(out);
        }
        return 0;
    }
};

static constexpr_output::factory creator;
