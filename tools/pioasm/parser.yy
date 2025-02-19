/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.4.2"
%defines

%define api.token.constructor
%define api.value.type variant
/*%define parse.assert*/
%define api.location.file "location.h"
%define parse.lac full
/* define parse.trace*/
%define parse.error verbose
%no-lines
%locations

%code requires {
  #include <string>
  #include <fstream>
  #include <sstream>
  #include "pio_types.h"
  struct pio_assembler;

  #ifdef _MSC_VER
  #pragma warning(disable : 4065) // default only switch statement
  #endif
}

// The parsing context.
%param { pio_assembler& pioasm }

%code {
    #include "pio_assembler.h"
  #ifdef _MSC_VER
  #pragma warning(disable : 4244) // possible loss of data (valid warning, but there is a software check / missing cast)
  #endif
}

%define api.token.prefix {TOK_}

%token
    END     0       "end of file"

    NEWLINE         "end of line"
    COMMA           ","
    COLON           ":"

    LPAREN          "("
    RPAREN          ")"
    LBRACKET        "["
    RBRACKET        "]"
    PLUS            "+"
    MINUS           "-"
    MULTIPLY        "*"
    DIVIDE          "/"
    OR              "|"
    AND             "&"
    XOR             "^"
    SHL             "<<"
    SHR             ">>"
    POST_DECREMENT  "--"
    NOT_EQUAL       "!="
    NOT             "!"
    REVERSE         "::"

    ASSIGN          "="
    LESSTHAN        "<"

    PROGRAM         ".program"
    WRAP_TARGET     ".wrap_target"
    WRAP            ".wrap"
    DEFINE          ".define"
    SIDE_SET        ".side_set"
    WORD            ".word"
    ORIGIN          ".origin"
    LANG_OPT        ".lang_opt"
    PIO_VERSION     ".pio_version"
    CLOCK_DIV       ".clock_div"
    FIFO            ".fifo"
    MOV_STATUS      ".mov_status"
    DOT_SET         ".set"
    DOT_OUT         ".out"
    DOT_IN          ".in"

    JMP             "jmp"
    WAIT            "wait"
    IN              "in"
    OUT             "out"
    PUSH            "push"
    PULL            "pull"
    MOV             "mov"
    IRQ             "irq"
    SET             "set"
    NOP             "nop"

    PIN             "pin"
    GPIO            "gpio"
    OSRE            "osre"
    JMPPIN          "jmppin"
    PREV            "prev"
    NEXT            "next"

    PINS            "pins"
    NULL            "null"
    PINDIRS         "pindirs"
    BLOCK           "block"
    NOBLOCK         "noblock"
    IFEMPTY         "ifempty"
    IFFULL          "iffull"
    NOWAIT          "nowait"
    CLEAR           "clear"
    REL             "rel"
    X               "x"
    Y               "y"
    EXEC            "exec"
    PC              "pc"
    ISR             "isr"
    OSR             "osr"
    OPTIONAL        "opt"
    SIDE            "side"
    STATUS          "status"
    PUBLIC          "public"

    RP2040          "rp2040"
    RP2350          "rp2350"
    RXFIFO          "rxfifo"
    TXFIFO          "txfifo"

    TXRX            "txrx"
    TX              "tx"
    RX              "rx"
    TXPUT           "txput"
    TXGET           "txget"
    PUTGET          "putget"

    LEFT            "left"
    RIGHT           "right"
    AUTO            "auto"
    MANUAL          "manual"
;

%token
    <std::string> ID "identifier"
    <std::string> STRING "string"
    <std::string> NON_WS "text"
    <std::string> CODE_BLOCK_START "code block"
    <std::string> CODE_BLOCK_CONTENTS "%}" // bit ugly but if there is no end this is what we will be missing
    <std::string> UNKNOWN_DIRECTIVE
    <int> INT "integer"
    <float> FLOAT "float"
;


