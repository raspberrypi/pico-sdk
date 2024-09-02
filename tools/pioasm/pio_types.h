/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PIO_TYPES_H
#define _PIO_TYPES_H

#include <string>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <memory>

#include "location.h"
#include "pio_enums.h"

struct src_item {
    yy::location location;

    src_item() = default;

    explicit src_item(const yy::location &location) : location(location) {}
};

struct program;
struct pio_assembler;

struct resolvable : public src_item {
    resolvable(const yy::location &l) : src_item(l) {}

    int resolve(const program &program);

    int resolve(pio_assembler *pioasm, const program *program) {
        return resolve(pioasm, program, *this);
    }

    virtual int resolve(pio_assembler *pioasm, const program *program, const resolvable &scope) = 0;

    virtual ~resolvable() = default;
};

using rvalue = std::shared_ptr<resolvable>;

enum struct inst_type {
    jmp = 0x0,
    wait = 0x1,
    in = 0x2,
    out = 0x3,
    push_pull = 0x4,
    mov = 0x5,
    irq = 0x6,
    set = 0x7,
};

/* condition codes */
enum struct condition {
    al = 0x0,
    xz = 0x1,
    xnz__ = 0x2,
    yz = 0x3,
    ynz__ = 0x4,
    xney = 0x5,
    pin = 0x6,
    osrez = 0x7,
};

/* in source / out / set target - not all valid */
enum struct in_out_set {
    in_out_set_pins = 0x0,
    in_out_set_x = 0x1,
    in_out_set_y = 0x2,
    in_out_null = 0x3,
    in_out_set_pindirs = 0x4,
    in_status = 0x5,
    out_set_pc = 0x5,
    in_out_isr = 0x6,
    in_osr = 0x7,
    out_exec = 0x7,
};

enum struct irq {
    set = 0x0,
    set_wait = 0x1,
    clear = 0x2,
};

// mov src/dest (not all valid)
enum struct mov {
    pins = 0x0,
    x = 0x1,
    y = 0x2,
    null = 0x3,
    pindirs = 0x3,
    exec = 0x4,
    pc = 0x5,
    status = 0x5,
    isr = 0x6,
    osr = 0x7,
    fifo_y = 0x8,
    fifo_index = 0x9,
};

enum struct mov_status_type {
    unspecified = -1,
    tx_lessthan = 0,
    rx_lessthan = 1,
    irq_set = 2,
};

struct extended_mov {
    mov loc;
    rvalue fifo_index;

    extended_mov() : loc(mov::pindirs), fifo_index(nullptr) {}
    extended_mov(mov _type) : loc(_type), fifo_index(nullptr) {}
    extended_mov(rvalue _fifo_index) : loc(mov::fifo_index), fifo_index(_fifo_index) {}

    bool uses_fifo(void) const { return loc == mov::fifo_index || loc == mov::fifo_y; }
};

enum struct mov_op {
    none = 0x0,
    invert = 0x1,
    bit_reverse = 0x2,
};

struct wait_source {
    enum type {
        gpio = 0x0,
        pin = 0x1,
        irq = 0x2,
        jmppin = 0x3,
    } target;
    rvalue param;
    int irq_type;

    wait_source(type target, rvalue param = 0, int irq_type = 0) : target(target), param(std::move(param)), irq_type(irq_type) {}
};

struct name_ref : public resolvable {
    std::string name;

    name_ref(const yy::location &l, std::string name) : resolvable(l), name(std::move(name)) {}

    int resolve(pio_assembler *pioasm, const program *program, const resolvable &scope) override;
};

struct code_block : public resolvable {
    std::string lang;
    std::string contents;

    code_block(const yy::location &l, std::string lang, std::string contents) : resolvable(l), lang(std::move(lang)),
                                                                                contents(std::move(contents)) {}

    int resolve(pio_assembler *pioasm, const program *program, const resolvable &scope) override {
        return 0;
    }
};

struct int_value : public resolvable {
    int value;

    int_value(const yy::location &l, int value) : resolvable(l), value(value) {}

    int resolve(pio_assembler *pioasm, const program *program, const resolvable &scope) override {
        return value;
    }
};

static inline rvalue resolvable_int(const yy::location &l, int v) {
    return std::shared_ptr<resolvable>(new int_value(l, v));
}

struct binary_operation : public resolvable {
    enum op_type {
        add,
        subtract,
        multiply,
        divide,
        and_, // pesky C++
        or_,
        xor_,
        shl_,
        shr_
    };

