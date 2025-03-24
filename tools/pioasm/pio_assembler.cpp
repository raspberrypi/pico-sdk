/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdio>
#include <iterator>
#include "pio_assembler.h"
#include "parser.hpp"

#ifdef _MSC_VER
#pragma warning(disable : 4996) // fopen
#endif

using syntax_error = yy::parser::syntax_error;

std::string output_format::default_name = "c-sdk";

pio_assembler::pio_assembler() {
}

int pio_assembler::generate(std::shared_ptr<output_format> _format, const std::string &_source,
                            const std::string &_dest, const std::vector<std::string> &_options) {
    format = _format;
    source = _source;
    dest = _dest;
    options = _options;
    location.initialize(&source);
    scan_begin();
    yy::parser parse(*this);
//    parse.set_debug_level(false);
    int res = parse();
    scan_end();
    return res;
}

void program::add_instruction(std::shared_ptr<instruction> inst) {
    uint limit = MAX_INSTRUCTIONS;
    if (instructions.size() >= limit) {
        // todo take offset into account
        std::stringstream msg;
        msg << "program instruction limit of " << limit << " instruction(s) exceeded";
        throw syntax_error(inst->location, msg.str());
    }
    if (!sideset_opt && !inst->sideset) {
        std::stringstream msg;
        msg << "instruction requires 'side' to specify side set value for the instruction because non optional sideset was specified for the program at " << sideset.location;
        throw syntax_error(inst->location, msg.str());
    }
    inst->pre_validate(*this);
    instructions.push_back(inst);
}

using syntax_error = syntax_error;

void program::set_pio_version(const yy::location &l, int version) {
    if (version < 0 || version > 1) {
        throw syntax_error(l, "only PIO versions 0 (rp2040) and 1 (rp2350) are supported");
    }
    pio_version = version;
}

void program::set_clock_div(const yy::location &l, float clock_div) {
    if (clock_div < 1.0f || clock_div >= 65536.0f) {
        throw syntax_error(l, "clock divider must be between 1 and 65535");
    }
    clock_div_int = (uint16_t)clock_div;
    if (clock_div_int == 0) {
        clock_div_frac = 0;
    } else {
        clock_div_frac = (uint8_t)((clock_div - (float)clock_div_frac) * (1u << 8u));
    }
}

void program::set_fifo_config(const yy::location &l, fifo_config config) {
    fifo_loc = l;
    fifo = config;
}

void program::add_symbol(std::shared_ptr<symbol> symbol) {
    const auto &existing = pioasm->get_symbol(symbol->name, this);
    if (existing) {
        std::stringstream msg;
        if (symbol->is_label != existing->is_label) {
            msg << "'" << symbol->name << "' was already defined as a " << (existing->is_label ? "label" : "value")
                << " at " << existing->location;
        } else if (symbol->is_label) {
            msg << "label '" << symbol->name << "' was already defined at " << existing->location;
        } else {
            msg << "'" << symbol->name << "' was already defined at " << existing->location;
        }
        throw syntax_error(symbol->location, msg.str());
    }
    symbols.insert(std::pair<std::string, std::shared_ptr<::symbol>>(symbol->name, symbol));
    ordered_symbols.push_back(symbol);
}

int resolvable::resolve(const program &program) {
    return resolve(program.pioasm, &program);
}

int unary_operation::resolve(pio_assembler *pioasm, const program *program, const resolvable &scope) {
    int value = arg->resolve(pioasm, program, scope);
    switch (op) {
        case negate:
            return -value;
        case reverse: {
            // slow is fine
            uint result = 0;
            for (uint i = 0; i < 32; i++) {
                result <<= 1u;
                if (value & 1u) {
                    result |= 1u;
                }
                value >>= 1u;
            }
            return result;
        }
        default:
            throw syntax_error(location, "internal error");
    }
}

