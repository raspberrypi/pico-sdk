/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <algorithm>
#include <iostream>
#include "output_format.h"
#include "pio_disassembler.h"

struct json_output : public output_format {
    struct factory {
        factory() {
            output_format::add(new json_output());
        }
    };

    json_output() : output_format("json") {}

    std::string get_description() override {
        return "Machine-formatted output for integrating with external tools";
    }

    int output(std::string destination, std::vector<std::string> output_options,
               const compiled_source &source) override {

        for (const auto &program : source.programs) {
            for(const auto &p : program.lang_opts) {
                if (p.first.size() >= name.size() && p.first.compare(0, name.size(), name) == 0) {
                    std::cerr << "warning: " << name << " does not support output options; " << p.first << " lang_opt ignored.\n";
                }
            }
        }
        FILE *out = open_single_output(destination);
        if (!out) return 1;

        fprintf(out, "{\n");
        bool first = true;
        // TODO: output symbols?

        for (const auto &program : source.programs) {
            // output trailing comma if necessary
            if (!first) {
                fprintf(out, ",\n");
            } else {
                first = false;
            }

            fprintf(out, "\t\"%s\": {\n", program.name.c_str());
            fprintf(out, "\t\t\"wrap_target\": %d,\n", program.wrap_target);
            fprintf(out, "\t\t\"wrap\": %d,\n", program.wrap);
            fprintf(out, "\t\t\"origin\": %d,\n", program.origin.get());

            if (program.sideset_bits_including_opt.is_specified()) {
                fprintf(out, "\t\t\"sideset\": {\"size\": %d, \"optional\": %s, \"pindirs\": %s},\n", program.sideset_bits_including_opt.get(),
                        program.sideset_opt ? "true" : "false",
                        program.sideset_pindirs ? "true" : "false");
            } else {
                fprintf(out, "\t\t\"sideset\": {\"size\": 0, \"optional\": false, \"pindirs\": false},\n", program.origin.get());
            }

            fprintf(out, "\t\t\"instructions\": [\n");
            bool first_inst = true;
            for (int i = 0; i < (int)program.instructions.size(); i++) {
                const auto &inst = program.instructions[i];
                // output trailing comma if necessary
                if (!first_inst) {
                    fprintf(out, ",\n");
                } else {
                    first_inst = false;
                }
                fprintf(out, "\t\t\t{\"hex\": \"%04x\", \"index\": %d, \"disassembly\": \"%s\"}", inst, i,
                        disassemble(inst, program.sideset_bits_including_opt.get(), program.sideset_opt).c_str());
            }
            fprintf(out, "\n");
            fprintf(out, "\t\t]\n");
            fprintf(out, "\t}");
        }
        fprintf(out, "\n}\n");
        if (out != stdout) { fclose(out); }
        return 0;
    }
};

static json_output::factory creator;