    op_type op;
    rvalue left, right;

    binary_operation(const yy::location &l, op_type op, rvalue left, rvalue right) :
            resolvable(l), op(op), left(std::move(left)), right(std::move(right)) {}

    int resolve(pio_assembler *pioasm, const program *program, const resolvable &scope) override;
};

struct unary_operation : public resolvable {
    enum op_type {
        negate,
        reverse
    };

    op_type op;
    rvalue arg;

    unary_operation(const yy::location &l, op_type op, const rvalue &arg) :
            resolvable(l), op(op), arg(arg) {}

    int resolve(pio_assembler *pioasm, const program *program, const resolvable &scope) override;
};

struct symbol : public src_item {
    std::string name;
    rvalue value;
    bool is_public;
    bool is_label;
    int resolve_started;

    symbol(const yy::location &l, std::string name, bool is_extern = false) : src_item(l), name(std::move(name)),
                                                                              is_public(is_extern), is_label(false),
                                                                              resolve_started(false) {}
};

struct raw_encoding {
    enum inst_type type;
    uint arg1;
    uint arg2;
};

struct instruction : public src_item {
    rvalue sideset; // possibly null
    rvalue delay;

    instruction(const yy::location &l) : src_item(l) {}

    // validate while adding instruciton
    virtual void pre_validate(program &program) {}

    virtual uint encode(program &program);

    virtual raw_encoding raw_encode(program &program);
};

struct pio_assembler;

// rvalue with extra encompassing location
struct rvalue_loc {
    rvalue value;
    yy::location location;

    rvalue_loc() = default;

    rvalue_loc(const rvalue &v, const yy::location &l) : value(v), location(l) {}
};

struct in_out {
    yy::location location;
    rvalue pin_count;
    bool right;
    bool autop;
    rvalue threshold;
    int final_pin_count = -1; // not specified
    int final_threshold;
};

struct program : public src_item {
    static const int MAX_INSTRUCTIONS = 32;

    pio_assembler *pioasm;
    std::string name;
    rvalue_loc origin;
    rvalue_loc sideset;
    rvalue_loc set_count;
    in_out in;
    in_out out;
    bool sideset_opt;
    bool sideset_pindirs;

    rvalue wrap_target;
    rvalue wrap;

    int pio_version = 0;
    uint clock_div_int = 1;
    uint clock_div_frac = 0;
    yy::location fifo_loc;
    fifo_config fifo = fifo_config::txrx;
    // 1 bit of bitmap per 16 pins used
    uint8_t used_gpio_ranges = 0;

    std::map<std::string, std::shared_ptr<symbol>> symbols;
    std::vector<std::shared_ptr<symbol>> ordered_symbols;
    std::vector<std::shared_ptr<instruction>> instructions;
    std::map<std::string, std::vector<code_block>> code_blocks;
    std::map<std::string, std::vector<std::pair<std::string,std::string>>> lang_opts;
    struct {
        mov_status_type type = mov_status_type::unspecified;
        rvalue n;
        int param;
        int final_n; // post finalization
    } mov_status;

    // post finalization
    int delay_max;
    int sideset_bits_including_opt; // specified side set bits + 1 if we need presence flag
    int sideset_max;
    int final_set_count = -1;
    int final_out_count = -1;
    int final_in_count = -1;

    program(pio_assembler *pioasm, const yy::location &l, std::string name) :
            src_item(l), pioasm(pioasm), name(std::move(name)), sideset_opt(true), sideset_pindirs(false) {}

    void set_origin(const yy::location &l, const rvalue &_origin) {
        origin = rvalue_loc(_origin, l);
    }

    void set_wrap_target(const yy::location &l);

    void set_wrap(const yy::location &l);

    void set_sideset(const yy::location &l, rvalue _sideset, bool optional, bool pindirs) {
        sideset = rvalue_loc(_sideset, l);
        sideset_opt = optional;
        sideset_pindirs = pindirs;
    }

    void set_out(const yy::location &l, rvalue v, bool right, bool autop, rvalue threshold) {
        out.location = l;
        out.pin_count = std::move(v);
        out.right = right;
        out.autop = autop;
        out.threshold = threshold;
    }

    void set_in(const yy::location &l, rvalue v, bool right, bool autop, rvalue threshold) {
        in.location = l;
        in.pin_count = std::move(v);
        in.right = right;
        in.autop = autop;
        in.threshold = threshold;
    }