%left REVERSE
%left SHL SHR
%left PLUS MINUS
%left MULTIPLY DIVIDE
%left AND OR XOR

%printer { yyo << "..."; } <*>;

%%

file:
    lines END { if (pioasm.error_count || pioasm.write_output()) YYABORT; }
    ;

lines:
  line
  | lines NEWLINE line;

line:
    PROGRAM ID                                  { if (!pioasm.add_program(@$, $2)) { std::stringstream msg; msg << "program " << $2 << " already exists"; error(@$, msg.str()); abort(); } }
  | directive
  | instruction                                 { pioasm.get_current_program(@1, "instruction").add_instruction($1); }
  | label_decl instruction                      { auto &p = pioasm.get_current_program(@2, "instruction"); p.add_label($1); p.add_instruction($2); }
  | label_decl                                  { pioasm.get_current_program(@1, "label").add_label($1); }
  | code_block
  | %empty
  | error                                       { if (pioasm.error_count > 6) { std::cerr << "\ntoo many errors; aborting.\n"; YYABORT; } }
  ;

code_block:
  CODE_BLOCK_START CODE_BLOCK_CONTENTS          { std::string of = $1; if (of.empty()) of = output_format::default_name; pioasm.get_current_program(@$, "code block", false, false).add_code_block( code_block(@$, of, $2)); }

%type <std::shared_ptr<symbol>> label_decl;
label_decl:
    symbol_def COLON        { $1->is_label = true; $$ = $1; }

directive:
    DEFINE symbol_def expression      { $2->is_label = false; $2->value = $3; pioasm.get_current_program(@1, ".define", false, false).add_symbol($2); }
  | ORIGIN value                      { pioasm.get_current_program(@1, ".origin", true).set_origin(@$, $2); }
  | PIO_VERSION pio_version           { pioasm.get_current_program(@1, ".pio_version", true, false).set_pio_version(@$, $2); }
  | SIDE_SET value OPTIONAL PINDIRS   { pioasm.get_current_program(@1, ".side_set", true).set_sideset(@$, $2, true, true); }
  | SIDE_SET value OPTIONAL           { pioasm.get_current_program(@1, ".side_set", true).set_sideset(@$, $2, true, false); }
  | SIDE_SET value PINDIRS            { pioasm.get_current_program(@1, ".side_set", true).set_sideset(@$, $2, false, true); }
  | SIDE_SET value                    { pioasm.get_current_program(@1, ".side_set", true).set_sideset(@$, $2, false, false); }
  | DOT_IN value direction autop threshold { pioasm.get_current_program(@1, ".in", true).set_in(@$, $2, $3, $4, $5); }
  | DOT_OUT value direction autop threshold { pioasm.get_current_program(@1, ".out", true).set_out(@$, $2, $3, $4, $5); }
  | DOT_SET value                     { pioasm.get_current_program(@1, ".set", true).set_set_count(@$, $2); }
  | WRAP_TARGET                       { pioasm.get_current_program(@1, ".wrap_target").set_wrap_target(@$); }
  | WRAP                              { pioasm.get_current_program(@1, ".wrap").set_wrap(@$); }
  | WORD value                        { pioasm.get_current_program(@1, "instruction").add_instruction(std::shared_ptr<instruction>(new instr_word(@$, $2))); }
  | LANG_OPT NON_WS NON_WS ASSIGN INT  { pioasm.get_current_program(@1, ".lang_opt").add_lang_opt($2, $3, std::to_string($5)); }
  | LANG_OPT NON_WS NON_WS ASSIGN STRING { pioasm.get_current_program(@1, ".lang_opt").add_lang_opt($2, $3, $5); }
  | LANG_OPT NON_WS NON_WS ASSIGN NON_WS { pioasm.get_current_program(@1, ".lang_opt").add_lang_opt($2, $3, $5); }
  | LANG_OPT error                    { error(@$, "expected format is .lang_opt language option_name = option_value"); }
  | CLOCK_DIV INT                     { pioasm.get_current_program(@1, ".clock_div").set_clock_div(@$, $2); }
  | CLOCK_DIV FLOAT                   { pioasm.get_current_program(@1, ".clock_div").set_clock_div(@$, $2); }
  | FIFO fifo_config                  { pioasm.get_current_program(@1, ".fifo", true).set_fifo_config(@$, $2); }
  | MOV_STATUS TXFIFO LESSTHAN value  { pioasm.get_current_program(@1, ".mov_status", true).set_mov_status(mov_status_type::tx_lessthan, $4); }
  | MOV_STATUS RXFIFO LESSTHAN value  { pioasm.get_current_program(@1, ".mov_status", true).set_mov_status(mov_status_type::rx_lessthan, $4); }
  | MOV_STATUS IRQ NEXT SET value          { pioasm.get_current_program(@1, ".mov_status", true).set_mov_status(mov_status_type::irq_set, $5, 2); }
  | MOV_STATUS IRQ PREV SET value          { pioasm.get_current_program(@1, ".mov_status", true).set_mov_status(mov_status_type::irq_set, $5, 1); }
  | MOV_STATUS IRQ SET value          { pioasm.get_current_program(@1, ".mov_status", true).set_mov_status(mov_status_type::irq_set, $4); }
  | MOV_STATUS                        { error(@0, "expected 'txfifo < N', 'rxfifo < N' or 'irq set N'"); }
  | UNKNOWN_DIRECTIVE                 { std::stringstream msg; msg << "unknown directive " << $1; throw syntax_error(@$, msg.str()); }
  ;

