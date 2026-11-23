/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "output_format.h"
#include <iostream>

struct hex_output : public output_format {
    struct factory {
        factory() {
            output_format::add(new hex_output());
        }
    };

    hex_output() : output_format("hex") {}

    std::string get_description() {
        return "Raw hex output";
    }

    virtual int output(std::string destination, std::vector<std::string> output_options,
                       const compiled_source &source) {
        FILE *out = open_single_output(destination);
        if (!out) return 1;

        int program_idx = -1;
        if (output_options.size() == 0) {
            // no explicit program name given
            if (source.programs.size() == 1) {
                // the only program
                program_idx = 0;
            } else {
                std::cerr << "error: hex output only supports a single program (use the -p option to specify a program name)" << std::endl;
                return 1;
            }
        } else if (output_options.size() == 1) {
            // look for program by name
            for (uint i=0; i<source.programs.size(); i++) {
                if (source.programs[i].name == output_options[0]) {
                    program_idx = i;
                    break;
                }
            }
            if (program_idx < 0) {
                std::cerr << "error: no matching program name found" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "error: too many options for hex format" << std::endl;
            return 1;
        }
        for (const auto &i : source.programs[program_idx].instructions) {
            fprintf(out, "%04x\n", (uint16_t)i);
        }
        if (out != stdout) { fclose(out); }
        return 0;
    }
};

static hex_output::factory creator;