int binary_operation::resolve(pio_assembler *pioasm, const program *program, const resolvable &scope) {
    int lvalue = left->resolve(pioasm, program, scope);
    int rvalue = right->resolve(pioasm, program, scope);
    switch (op) {
        case add:
            return lvalue + rvalue;
        case subtract:
            return lvalue - rvalue;
        case multiply:
            return lvalue * rvalue;
        case divide:
            return lvalue / rvalue;
        case and_:
            return lvalue & rvalue;
        case or_:
            return lvalue | rvalue;
        case xor_:
            return lvalue ^ rvalue;
        case shl_:
            return lvalue << rvalue;
        case shr_:
            return lvalue >> rvalue;
        default:
            throw syntax_error(location, "internal error");
    }
}

void program::set_wrap(const yy::location &l) {
    if (wrap) {
        std::stringstream msg;
        msg << ".wrap was already specified at " << wrap->location;
        throw syntax_error(l, msg.str());
    }
    if (instructions.empty()) {
        throw syntax_error(l, ".wrap cannot be placed before the first program instruction");
    }
    wrap = resolvable_int(l, instructions.size() - 1);
}

void program::set_wrap_target(const yy::location &l) {
    if (wrap_target) {
        std::stringstream msg;
        msg << ".wrap_target was already specified at " << wrap_target->location;
        throw syntax_error(l, msg.str());
    }
    wrap_target = resolvable_int(l, instructions.size());
}

void program::add_code_block(const code_block &block) {
    code_blocks[block.lang].push_back(block);
}

void program::add_lang_opt(std::string lang, std::string name, std::string value) {
    lang_opts[lang].emplace_back(name, value);
}

void program::finalize() {
    if (mov_status.type != mov_status_type::unspecified) {
        uint n = mov_status.n->resolve(*this);
        if (mov_status.type == mov_status_type::irq_set) {
            if (n > 7) throw syntax_error(mov_status.n->location, "irq number should be >= 0 and <= 7");
            mov_status.final_n = mov_status.param * 8 + n;
        } else {
            if (n > 31) throw syntax_error(mov_status.n->location, "fido depth should be >= 0 and <= 31");
            mov_status.final_n = n;
        }
    }
    if (in.pin_count) {
        in.final_pin_count = in.pin_count->resolve(*this);
        if (!pio_version && in.final_pin_count != 32) throw syntax_error(in.pin_count->location, "in pin count must be 32 for PIO version 0");
        if (in.final_pin_count < 1 || in.final_pin_count > 32) throw syntax_error(in.pin_count->location, "in pin count should be >= 1 and <= 32");
        in.final_threshold = in.threshold->resolve(*this);
        if (in.final_threshold < 1 || in.final_threshold > 32) throw syntax_error(in.threshold->location, "threshold should be >= 1 and <= 32");
    }
    if (out.pin_count) {
        out.final_pin_count = out.pin_count->resolve(*this);
        if (out.final_pin_count < 0 || out.final_pin_count > 32) throw syntax_error(out.pin_count->location, "out pin count should be >= 0 and <= 32");
        out.final_threshold = out.threshold->resolve(*this);
        if (out.final_threshold < 1 || out.final_threshold > 32) throw syntax_error(out.threshold->location, "threshold should be >= 1 and <= 32");
    }
    if (set_count.value) {
        final_set_count = set_count.value->resolve(*this);
        if (final_set_count < 0 || final_set_count > 5) throw syntax_error(set_count.location, "set pin count should be >= 0 and <= 5");
    }
    if (sideset.value) {
        int bits = sideset.value->resolve(*this);
        if (bits < 0) {
            throw syntax_error(sideset.value->location, "number of side set bits must be positive");
        }
        sideset_max = (1u << bits) - 1;
        if (sideset_opt) bits++;
        sideset_bits_including_opt = bits;
        if (bits > 5) {
            if (sideset_opt)
                throw syntax_error(sideset.value->location, "maximum number of side set bits with optional is 4");
            else
                throw syntax_error(sideset.value->location, "maximum number of side set bits is 5");
        }
        delay_max = (1u << (5 - bits)) - 1;
    } else {
        sideset_max = 0;
        delay_max = 31;
    }
    if (fifo != fifo_config::rx && fifo != fifo_config::tx && fifo != fifo_config::txrx) {
        std::stringstream msg;
        if (in.pin_count && in.autop) {
            msg << "autopush is incompatible with your selected FIFO configuration specified at " << fifo_loc;
            throw syntax_error(in.location, msg.str());
        }
    }
}