/* value is a more limited top level expression... requiring parenthesis */
%type <std::shared_ptr<resolvable>> value;
value: INT { $$ = resolvable_int(@$, $1); }
     | ID { $$ = std::shared_ptr<resolvable>(new name_ref(@$, $1)); }
     | LPAREN expression RPAREN { $$ = $2; }

%type <std::shared_ptr<resolvable>> expression;
expression:
     value
     | expression PLUS expression { $$ = std::shared_ptr<binary_operation>(new binary_operation(@$, binary_operation::add, $1, $3)); }
     | expression MINUS expression { $$ = std::shared_ptr<binary_operation>(new binary_operation(@$, binary_operation::subtract, $1, $3)); }
     | expression MULTIPLY expression { $$ = std::shared_ptr<binary_operation>(new binary_operation(@$, binary_operation::multiply, $1, $3));  }
     | expression DIVIDE expression { $$ = std::shared_ptr<binary_operation>(new binary_operation(@$, binary_operation::divide, $1, $3)); }
     | expression OR expression { $$ = std::shared_ptr<binary_operation>(new binary_operation(@$, binary_operation::or_, $1, $3)); }
     | expression AND expression { $$ = std::shared_ptr<binary_operation>(new binary_operation(@$, binary_operation::and_, $1, $3)); }
     | expression XOR expression { $$ = std::shared_ptr<binary_operation>(new binary_operation(@$, binary_operation::xor_, $1, $3)); }
     | expression SHL expression { $$ = std::shared_ptr<binary_operation>(new binary_operation(@$, binary_operation::shl_, $1, $3)); }
     | expression SHR expression { $$ = std::shared_ptr<binary_operation>(new binary_operation(@$, binary_operation::shr_, $1, $3)); }
     | MINUS expression { $$ = std::shared_ptr<unary_operation>(new unary_operation(@$, unary_operation::negate, $2)); }
     | REVERSE expression { $$ = std::shared_ptr<unary_operation>(new unary_operation(@$, unary_operation::reverse, $2)); }

%type <uint> pio_version;
pio_version: INT { $$ = $1; }
     | RP2040 { $$ = 0; }
     | RP2350 { $$ = 1; }

