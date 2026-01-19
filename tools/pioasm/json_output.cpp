/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <algorithm>
#include <iostream>
#include "output_format.h"
#include "pio_disassembler.h"
#include "version.h"

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

    std::string json_escape(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (unsigned char c : s) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c < 0x20 || c == 0x7F) {
                        // Control characters
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        result += buf;
                    } else {
                        result += c;
                    }
            }
        }
        return result;
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
                fprintf(out, "%s\t\"%s\": %d", prefix.c_str(), json_escape(s.name).c_str(), s.value);
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
                fprintf(out, "%s\t\"%s\": %d", prefix.c_str(), json_escape(s.name).c_str(), s.value);
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

        fprintf(out, "\t\"pioASMVersion\": \"%s\",\n", json_escape(PIOASM_VERSION_STRING).c_str());
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

            fprintf(out, "%s\"name\": \"%s\",\n", tabs, json_escape(program.name).c_str());
            fprintf(out, "%s\"pioVersion\": %d,\n", tabs, program.pio_version);
            fprintf(out, "%s\"wrapTarget\": %d,\n", tabs, program.wrap_target);
            fprintf(out, "%s\"wrap\": %d,\n", tabs, program.wrap);
            fprintf(out, "%s\"origin\": %d,\n", tabs, program.origin.get());

            fprintf(out, "%s\"usedGPIORanges\": %d,\n", tabs, program.used_gpio_ranges);

            output_symbols(out, true, tabs, program.symbols);

            if (program.in.pin_count >= 0) {
                fprintf(out, "%s\"in\": {\"pinCount\": %d, \"right\": %s, \"autopush\": %s, \"threshold\": %d},\n",
                        tabs, program.in.pin_count,
                        program.in.right ? "true" : "false",
                        program.in.autop ? "true" : "false",
                        program.in.threshold);
            } else {
                fprintf(out, "%s\"in\": null,\n", tabs);
            }

            if (program.out.pin_count >= 0) {
                fprintf(out, "%s\"out\": {\"pinCount\": %d, \"right\": %s, \"autopull\": %s, \"threshold\": %d},\n",
                        tabs, program.out.pin_count,
                        program.out.right ? "true" : "false",
                        program.out.autop ? "true" : "false",
                        program.out.threshold);
            } else {
                fprintf(out, "%s\"out\": null,\n", tabs);
            }

            if (program.set_count >= 0) {
                fprintf(out, "%s\"setCount\": %d,\n", tabs, program.set_count);
            } else {
                fprintf(out, "%s\"setCount\": null,\n", tabs);
            }

            if (program.sideset_bits_including_opt.is_specified()) {
                fprintf(out, "%s\"sideset\": {\"size\": %d, \"optional\": %s, \"pindirs\": %s},\n", tabs, program.sideset_bits_including_opt.get(),
                        program.sideset_opt ? "true" : "false",
                        program.sideset_pindirs ? "true" : "false");
            } else {
                fprintf(out, "%s\"sideset\": null,\n", tabs);
            }

            if (program.mov_status_type != -1) {
                const char *types[] = {
                        "STATUS_TX_LESSTHAN",
                        "STATUS_RX_LESSTHAN",
                        "STATUS_IRQ_SET",
                };
                if (program.mov_status_type < 0 || program.mov_status_type >= 3) {
                    throw std::runtime_error("unknown mov_status type");
                }
                fprintf(out, "%s\"movStatus\": {\"type\": \"%s\", \"n\": %d},\n", tabs, types[program.mov_status_type], program.mov_status_n);
            } else {
                fprintf(out, "%s\"movStatus\": null,\n", tabs);
            }

            if (program.fifo != fifo_config::txrx) {
                const char *type;
                switch (program.fifo) {
                    case fifo_config::tx:     type = "PIO_FIFO_JOIN_TX"; break;
                    case fifo_config::rx:     type = "PIO_FIFO_JOIN_RX"; break;
                    case fifo_config::txput:  type = "PIO_FIFO_JOIN_TXPUT"; break;
                    case fifo_config::txget:  type = "PIO_FIFO_JOIN_TXGET"; break;
                    case fifo_config::putget: type = "PIO_FIFO_JOIN_PUTGET"; break;
                    default: throw std::runtime_error("unknown fifo_config type");
                }
                fprintf(out, "%s\"fifoConfig\": \"%s\",\n", tabs, type);
            } else {
                fprintf(out, "%s\"fifoConfig\": null,\n", tabs);
            }

            if (program.clock_div_int != 1 || program.clock_div_frac != 0) {
                fprintf(out, "%s\"clockDiv\": {\"int\": %u, \"frac\": %u},\n", tabs, program.clock_div_int, program.clock_div_frac);
            } else {
                fprintf(out, "%s\"clockDiv\": null,\n", tabs);
            }

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
                // but that will likely wait for now. A dissasembly string instruction is added
                // for the time being.
                fprintf(out, "%s\t{\"hex\": \"%04X\", \"instruction\": \"%s\"}",
                        tabs, inst,
                        json_escape(disassemble(inst, program.sideset_bits_including_opt.get(), program.sideset_opt)).c_str()
                        );
            }
            fprintf(out, "\n");
            fprintf(out, "%s],\n", tabs);

            // This prints: codeBlocks: [
            //   { "lang": "c-sdk", "contents": "...." },
            //   { "lang": "c-sdk", "contents": "...." },
            //   { "lang": "python", "contents": "...." },
            //   { "lang": "c-sdk", "contents": "...." }
            // ]
            // This is how order is preserved while supporting multiple blocks per language.
            fprintf(out, "%s\"codeBlocks\": [\n", tabs);
            bool first_block = true;
            for(const auto& o : program.code_blocks) {
                for(const auto &contents : o.second) {
                    // output trailing comma if necessary
                    if (!first_block) {
                        fprintf(out, ",\n");
                    } else {
                        first_block = false;
                    }
                    fprintf(out, "%s\t{\n", tabs);
                    fprintf(out, "%s\t\t\"lang\": \"%s\",\n", tabs, json_escape(o.first).c_str());
                    fprintf(out, "%s\t\t\"contents\": \"%s\"\n", tabs, json_escape(contents).c_str());
                    fprintf(out, "%s\t}", tabs);
                }
            }
            fprintf(out, "\n%s],\n", tabs);

            // Similarly to codeBlocks, order is preserved while supporting multiple lang_opts.
            fprintf(out, "%s\"langOpts\": [\n", tabs);
            bool first_opt = true;
            for(const auto& o : program.lang_opts) {
                for(const auto &pair : o.second) {
                    // output trailing comma if necessary
                    if (!first_opt) {
                        fprintf(out, ",\n");
                    } else {
                        first_opt = false;
                    }
                    fprintf(out, "%s\t{\n", tabs);
                    fprintf(out, "%s\t\t\"lang\": \"%s\",\n", tabs, json_escape(o.first).c_str());
                    fprintf(out, "%s\t\t\"name\": \"%s\",\n", tabs, json_escape(pair.first).c_str());
                    fprintf(out, "%s\t\t\"value\": \"%s\"\n", tabs, json_escape(pair.second).c_str());
                    fprintf(out, "%s\t}", tabs);
                }
            }
            fprintf(out, "\n%s]\n", tabs);

            fprintf(out, "\t\t}");
        }
        fprintf(out, "\n\t]\n}\n");
        if (out != stdout) { fclose(out); }
        return 0;
    }
};

static json_output::factory creator;