int name_ref::resolve(pio_assembler *pioasm, const program *program, const resolvable &scope) {
    auto symbol = pioasm->get_symbol(name, program);
    if (symbol) {
        if (symbol->resolve_started) {
            std::stringstream msg;
            msg << "circular dependency in definition of '" << name << "'; detected at " << location << ")";
            throw syntax_error(scope.location, msg.str());
        }
        try {
            symbol->resolve_started++;
            int rc = symbol->value->resolve(pioasm, program, scope);
            symbol->resolve_started--;
            return rc;
        } catch (syntax_error &e) {
            symbol->resolve_started--;
            throw e;
        }
    } else {
        std::stringstream msg;
        msg << "undefined symbol '" << name << "'";
        throw syntax_error(location, msg.str());
    }
}

uint instruction::encode(program &program) {
    raw_encoding raw = raw_encode(program);
    int _delay = delay->resolve(program);
    if (_delay < 0) {
        throw syntax_error(delay->location, "instruction delay must be positive");
    }
    if (_delay > program.delay_max) {
        if (program.delay_max == 31) {
            throw syntax_error(delay->location, "instruction delay must be <= 31");
        } else {
            std::stringstream msg;
            msg << "the instruction delay limit is " << program.delay_max << " because of the side set specified at "
                << program.sideset.location;
            throw syntax_error(delay->location, msg.str());
        }
    }
    int _sideset = 0;
    if (sideset) {
        _sideset = sideset->resolve(program);
        if (_sideset < 0) {
            throw syntax_error(sideset->location, "side set value must be >=0");
        }
        if (_sideset > program.sideset_max) {
            std::stringstream msg;
            msg << "the maximum side set value is " << program.sideset_max << " based on the configuration specified at "
                << program.sideset.location;
            throw syntax_error(sideset->location, msg.str());
        }
        _sideset <<= (5u - program.sideset_bits_including_opt);
        if (program.sideset_opt) {
            _sideset |= 0x10u;
        }
    }
    // note we store the 6th bit of arg2 above the 16 bits of instruction
    return (((uint) raw.type) << 13u) | (((uint) _delay | (uint) _sideset) << 8u) | (raw.arg1 << 5u) | (raw.arg2 & 0x1fu) | ((raw.arg2 >> 5) << 16);
}

raw_encoding instruction::raw_encode(program& program) {
    throw syntax_error(location, "internal error");
}

uint instr_word::encode(program &program) {
    uint value = encoding->resolve(program);
    if (value > 0xffffu) {
        throw syntax_error(location, ".word value must be a positive 16 bit value");
    }
    return value;
}

uint instr_mov::get_push_get_index(const program &program, extended_mov index) {
    if (index.loc == mov::fifo_y) {
        return 0;
    } else {
        uint v = index.fifo_index->resolve(program);
        if (v > 7) {
            throw syntax_error(index.fifo_index->location, "FIFO index myst be between 0 and 7");
        }
        return v | 8;
    }
}

void instr_push::pre_validate(program& program) {
    if (program.fifo != fifo_config::rx && program.fifo != fifo_config::txrx) {
        throw syntax_error(location, "FIFO must be configured for 'txrx' or 'rx' to use this instruction");
    }
}

void instr_mov::pre_validate(program &program) {
    if (dest.uses_fifo()) {
        if (src.loc != mov::isr) {
            throw syntax_error(location, "mov rxfifo[] source must be isr");
        }
        if (program.fifo != fifo_config::txput && program.fifo != fifo_config::putget) {
            throw syntax_error(location, "FIFO must be configured for 'txput' or 'putget' to use this instruction");
        }
    } else if (src.uses_fifo()) {
        if (dest.loc != mov::osr) {
            throw syntax_error(location, "mov ,txfifo[] target must be osr");
        }
        if (program.fifo != fifo_config::txget && program.fifo != fifo_config::putget) {
            throw syntax_error(location, "FIFO must be configured for 'txget' or 'putget' to use this instruction");
        }
    }
}