    void set_set_count(const yy::location &l, rvalue v) {
        set_count = rvalue_loc(v, l);
    }

    void add_label(std::shared_ptr<symbol> label) {
        label->value = resolvable_int(label->location, instructions.size());
        add_symbol(label);
    }

    void add_symbol(std::shared_ptr<symbol> symbol);

    void add_instruction(std::shared_ptr<instruction> inst);

    void add_code_block(const code_block &block);

    void add_lang_opt(std::string lang, std::string name, std::string value);

    void set_pio_version(const yy::location &l, int version);

    void set_clock_div(const yy::location &l, float clock_div);

    void set_fifo_config(const yy::location &l, fifo_config config);

    void set_mov_status(mov_status_type type, rvalue n, int param = 0) {
        mov_status.type = type;
        mov_status.n = n;
        mov_status.param = param;
    }

    void finalize();
protected:
    void init_pio_version();
};

struct instr_jmp : public instruction {
    condition cond;
    rvalue target;

    instr_jmp(const yy::location &l, condition c, rvalue target) : instruction(l), cond(c), target(std::move(target)) { }

    raw_encoding raw_encode(program &program) override;
};

struct instr_wait : public instruction {
    rvalue polarity;
    std::shared_ptr<wait_source> source;

    instr_wait(const yy::location &l, rvalue polarity, std::shared_ptr<wait_source> source) : instruction(l), polarity(
            std::move(polarity)), source(std::move(source)) {}

    raw_encoding raw_encode(program &program) override;
};

struct instr_in : public instruction {
    enum in_out_set src;
    rvalue value;

    instr_in(const yy::location &l, const enum in_out_set &src, rvalue value) : instruction(l), src(src),
                                                                                value(std::move(value)) {}

    raw_encoding raw_encode(program &program) override;
};

struct instr_out : public instruction {
    enum in_out_set dest;
    rvalue value;

    instr_out(const yy::location &l, const enum in_out_set &dest, rvalue value) : instruction(l), dest(dest),
                                                                                  value(std::move(value)) {}

    raw_encoding raw_encode(program &program) override;
};

struct instr_set : public instruction {
    enum in_out_set dest;
    rvalue value;

    instr_set(const yy::location &l, const enum in_out_set &dest, rvalue value) : instruction(l), dest(dest),
                                                                                  value(std::move(value)) {}

    raw_encoding raw_encode(program &program) override;
};


struct instr_push : public instruction {
    bool if_full, blocking;

    instr_push(const yy::location &l, bool if_full, bool blocking) : instruction(l), if_full(if_full),
                                                                     blocking(blocking) {}


    void pre_validate(program& program) override;
    raw_encoding raw_encode(program &program) override {
        uint arg1 = (blocking ? 1u : 0u) | (if_full ? 0x2u : 0);
        return {inst_type::push_pull, arg1, 0};
    }
};

struct instr_pull : public instruction {
    bool if_empty, blocking;

    instr_pull(const yy::location &l, bool if_empty, bool blocking) : instruction(l), if_empty(if_empty),
                                                                      blocking(blocking) {}

    raw_encoding raw_encode(program &program) override {
        uint arg1 = (blocking ? 1u : 0u) | (if_empty ? 0x2u : 0) | 0x4u;
        return {inst_type::push_pull, arg1, 0};
    }
};

struct instr_mov : public instruction {
    extended_mov dest, src;
    mov_op op;

    instr_mov(const yy::location &l, const extended_mov &dest, const extended_mov &src, const mov_op& op = mov_op::none) :
            instruction(l), dest(dest), src(src), op(op) {}

    uint get_push_get_index(const program &program, extended_mov index);
    void pre_validate(program& program) override;
    raw_encoding raw_encode(program &program) override;
};

struct instr_irq : public instruction {
    enum irq modifiers;
    rvalue num;
    int irq_type;

    instr_irq(const yy::location &l, const enum irq &modifiers, rvalue num, int irq_type = 0) :
            instruction(l), modifiers(modifiers), num(std::move(num)), irq_type(irq_type) {}

    raw_encoding raw_encode(program &program) override;
};


struct instr_nop : public instr_mov {
    instr_nop(const yy::location &l) : instr_mov(l, mov::y, mov::y) {}
};

struct instr_word : public instruction {
    rvalue encoding;

    instr_word(const yy::location &l, rvalue encoding) : instruction(l), encoding(std::move(encoding)) {}

    uint encode(program &program) override;
};

#endif
