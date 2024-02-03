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

    void output_symbols(FILE *out, bool output_labels, std::string prefix, const std::vector<compiled_source::symbol> &symbols) {

        fprintf(out, "%s\"publicSymbols\": {\n", prefix.c_str());
        bool first = true;
        for (const auto &s : symbols) {
            if (!s.is_label) {
                // output trailing comma if necessary
                if (!first) {
                    fprintf(out, ",\n");
                } else {
                    first = false;
                }
                fprintf(out, "%s\t\"%s\": %d", prefix.c_str(), s.name.c_str(), s.value);
            }
        }
        if (!first)
            fprintf(out, "\n");
        fprintf(out, "%s},\n", prefix.c_str());

        if (!output_labels)
            return;

        fprintf(out, "%s\"publicLabels\": {\n", prefix.c_str());
        first = true;
        for (const auto &s : symbols) {
            if (s.is_label) {
                // output trailing comma if necessary
                if (!first) {
                    fprintf(out, ",\n");
                } else {
                    first = false;
                }
                fprintf(out, "%s\t\"%s\": %d", prefix.c_str(), s.name.c_str(), s.value);
            }
        }
        if (!first)
            fprintf(out, "\n");
        fprintf(out, "%s},\n", prefix.c_str());
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

        output_symbols(out, false, "\t", source.global_symbols);

        const char* tabs = "\t\t\t";
        fprintf(out, "\t\"programs\": [\n");

        for (const auto &program : source.programs) {
            // output trailing comma if necessary
            if (!first) {
                fprintf(out, ",\n");
            } else {
                first = false;
            }
            fprintf(out, "\t\t{\n");

            fprintf(out, "%s\"name\": \"%s\",\n", tabs, program.name.c_str());
            fprintf(out, "%s\"wrapTarget\": %d,\n", tabs, program.wrap_target);
            fprintf(out, "%s\"wrap\": %d,\n", tabs, program.wrap);
            fprintf(out, "%s\"origin\": %d,\n", tabs, program.origin.get());

            if (program.sideset_bits_including_opt.is_specified()) {
                fprintf(out, "%s\"sideset\": {\"size\": %d, \"optional\": %s, \"pindirs\": %s},\n", tabs, program.sideset_bits_including_opt.get(),
                        program.sideset_opt ? "true" : "false",
                        program.sideset_pindirs ? "true" : "false");
            } else {
                fprintf(out, "%s\"sideset\": {\"size\": 0, \"optional\": false, \"pindirs\": false},\n", tabs);
            }

            output_symbols(out, true, tabs, program.symbols);

            fprintf(out, "%s\"instructions\": [\n", tabs);
            bool first_inst = true;
            for (int i = 0; i < (int)program.instructions.size(); i++) {
                const auto &inst = program.instructions[i];
                // output trailing comma if necessary
                if (!first_inst) {
                    fprintf(out, ",\n");
                } else {
                    first_inst = false;
                }
                // TODO: find a nice, supportable, output format for instruction disassembly.
                // Note that may require tearing apart pio_disassembler to return to us something
                // that we could output as {"instr":"mov", args: ["x", "~y"], side: 3, delay: 1}
                // but that will likely wait for now
                fprintf(out, "%s\t{\"hex\": \"%04X\"}",// \"disassembly\": \"%s\"}",
                        tabs, inst
                        //, disassemble(inst, program.sideset_bits_including_opt.get(), program.sideset_opt).c_str()
                        );
            }
            fprintf(out, "\n");
            fprintf(out, "%s]\n", tabs);
            fprintf(out, "\t\t}");
        }
        fprintf(out, "\n\t]\n}\n");
        if (out != stdout) { fclose(out); }
        return 0;
    }
};

static json_output::factory creator;