%type <std::shared_ptr<instruction>> instruction;
instruction:
    base_instruction sideset delay { $$ = $1; $$->sideset = $2; $$->delay = $3; }
  | base_instruction delay sideset { $$ = $1; $$->delay = $2; $$->sideset = $3; }
  | base_instruction sideset { $$ = $1; $$->sideset = $2; $$->delay = resolvable_int(@$, 0); }
  | base_instruction delay { $$ = $1; $$->delay = $2; }
  | base_instruction { $$ = $1; $$->delay = resolvable_int(@$, 0); }

%type <std::shared_ptr<instruction>> base_instruction;
base_instruction:
    NOP                                                   { $$ = std::shared_ptr<instruction>(new instr_nop(@$)); }
    | JMP condition comma expression                      { $$ = std::shared_ptr<instruction>(new instr_jmp(@$, $2, $4)); }
    | WAIT value wait_source                              { $$ = std::shared_ptr<instruction>(new instr_wait(@$, $2, $3)); }
    | WAIT wait_source                                    { $$ = std::shared_ptr<instruction>(new instr_wait(@$, resolvable_int(@$, 1),  $2)); }
    | IN in_source comma value                            { $$ = std::shared_ptr<instruction>(new instr_in(@$, $2, $4)); }
    | OUT out_target comma value                          { $$ = std::shared_ptr<instruction>(new instr_out(@$, $2, $4)); }
    | PUSH if_full blocking                               { $$ = std::shared_ptr<instruction>(new instr_push(@$, $2, $3)); }
    | PULL if_empty blocking                              { $$ = std::shared_ptr<instruction>(new instr_pull(@$, $2, $3)); }
    | MOV mov_target comma mov_op mov_source              { $$ = std::shared_ptr<instruction>(new instr_mov(@$, $2, $5, $4)); }
    | IRQ irq_modifiers value REL                         { $$ = std::shared_ptr<instruction>(new instr_irq(@$, $2, $3, 2)); }
    | IRQ PREV irq_modifiers value                        { pioasm.check_version(1, @$, "irq prev"); $$ = std::shared_ptr<instruction>(new instr_irq(@$, $3, $4, 1)); }
    | IRQ NEXT irq_modifiers value                        { pioasm.check_version(1, @$, "irq next"); $$ = std::shared_ptr<instruction>(new instr_irq(@$, $3, $4, 3)); }
    | IRQ PREV irq_modifiers value REL                    { pioasm.check_version(1, @$, "irq prev"); error(@5, "'rel' is not supported for 'irq prev'"); }
    | IRQ NEXT irq_modifiers value REL                    { pioasm.check_version(1, @$, "irq next"); error(@5, "'rel' is not supported for 'irq next'"); }
    | IRQ irq_modifiers value                             { $$ = std::shared_ptr<instruction>(new instr_irq(@$, $2, $3)); }
    | SET set_target comma value                          { $$ = std::shared_ptr<instruction>(new instr_set(@$, $2, $4)); }
;

%type <std::shared_ptr<resolvable>> delay;
delay:
    LBRACKET expression RBRACKET { $$ = $2; }

%type <std::shared_ptr<resolvable>> sideset;
sideset:
    SIDE value { $$ = $2; }

%type <enum condition> condition;
condition:
    NOT X                   { $$ = condition::xz; }
  | X POST_DECREMENT        { $$ = condition::xnz__; }
  | NOT Y                   { $$ = condition::yz; }
  | Y POST_DECREMENT        { $$ = condition::ynz__; }
  | X NOT_EQUAL Y           { $$ = condition::xney; }
  | PIN                     { $$ = condition::pin; }
  | NOT OSRE                { $$ = condition::osrez; }
  | %empty                  { $$ = condition::al; }

