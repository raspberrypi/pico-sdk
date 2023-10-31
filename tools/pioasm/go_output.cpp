/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Go generated by this assembler is compatible with TinyGo.
 *
 * https://tinygo.org
 * https://github.com/tinygo-org/tinygo
 */

#include <algorithm>
#include <iostream>
#include "output_format.h"
#include "pio_disassembler.h"

struct go_output : public output_format {
    struct factory {
        factory() {
            output_format::add(new go_output());
        }
    };

    go_output() : output_format("go") {}

    std::string get_description() override {
        return "Go file suitable for use with TinyGo";
    }

    void output_symbols(FILE *out, std::string prefix, const std::vector<compiled_source::symbol> &symbols) {
        int count = 0;
        for (const auto &s : symbols) {
            if (!s.is_label) {
                fprintf(out, "const %s%s = %d\n", prefix.c_str(), s.name.c_str(), s.value);
                count++;
            }
        }
        if (count) {
            fprintf(out, "\n");
            count = 0;
        }
        for (const auto &s : symbols) {
            if (s.is_label) {
                fprintf(out, "const %soffset_%s = %d\n", prefix.c_str(), s.name.c_str(), s.value);
                count++;
            }
        }
        if (count) {
            fprintf(out, "\n");
        }
    }

    void header(FILE *out, std::string msg) {
        fprintf(out, "// %s\n\n", msg.c_str());
    }

    int output(std::string destination, std::vector<std::string> output_options,
               const compiled_source &source) override {

        FILE *out = open_single_output(destination);
        if (!out) return 1;

        header(out, "Code generated by pioasm; DO NOT EDIT.");
        
        // First we give priority to user's code blocks since 
        //  1. In Go our imports always precede our code.
        //  2. We give users freedom to use their own PIO implementation.
        for (const auto &program : source.programs) {
            for(const auto& o : program.code_blocks) {
                if (o.first == name) {
                    for(const auto &contents : o.second) {
                        fprintf(out, "%s", contents.c_str());
                    }
                }
            }
        }

        for (const auto &program : source.programs) {
            header(out, program.name);

            std::string prefix = program.name;

            fprintf(out, "const %sWrapTarget = %d\n", prefix.c_str(), program.wrap_target);
            fprintf(out, "const %sWrap = %d\n", prefix.c_str(), program.wrap);
            fprintf(out, "\n");

            output_symbols(out, prefix, program.symbols);

            fprintf(out, "var %sInstructions = []uint16{\n", prefix.c_str());
            for (int i = 0; i < (int)program.instructions.size(); i++) {
                const auto &inst = program.instructions[i];
                if (i == program.wrap_target) {
                    fprintf(out, "\t\t//     .wrap_target\n");
                }
                fprintf(out, "\t\t0x%04x, // %2d: %s\n", inst, i,
                        disassemble(inst, program.sideset_bits_including_opt.get(), program.sideset_opt).c_str());
                if (i == program.wrap) {
                    fprintf(out, "\t\t//     .wrap\n");
                }
            }
            fprintf(out, "}\n");
            fprintf(out, "const %sOrigin = %d\n", prefix.c_str(), program.origin.get());
            
            fprintf(out, "func %sProgramDefaultConfig(offset uint8) pio.StateMachineConfig {\n", prefix.c_str());
            fprintf(out, "\tcfg := pio.DefaultStateMachineConfig()\n");
            fprintf(out, "\tcfg.SetWrap(offset+%sWrapTarget, offset+%sWrap)\n", prefix.c_str(),
                    prefix.c_str());
            if (program.sideset_bits_including_opt.is_specified()) {
                fprintf(out, "\tcfg.SetSidesetParams(%d, %s, %s)\n", program.sideset_bits_including_opt.get(),
                        program.sideset_opt ? "true" : "false",
                        program.sideset_pindirs ? "true" : "false");
            }
            fprintf(out, "\treturn cfg;\n");
            fprintf(out, "}\n\n");
        }
        
        output_symbols(out, "", source.global_symbols);

        if (out != stdout) { fclose(out); }
        return 0;
    }
};

static go_output::factory creator;