raw_encoding instr_mov::raw_encode(program& program) {
    if (!dest.uses_fifo() && !src.uses_fifo()) {
        // regular mov
        return {inst_type::mov, (uint) dest.loc, (uint) src.loc | ((uint) op << 3u)};
    }
    if (dest.uses_fifo()) {
        return {inst_type::push_pull, 0, 0x10 | get_push_get_index(program, dest) };
    } else {
        return {inst_type::push_pull, 0x4, 0x10 | get_push_get_index(program, src) };
    }
}

raw_encoding instr_jmp::raw_encode(program& program) {
    int dest = target->resolve(program);
    if (dest < 0) {
        throw syntax_error(target->location, "jmp target address must be positive");
    } else if (dest >= (int)program.instructions.size()) {
        std::stringstream msg;
        msg << "jmp target address " << dest << " is beyond the end of the program";
        throw syntax_error(target->location, msg.str());
    }
    return {inst_type::jmp, (uint) cond, (uint) dest};
}

raw_encoding instr_in::raw_encode(program& program) {
    int v = value->resolve(program);
    if (v < 1 || v > 32) {
        throw syntax_error(value->location, "'in' bit count must be >= 1 and <= 32");
    }
    return {inst_type::in, (uint) src, (uint) v & 0x1fu};
}

raw_encoding instr_out::raw_encode(program& program) {
    int v = value->resolve(program);
    if (v < 1 || v > 32) {
        throw syntax_error(value->location, "'out' bit count must be >= 1 and <= 32");
    }
    return {inst_type::out, (uint) dest, (uint) v & 0x1fu};
}

raw_encoding instr_set::raw_encode(program& program) {
    int v = value->resolve(program);
    if (v < 0 || v > 31) {
        throw syntax_error(value->location, "'set' bit count must be >= 0 and <= 31");
    }
    return {inst_type::set, (uint) dest, (uint) v};
}

raw_encoding instr_wait::raw_encode(program& program) {
    uint pol = polarity->resolve(program);
    if (pol > 1) {
        throw syntax_error(polarity->location, "'wait' polarity must be 0 or 1");
    }
    uint arg2 = source->param->resolve(program);
    switch (source->target) {
        case wait_source::irq:
            if (arg2 > 7) throw syntax_error(source->param->location, "irq number must be must be >= 0 and <= 7");
            break;
        case wait_source::gpio: {
            if (!program.pio_version) {
                if (arg2 > 31)
                    throw syntax_error(source->param->location, "absolute GPIO number must be must be >= 0 and <= 31");
            } else {
                if (arg2 > 47)
                    throw syntax_error(source->param->location, "absolute GPIO number must be must be >= 0 and <= 47");
            }
            int bitmap = 1u << (arg2 >> 4);
            if (bitmap == 4 && program.used_gpio_ranges & 1) {
                throw syntax_error(source->param->location, "absolute GPIO number must be must be >= 0 and <= 31 as a GPIO number <16 has already been used");
            }
            if (bitmap == 1 && program.used_gpio_ranges & 4) {
                throw syntax_error(source->param->location, "absolute GPIO number must be must be >= 16 and <= 47 as a GPIO number >32 has already been used");
            }
            program.used_gpio_ranges |= bitmap;
            break;
        }
        case wait_source::pin:
            if (arg2 > 31) throw syntax_error(source->param->location, "pin number must be must be >= 0 and <= 31");
            break;
        case wait_source::jmppin:
            if (arg2 > 3) throw syntax_error(source->param->location, "jmppin offset must be must be >= 0 and <= 3");
            break;
    }
    return {inst_type::wait, (pol << 2u) | (uint) source->target, arg2 | (source->irq_type << 3)};
}

raw_encoding instr_irq::raw_encode(program& program) {
    uint arg2 = num->resolve(program);
    if (arg2 > 7) throw syntax_error(num->location, "irq number must be must be >= 0 and <= 7");
    arg2 |= irq_type << 3;
    return {inst_type::irq, (uint)modifiers, arg2};
}

std::vector<compiled_source::symbol> pio_assembler::public_symbols(program &program) {
    std::vector<std::shared_ptr<symbol>> public_symbols;
    std::remove_copy_if(program.ordered_symbols.begin(), program.ordered_symbols.end(),
                        std::inserter(public_symbols, public_symbols.end()),
                        [](const std::shared_ptr<symbol> &s) { return !s->is_public; });

    std::vector<compiled_source::symbol> rc;
    std::transform(public_symbols.begin(), public_symbols.end(), std::back_inserter(rc),
                   [&](const std::shared_ptr<symbol> &s) {
                       return compiled_source::symbol(s->name, s->value->resolve(program), s->is_label);
                   });
    return rc;
}