%type <std::shared_ptr<wait_source>> wait_source;
wait_source:
    IRQ comma value REL     { $$ = std::shared_ptr<wait_source>(new wait_source(wait_source::irq, $3, 2)); }
  | IRQ PREV comma value    { pioasm.check_version(1, @$, "irq prev"); $$ = std::shared_ptr<wait_source>(new wait_source(wait_source::irq, $4, 1)); }
  | IRQ NEXT comma value    { pioasm.check_version(1, @$, "irq next"); $$ = std::shared_ptr<wait_source>(new wait_source(wait_source::irq, $4, 3)); }
  | IRQ PREV comma value REL { pioasm.check_version(1, @$, "irq prev"); error(@5, "'rel' is not supported for 'irq prev'"); }
  | IRQ NEXT comma value REL { pioasm.check_version(1, @$, "irq next"); error(@5, "'rel' is not supported for 'irq next'"); }
  | IRQ comma value         { $$ = std::shared_ptr<wait_source>(new wait_source(wait_source::irq, $3, 0)); }
  | GPIO comma value        { $$ = std::shared_ptr<wait_source>(new wait_source(wait_source::gpio, $3)); }
  | PIN comma value         { $$ = std::shared_ptr<wait_source>(new wait_source(wait_source::pin, $3)); }
  | JMPPIN                  { pioasm.check_version(1, @$, "wait jmppin"); $$ = std::shared_ptr<wait_source>(new wait_source(wait_source::jmppin, std::make_shared<int_value>(@$, 0))); }
  | JMPPIN PLUS value       { pioasm.check_version(1, @$, "wait jmppin"); $$ = std::shared_ptr<wait_source>(new wait_source(wait_source::jmppin, $3)); }
  | %empty                  { error(@0, pioasm.version_string(1, "expected irq, gpio, pin or jmp_pin", "expected irq, gpio or pin")); }

%type <enum fifo_config> fifo_config;
fifo_config: TXRX { $$ = fifo_config::txrx; }
    | TX        { $$ = fifo_config::tx; }
    | RX        { $$ = fifo_config::rx; }
    | TXPUT     { pioasm.check_version(1, @$, "txput"); $$ = fifo_config::txput; }
    | TXGET     { pioasm.check_version(1, @$, "rxput"); $$ = fifo_config::txget; }
    | PUTGET    { pioasm.check_version(1, @$, "putget"); $$ = fifo_config::putget; }
    | %empty    { error(@0, pioasm.version_string(1, "expected txrx, tx, rx, txput, rxget or putget", "expected txrx, tx or rx")); }


comma: COMMA | %empty        /* not a huge fan of forcing commas */

%type <enum in_out_set> in_source;
in_source: PINS { $$ = in_out_set::in_out_set_pins; }
    | X         { $$ = in_out_set::in_out_set_x; }
    | Y         { $$ = in_out_set::in_out_set_y; }
    | NULL      { $$ = in_out_set::in_out_null; }
    | ISR       { $$ = in_out_set::in_out_isr; }
    | OSR       { $$ = in_out_set::in_osr; }
    | STATUS    { $$ = in_out_set::in_status; }

%type <enum in_out_set> out_target;
out_target: PINS { $$ = in_out_set::in_out_set_pins; }
    | X          { $$ = in_out_set::in_out_set_x; }
    | Y          { $$ = in_out_set::in_out_set_y; }
    | NULL       { $$ = in_out_set::in_out_null; }
    | PINDIRS    { $$ = in_out_set::in_out_set_pindirs; }
    | ISR        { $$ = in_out_set::in_out_isr; }
    | PC         { $$ = in_out_set::out_set_pc; }
    | EXEC       { $$ = in_out_set::out_exec; }

%type <extended_mov> mov_target;
mov_target: PINS { $$ = mov::pins; }
    | X          { $$ = mov::x; }
    | Y          { $$ = mov::y; }
    | EXEC       { $$ = mov::exec; }
    | PC         { $$ = mov::pc; }
    | ISR        { $$ = mov::isr; }
    | OSR        { $$ = mov::osr; }
    | PINDIRS    { pioasm.check_version(1, @$, "mov pindirs"); $$ = mov::pindirs; }
    | RXFIFO LBRACKET Y RBRACKET { pioasm.check_version(1, @$, "mov rxfifo[], "); $$ = mov::fifo_y; }
    | RXFIFO LBRACKET value RBRACKET { pioasm.check_version(1, @$, "mov rxfifo[], "); $$ = extended_mov($3); }

