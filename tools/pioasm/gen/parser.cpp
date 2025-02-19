// A Bison parser, made by GNU Bison 3.7.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2020 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.





#include "parser.hpp"


// Unqualified %code blocks.

    #include "pio_assembler.h"
  #ifdef _MSC_VER
  #pragma warning(disable : 4244) // possible loss of data (valid warning, but there is a software check / missing cast)
  #endif



#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

namespace yy {

  /// Build a parser object.
  parser::parser (pio_assembler& pioasm_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      yy_lac_established_ (false),
      pioasm (pioasm_yyarg)
  {}

  parser::~parser ()
  {}

  parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | symbol kinds.  |
  `---------------*/



  // by_state.
  parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  parser::symbol_kind_type
  parser::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_direction: // direction
      case symbol_kind::S_autop: // autop
      case symbol_kind::S_if_full: // if_full
      case symbol_kind::S_if_empty: // if_empty
      case symbol_kind::S_blocking: // blocking
        value.YY_MOVE_OR_COPY< bool > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_condition: // condition
        value.YY_MOVE_OR_COPY< enum condition > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_fifo_config: // fifo_config
        value.YY_MOVE_OR_COPY< enum fifo_config > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_in_source: // in_source
      case symbol_kind::S_out_target: // out_target
      case symbol_kind::S_set_target: // set_target
        value.YY_MOVE_OR_COPY< enum in_out_set > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
        value.YY_MOVE_OR_COPY< enum irq > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_mov_op: // mov_op
        value.YY_MOVE_OR_COPY< enum mov_op > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_mov_target: // mov_target
      case symbol_kind::S_mov_source: // mov_source
        value.YY_MOVE_OR_COPY< extended_mov > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FLOAT: // "float"
        value.YY_MOVE_OR_COPY< float > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_INT: // "integer"
        value.YY_MOVE_OR_COPY< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_instruction: // instruction
      case symbol_kind::S_base_instruction: // base_instruction
        value.YY_MOVE_OR_COPY< std::shared_ptr<instruction> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_value: // value
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_delay: // delay
      case symbol_kind::S_sideset: // sideset
      case symbol_kind::S_threshold: // threshold
        value.YY_MOVE_OR_COPY< std::shared_ptr<resolvable> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_label_decl: // label_decl
      case symbol_kind::S_symbol_def: // symbol_def
        value.YY_MOVE_OR_COPY< std::shared_ptr<symbol> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_wait_source: // wait_source
        value.YY_MOVE_OR_COPY< std::shared_ptr<wait_source> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ID: // "identifier"
      case symbol_kind::S_STRING: // "string"
      case symbol_kind::S_NON_WS: // "text"
      case symbol_kind::S_CODE_BLOCK_START: // "code block"
      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_pio_version: // pio_version
        value.YY_MOVE_OR_COPY< uint > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_direction: // direction
      case symbol_kind::S_autop: // autop
      case symbol_kind::S_if_full: // if_full
      case symbol_kind::S_if_empty: // if_empty
      case symbol_kind::S_blocking: // blocking
        value.move< bool > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_condition: // condition
        value.move< enum condition > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_fifo_config: // fifo_config
        value.move< enum fifo_config > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_in_source: // in_source
      case symbol_kind::S_out_target: // out_target
      case symbol_kind::S_set_target: // set_target
        value.move< enum in_out_set > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
        value.move< enum irq > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_mov_op: // mov_op
        value.move< enum mov_op > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_mov_target: // mov_target
      case symbol_kind::S_mov_source: // mov_source
        value.move< extended_mov > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FLOAT: // "float"
        value.move< float > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_INT: // "integer"
        value.move< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_instruction: // instruction
      case symbol_kind::S_base_instruction: // base_instruction
        value.move< std::shared_ptr<instruction> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_value: // value
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_delay: // delay
      case symbol_kind::S_sideset: // sideset
      case symbol_kind::S_threshold: // threshold
        value.move< std::shared_ptr<resolvable> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_label_decl: // label_decl
      case symbol_kind::S_symbol_def: // symbol_def
        value.move< std::shared_ptr<symbol> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_wait_source: // wait_source
        value.move< std::shared_ptr<wait_source> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ID: // "identifier"
      case symbol_kind::S_STRING: // "string"
      case symbol_kind::S_NON_WS: // "text"
      case symbol_kind::S_CODE_BLOCK_START: // "code block"
      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
        value.move< std::string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_pio_version: // pio_version
        value.move< uint > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_direction: // direction
      case symbol_kind::S_autop: // autop
      case symbol_kind::S_if_full: // if_full
      case symbol_kind::S_if_empty: // if_empty
      case symbol_kind::S_blocking: // blocking
        value.copy< bool > (that.value);
        break;

      case symbol_kind::S_condition: // condition
        value.copy< enum condition > (that.value);
        break;

      case symbol_kind::S_fifo_config: // fifo_config
        value.copy< enum fifo_config > (that.value);
        break;

      case symbol_kind::S_in_source: // in_source
      case symbol_kind::S_out_target: // out_target
      case symbol_kind::S_set_target: // set_target
        value.copy< enum in_out_set > (that.value);
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
        value.copy< enum irq > (that.value);
        break;

      case symbol_kind::S_mov_op: // mov_op
        value.copy< enum mov_op > (that.value);
        break;

      case symbol_kind::S_mov_target: // mov_target
      case symbol_kind::S_mov_source: // mov_source
        value.copy< extended_mov > (that.value);
        break;

      case symbol_kind::S_FLOAT: // "float"
        value.copy< float > (that.value);
        break;

      case symbol_kind::S_INT: // "integer"
        value.copy< int > (that.value);
        break;

      case symbol_kind::S_instruction: // instruction
      case symbol_kind::S_base_instruction: // base_instruction
        value.copy< std::shared_ptr<instruction> > (that.value);
        break;

      case symbol_kind::S_value: // value
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_delay: // delay
      case symbol_kind::S_sideset: // sideset
      case symbol_kind::S_threshold: // threshold
        value.copy< std::shared_ptr<resolvable> > (that.value);
        break;

      case symbol_kind::S_label_decl: // label_decl
      case symbol_kind::S_symbol_def: // symbol_def
        value.copy< std::shared_ptr<symbol> > (that.value);
        break;

      case symbol_kind::S_wait_source: // wait_source
        value.copy< std::shared_ptr<wait_source> > (that.value);
        break;

      case symbol_kind::S_ID: // "identifier"
      case symbol_kind::S_STRING: // "string"
      case symbol_kind::S_NON_WS: // "text"
      case symbol_kind::S_CODE_BLOCK_START: // "code block"
      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
        value.copy< std::string > (that.value);
        break;

      case symbol_kind::S_pio_version: // pio_version
        value.copy< uint > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_direction: // direction
      case symbol_kind::S_autop: // autop
      case symbol_kind::S_if_full: // if_full
      case symbol_kind::S_if_empty: // if_empty
      case symbol_kind::S_blocking: // blocking
        value.move< bool > (that.value);
        break;

      case symbol_kind::S_condition: // condition
        value.move< enum condition > (that.value);
        break;

      case symbol_kind::S_fifo_config: // fifo_config
        value.move< enum fifo_config > (that.value);
        break;

      case symbol_kind::S_in_source: // in_source
      case symbol_kind::S_out_target: // out_target
      case symbol_kind::S_set_target: // set_target
        value.move< enum in_out_set > (that.value);
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
        value.move< enum irq > (that.value);
        break;

      case symbol_kind::S_mov_op: // mov_op
        value.move< enum mov_op > (that.value);
        break;

      case symbol_kind::S_mov_target: // mov_target
      case symbol_kind::S_mov_source: // mov_source
        value.move< extended_mov > (that.value);
        break;

      case symbol_kind::S_FLOAT: // "float"
        value.move< float > (that.value);
        break;

      case symbol_kind::S_INT: // "integer"
        value.move< int > (that.value);
        break;

      case symbol_kind::S_instruction: // instruction
      case symbol_kind::S_base_instruction: // base_instruction
        value.move< std::shared_ptr<instruction> > (that.value);
        break;

      case symbol_kind::S_value: // value
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_delay: // delay
      case symbol_kind::S_sideset: // sideset
      case symbol_kind::S_threshold: // threshold
        value.move< std::shared_ptr<resolvable> > (that.value);
        break;

      case symbol_kind::S_label_decl: // label_decl
      case symbol_kind::S_symbol_def: // symbol_def
        value.move< std::shared_ptr<symbol> > (that.value);
        break;

      case symbol_kind::S_wait_source: // wait_source
        value.move< std::shared_ptr<wait_source> > (that.value);
        break;

      case symbol_kind::S_ID: // "identifier"
      case symbol_kind::S_STRING: // "string"
      case symbol_kind::S_NON_WS: // "text"
      case symbol_kind::S_CODE_BLOCK_START: // "code block"
      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
        value.move< std::string > (that.value);
        break;

      case symbol_kind::S_pio_version: // pio_version
        value.move< uint > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " ("
            << yysym.location << ": ";
        switch (yykind)
    {
      case symbol_kind::S_ID: // "identifier"
                 { yyo << "..."; }
        break;

      case symbol_kind::S_STRING: // "string"
                 { yyo << "..."; }
        break;

      case symbol_kind::S_NON_WS: // "text"
                 { yyo << "..."; }
        break;

      case symbol_kind::S_CODE_BLOCK_START: // "code block"
                 { yyo << "..."; }
        break;

      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
                 { yyo << "..."; }
        break;

      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
                 { yyo << "..."; }
        break;

      case symbol_kind::S_INT: // "integer"
                 { yyo << "..."; }
        break;

      case symbol_kind::S_FLOAT: // "float"
                 { yyo << "..."; }
        break;

      case symbol_kind::S_label_decl: // label_decl
                 { yyo << "..."; }
        break;

      case symbol_kind::S_value: // value
                 { yyo << "..."; }
        break;

      case symbol_kind::S_expression: // expression
                 { yyo << "..."; }
        break;

      case symbol_kind::S_pio_version: // pio_version
                 { yyo << "..."; }
        break;

      case symbol_kind::S_instruction: // instruction
                 { yyo << "..."; }
        break;

      case symbol_kind::S_base_instruction: // base_instruction
                 { yyo << "..."; }
        break;

      case symbol_kind::S_delay: // delay
                 { yyo << "..."; }
        break;

      case symbol_kind::S_sideset: // sideset
                 { yyo << "..."; }
        break;

      case symbol_kind::S_condition: // condition
                 { yyo << "..."; }
        break;

      case symbol_kind::S_wait_source: // wait_source
                 { yyo << "..."; }
        break;

      case symbol_kind::S_fifo_config: // fifo_config
                 { yyo << "..."; }
        break;

      case symbol_kind::S_in_source: // in_source
                 { yyo << "..."; }
        break;

      case symbol_kind::S_out_target: // out_target
                 { yyo << "..."; }
        break;

      case symbol_kind::S_mov_target: // mov_target
                 { yyo << "..."; }
        break;

      case symbol_kind::S_mov_source: // mov_source
                 { yyo << "..."; }
        break;

      case symbol_kind::S_mov_op: // mov_op
                 { yyo << "..."; }
        break;

      case symbol_kind::S_set_target: // set_target
                 { yyo << "..."; }
        break;

      case symbol_kind::S_direction: // direction
                 { yyo << "..."; }
        break;

      case symbol_kind::S_autop: // autop
                 { yyo << "..."; }
        break;

      case symbol_kind::S_threshold: // threshold
                 { yyo << "..."; }
        break;

      case symbol_kind::S_if_full: // if_full
                 { yyo << "..."; }
        break;

      case symbol_kind::S_if_empty: // if_empty
                 { yyo << "..."; }
        break;

      case symbol_kind::S_blocking: // blocking
                 { yyo << "..."; }
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
                 { yyo << "..."; }
        break;

      case symbol_kind::S_symbol_def: // symbol_def
                 { yyo << "..."; }
        break;

      default:
        break;
    }
        yyo << ')';
      }
  }
#endif

  void
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

    /// Discard the LAC context in case there still is one left from a
    /// previous invocation.
    yy_lac_discard_ ("init");

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (pioasm));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        if (!yy_lac_establish_ (yyla.kind ()))
           goto yyerrlab;
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        if (!yy_lac_establish_ (yyla.kind ()))
           goto yyerrlab;

        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    yy_lac_discard_ ("shift");
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case symbol_kind::S_direction: // direction
      case symbol_kind::S_autop: // autop
      case symbol_kind::S_if_full: // if_full
      case symbol_kind::S_if_empty: // if_empty
      case symbol_kind::S_blocking: // blocking
        yylhs.value.emplace< bool > ();
        break;

      case symbol_kind::S_condition: // condition
        yylhs.value.emplace< enum condition > ();
        break;

      case symbol_kind::S_fifo_config: // fifo_config
        yylhs.value.emplace< enum fifo_config > ();
        break;

      case symbol_kind::S_in_source: // in_source
      case symbol_kind::S_out_target: // out_target
      case symbol_kind::S_set_target: // set_target
        yylhs.value.emplace< enum in_out_set > ();
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
        yylhs.value.emplace< enum irq > ();
        break;

      case symbol_kind::S_mov_op: // mov_op
        yylhs.value.emplace< enum mov_op > ();
        break;

      case symbol_kind::S_mov_target: // mov_target
      case symbol_kind::S_mov_source: // mov_source
        yylhs.value.emplace< extended_mov > ();
        break;

      case symbol_kind::S_FLOAT: // "float"
        yylhs.value.emplace< float > ();
        break;

      case symbol_kind::S_INT: // "integer"
        yylhs.value.emplace< int > ();
        break;

      case symbol_kind::S_instruction: // instruction
      case symbol_kind::S_base_instruction: // base_instruction
        yylhs.value.emplace< std::shared_ptr<instruction> > ();
        break;

      case symbol_kind::S_value: // value
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_delay: // delay
      case symbol_kind::S_sideset: // sideset
      case symbol_kind::S_threshold: // threshold
        yylhs.value.emplace< std::shared_ptr<resolvable> > ();
        break;

      case symbol_kind::S_label_decl: // label_decl
      case symbol_kind::S_symbol_def: // symbol_def
        yylhs.value.emplace< std::shared_ptr<symbol> > ();
        break;

      case symbol_kind::S_wait_source: // wait_source
        yylhs.value.emplace< std::shared_ptr<wait_source> > ();
        break;

      case symbol_kind::S_ID: // "identifier"
      case symbol_kind::S_STRING: // "string"
      case symbol_kind::S_NON_WS: // "text"
      case symbol_kind::S_CODE_BLOCK_START: // "code block"
      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
        yylhs.value.emplace< std::string > ();
        break;

      case symbol_kind::S_pio_version: // pio_version
        yylhs.value.emplace< uint > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2: // file: lines "end of file"
              { if (pioasm.error_count || pioasm.write_output()) YYABORT; }
    break;

  case 5: // line: ".program" "identifier"
                                                { if (!pioasm.add_program(yylhs.location, yystack_[0].value.as < std::string > ())) { std::stringstream msg; msg << "program " << yystack_[0].value.as < std::string > () << " already exists"; error(yylhs.location, msg.str()); abort(); } }
    break;

  case 7: // line: instruction
                                                { pioasm.get_current_program(yystack_[0].location, "instruction").add_instruction(yystack_[0].value.as < std::shared_ptr<instruction> > ()); }
    break;

  case 8: // line: label_decl instruction
                                                { auto &p = pioasm.get_current_program(yystack_[0].location, "instruction"); p.add_label(yystack_[1].value.as < std::shared_ptr<symbol> > ()); p.add_instruction(yystack_[0].value.as < std::shared_ptr<instruction> > ()); }
    break;

  case 9: // line: label_decl
                                                { pioasm.get_current_program(yystack_[0].location, "label").add_label(yystack_[0].value.as < std::shared_ptr<symbol> > ()); }
    break;

  case 12: // line: error
                                                { if (pioasm.error_count > 6) { std::cerr << "\ntoo many errors; aborting.\n"; YYABORT; } }
    break;

  case 13: // code_block: "code block" "%}"
                                                { std::string of = yystack_[1].value.as < std::string > (); if (of.empty()) of = output_format::default_name; pioasm.get_current_program(yylhs.location, "code block", false, false).add_code_block( code_block(yylhs.location, of, yystack_[0].value.as < std::string > ())); }
    break;

  case 14: // label_decl: symbol_def ":"
                            { yystack_[1].value.as < std::shared_ptr<symbol> > ()->is_label = true; yylhs.value.as < std::shared_ptr<symbol> > () = yystack_[1].value.as < std::shared_ptr<symbol> > (); }
    break;

  case 15: // directive: ".define" symbol_def expression
                                      { yystack_[1].value.as < std::shared_ptr<symbol> > ()->is_label = false; yystack_[1].value.as < std::shared_ptr<symbol> > ()->value = yystack_[0].value.as < std::shared_ptr<resolvable> > (); pioasm.get_current_program(yystack_[2].location, ".define", false, false).add_symbol(yystack_[1].value.as < std::shared_ptr<symbol> > ()); }
    break;

  case 16: // directive: ".origin" value
                                      { pioasm.get_current_program(yystack_[1].location, ".origin", true).set_origin(yylhs.location, yystack_[0].value.as < std::shared_ptr<resolvable> > ()); }
    break;

  case 17: // directive: ".pio_version" pio_version
                                      { pioasm.get_current_program(yystack_[1].location, ".pio_version", true, false).set_pio_version(yylhs.location, yystack_[0].value.as < uint > ()); }
    break;

  case 18: // directive: ".side_set" value "opt" "pindirs"
                                      { pioasm.get_current_program(yystack_[3].location, ".side_set", true).set_sideset(yylhs.location, yystack_[2].value.as < std::shared_ptr<resolvable> > (), true, true); }
    break;

  case 19: // directive: ".side_set" value "opt"
                                      { pioasm.get_current_program(yystack_[2].location, ".side_set", true).set_sideset(yylhs.location, yystack_[1].value.as < std::shared_ptr<resolvable> > (), true, false); }
    break;

  case 20: // directive: ".side_set" value "pindirs"
                                      { pioasm.get_current_program(yystack_[2].location, ".side_set", true).set_sideset(yylhs.location, yystack_[1].value.as < std::shared_ptr<resolvable> > (), false, true); }
    break;

  case 21: // directive: ".side_set" value
                                      { pioasm.get_current_program(yystack_[1].location, ".side_set", true).set_sideset(yylhs.location, yystack_[0].value.as < std::shared_ptr<resolvable> > (), false, false); }
    break;

  case 22: // directive: ".in" value direction autop threshold
                                           { pioasm.get_current_program(yystack_[4].location, ".in", true).set_in(yylhs.location, yystack_[3].value.as < std::shared_ptr<resolvable> > (), yystack_[2].value.as < bool > (), yystack_[1].value.as < bool > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ()); }
    break;

  case 23: // directive: ".out" value direction autop threshold
                                            { pioasm.get_current_program(yystack_[4].location, ".out", true).set_out(yylhs.location, yystack_[3].value.as < std::shared_ptr<resolvable> > (), yystack_[2].value.as < bool > (), yystack_[1].value.as < bool > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ()); }
    break;

  case 24: // directive: ".set" value
                                      { pioasm.get_current_program(yystack_[1].location, ".set", true).set_set_count(yylhs.location, yystack_[0].value.as < std::shared_ptr<resolvable> > ()); }
    break;

  case 25: // directive: ".wrap_target"
                                      { pioasm.get_current_program(yystack_[0].location, ".wrap_target").set_wrap_target(yylhs.location); }
    break;

  case 26: // directive: ".wrap"
                                      { pioasm.get_current_program(yystack_[0].location, ".wrap").set_wrap(yylhs.location); }
    break;

  case 27: // directive: ".word" value
                                      { pioasm.get_current_program(yystack_[1].location, "instruction").add_instruction(std::shared_ptr<instruction>(new instr_word(yylhs.location, yystack_[0].value.as < std::shared_ptr<resolvable> > ()))); }
    break;

  case 28: // directive: ".lang_opt" "text" "text" "=" "integer"
                                       { pioasm.get_current_program(yystack_[4].location, ".lang_opt").add_lang_opt(yystack_[3].value.as < std::string > (), yystack_[2].value.as < std::string > (), std::to_string(yystack_[0].value.as < int > ())); }
    break;

  case 29: // directive: ".lang_opt" "text" "text" "=" "string"
                                         { pioasm.get_current_program(yystack_[4].location, ".lang_opt").add_lang_opt(yystack_[3].value.as < std::string > (), yystack_[2].value.as < std::string > (), yystack_[0].value.as < std::string > ()); }
    break;

  case 30: // directive: ".lang_opt" "text" "text" "=" "text"
                                         { pioasm.get_current_program(yystack_[4].location, ".lang_opt").add_lang_opt(yystack_[3].value.as < std::string > (), yystack_[2].value.as < std::string > (), yystack_[0].value.as < std::string > ()); }
    break;

  case 31: // directive: ".lang_opt" error
                                      { error(yylhs.location, "expected format is .lang_opt language option_name = option_value"); }
    break;

  case 32: // directive: ".clock_div" "integer"
                                      { pioasm.get_current_program(yystack_[1].location, ".clock_div").set_clock_div(yylhs.location, yystack_[0].value.as < int > ()); }
    break;

  case 33: // directive: ".clock_div" "float"
                                      { pioasm.get_current_program(yystack_[1].location, ".clock_div").set_clock_div(yylhs.location, yystack_[0].value.as < float > ()); }
    break;

  case 34: // directive: ".fifo" fifo_config
                                      { pioasm.get_current_program(yystack_[1].location, ".fifo", true).set_fifo_config(yylhs.location, yystack_[0].value.as < enum fifo_config > ()); }
    break;

  case 35: // directive: ".mov_status" "txfifo" "<" value
                                      { pioasm.get_current_program(yystack_[3].location, ".mov_status", true).set_mov_status(mov_status_type::tx_lessthan, yystack_[0].value.as < std::shared_ptr<resolvable> > ()); }
    break;

  case 36: // directive: ".mov_status" "rxfifo" "<" value
                                      { pioasm.get_current_program(yystack_[3].location, ".mov_status", true).set_mov_status(mov_status_type::rx_lessthan, yystack_[0].value.as < std::shared_ptr<resolvable> > ()); }
    break;

  case 37: // directive: ".mov_status" "irq" "next" "set" value
                                           { pioasm.get_current_program(yystack_[4].location, ".mov_status", true).set_mov_status(mov_status_type::irq_set, yystack_[0].value.as < std::shared_ptr<resolvable> > (), 2); }
    break;

  case 38: // directive: ".mov_status" "irq" "prev" "set" value
                                           { pioasm.get_current_program(yystack_[4].location, ".mov_status", true).set_mov_status(mov_status_type::irq_set, yystack_[0].value.as < std::shared_ptr<resolvable> > (), 1); }
    break;

  case 39: // directive: ".mov_status" "irq" "set" value
                                      { pioasm.get_current_program(yystack_[3].location, ".mov_status", true).set_mov_status(mov_status_type::irq_set, yystack_[0].value.as < std::shared_ptr<resolvable> > ()); }
    break;

  case 40: // directive: ".mov_status"
                                      { error(yystack_[1].location, "expected 'txfifo < N', 'rxfifo < N' or 'irq set N'"); }
    break;

  case 41: // directive: UNKNOWN_DIRECTIVE
                                      { std::stringstream msg; msg << "unknown directive " << yystack_[0].value.as < std::string > (); throw syntax_error(yylhs.location, msg.str()); }
    break;

  case 42: // value: "integer"
           { yylhs.value.as < std::shared_ptr<resolvable> > () = resolvable_int(yylhs.location, yystack_[0].value.as < int > ()); }
    break;

  case 43: // value: "identifier"
          { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<resolvable>(new name_ref(yylhs.location, yystack_[0].value.as < std::string > ())); }
    break;

  case 44: // value: "(" expression ")"
                                { yylhs.value.as < std::shared_ptr<resolvable> > () = yystack_[1].value.as < std::shared_ptr<resolvable> > (); }
    break;

  case 45: // expression: value
     { yylhs.value.as < std::shared_ptr<resolvable> > () = yystack_[0].value.as < std::shared_ptr<resolvable> > (); }
    break;

  case 46: // expression: expression "+" expression
                                  { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<binary_operation>(new binary_operation(yylhs.location, binary_operation::add, yystack_[2].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 47: // expression: expression "-" expression
                                   { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<binary_operation>(new binary_operation(yylhs.location, binary_operation::subtract, yystack_[2].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 48: // expression: expression "*" expression
                                      { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<binary_operation>(new binary_operation(yylhs.location, binary_operation::multiply, yystack_[2].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ()));  }
    break;

  case 49: // expression: expression "/" expression
                                    { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<binary_operation>(new binary_operation(yylhs.location, binary_operation::divide, yystack_[2].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 50: // expression: expression "|" expression
                                { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<binary_operation>(new binary_operation(yylhs.location, binary_operation::or_, yystack_[2].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 51: // expression: expression "&" expression
                                 { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<binary_operation>(new binary_operation(yylhs.location, binary_operation::and_, yystack_[2].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 52: // expression: expression "^" expression
                                 { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<binary_operation>(new binary_operation(yylhs.location, binary_operation::xor_, yystack_[2].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 53: // expression: expression "<<" expression
                                 { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<binary_operation>(new binary_operation(yylhs.location, binary_operation::shl_, yystack_[2].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 54: // expression: expression ">>" expression
                                 { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<binary_operation>(new binary_operation(yylhs.location, binary_operation::shr_, yystack_[2].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 55: // expression: "-" expression
                        { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<unary_operation>(new unary_operation(yylhs.location, unary_operation::negate, yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 56: // expression: "::" expression
                          { yylhs.value.as < std::shared_ptr<resolvable> > () = std::shared_ptr<unary_operation>(new unary_operation(yylhs.location, unary_operation::reverse, yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 57: // pio_version: "integer"
                 { yylhs.value.as < uint > () = yystack_[0].value.as < int > (); }
    break;

  case 58: // pio_version: "rp2040"
              { yylhs.value.as < uint > () = 0; }
    break;

  case 59: // pio_version: "rp2350"
              { yylhs.value.as < uint > () = 1; }
    break;

  case 60: // instruction: base_instruction sideset delay
                                   { yylhs.value.as < std::shared_ptr<instruction> > () = yystack_[2].value.as < std::shared_ptr<instruction> > (); yylhs.value.as < std::shared_ptr<instruction> > ()->sideset = yystack_[1].value.as < std::shared_ptr<resolvable> > (); yylhs.value.as < std::shared_ptr<instruction> > ()->delay = yystack_[0].value.as < std::shared_ptr<resolvable> > (); }
    break;

  case 61: // instruction: base_instruction delay sideset
                                   { yylhs.value.as < std::shared_ptr<instruction> > () = yystack_[2].value.as < std::shared_ptr<instruction> > (); yylhs.value.as < std::shared_ptr<instruction> > ()->delay = yystack_[1].value.as < std::shared_ptr<resolvable> > (); yylhs.value.as < std::shared_ptr<instruction> > ()->sideset = yystack_[0].value.as < std::shared_ptr<resolvable> > (); }
    break;

  case 62: // instruction: base_instruction sideset
                             { yylhs.value.as < std::shared_ptr<instruction> > () = yystack_[1].value.as < std::shared_ptr<instruction> > (); yylhs.value.as < std::shared_ptr<instruction> > ()->sideset = yystack_[0].value.as < std::shared_ptr<resolvable> > (); yylhs.value.as < std::shared_ptr<instruction> > ()->delay = resolvable_int(yylhs.location, 0); }
    break;

  case 63: // instruction: base_instruction delay
                           { yylhs.value.as < std::shared_ptr<instruction> > () = yystack_[1].value.as < std::shared_ptr<instruction> > (); yylhs.value.as < std::shared_ptr<instruction> > ()->delay = yystack_[0].value.as < std::shared_ptr<resolvable> > (); }
    break;

  case 64: // instruction: base_instruction
                     { yylhs.value.as < std::shared_ptr<instruction> > () = yystack_[0].value.as < std::shared_ptr<instruction> > (); yylhs.value.as < std::shared_ptr<instruction> > ()->delay = resolvable_int(yylhs.location, 0); }
    break;

  case 65: // base_instruction: "nop"
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_nop(yylhs.location)); }
    break;

  case 66: // base_instruction: "jmp" condition comma expression
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_jmp(yylhs.location, yystack_[2].value.as < enum condition > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 67: // base_instruction: "wait" value wait_source
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_wait(yylhs.location, yystack_[1].value.as < std::shared_ptr<resolvable> > (), yystack_[0].value.as < std::shared_ptr<wait_source> > ())); }
    break;

  case 68: // base_instruction: "wait" wait_source
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_wait(yylhs.location, resolvable_int(yylhs.location, 1),  yystack_[0].value.as < std::shared_ptr<wait_source> > ())); }
    break;

  case 69: // base_instruction: "in" in_source comma value
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_in(yylhs.location, yystack_[2].value.as < enum in_out_set > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 70: // base_instruction: "out" out_target comma value
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_out(yylhs.location, yystack_[2].value.as < enum in_out_set > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 71: // base_instruction: "push" if_full blocking
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_push(yylhs.location, yystack_[1].value.as < bool > (), yystack_[0].value.as < bool > ())); }
    break;

  case 72: // base_instruction: "pull" if_empty blocking
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_pull(yylhs.location, yystack_[1].value.as < bool > (), yystack_[0].value.as < bool > ())); }
    break;

  case 73: // base_instruction: "mov" mov_target comma mov_op mov_source
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_mov(yylhs.location, yystack_[3].value.as < extended_mov > (), yystack_[0].value.as < extended_mov > (), yystack_[1].value.as < enum mov_op > ())); }
    break;

  case 74: // base_instruction: "irq" irq_modifiers value "rel"
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_irq(yylhs.location, yystack_[2].value.as < enum irq > (), yystack_[1].value.as < std::shared_ptr<resolvable> > (), 2)); }
    break;

  case 75: // base_instruction: "irq" "prev" irq_modifiers value
                                                          { pioasm.check_version(1, yylhs.location, "irq prev"); yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_irq(yylhs.location, yystack_[1].value.as < enum irq > (), yystack_[0].value.as < std::shared_ptr<resolvable> > (), 1)); }
    break;

  case 76: // base_instruction: "irq" "next" irq_modifiers value
                                                          { pioasm.check_version(1, yylhs.location, "irq next"); yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_irq(yylhs.location, yystack_[1].value.as < enum irq > (), yystack_[0].value.as < std::shared_ptr<resolvable> > (), 3)); }
    break;

  case 77: // base_instruction: "irq" "prev" irq_modifiers value "rel"
                                                          { pioasm.check_version(1, yylhs.location, "irq prev"); error(yystack_[0].location, "'rel' is not supported for 'irq prev'"); }
    break;

  case 78: // base_instruction: "irq" "next" irq_modifiers value "rel"
                                                          { pioasm.check_version(1, yylhs.location, "irq next"); error(yystack_[0].location, "'rel' is not supported for 'irq next'"); }
    break;

  case 79: // base_instruction: "irq" irq_modifiers value
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_irq(yylhs.location, yystack_[1].value.as < enum irq > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 80: // base_instruction: "set" set_target comma value
                                                          { yylhs.value.as < std::shared_ptr<instruction> > () = std::shared_ptr<instruction>(new instr_set(yylhs.location, yystack_[2].value.as < enum in_out_set > (), yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 81: // delay: "[" expression "]"
                                 { yylhs.value.as < std::shared_ptr<resolvable> > () = yystack_[1].value.as < std::shared_ptr<resolvable> > (); }
    break;

  case 82: // sideset: "side" value
               { yylhs.value.as < std::shared_ptr<resolvable> > () = yystack_[0].value.as < std::shared_ptr<resolvable> > (); }
    break;

  case 83: // condition: "!" "x"
                            { yylhs.value.as < enum condition > () = condition::xz; }
    break;

  case 84: // condition: "x" "--"
                            { yylhs.value.as < enum condition > () = condition::xnz__; }
    break;

  case 85: // condition: "!" "y"
                            { yylhs.value.as < enum condition > () = condition::yz; }
    break;

  case 86: // condition: "y" "--"
                            { yylhs.value.as < enum condition > () = condition::ynz__; }
    break;

  case 87: // condition: "x" "!=" "y"
                            { yylhs.value.as < enum condition > () = condition::xney; }
    break;

  case 88: // condition: "pin"
                            { yylhs.value.as < enum condition > () = condition::pin; }
    break;

  case 89: // condition: "!" "osre"
                            { yylhs.value.as < enum condition > () = condition::osrez; }
    break;

  case 90: // condition: %empty
                            { yylhs.value.as < enum condition > () = condition::al; }
    break;

  case 91: // wait_source: "irq" comma value "rel"
                            { yylhs.value.as < std::shared_ptr<wait_source> > () = std::shared_ptr<wait_source>(new wait_source(wait_source::irq, yystack_[1].value.as < std::shared_ptr<resolvable> > (), 2)); }
    break;

  case 92: // wait_source: "irq" "prev" comma value
                            { pioasm.check_version(1, yylhs.location, "irq prev"); yylhs.value.as < std::shared_ptr<wait_source> > () = std::shared_ptr<wait_source>(new wait_source(wait_source::irq, yystack_[0].value.as < std::shared_ptr<resolvable> > (), 1)); }
    break;

  case 93: // wait_source: "irq" "next" comma value
                            { pioasm.check_version(1, yylhs.location, "irq next"); yylhs.value.as < std::shared_ptr<wait_source> > () = std::shared_ptr<wait_source>(new wait_source(wait_source::irq, yystack_[0].value.as < std::shared_ptr<resolvable> > (), 3)); }
    break;

  case 94: // wait_source: "irq" "prev" comma value "rel"
                             { pioasm.check_version(1, yylhs.location, "irq prev"); error(yystack_[0].location, "'rel' is not supported for 'irq prev'"); }
    break;

  case 95: // wait_source: "irq" "next" comma value "rel"
                             { pioasm.check_version(1, yylhs.location, "irq next"); error(yystack_[0].location, "'rel' is not supported for 'irq next'"); }
    break;

  case 96: // wait_source: "irq" comma value
                            { yylhs.value.as < std::shared_ptr<wait_source> > () = std::shared_ptr<wait_source>(new wait_source(wait_source::irq, yystack_[0].value.as < std::shared_ptr<resolvable> > (), 0)); }
    break;

  case 97: // wait_source: "gpio" comma value
                            { yylhs.value.as < std::shared_ptr<wait_source> > () = std::shared_ptr<wait_source>(new wait_source(wait_source::gpio, yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 98: // wait_source: "pin" comma value
                            { yylhs.value.as < std::shared_ptr<wait_source> > () = std::shared_ptr<wait_source>(new wait_source(wait_source::pin, yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 99: // wait_source: "jmppin"
                            { pioasm.check_version(1, yylhs.location, "wait jmppin"); yylhs.value.as < std::shared_ptr<wait_source> > () = std::shared_ptr<wait_source>(new wait_source(wait_source::jmppin, std::make_shared<int_value>(yylhs.location, 0))); }
    break;

  case 100: // wait_source: "jmppin" "+" value
                            { pioasm.check_version(1, yylhs.location, "wait jmppin"); yylhs.value.as < std::shared_ptr<wait_source> > () = std::shared_ptr<wait_source>(new wait_source(wait_source::jmppin, yystack_[0].value.as < std::shared_ptr<resolvable> > ())); }
    break;

  case 101: // wait_source: %empty
                            { error(yystack_[0].location, pioasm.version_string(1, "expected irq, gpio, pin or jmp_pin", "expected irq, gpio or pin")); }
    break;

  case 102: // fifo_config: "txrx"
                  { yylhs.value.as < enum fifo_config > () = fifo_config::txrx; }
    break;

  case 103: // fifo_config: "tx"
                { yylhs.value.as < enum fifo_config > () = fifo_config::tx; }
    break;

  case 104: // fifo_config: "rx"
                { yylhs.value.as < enum fifo_config > () = fifo_config::rx; }
    break;

  case 105: // fifo_config: "txput"
                { pioasm.check_version(1, yylhs.location, "txput"); yylhs.value.as < enum fifo_config > () = fifo_config::txput; }
    break;

  case 106: // fifo_config: "txget"
                { pioasm.check_version(1, yylhs.location, "rxput"); yylhs.value.as < enum fifo_config > () = fifo_config::txget; }
    break;

  case 107: // fifo_config: "putget"
                { pioasm.check_version(1, yylhs.location, "putget"); yylhs.value.as < enum fifo_config > () = fifo_config::putget; }
    break;

  case 108: // fifo_config: %empty
                { error(yystack_[0].location, pioasm.version_string(1, "expected txrx, tx, rx, txput, rxget or putget", "expected txrx, tx or rx")); }
    break;

  case 111: // in_source: "pins"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_pins; }
    break;

  case 112: // in_source: "x"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_x; }
    break;

  case 113: // in_source: "y"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_y; }
    break;

  case 114: // in_source: "null"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_null; }
    break;

  case 115: // in_source: "isr"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_isr; }
    break;

  case 116: // in_source: "osr"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_osr; }
    break;

  case 117: // in_source: "status"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_status; }
    break;

  case 118: // out_target: "pins"
                 { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_pins; }
    break;

  case 119: // out_target: "x"
                 { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_x; }
    break;

  case 120: // out_target: "y"
                 { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_y; }
    break;

  case 121: // out_target: "null"
                 { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_null; }
    break;

  case 122: // out_target: "pindirs"
                 { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_pindirs; }
    break;

  case 123: // out_target: "isr"
                 { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_isr; }
    break;

  case 124: // out_target: "pc"
                 { yylhs.value.as < enum in_out_set > () = in_out_set::out_set_pc; }
    break;

  case 125: // out_target: "exec"
                 { yylhs.value.as < enum in_out_set > () = in_out_set::out_exec; }
    break;

  case 126: // mov_target: "pins"
                 { yylhs.value.as < extended_mov > () = mov::pins; }
    break;

  case 127: // mov_target: "x"
                 { yylhs.value.as < extended_mov > () = mov::x; }
    break;

  case 128: // mov_target: "y"
                 { yylhs.value.as < extended_mov > () = mov::y; }
    break;

  case 129: // mov_target: "exec"
                 { yylhs.value.as < extended_mov > () = mov::exec; }
    break;

  case 130: // mov_target: "pc"
                 { yylhs.value.as < extended_mov > () = mov::pc; }
    break;

  case 131: // mov_target: "isr"
                 { yylhs.value.as < extended_mov > () = mov::isr; }
    break;

  case 132: // mov_target: "osr"
                 { yylhs.value.as < extended_mov > () = mov::osr; }
    break;

  case 133: // mov_target: "pindirs"
                 { pioasm.check_version(1, yylhs.location, "mov pindirs"); yylhs.value.as < extended_mov > () = mov::pindirs; }
    break;

  case 134: // mov_target: "rxfifo" "[" "y" "]"
                                 { pioasm.check_version(1, yylhs.location, "mov rxfifo[], "); yylhs.value.as < extended_mov > () = mov::fifo_y; }
    break;

  case 135: // mov_target: "rxfifo" "[" value "]"
                                     { pioasm.check_version(1, yylhs.location, "mov rxfifo[], "); yylhs.value.as < extended_mov > () = extended_mov(yystack_[1].value.as < std::shared_ptr<resolvable> > ()); }
    break;

  case 136: // mov_source: "pins"
                 { yylhs.value.as < extended_mov > () = mov::pins; }
    break;

  case 137: // mov_source: "x"
                 { yylhs.value.as < extended_mov > () = mov::x; }
    break;

  case 138: // mov_source: "y"
                 { yylhs.value.as < extended_mov > () = mov::y; }
    break;

  case 139: // mov_source: "null"
                 { yylhs.value.as < extended_mov > () = mov::null; }
    break;

  case 140: // mov_source: "status"
                 { yylhs.value.as < extended_mov > () = mov::status; }
    break;

  case 141: // mov_source: "isr"
                 { yylhs.value.as < extended_mov > () = mov::isr; }
    break;

  case 142: // mov_source: "osr"
                 { yylhs.value.as < extended_mov > () = mov::osr; }
    break;

  case 143: // mov_source: "rxfifo" "[" "y" "]"
                                 { pioasm.check_version(1, yylhs.location, "mov rxfifo[], "); yylhs.value.as < extended_mov > () = mov::fifo_y; }
    break;

  case 144: // mov_source: "rxfifo" "[" value "]"
                                     { pioasm.check_version(1, yylhs.location, "mov rxfifo[], "); yylhs.value.as < extended_mov > () = extended_mov(yystack_[1].value.as < std::shared_ptr<resolvable> > ()); }
    break;

  case 145: // mov_op: "!"
                { yylhs.value.as < enum mov_op > () = mov_op::invert; }
    break;

  case 146: // mov_op: "::"
                { yylhs.value.as < enum mov_op > () = mov_op::bit_reverse; }
    break;

  case 147: // mov_op: %empty
                { yylhs.value.as < enum mov_op > () = mov_op::none; }
    break;

  case 148: // set_target: "pins"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_pins; }
    break;

  case 149: // set_target: "x"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_x; }
    break;

  case 150: // set_target: "y"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_y; }
    break;

  case 151: // set_target: "pindirs"
                { yylhs.value.as < enum in_out_set > () = in_out_set::in_out_set_pindirs; }
    break;

  case 152: // direction: "left"
         { yylhs.value.as < bool > () = false; }
    break;

  case 153: // direction: "right"
          { yylhs.value.as < bool > () = true; }
    break;

  case 154: // direction: %empty
           { yylhs.value.as < bool > () = true; }
    break;

  case 155: // autop: "auto"
         { yylhs.value.as < bool > () = true; }
    break;

  case 156: // autop: "manual"
           { yylhs.value.as < bool > () = false; }
    break;

  case 157: // autop: %empty
           { yylhs.value.as < bool > () = false; }
    break;

  case 158: // threshold: value
                 { yylhs.value.as < std::shared_ptr<resolvable> > () = yystack_[0].value.as < std::shared_ptr<resolvable> > (); }
    break;

  case 159: // threshold: %empty
           { yylhs.value.as < std::shared_ptr<resolvable> > () = resolvable_int(yylhs.location, 32); }
    break;

  case 160: // if_full: "iffull"
           { yylhs.value.as < bool > () = true; }
    break;

  case 161: // if_full: %empty
           { yylhs.value.as < bool > () = false; }
    break;

  case 162: // if_empty: "ifempty"
            { yylhs.value.as < bool > () = true; }
    break;

  case 163: // if_empty: %empty
            { yylhs.value.as < bool > () = false; }
    break;

  case 164: // blocking: "block"
            { yylhs.value.as < bool > () = true; }
    break;

  case 165: // blocking: "noblock"
            { yylhs.value.as < bool > () = false; }
    break;

  case 166: // blocking: %empty
            { yylhs.value.as < bool > () = true; }
    break;

  case 167: // irq_modifiers: "clear"
                   { yylhs.value.as < enum irq > () = irq::clear; }
    break;

  case 168: // irq_modifiers: "wait"
                   { yylhs.value.as < enum irq > () = irq::set_wait; }
    break;

  case 169: // irq_modifiers: "nowait"
                   { yylhs.value.as < enum irq > () = irq::set; }
    break;

  case 170: // irq_modifiers: "set"
                   { yylhs.value.as < enum irq > () = irq::set; }
    break;

  case 171: // irq_modifiers: %empty
                   { yylhs.value.as < enum irq > () = irq::set; }
    break;

  case 172: // symbol_def: "identifier"
                    { yylhs.value.as < std::shared_ptr<symbol> > () = std::shared_ptr<symbol>(new symbol(yylhs.location, yystack_[0].value.as < std::string > ())); }
    break;

  case 173: // symbol_def: "public" "identifier"
                    { yylhs.value.as < std::shared_ptr<symbol> > () = std::shared_ptr<symbol>(new symbol(yylhs.location, yystack_[0].value.as < std::string > (), true)); }
    break;

  case 174: // symbol_def: "*" "identifier"
                    { yylhs.value.as < std::shared_ptr<symbol> > () = std::shared_ptr<symbol>(new symbol(yylhs.location, yystack_[0].value.as < std::string > (), true)); }
    break;



            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        context yyctx (*this, yyla);
        std::string msg = yysyntax_error_ (yyctx);
        error (yyla.location, YY_MOVE (msg));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yyerror_range[1].location = yystack_[0].location;
        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      yy_lac_discard_ ("error recovery");
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }

  std::string
  parser::symbol_name (symbol_kind_type yysymbol)
  {
    return yytnamerr_ (yytname_[yysymbol]);
  }



  // parser::context.
  parser::context::context (const parser& yyparser, const symbol_type& yyla)
    : yyparser_ (yyparser)
    , yyla_ (yyla)
  {}

  int
  parser::context::expected_tokens (symbol_kind_type yyarg[], int yyargn) const
  {
    // Actual number of expected tokens
    int yycount = 0;

#if YYDEBUG
    // Execute LAC once. We don't care if it is successful, we
    // only do it for the sake of debugging output.
    if (!yyparser_.yy_lac_established_)
      yyparser_.yy_lac_check_ (yyla_.kind ());
#endif

    for (int yyx = 0; yyx < YYNTOKENS; ++yyx)
      {
        symbol_kind_type yysym = YY_CAST (symbol_kind_type, yyx);
        if (yysym != symbol_kind::S_YYerror
            && yysym != symbol_kind::S_YYUNDEF
            && yyparser_.yy_lac_check_ (yysym))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = yysym;
          }
      }
    if (yyarg && yycount == 0 && 0 < yyargn)
      yyarg[0] = symbol_kind::S_YYEMPTY;
    return yycount;
  }


  bool
  parser::yy_lac_check_ (symbol_kind_type yytoken) const
  {
    // Logically, the yylac_stack's lifetime is confined to this function.
    // Clear it, to get rid of potential left-overs from previous call.
    yylac_stack_.clear ();
    // Reduce until we encounter a shift and thereby accept the token.
#if YYDEBUG
    YYCDEBUG << "LAC: checking lookahead " << symbol_name (yytoken) << ':';
#endif
    std::ptrdiff_t lac_top = 0;
    while (true)
      {
        state_type top_state = (yylac_stack_.empty ()
                                ? yystack_[lac_top].state
                                : yylac_stack_.back ());
        int yyrule = yypact_[+top_state];
        if (yy_pact_value_is_default_ (yyrule)
            || (yyrule += yytoken) < 0 || yylast_ < yyrule
            || yycheck_[yyrule] != yytoken)
          {
            // Use the default action.
            yyrule = yydefact_[+top_state];
            if (yyrule == 0)
              {
                YYCDEBUG << " Err\n";
                return false;
              }
          }
        else
          {
            // Use the action from yytable.
            yyrule = yytable_[yyrule];
            if (yy_table_value_is_error_ (yyrule))
              {
                YYCDEBUG << " Err\n";
                return false;
              }
            if (0 < yyrule)
              {
                YYCDEBUG << " S" << yyrule << '\n';
                return true;
              }
            yyrule = -yyrule;
          }
        // By now we know we have to simulate a reduce.
        YYCDEBUG << " R" << yyrule - 1;
        // Pop the corresponding number of values from the stack.
        {
          std::ptrdiff_t yylen = yyr2_[yyrule];
          // First pop from the LAC stack as many tokens as possible.
          std::ptrdiff_t lac_size = std::ptrdiff_t (yylac_stack_.size ());
          if (yylen < lac_size)
            {
              yylac_stack_.resize (std::size_t (lac_size - yylen));
              yylen = 0;
            }
          else if (lac_size)
            {
              yylac_stack_.clear ();
              yylen -= lac_size;
            }
          // Only afterwards look at the main stack.
          // We simulate popping elements by incrementing lac_top.
          lac_top += yylen;
        }
        // Keep top_state in sync with the updated stack.
        top_state = (yylac_stack_.empty ()
                     ? yystack_[lac_top].state
                     : yylac_stack_.back ());
        // Push the resulting state of the reduction.
        state_type state = yy_lr_goto_state_ (top_state, yyr1_[yyrule]);
        YYCDEBUG << " G" << int (state);
        yylac_stack_.push_back (state);
      }
  }

  // Establish the initial context if no initial context currently exists.
  bool
  parser::yy_lac_establish_ (symbol_kind_type yytoken)
  {
    /* Establish the initial context for the current lookahead if no initial
       context is currently established.

       We define a context as a snapshot of the parser stacks.  We define
       the initial context for a lookahead as the context in which the
       parser initially examines that lookahead in order to select a
       syntactic action.  Thus, if the lookahead eventually proves
       syntactically unacceptable (possibly in a later context reached via a
       series of reductions), the initial context can be used to determine
       the exact set of tokens that would be syntactically acceptable in the
       lookahead's place.  Moreover, it is the context after which any
       further semantic actions would be erroneous because they would be
       determined by a syntactically unacceptable token.

       yy_lac_establish_ should be invoked when a reduction is about to be
       performed in an inconsistent state (which, for the purposes of LAC,
       includes consistent states that don't know they're consistent because
       their default reductions have been disabled).

       For parse.lac=full, the implementation of yy_lac_establish_ is as
       follows.  If no initial context is currently established for the
       current lookahead, then check if that lookahead can eventually be
       shifted if syntactic actions continue from the current context.  */
    if (!yy_lac_established_)
      {
#if YYDEBUG
        YYCDEBUG << "LAC: initial context established for "
                 << symbol_name (yytoken) << '\n';
#endif
        yy_lac_established_ = true;
        return yy_lac_check_ (yytoken);
      }
    return true;
  }

  // Discard any previous initial lookahead context.
  void
  parser::yy_lac_discard_ (const char* evt)
  {
   /* Discard any previous initial lookahead context because of Event,
      which may be a lookahead change or an invalidation of the currently
      established initial context for the current lookahead.

      The most common example of a lookahead change is a shift.  An example
      of both cases is syntax error recovery.  That is, a syntax error
      occurs when the lookahead is syntactically erroneous for the
      currently established initial context, so error recovery manipulates
      the parser stacks to try to find a new initial context in which the
      current lookahead is syntactically acceptable.  If it fails to find
      such a context, it discards the lookahead.  */
    if (yy_lac_established_)
      {
        YYCDEBUG << "LAC: initial context discarded due to "
                 << evt << '\n';
        yy_lac_established_ = false;
      }
  }

  int
  parser::yy_syntax_error_arguments_ (const context& yyctx,
                                                 symbol_kind_type yyarg[], int yyargn) const
  {
    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
         In the first two cases, it might appear that the current syntax
         error should have been detected in the previous state when
         yy_lac_check was invoked.  However, at that time, there might
         have been a different syntax error that discarded a different
         initial context during error recovery, leaving behind the
         current lookahead.
    */

    if (!yyctx.lookahead ().empty ())
      {
        if (yyarg)
          yyarg[0] = yyctx.token ();
        int yyn = yyctx.expected_tokens (yyarg ? yyarg + 1 : yyarg, yyargn - 1);
        return yyn + 1;
      }
    return 0;
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (const context& yyctx) const
  {
    // Its maximum.
    enum { YYARGS_MAX = 5 };
    // Arguments of yyformat.
    symbol_kind_type yyarg[YYARGS_MAX];
    int yycount = yy_syntax_error_arguments_ (yyctx, yyarg, YYARGS_MAX);

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += symbol_name (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const signed char parser::yypact_ninf_ = -76;

  const signed char parser::yytable_ninf_ = -12;

  const short
  parser::yypact_[] =
  {
       4,   -76,   -75,   -68,   -76,   -76,    -6,    15,    15,    15,
      10,   -10,    27,   203,    35,    15,    15,    15,     6,     8,
     141,   163,   -34,    -1,   117,    67,    60,   -76,   -33,   -76,
     -32,   -76,    65,    23,   -76,   -76,   206,   -76,   -76,    12,
      72,   -76,   -76,    70,    70,   -76,   -76,    -4,   -76,   -76,
     -76,    -9,   -76,   -76,   -76,   -76,   -76,   -76,   -76,   -76,
     -76,   -76,   -76,   -76,   -76,   -30,    63,    76,   -76,    42,
      42,    22,   -76,   119,    74,    87,     9,    87,    87,    96,
     129,   -76,   -76,   -76,   -76,   -76,   -76,   -76,   -76,    87,
     -76,   -76,   -76,   -76,   -76,   -76,   -76,   -76,    87,   -76,
     157,   -76,   157,   -76,   -76,   -76,   -76,   -76,   -76,   -76,
     -76,   112,    87,   -76,   -76,    69,    69,   -76,   -76,    15,
     -76,   -76,   -76,   -76,    87,   -76,   -76,   -76,   -76,     4,
     -76,    70,    15,    90,   137,   -76,    70,    70,   -76,   257,
     227,   -76,   107,   145,    15,   123,   126,    15,    15,   -76,
     -76,   147,   147,   -76,   -76,   -76,   -76,   110,   -76,   -76,
      70,    87,    87,    15,    15,    15,    15,   -76,    15,    15,
     -76,   -76,   -76,   -76,    11,   268,    15,    15,   116,    15,
     -76,   248,   -76,   -76,   -76,   210,   257,    70,    70,    70,
      70,    70,    70,    70,    70,    70,   -76,   -76,   -21,   -76,
      15,    15,   -76,   -76,   -76,   -76,    15,    15,   -76,   257,
      15,    15,   124,   -76,   -76,   -76,   -76,   -76,   187,   201,
     -76,   -76,   135,   149,   153,   -76,   -76,   -76,   210,   210,
     120,   120,   -76,   -76,   -76,   266,   266,   -76,   -76,   -76,
     -76,   -76,   -76,   -76,   -76,   228,   229,   -76,   -76,   -76,
     -76,   -76,   -76,   -76,   -76,   -76,   -76,   287,   -76,   -76,
     -76,   -76,   -76,    13,   288,   289,   -76,   -76
  };

  const unsigned char
  parser::yydefact_[] =
  {
       0,    12,     0,     0,    25,    26,     0,     0,     0,     0,
       0,     0,     0,   108,    40,     0,     0,     0,    90,   101,
       0,     0,   161,   163,     0,   171,     0,    65,     0,   172,
       0,    41,     0,     0,     3,    10,     9,     6,     7,    64,
       0,   174,     5,     0,     0,    43,    42,    21,    27,    16,
      31,     0,    58,    59,    57,    17,    32,    33,   102,   103,
     104,   105,   106,   107,    34,     0,     0,     0,    24,   154,
     154,     0,    88,     0,     0,   110,   110,   110,   110,    99,
     101,    68,   111,   114,   112,   113,   115,   116,   117,   110,
     118,   121,   122,   119,   120,   125,   124,   123,   110,   160,
     166,   162,   166,   126,   133,   127,   128,   129,   130,   131,
     132,     0,   110,   168,   170,   171,   171,   169,   167,     0,
     148,   151,   149,   150,   110,   173,    13,     1,     2,     0,
       8,     0,     0,    63,    62,    14,     0,     0,    45,    15,
       0,    20,    19,     0,     0,     0,     0,     0,     0,   152,
     153,   157,   157,    89,    83,    85,    84,     0,    86,   109,
       0,   110,   110,     0,     0,     0,     0,    67,     0,     0,
     164,   165,    71,    72,     0,   147,     0,     0,    79,     0,
       4,     0,    82,    61,    60,    55,    56,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    44,    18,     0,    39,
       0,     0,    36,    35,   155,   156,   159,   159,    87,    66,
       0,     0,    96,    98,    97,   100,    69,    70,     0,     0,
     145,   146,     0,    75,    76,    74,    80,    81,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    29,    30,    28,
      38,    37,   158,    23,    22,    92,    93,    91,   134,   135,
     136,   139,   137,   138,   141,   142,   140,     0,    73,    77,
      78,    94,    95,     0,     0,     0,   143,   144
  };

  const short
  parser::yypgoto_[] =
  {
     -76,   -76,   -76,   167,   -76,   -76,   -76,    -7,   -41,   -76,
     263,   -76,   166,   168,   -76,   222,   -76,    66,   -76,   -76,
     -76,   -76,   -76,   -76,   233,   152,    98,   -76,   -76,   204,
     176,   301
  };

  const short
  parser::yydefgoto_[] =
  {
      -1,    32,    33,    34,    35,    36,    37,   138,   139,    55,
      38,    39,   133,   134,    75,    81,    64,   160,    89,    98,
     112,   258,   222,   124,   151,   206,   243,   100,   102,   172,
     119,    40
  };

  const short
  parser::yytable_[] =
  {
      47,    48,    49,   140,   -11,     1,     2,   -11,    68,    69,
      70,    50,    80,   159,    44,    41,     2,    44,   144,    44,
     131,    44,    42,   128,   145,   146,   129,    71,    99,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   141,    76,    72,   125,    77,    78,
     101,    79,   126,   161,   162,   127,    52,    53,   142,    28,
     237,   238,    73,    74,   153,   239,    44,   135,   218,    28,
     264,   136,    65,   143,    29,   132,    54,   147,   154,   155,
     181,   159,   137,   158,    29,   185,   186,    30,    45,    31,
     148,    45,    51,    45,    46,    45,   166,    46,   113,    46,
     113,    46,   178,    66,    67,   114,   120,   114,   121,   209,
     174,   115,   116,    56,    57,   182,   122,   123,   149,   150,
     117,   118,   117,   118,   191,   192,   193,   199,   156,   157,
     202,   203,   163,   164,   165,   131,   228,   229,   230,   231,
     232,   233,   234,   235,   236,   168,   212,   213,   214,   215,
      45,   216,   217,   132,   169,   197,    46,   219,   198,   223,
     224,   200,   226,   103,   201,   104,    76,   208,   175,    77,
      78,   225,    79,   105,   106,   107,   108,   109,   110,   247,
     179,   250,   251,   240,   241,   111,   248,    82,    83,   242,
     242,   252,   253,   245,   246,   254,   255,    84,    85,   256,
     249,    86,    87,   257,   259,    88,   170,   171,   260,    90,
      91,    92,   189,   190,   191,   192,   193,   210,   211,    93,
      94,    95,    96,    97,   196,   204,   205,   187,   188,   189,
     190,   191,   192,   193,   194,   195,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,   265,   227,   187,   188,
     189,   190,   191,   192,   193,   194,   195,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   187,   188,   189,   190,
     191,   192,   193,    58,    59,    60,    61,    62,    63,   220,
     221,   176,   177,   261,   262,   263,   180,   266,   267,   130,
     184,   183,   167,   152,   207,   244,   173,    43
  };

  const short
  parser::yycheck_[] =
  {
       7,     8,     9,    44,     0,     1,    12,     3,    15,    16,
      17,     1,    19,     4,     6,    90,    12,     6,    48,     6,
       8,     6,    90,     0,    54,    55,     3,    21,    62,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    58,    47,    50,    90,    50,    51,
      61,    53,    94,    54,    55,     0,    76,    77,    72,    75,
      91,    92,    66,    67,    52,    96,     6,     5,    67,    75,
      67,    11,    47,    92,    90,    73,    96,    24,    66,    67,
     131,     4,    22,    19,    90,   136,   137,    93,    90,    95,
      24,    90,    92,    90,    96,    90,    10,    96,    41,    96,
      41,    96,   119,    78,    79,    48,    56,    48,    58,   160,
       8,    54,    55,    96,    97,   132,    66,    67,    86,    87,
      63,    64,    63,    64,    14,    15,    16,   144,    19,    20,
     147,   148,    76,    77,    78,     8,   187,   188,   189,   190,
     191,   192,   193,   194,   195,    89,   163,   164,   165,   166,
      90,   168,   169,    73,    98,    58,    96,   174,    23,   176,
     177,    48,   179,    56,    48,    58,    47,    67,   112,    50,
      51,    65,    53,    66,    67,    68,    69,    70,    71,    65,
     124,    56,    57,   200,   201,    78,     9,    56,    57,   206,
     207,    66,    67,   210,   211,    70,    71,    66,    67,    74,
       9,    70,    71,    78,    65,    74,    59,    60,    65,    56,
      57,    58,    12,    13,    14,    15,    16,   161,   162,    66,
      67,    68,    69,    70,     7,    88,    89,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,   263,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    10,    11,    12,    13,
      14,    15,    16,    80,    81,    82,    83,    84,    85,    21,
      22,   115,   116,    65,    65,     8,   129,     9,     9,    36,
     134,   133,    80,    70,   152,   207,   102,     6
  };

  const unsigned char
  parser::yystos_[] =
  {
       0,     1,    12,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    75,    90,
      93,    95,    99,   100,   101,   102,   103,   104,   108,   109,
     129,    90,    90,   129,     6,    90,    96,   105,   105,   105,
       1,    92,    76,    77,    96,   107,    96,    97,    80,    81,
      82,    83,    84,    85,   114,    47,    78,    79,   105,   105,
     105,    21,    50,    66,    67,   112,    47,    50,    51,    53,
     105,   113,    56,    57,    66,    67,    70,    71,    74,   116,
      56,    57,    58,    66,    67,    68,    69,    70,   117,    62,
     125,    61,   126,    56,    58,    66,    67,    68,    69,    70,
      71,    78,   118,    41,    48,    54,    55,    63,    64,   128,
      56,    58,    66,    67,   121,    90,    94,     0,     0,     3,
     108,     8,    73,   110,   111,     5,    11,    22,   105,   106,
     106,    58,    72,    92,    48,    54,    55,    24,    24,    86,
      87,   122,   122,    52,    66,    67,    19,    20,    19,     4,
     115,    54,    55,   115,   115,   115,    10,   113,   115,   115,
      59,    60,   127,   127,     8,   115,   128,   128,   105,   115,
     101,   106,   105,   111,   110,   106,   106,    10,    11,    12,
      13,    14,    15,    16,    17,    18,     7,    58,    23,   105,
      48,    48,   105,   105,    88,    89,   123,   123,    67,   106,
     115,   115,   105,   105,   105,   105,   105,   105,    67,   105,
      21,    22,   120,   105,   105,    65,   105,     9,   106,   106,
     106,   106,   106,   106,   106,   106,   106,    91,    92,    96,
     105,   105,   105,   124,   124,   105,   105,    65,     9,     9,
      56,    57,    66,    67,    70,    71,    74,    78,   119,    65,
      65,    65,    65,     8,    67,   105,     9,     9
  };

  const unsigned char
  parser::yyr1_[] =
  {
       0,    98,    99,   100,   100,   101,   101,   101,   101,   101,
     101,   101,   101,   102,   103,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   105,   105,   105,   106,   106,   106,   106,   106,
     106,   106,   106,   106,   106,   106,   106,   107,   107,   107,
     108,   108,   108,   108,   108,   109,   109,   109,   109,   109,
     109,   109,   109,   109,   109,   109,   109,   109,   109,   109,
     109,   110,   111,   112,   112,   112,   112,   112,   112,   112,
     112,   113,   113,   113,   113,   113,   113,   113,   113,   113,
     113,   113,   114,   114,   114,   114,   114,   114,   114,   115,
     115,   116,   116,   116,   116,   116,   116,   116,   117,   117,
     117,   117,   117,   117,   117,   117,   118,   118,   118,   118,
     118,   118,   118,   118,   118,   118,   119,   119,   119,   119,
     119,   119,   119,   119,   119,   120,   120,   120,   121,   121,
     121,   121,   122,   122,   122,   123,   123,   123,   124,   124,
     125,   125,   126,   126,   127,   127,   127,   128,   128,   128,
     128,   128,   129,   129,   129
  };

  const signed char
  parser::yyr2_[] =
  {
       0,     2,     2,     1,     3,     2,     1,     1,     2,     1,
       1,     0,     1,     2,     2,     3,     2,     2,     4,     3,
       3,     2,     5,     5,     2,     1,     1,     2,     5,     5,
       5,     2,     2,     2,     2,     4,     4,     5,     5,     4,
       1,     1,     1,     1,     3,     1,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     2,     2,     1,     1,     1,
       3,     3,     2,     2,     1,     1,     4,     3,     2,     4,
       4,     3,     3,     5,     4,     4,     4,     5,     5,     3,
       4,     3,     2,     2,     2,     2,     2,     3,     1,     2,
       0,     4,     4,     4,     5,     5,     3,     3,     3,     1,
       3,     0,     1,     1,     1,     1,     1,     1,     0,     1,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     4,     1,     1,     1,     1,
       1,     1,     1,     4,     4,     1,     1,     0,     1,     1,
       1,     1,     1,     1,     0,     1,     1,     0,     1,     0,
       1,     0,     1,     0,     1,     1,     0,     1,     1,     1,
       1,     0,     1,     2,     2
  };


#if YYDEBUG || 1
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "\"end of line\"",
  "\",\"", "\":\"", "\"(\"", "\")\"", "\"[\"", "\"]\"", "\"+\"", "\"-\"",
  "\"*\"", "\"/\"", "\"|\"", "\"&\"", "\"^\"", "\"<<\"", "\">>\"",
  "\"--\"", "\"!=\"", "\"!\"", "\"::\"", "\"=\"", "\"<\"", "\".program\"",
  "\".wrap_target\"", "\".wrap\"", "\".define\"", "\".side_set\"",
  "\".word\"", "\".origin\"", "\".lang_opt\"", "\".pio_version\"",
  "\".clock_div\"", "\".fifo\"", "\".mov_status\"", "\".set\"", "\".out\"",
  "\".in\"", "\"jmp\"", "\"wait\"", "\"in\"", "\"out\"", "\"push\"",
  "\"pull\"", "\"mov\"", "\"irq\"", "\"set\"", "\"nop\"", "\"pin\"",
  "\"gpio\"", "\"osre\"", "\"jmppin\"", "\"prev\"", "\"next\"", "\"pins\"",
  "\"null\"", "\"pindirs\"", "\"block\"", "\"noblock\"", "\"ifempty\"",
  "\"iffull\"", "\"nowait\"", "\"clear\"", "\"rel\"", "\"x\"", "\"y\"",
  "\"exec\"", "\"pc\"", "\"isr\"", "\"osr\"", "\"opt\"", "\"side\"",
  "\"status\"", "\"public\"", "\"rp2040\"", "\"rp2350\"", "\"rxfifo\"",
  "\"txfifo\"", "\"txrx\"", "\"tx\"", "\"rx\"", "\"txput\"", "\"txget\"",
  "\"putget\"", "\"left\"", "\"right\"", "\"auto\"", "\"manual\"",
  "\"identifier\"", "\"string\"", "\"text\"", "\"code block\"", "\"%}\"",
  "UNKNOWN_DIRECTIVE", "\"integer\"", "\"float\"", "$accept", "file",
  "lines", "line", "code_block", "label_decl", "directive", "value",
  "expression", "pio_version", "instruction", "base_instruction", "delay",
  "sideset", "condition", "wait_source", "fifo_config", "comma",
  "in_source", "out_target", "mov_target", "mov_source", "mov_op",
  "set_target", "direction", "autop", "threshold", "if_full", "if_empty",
  "blocking", "irq_modifiers", "symbol_def", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
  parser::yyrline_[] =
  {
       0,   169,   169,   173,   174,   177,   178,   179,   180,   181,
     182,   183,   184,   188,   192,   195,   196,   197,   198,   199,
     200,   201,   202,   203,   204,   205,   206,   207,   208,   209,
     210,   211,   212,   213,   214,   215,   216,   217,   218,   219,
     220,   221,   226,   227,   228,   232,   233,   234,   235,   236,
     237,   238,   239,   240,   241,   242,   243,   246,   247,   248,
     252,   253,   254,   255,   256,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   280,   284,   288,   289,   290,   291,   292,   293,   294,
     295,   299,   300,   301,   302,   303,   304,   305,   306,   307,
     308,   309,   312,   313,   314,   315,   316,   317,   318,   321,
     321,   324,   325,   326,   327,   328,   329,   330,   333,   334,
     335,   336,   337,   338,   339,   340,   343,   344,   345,   346,
     347,   348,   349,   350,   351,   352,   355,   356,   357,   358,
     359,   360,   361,   362,   363,   367,   368,   369,   373,   374,
     375,   376,   380,   381,   382,   386,   387,   388,   391,   392,
     396,   397,   401,   402,   406,   407,   408,   412,   413,   414,
     415,   416,   420,   421,   422
  };

  void
  parser::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
  parser::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


} // yy


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