int pio_assembler::write_output() {
    std::set<std::string> known_output_formats;
    std::transform(output_format::all().begin(), output_format::all().end(),
                   std::inserter(known_output_formats, known_output_formats.begin()),
                   [&](std::shared_ptr<output_format> &f) {
                       return f->name;
                   });

    compiled_source source;
    source.global_symbols = public_symbols(get_dummy_global_program());
    for (auto &program : programs) {
        program.finalize();
        source.programs.emplace_back(program.name);
        auto &cprogram = source.programs[source.programs.size() - 1];
        cprogram.pio_version = program.pio_version;

        // encode the instructions
        std::transform(program.instructions.begin(), program.instructions.end(),
                       std::back_inserter(cprogram.instructions), [&](std::shared_ptr<instruction> &inst) {
                    return inst->encode(program);
                });

        for (const auto &e : program.code_blocks) {
            bool ok = false;
            for(const auto &o : known_output_formats) {
                if (o == e.first || 0 == e.first.find(o+"-")) {
                    ok = true;
                    break;
                }
            }
            if (!ok) {
                std::cerr << e.second[0].location << ": warning, unknown code block output type '" << e.first << "'\n";
                known_output_formats.insert(e.first);
            }
        }

        if (program.wrap) cprogram.wrap = program.wrap->resolve(program); else cprogram.wrap = std::max((int)program.instructions.size() - 1, 0);
        cprogram.clock_div_int = program.clock_div_int;
        cprogram.clock_div_frac = program.clock_div_frac;
        if (program.wrap_target) {
            cprogram.wrap_target = program.wrap_target->resolve(program);
            if (cprogram.wrap_target >= program.instructions.size()) {
                throw syntax_error(program.wrap_target->location, ".wrap_target cannot be placed after the last program instruction");
            }
        } else {
            cprogram.wrap_target = 0;
        }
        if (program.origin.value) cprogram.origin = program.origin.value->resolve(program);
        cprogram.mov_status_type = program.mov_status.type == mov_status_type::unspecified ? -1 : (int)program.mov_status.type;
        cprogram.mov_status_n = program.mov_status.final_n;
        cprogram.fifo = program.fifo;
        cprogram.used_gpio_ranges = program.used_gpio_ranges;
        auto in_out_convert = [](const in_out &io) {
            return compiled_source::in_out{
                .pin_count = io.final_pin_count,
                .right = io.right,
                .autop = io.autop,
                .threshold = io.final_threshold,
            };
        };
        cprogram.in = in_out_convert(program.in);
        cprogram.out = in_out_convert(program.out);
        cprogram.set_count = program.final_set_count;
        if (program.sideset.value) {
            cprogram.sideset_bits_including_opt = program.sideset_bits_including_opt;
            cprogram.sideset_opt = program.sideset_opt;
            cprogram.sideset_pindirs = program.sideset_pindirs;
        }
        std::transform(program.code_blocks.begin(), program.code_blocks.end(), std::inserter(cprogram.code_blocks, cprogram.code_blocks.begin()), [](const std::pair<std::string, std::vector<code_block>>&e) {
            std::vector<std::string> blocks;
            std::transform(e.second.begin(), e.second.end(), std::back_inserter(blocks), [&](const code_block& block) {
                return block.contents;
            });
            return std::pair<std::string, std::vector<std::string>>(e.first, blocks);
        });
        cprogram.lang_opts = program.lang_opts;
        cprogram.symbols = public_symbols(program);
    }
    if (programs.empty()) {
        std::cout << "warning: input contained no programs" << std::endl;
    }
    return format->output(dest, options, source);
}

FILE *output_format::open_single_output(std::string destination) {
    FILE *out = destination == "-" ? stdout : fopen(destination.c_str(), "w");
    if (!out) {
        std::cerr << "Can't open output file '" << destination << "'" << std::endl;
    }
    return out;
}