%type <extended_mov> mov_source;
mov_source: PINS { $$ = mov::pins; }
    | X          { $$ = mov::x; }
    | Y          { $$ = mov::y; }
    | NULL       { $$ = mov::null; }
    | STATUS     { $$ = mov::status; }
    | ISR        { $$ = mov::isr; }
    | OSR        { $$ = mov::osr; }
    | RXFIFO LBRACKET Y RBRACKET { pioasm.check_version(1, @$, "mov rxfifo[], "); $$ = mov::fifo_y; }
    | RXFIFO LBRACKET value RBRACKET { pioasm.check_version(1, @$, "mov rxfifo[], "); $$ = extended_mov($3); }

%type <enum mov_op> mov_op;
mov_op:
    NOT         { $$ = mov_op::invert; }
  | REVERSE     { $$ = mov_op::bit_reverse; }
  | %empty      { $$ = mov_op::none; }

%type <enum in_out_set> set_target;
set_target:
    PINS        { $$ = in_out_set::in_out_set_pins; }
  | X           { $$ = in_out_set::in_out_set_x; }
  | Y           { $$ = in_out_set::in_out_set_y; }
  | PINDIRS     { $$ = in_out_set::in_out_set_pindirs; }

%type <bool> direction;
direction:
    LEFT { $$ = false; }
  | RIGHT { $$ = true; }
  | %empty { $$ = true; }

%type <bool> autop;
autop:
    AUTO { $$ = true; }
  | MANUAL { $$ = false; }
  | %empty { $$ = false; }

%type <std::shared_ptr<resolvable>> threshold;
threshold: value { $$ = $1; }
  | %empty { $$ = resolvable_int(@$, 32); }

%type <bool> if_full;
if_full:
    IFFULL { $$ = true; }
  | %empty { $$ = false; }

%type <bool> if_empty;
if_empty:
    IFEMPTY { $$ = true; }
  | %empty  { $$ = false; }

%type <bool> blocking;
blocking:
    BLOCK   { $$ = true; }
  | NOBLOCK { $$ = false; }
  | %empty  { $$ = true; }

%type <enum irq> irq_modifiers;
irq_modifiers:
    CLEAR          { $$ = irq::clear; }
  | WAIT           { $$ = irq::set_wait; }
  | NOWAIT         { $$ = irq::set; }
  | SET            { $$ = irq::set; }
  | %empty         { $$ = irq::set; }

%type <std::shared_ptr<symbol>> symbol_def;
symbol_def:
    ID              { $$ = std::shared_ptr<symbol>(new symbol(@$, $1)); }
  | PUBLIC ID       { $$ = std::shared_ptr<symbol>(new symbol(@$, $2, true)); }
  | MULTIPLY ID     { $$ = std::shared_ptr<symbol>(new symbol(@$, $2, true)); }

%%
void yy::parser::error(const location_type& l, const std::string& m)
{
   if (l.begin.filename) {
      std::cerr << l << ": " << m << '\n';
      pioasm.error_count++;
      if (l.begin.line == l.end.line && *l.begin.filename == *l.end.filename) {
        std::ifstream file(l.begin.filename->c_str());
        std::string line;
        for(int i = 0; i < l.begin.line; ++i) {
             std::getline(file, line);
        }
        fprintf(stderr, "%5d | %s\n", l.begin.line, line.c_str());
        fprintf(stderr, "%5s | %*s", "", l.begin.column, "^");
        for (int i = l.begin.column; i < l.end.column - 1; i++) {
              putc ('~', stderr);
        }
        putc ('\n', stderr);
      }
  } else {
      std::cerr << m << '\n';
  }
}
