// A Bison parser, made by GNU Bison 3.7.2.

// Skeleton interface for Bison LALR(1) parsers in C++

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


/**
 ** \file tools/pioasm/gen/parser.hpp
 ** Define the yy::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

#ifndef YY_YY_HOME_GRAHAM_DEV_AMY_AMY_SDK_TOOLS_PIOASM_GEN_PARSER_HPP_INCLUDED
# define YY_YY_HOME_GRAHAM_DEV_AMY_AMY_SDK_TOOLS_PIOASM_GEN_PARSER_HPP_INCLUDED
// "%code requires" blocks.

  #include <string>
  #include <fstream>
  #include <sstream>
  #include "pio_types.h"
  struct pio_assembler;

  #ifdef _MSC_VER
  #pragma warning(disable : 4065) // default only switch statement
  #endif



# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif
# include "location.h"

#ifndef YY_ASSERT
# include <cassert>
# define YY_ASSERT assert
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

namespace yy {




  /// A Bison parser.
  class parser
  {
  public:
#ifndef YYSTYPE
  /// A buffer to store and retrieve objects.
  ///
  /// Sort of a variant, but does not keep track of the nature
  /// of the stored data, since that knowledge is available
  /// via the current parser state.
  class semantic_type
  {
  public:
    /// Type of *this.
    typedef semantic_type self_type;

    /// Empty construction.
    semantic_type () YY_NOEXCEPT
      : yybuffer_ ()
    {}

    /// Construct and fill.
    template <typename T>
    semantic_type (YY_RVREF (T) t)
    {
      YY_ASSERT (sizeof (T) <= size);
      new (yyas_<T> ()) T (YY_MOVE (t));
    }

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    semantic_type (const self_type&) = delete;
    /// Non copyable.
    self_type& operator= (const self_type&) = delete;
#endif

    /// Destruction, allowed only if empty.
    ~semantic_type () YY_NOEXCEPT
    {}

# if 201103L <= YY_CPLUSPLUS
    /// Instantiate a \a T in here from \a t.
    template <typename T, typename... U>
    T&
    emplace (U&&... u)
    {
      return *new (yyas_<T> ()) T (std::forward <U>(u)...);
    }
# else
    /// Instantiate an empty \a T in here.
    template <typename T>
    T&
    emplace ()
    {
      return *new (yyas_<T> ()) T ();
    }

    /// Instantiate a \a T in here from \a t.
    template <typename T>
    T&
    emplace (const T& t)
    {
      return *new (yyas_<T> ()) T (t);
    }
# endif

    /// Instantiate an empty \a T in here.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build ()
    {
      return emplace<T> ();
    }

    /// Instantiate a \a T in here from \a t.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build (const T& t)
    {
      return emplace<T> (t);
    }

    /// Accessor to a built \a T.
    template <typename T>
    T&
    as () YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Const accessor to a built \a T (for %printer).
    template <typename T>
    const T&
    as () const YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Swap the content with \a that, of same type.
    ///
    /// Both variants must be built beforehand, because swapping the actual
    /// data requires reading it (with as()), and this is not possible on
    /// unconstructed variants: it would require some dynamic testing, which
    /// should not be the variant's responsibility.
    /// Swapping between built and (possibly) non-built is done with
    /// self_type::move ().
    template <typename T>
    void
    swap (self_type& that) YY_NOEXCEPT
    {
      std::swap (as<T> (), that.as<T> ());
    }

    /// Move the content of \a that to this.
    ///
    /// Destroys \a that.
    template <typename T>
    void
    move (self_type& that)
    {
# if 201103L <= YY_CPLUSPLUS
      emplace<T> (std::move (that.as<T> ()));
# else
      emplace<T> ();
      swap<T> (that);
# endif
      that.destroy<T> ();
    }

# if 201103L <= YY_CPLUSPLUS
    /// Move the content of \a that to this.
    template <typename T>
    void
    move (self_type&& that)
    {
      emplace<T> (std::move (that.as<T> ()));
      that.destroy<T> ();
    }
#endif

    /// Copy the content of \a that to this.
    template <typename T>
    void
    copy (const self_type& that)
    {
      emplace<T> (that.as<T> ());
    }

    /// Destroy the stored \a T.
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
    }

  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    semantic_type (const self_type&);
    /// Non copyable.
    self_type& operator= (const self_type&);
#endif

    /// Accessor to raw memory as \a T.
    template <typename T>
    T*
    yyas_ () YY_NOEXCEPT
    {
      void *yyp = yybuffer_.yyraw;
      return static_cast<T*> (yyp);
     }

    /// Const accessor to raw memory as \a T.
    template <typename T>
    const T*
    yyas_ () const YY_NOEXCEPT
    {
      const void *yyp = yybuffer_.yyraw;
      return static_cast<const T*> (yyp);
     }

    /// An auxiliary type to compute the largest semantic type.
    union union_type
    {
      // direction
      // autop
      // if_full
      // if_empty
      // blocking
      char dummy1[sizeof (bool)];

      // condition
      char dummy2[sizeof (enum condition)];

      // fifo_config
      char dummy3[sizeof (enum fifo_config)];

      // in_source
      // out_target
      // set_target
      char dummy4[sizeof (enum in_out_set)];

      // irq_modifiers
      char dummy5[sizeof (enum irq)];

      // mov_op
      char dummy6[sizeof (enum mov_op)];

      // mov_target
      // mov_source
      char dummy7[sizeof (extended_mov)];

      // "float"
      char dummy8[sizeof (float)];

      // "integer"
      char dummy9[sizeof (int)];

      // instruction
      // base_instruction
      char dummy10[sizeof (std::shared_ptr<instruction>)];

      // value
      // expression
      // delay
      // sideset
      // threshold
      char dummy11[sizeof (std::shared_ptr<resolvable>)];

      // label_decl
      // symbol_def
      char dummy12[sizeof (std::shared_ptr<symbol>)];

      // wait_source
      char dummy13[sizeof (std::shared_ptr<wait_source>)];

      // "identifier"
      // "string"
      // "text"
      // "code block"
      // "%}"
      // UNKNOWN_DIRECTIVE
      char dummy14[sizeof (std::string)];

      // pio_version
      char dummy15[sizeof (uint)];
    };

    /// The size of the largest semantic type.
    enum { size = sizeof (union_type) };

    /// A buffer to store semantic values.
    union
    {
      /// Strongest alignment constraints.
      long double yyalign_me;
      /// A buffer large enough to store any of the semantic values.
      char yyraw[size];
    } yybuffer_;
  };

#else
    typedef YYSTYPE semantic_type;
#endif
    /// Symbol locations.
    typedef location location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Token kinds.
    struct token
    {
      enum token_kind_type
      {
        TOK_YYEMPTY = -2,
    TOK_END = 0,                   // "end of file"
    TOK_YYerror = 256,             // error
    TOK_YYUNDEF = 257,             // "invalid token"
    TOK_NEWLINE = 258,             // "end of line"
    TOK_COMMA = 259,               // ","
    TOK_COLON = 260,               // ":"
    TOK_LPAREN = 261,              // "("
    TOK_RPAREN = 262,              // ")"
    TOK_LBRACKET = 263,            // "["
    TOK_RBRACKET = 264,            // "]"
    TOK_PLUS = 265,                // "+"
    TOK_MINUS = 266,               // "-"
    TOK_MULTIPLY = 267,            // "*"
    TOK_DIVIDE = 268,              // "/"
    TOK_OR = 269,                  // "|"
    TOK_AND = 270,                 // "&"
    TOK_XOR = 271,                 // "^"
    TOK_SHL = 272,                 // "<<"
    TOK_SHR = 273,                 // ">>"
    TOK_POST_DECREMENT = 274,      // "--"
    TOK_NOT_EQUAL = 275,           // "!="
    TOK_NOT = 276,                 // "!"
    TOK_REVERSE = 277,             // "::"
    TOK_ASSIGN = 278,              // "="
    TOK_LESSTHAN = 279,            // "<"
    TOK_PROGRAM = 280,             // ".program"
    TOK_WRAP_TARGET = 281,         // ".wrap_target"
    TOK_WRAP = 282,                // ".wrap"
    TOK_DEFINE = 283,              // ".define"
    TOK_SIDE_SET = 284,            // ".side_set"
    TOK_WORD = 285,                // ".word"
    TOK_ORIGIN = 286,              // ".origin"
    TOK_LANG_OPT = 287,            // ".lang_opt"
    TOK_PIO_VERSION = 288,         // ".pio_version"
    TOK_CLOCK_DIV = 289,           // ".clock_div"
    TOK_FIFO = 290,                // ".fifo"
    TOK_MOV_STATUS = 291,          // ".mov_status"
    TOK_DOT_SET = 292,             // ".set"
    TOK_DOT_OUT = 293,             // ".out"
    TOK_DOT_IN = 294,              // ".in"
    TOK_JMP = 295,                 // "jmp"
    TOK_WAIT = 296,                // "wait"
    TOK_IN = 297,                  // "in"
    TOK_OUT = 298,                 // "out"
    TOK_PUSH = 299,                // "push"
    TOK_PULL = 300,                // "pull"
    TOK_MOV = 301,                 // "mov"
    TOK_IRQ = 302,                 // "irq"
    TOK_SET = 303,                 // "set"
    TOK_NOP = 304,                 // "nop"
    TOK_PIN = 305,                 // "pin"
    TOK_GPIO = 306,                // "gpio"
    TOK_OSRE = 307,                // "osre"
    TOK_JMPPIN = 308,              // "jmppin"
    TOK_PREV = 309,                // "prev"
    TOK_NEXT = 310,                // "next"
    TOK_PINS = 311,                // "pins"
    TOK_NULL = 312,                // "null"
    TOK_PINDIRS = 313,             // "pindirs"
    TOK_BLOCK = 314,               // "block"
    TOK_NOBLOCK = 315,             // "noblock"
    TOK_IFEMPTY = 316,             // "ifempty"
    TOK_IFFULL = 317,              // "iffull"
    TOK_NOWAIT = 318,              // "nowait"
    TOK_CLEAR = 319,               // "clear"
    TOK_REL = 320,                 // "rel"
    TOK_X = 321,                   // "x"
    TOK_Y = 322,                   // "y"
    TOK_EXEC = 323,                // "exec"
    TOK_PC = 324,                  // "pc"
    TOK_ISR = 325,                 // "isr"
    TOK_OSR = 326,                 // "osr"
    TOK_OPTIONAL = 327,            // "opt"
    TOK_SIDE = 328,                // "side"
    TOK_STATUS = 329,              // "status"
    TOK_PUBLIC = 330,              // "public"
    TOK_RP2040 = 331,              // "rp2040"
    TOK_RP2350 = 332,              // "rp2350"
    TOK_RXFIFO = 333,              // "rxfifo"
    TOK_TXFIFO = 334,              // "txfifo"
    TOK_TXRX = 335,                // "txrx"
    TOK_TX = 336,                  // "tx"
    TOK_RX = 337,                  // "rx"
    TOK_TXPUT = 338,               // "txput"
    TOK_TXGET = 339,               // "txget"
    TOK_PUTGET = 340,              // "putget"
    TOK_LEFT = 341,                // "left"
    TOK_RIGHT = 342,               // "right"
    TOK_AUTO = 343,                // "auto"
    TOK_MANUAL = 344,              // "manual"
    TOK_ID = 345,                  // "identifier"
    TOK_STRING = 346,              // "string"
    TOK_NON_WS = 347,              // "text"
    TOK_CODE_BLOCK_START = 348,    // "code block"
    TOK_CODE_BLOCK_CONTENTS = 349, // "%}"
    TOK_UNKNOWN_DIRECTIVE = 350,   // UNKNOWN_DIRECTIVE
    TOK_INT = 351,                 // "integer"
    TOK_FLOAT = 352                // "float"
      };
      /// Backward compatibility alias (Bison 3.6).
      typedef token_kind_type yytokentype;
    };

    /// Token kind, as returned by yylex.
    typedef token::yytokentype token_kind_type;

    /// Backward compatibility alias (Bison 3.6).
    typedef token_kind_type token_type;

    /// Symbol kinds.
    struct symbol_kind
    {
      enum symbol_kind_type
      {
        YYNTOKENS = 98, ///< Number of tokens.
        S_YYEMPTY = -2,
        S_YYEOF = 0,                             // "end of file"
        S_YYerror = 1,                           // error
        S_YYUNDEF = 2,                           // "invalid token"
        S_NEWLINE = 3,                           // "end of line"
        S_COMMA = 4,                             // ","
        S_COLON = 5,                             // ":"
        S_LPAREN = 6,                            // "("
        S_RPAREN = 7,                            // ")"
        S_LBRACKET = 8,                          // "["
        S_RBRACKET = 9,                          // "]"
        S_PLUS = 10,                             // "+"
        S_MINUS = 11,                            // "-"
        S_MULTIPLY = 12,                         // "*"
        S_DIVIDE = 13,                           // "/"
        S_OR = 14,                               // "|"
        S_AND = 15,                              // "&"
        S_XOR = 16,                              // "^"
        S_SHL = 17,                              // "<<"
        S_SHR = 18,                              // ">>"
        S_POST_DECREMENT = 19,                   // "--"
        S_NOT_EQUAL = 20,                        // "!="
        S_NOT = 21,                              // "!"
        S_REVERSE = 22,                          // "::"
        S_ASSIGN = 23,                           // "="
        S_LESSTHAN = 24,                         // "<"
        S_PROGRAM = 25,                          // ".program"
        S_WRAP_TARGET = 26,                      // ".wrap_target"
        S_WRAP = 27,                             // ".wrap"
        S_DEFINE = 28,                           // ".define"
        S_SIDE_SET = 29,                         // ".side_set"
        S_WORD = 30,                             // ".word"
        S_ORIGIN = 31,                           // ".origin"
        S_LANG_OPT = 32,                         // ".lang_opt"
        S_PIO_VERSION = 33,                      // ".pio_version"
        S_CLOCK_DIV = 34,                        // ".clock_div"
        S_FIFO = 35,                             // ".fifo"
        S_MOV_STATUS = 36,                       // ".mov_status"
        S_DOT_SET = 37,                          // ".set"
        S_DOT_OUT = 38,                          // ".out"
        S_DOT_IN = 39,                           // ".in"
        S_JMP = 40,                              // "jmp"
        S_WAIT = 41,                             // "wait"
        S_IN = 42,                               // "in"
        S_OUT = 43,                              // "out"
        S_PUSH = 44,                             // "push"
        S_PULL = 45,                             // "pull"
        S_MOV = 46,                              // "mov"
        S_IRQ = 47,                              // "irq"
        S_SET = 48,                              // "set"
        S_NOP = 49,                              // "nop"
        S_PIN = 50,                              // "pin"
        S_GPIO = 51,                             // "gpio"
        S_OSRE = 52,                             // "osre"
        S_JMPPIN = 53,                           // "jmppin"
        S_PREV = 54,                             // "prev"
        S_NEXT = 55,                             // "next"
        S_PINS = 56,                             // "pins"
        S_NULL = 57,                             // "null"
        S_PINDIRS = 58,                          // "pindirs"
        S_BLOCK = 59,                            // "block"
        S_NOBLOCK = 60,                          // "noblock"
        S_IFEMPTY = 61,                          // "ifempty"
        S_IFFULL = 62,                           // "iffull"
        S_NOWAIT = 63,                           // "nowait"
        S_CLEAR = 64,                            // "clear"
        S_REL = 65,                              // "rel"
        S_X = 66,                                // "x"
        S_Y = 67,                                // "y"
        S_EXEC = 68,                             // "exec"
        S_PC = 69,                               // "pc"
        S_ISR = 70,                              // "isr"
        S_OSR = 71,                              // "osr"
        S_OPTIONAL = 72,                         // "opt"
        S_SIDE = 73,                             // "side"
        S_STATUS = 74,                           // "status"
        S_PUBLIC = 75,                           // "public"
        S_RP2040 = 76,                           // "rp2040"
        S_RP2350 = 77,                           // "rp2350"
        S_RXFIFO = 78,                           // "rxfifo"
        S_TXFIFO = 79,                           // "txfifo"
        S_TXRX = 80,                             // "txrx"
        S_TX = 81,                               // "tx"
        S_RX = 82,                               // "rx"
        S_TXPUT = 83,                            // "txput"
        S_TXGET = 84,                            // "txget"
        S_PUTGET = 85,                           // "putget"
        S_LEFT = 86,                             // "left"
        S_RIGHT = 87,                            // "right"
        S_AUTO = 88,                             // "auto"
        S_MANUAL = 89,                           // "manual"
        S_ID = 90,                               // "identifier"
        S_STRING = 91,                           // "string"
        S_NON_WS = 92,                           // "text"
        S_CODE_BLOCK_START = 93,                 // "code block"
        S_CODE_BLOCK_CONTENTS = 94,              // "%}"
        S_UNKNOWN_DIRECTIVE = 95,                // UNKNOWN_DIRECTIVE
        S_INT = 96,                              // "integer"
        S_FLOAT = 97,                            // "float"
        S_YYACCEPT = 98,                         // $accept
        S_file = 99,                             // file
        S_lines = 100,                           // lines
        S_line = 101,                            // line
        S_code_block = 102,                      // code_block
        S_label_decl = 103,                      // label_decl
        S_directive = 104,                       // directive
        S_value = 105,                           // value
        S_expression = 106,                      // expression
        S_pio_version = 107,                     // pio_version
        S_instruction = 108,                     // instruction
        S_base_instruction = 109,                // base_instruction
        S_delay = 110,                           // delay
        S_sideset = 111,                         // sideset
        S_condition = 112,                       // condition
        S_wait_source = 113,                     // wait_source
        S_fifo_config = 114,                     // fifo_config
        S_comma = 115,                           // comma
        S_in_source = 116,                       // in_source
        S_out_target = 117,                      // out_target
        S_mov_target = 118,                      // mov_target
        S_mov_source = 119,                      // mov_source
        S_mov_op = 120,                          // mov_op
        S_set_target = 121,                      // set_target
        S_direction = 122,                       // direction
        S_autop = 123,                           // autop
        S_threshold = 124,                       // threshold
        S_if_full = 125,                         // if_full
        S_if_empty = 126,                        // if_empty
        S_blocking = 127,                        // blocking
        S_irq_modifiers = 128,                   // irq_modifiers
        S_symbol_def = 129                       // symbol_def
      };
    };

    /// (Internal) symbol kind.
    typedef symbol_kind::symbol_kind_type symbol_kind_type;

    /// The number of tokens.
    static const symbol_kind_type YYNTOKENS = symbol_kind::YYNTOKENS;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol kind
    /// via kind ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol ()
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that)
        : Base (std::move (that))
        , value ()
        , location (std::move (that.location))
      {
        switch (this->kind ())
    {
      case symbol_kind::S_direction: // direction
      case symbol_kind::S_autop: // autop
      case symbol_kind::S_if_full: // if_full
      case symbol_kind::S_if_empty: // if_empty
      case symbol_kind::S_blocking: // blocking
        value.move< bool > (std::move (that.value));
        break;

      case symbol_kind::S_condition: // condition
        value.move< enum condition > (std::move (that.value));
        break;

      case symbol_kind::S_fifo_config: // fifo_config
        value.move< enum fifo_config > (std::move (that.value));
        break;

      case symbol_kind::S_in_source: // in_source
      case symbol_kind::S_out_target: // out_target
      case symbol_kind::S_set_target: // set_target
        value.move< enum in_out_set > (std::move (that.value));
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
        value.move< enum irq > (std::move (that.value));
        break;

      case symbol_kind::S_mov_op: // mov_op
        value.move< enum mov_op > (std::move (that.value));
        break;

      case symbol_kind::S_mov_target: // mov_target
      case symbol_kind::S_mov_source: // mov_source
        value.move< extended_mov > (std::move (that.value));
        break;

      case symbol_kind::S_FLOAT: // "float"
        value.move< float > (std::move (that.value));
        break;

      case symbol_kind::S_INT: // "integer"
        value.move< int > (std::move (that.value));
        break;

      case symbol_kind::S_instruction: // instruction
      case symbol_kind::S_base_instruction: // base_instruction
        value.move< std::shared_ptr<instruction> > (std::move (that.value));
        break;

      case symbol_kind::S_value: // value
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_delay: // delay
      case symbol_kind::S_sideset: // sideset
      case symbol_kind::S_threshold: // threshold
        value.move< std::shared_ptr<resolvable> > (std::move (that.value));
        break;

      case symbol_kind::S_label_decl: // label_decl
      case symbol_kind::S_symbol_def: // symbol_def
        value.move< std::shared_ptr<symbol> > (std::move (that.value));
        break;

      case symbol_kind::S_wait_source: // wait_source
        value.move< std::shared_ptr<wait_source> > (std::move (that.value));
        break;

      case symbol_kind::S_ID: // "identifier"
      case symbol_kind::S_STRING: // "string"
      case symbol_kind::S_NON_WS: // "text"
      case symbol_kind::S_CODE_BLOCK_START: // "code block"
      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
        value.move< std::string > (std::move (that.value));
        break;

      case symbol_kind::S_pio_version: // pio_version
        value.move< uint > (std::move (that.value));
        break;

      default:
        break;
    }

      }
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, location_type&& l)
        : Base (t)
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const location_type& l)
        : Base (t)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, bool&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const bool& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, enum condition&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const enum condition& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, enum fifo_config&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const enum fifo_config& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, enum in_out_set&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const enum in_out_set& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, enum irq&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const enum irq& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, enum mov_op&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const enum mov_op& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, extended_mov&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const extended_mov& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, float&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const float& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, int&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const int& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::shared_ptr<instruction>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::shared_ptr<instruction>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::shared_ptr<resolvable>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::shared_ptr<resolvable>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::shared_ptr<symbol>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::shared_ptr<symbol>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::shared_ptr<wait_source>&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::shared_ptr<wait_source>& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, std::string&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const std::string& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, uint&& v, location_type&& l)
        : Base (t)
        , value (std::move (v))
        , location (std::move (l))
      {}
#else
      basic_symbol (typename Base::kind_type t, const uint& v, const location_type& l)
        : Base (t)
        , value (v)
        , location (l)
      {}
#endif

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }

      /// Destroy contents, and record that is empty.
      void clear ()
      {
        // User destructor.
        symbol_kind_type yykind = this->kind ();
        basic_symbol<Base>& yysym = *this;
        (void) yysym;
        switch (yykind)
        {
       default:
          break;
        }

        // Value type destructor.
switch (yykind)
    {
      case symbol_kind::S_direction: // direction
      case symbol_kind::S_autop: // autop
      case symbol_kind::S_if_full: // if_full
      case symbol_kind::S_if_empty: // if_empty
      case symbol_kind::S_blocking: // blocking
        value.template destroy< bool > ();
        break;

      case symbol_kind::S_condition: // condition
        value.template destroy< enum condition > ();
        break;

      case symbol_kind::S_fifo_config: // fifo_config
        value.template destroy< enum fifo_config > ();
        break;

      case symbol_kind::S_in_source: // in_source
      case symbol_kind::S_out_target: // out_target
      case symbol_kind::S_set_target: // set_target
        value.template destroy< enum in_out_set > ();
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
        value.template destroy< enum irq > ();
        break;

      case symbol_kind::S_mov_op: // mov_op
        value.template destroy< enum mov_op > ();
        break;

      case symbol_kind::S_mov_target: // mov_target
      case symbol_kind::S_mov_source: // mov_source
        value.template destroy< extended_mov > ();
        break;

      case symbol_kind::S_FLOAT: // "float"
        value.template destroy< float > ();
        break;

      case symbol_kind::S_INT: // "integer"
        value.template destroy< int > ();
        break;

      case symbol_kind::S_instruction: // instruction
      case symbol_kind::S_base_instruction: // base_instruction
        value.template destroy< std::shared_ptr<instruction> > ();
        break;

      case symbol_kind::S_value: // value
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_delay: // delay
      case symbol_kind::S_sideset: // sideset
      case symbol_kind::S_threshold: // threshold
        value.template destroy< std::shared_ptr<resolvable> > ();
        break;

      case symbol_kind::S_label_decl: // label_decl
      case symbol_kind::S_symbol_def: // symbol_def
        value.template destroy< std::shared_ptr<symbol> > ();
        break;

      case symbol_kind::S_wait_source: // wait_source
        value.template destroy< std::shared_ptr<wait_source> > ();
        break;

      case symbol_kind::S_ID: // "identifier"
      case symbol_kind::S_STRING: // "string"
      case symbol_kind::S_NON_WS: // "text"
      case symbol_kind::S_CODE_BLOCK_START: // "code block"
      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
        value.template destroy< std::string > ();
        break;

      case symbol_kind::S_pio_version: // pio_version
        value.template destroy< uint > ();
        break;

      default:
        break;
    }

        Base::clear ();
      }

      /// The user-facing name of this symbol.
      std::string name () const YY_NOEXCEPT
      {
        return parser::symbol_name (this->kind ());
      }

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      semantic_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_kind
    {
      /// Default constructor.
      by_kind ();

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_kind (by_kind&& that);
#endif

      /// Copy constructor.
      by_kind (const by_kind& that);

      /// The symbol kind as needed by the constructor.
      typedef token_kind_type kind_type;

      /// Constructor from (external) token numbers.
      by_kind (kind_type t);

      /// Record that this symbol is empty.
      void clear ();

      /// Steal the symbol kind from \a that.
      void move (by_kind& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// The symbol kind.
      /// \a S_YYEMPTY when empty.
      symbol_kind_type kind_;
    };

    /// Backward compatibility for a private implementation detail (Bison 3.6).
    typedef by_kind by_type;

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_kind>
    {
      /// Superclass.
      typedef basic_symbol<by_kind> super_type;

      /// Empty symbol.
      symbol_type () {}

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, location_type l)
        : super_type(token_type (tok), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_END || tok == token::TOK_YYerror || tok == token::TOK_YYUNDEF || tok == token::TOK_NEWLINE || tok == token::TOK_COMMA || tok == token::TOK_COLON || tok == token::TOK_LPAREN || tok == token::TOK_RPAREN || tok == token::TOK_LBRACKET || tok == token::TOK_RBRACKET || tok == token::TOK_PLUS || tok == token::TOK_MINUS || tok == token::TOK_MULTIPLY || tok == token::TOK_DIVIDE || tok == token::TOK_OR || tok == token::TOK_AND || tok == token::TOK_XOR || tok == token::TOK_SHL || tok == token::TOK_SHR || tok == token::TOK_POST_DECREMENT || tok == token::TOK_NOT_EQUAL || tok == token::TOK_NOT || tok == token::TOK_REVERSE || tok == token::TOK_ASSIGN || tok == token::TOK_LESSTHAN || tok == token::TOK_PROGRAM || tok == token::TOK_WRAP_TARGET || tok == token::TOK_WRAP || tok == token::TOK_DEFINE || tok == token::TOK_SIDE_SET || tok == token::TOK_WORD || tok == token::TOK_ORIGIN || tok == token::TOK_LANG_OPT || tok == token::TOK_PIO_VERSION || tok == token::TOK_CLOCK_DIV || tok == token::TOK_FIFO || tok == token::TOK_MOV_STATUS || tok == token::TOK_DOT_SET || tok == token::TOK_DOT_OUT || tok == token::TOK_DOT_IN || tok == token::TOK_JMP || tok == token::TOK_WAIT || tok == token::TOK_IN || tok == token::TOK_OUT || tok == token::TOK_PUSH || tok == token::TOK_PULL || tok == token::TOK_MOV || tok == token::TOK_IRQ || tok == token::TOK_SET || tok == token::TOK_NOP || tok == token::TOK_PIN || tok == token::TOK_GPIO || tok == token::TOK_OSRE || tok == token::TOK_JMPPIN || tok == token::TOK_PREV || tok == token::TOK_NEXT || tok == token::TOK_PINS || tok == token::TOK_NULL || tok == token::TOK_PINDIRS || tok == token::TOK_BLOCK || tok == token::TOK_NOBLOCK || tok == token::TOK_IFEMPTY || tok == token::TOK_IFFULL || tok == token::TOK_NOWAIT || tok == token::TOK_CLEAR || tok == token::TOK_REL || tok == token::TOK_X || tok == token::TOK_Y || tok == token::TOK_EXEC || tok == token::TOK_PC || tok == token::TOK_ISR || tok == token::TOK_OSR || tok == token::TOK_OPTIONAL || tok == token::TOK_SIDE || tok == token::TOK_STATUS || tok == token::TOK_PUBLIC || tok == token::TOK_RP2040 || tok == token::TOK_RP2350 || tok == token::TOK_RXFIFO || tok == token::TOK_TXFIFO || tok == token::TOK_TXRX || tok == token::TOK_TX || tok == token::TOK_RX || tok == token::TOK_TXPUT || tok == token::TOK_TXGET || tok == token::TOK_PUTGET || tok == token::TOK_LEFT || tok == token::TOK_RIGHT || tok == token::TOK_AUTO || tok == token::TOK_MANUAL);
      }
#else
      symbol_type (int tok, const location_type& l)
        : super_type(token_type (tok), l)
      {
        YY_ASSERT (tok == token::TOK_END || tok == token::TOK_YYerror || tok == token::TOK_YYUNDEF || tok == token::TOK_NEWLINE || tok == token::TOK_COMMA || tok == token::TOK_COLON || tok == token::TOK_LPAREN || tok == token::TOK_RPAREN || tok == token::TOK_LBRACKET || tok == token::TOK_RBRACKET || tok == token::TOK_PLUS || tok == token::TOK_MINUS || tok == token::TOK_MULTIPLY || tok == token::TOK_DIVIDE || tok == token::TOK_OR || tok == token::TOK_AND || tok == token::TOK_XOR || tok == token::TOK_SHL || tok == token::TOK_SHR || tok == token::TOK_POST_DECREMENT || tok == token::TOK_NOT_EQUAL || tok == token::TOK_NOT || tok == token::TOK_REVERSE || tok == token::TOK_ASSIGN || tok == token::TOK_LESSTHAN || tok == token::TOK_PROGRAM || tok == token::TOK_WRAP_TARGET || tok == token::TOK_WRAP || tok == token::TOK_DEFINE || tok == token::TOK_SIDE_SET || tok == token::TOK_WORD || tok == token::TOK_ORIGIN || tok == token::TOK_LANG_OPT || tok == token::TOK_PIO_VERSION || tok == token::TOK_CLOCK_DIV || tok == token::TOK_FIFO || tok == token::TOK_MOV_STATUS || tok == token::TOK_DOT_SET || tok == token::TOK_DOT_OUT || tok == token::TOK_DOT_IN || tok == token::TOK_JMP || tok == token::TOK_WAIT || tok == token::TOK_IN || tok == token::TOK_OUT || tok == token::TOK_PUSH || tok == token::TOK_PULL || tok == token::TOK_MOV || tok == token::TOK_IRQ || tok == token::TOK_SET || tok == token::TOK_NOP || tok == token::TOK_PIN || tok == token::TOK_GPIO || tok == token::TOK_OSRE || tok == token::TOK_JMPPIN || tok == token::TOK_PREV || tok == token::TOK_NEXT || tok == token::TOK_PINS || tok == token::TOK_NULL || tok == token::TOK_PINDIRS || tok == token::TOK_BLOCK || tok == token::TOK_NOBLOCK || tok == token::TOK_IFEMPTY || tok == token::TOK_IFFULL || tok == token::TOK_NOWAIT || tok == token::TOK_CLEAR || tok == token::TOK_REL || tok == token::TOK_X || tok == token::TOK_Y || tok == token::TOK_EXEC || tok == token::TOK_PC || tok == token::TOK_ISR || tok == token::TOK_OSR || tok == token::TOK_OPTIONAL || tok == token::TOK_SIDE || tok == token::TOK_STATUS || tok == token::TOK_PUBLIC || tok == token::TOK_RP2040 || tok == token::TOK_RP2350 || tok == token::TOK_RXFIFO || tok == token::TOK_TXFIFO || tok == token::TOK_TXRX || tok == token::TOK_TX || tok == token::TOK_RX || tok == token::TOK_TXPUT || tok == token::TOK_TXGET || tok == token::TOK_PUTGET || tok == token::TOK_LEFT || tok == token::TOK_RIGHT || tok == token::TOK_AUTO || tok == token::TOK_MANUAL);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, float v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_FLOAT);
      }
#else
      symbol_type (int tok, const float& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_FLOAT);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, int v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_INT);
      }
#else
      symbol_type (int tok, const int& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_INT);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, std::string v, location_type l)
        : super_type(token_type (tok), std::move (v), std::move (l))
      {
        YY_ASSERT (tok == token::TOK_ID || tok == token::TOK_STRING || tok == token::TOK_NON_WS || tok == token::TOK_CODE_BLOCK_START || tok == token::TOK_CODE_BLOCK_CONTENTS || tok == token::TOK_UNKNOWN_DIRECTIVE);
      }
#else
      symbol_type (int tok, const std::string& v, const location_type& l)
        : super_type(token_type (tok), v, l)
      {
        YY_ASSERT (tok == token::TOK_ID || tok == token::TOK_STRING || tok == token::TOK_NON_WS || tok == token::TOK_CODE_BLOCK_START || tok == token::TOK_CODE_BLOCK_CONTENTS || tok == token::TOK_UNKNOWN_DIRECTIVE);
      }
#endif
    };

    /// Build a parser object.
    parser (pio_assembler& pioasm_yyarg);
    virtual ~parser ();

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    parser (const parser&) = delete;
    /// Non copyable.
    parser& operator= (const parser&) = delete;
#endif

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

    /// The user-facing name of the symbol whose (internal) number is
    /// YYSYMBOL.  No bounds checking.
    static std::string symbol_name (symbol_kind_type yysymbol);

    // Implementation of make_symbol for each symbol type.
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_END (location_type l)
      {
        return symbol_type (token::TOK_END, std::move (l));
      }
#else
      static
      symbol_type
      make_END (const location_type& l)
      {
        return symbol_type (token::TOK_END, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYerror (location_type l)
      {
        return symbol_type (token::TOK_YYerror, std::move (l));
      }
#else
      static
      symbol_type
      make_YYerror (const location_type& l)
      {
        return symbol_type (token::TOK_YYerror, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYUNDEF (location_type l)
      {
        return symbol_type (token::TOK_YYUNDEF, std::move (l));
      }
#else
      static
      symbol_type
      make_YYUNDEF (const location_type& l)
      {
        return symbol_type (token::TOK_YYUNDEF, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NEWLINE (location_type l)
      {
        return symbol_type (token::TOK_NEWLINE, std::move (l));
      }
#else
      static
      symbol_type
      make_NEWLINE (const location_type& l)
      {
        return symbol_type (token::TOK_NEWLINE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COMMA (location_type l)
      {
        return symbol_type (token::TOK_COMMA, std::move (l));
      }
#else
      static
      symbol_type
      make_COMMA (const location_type& l)
      {
        return symbol_type (token::TOK_COMMA, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COLON (location_type l)
      {
        return symbol_type (token::TOK_COLON, std::move (l));
      }
#else
      static
      symbol_type
      make_COLON (const location_type& l)
      {
        return symbol_type (token::TOK_COLON, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LPAREN (location_type l)
      {
        return symbol_type (token::TOK_LPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_LPAREN (const location_type& l)
      {
        return symbol_type (token::TOK_LPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RPAREN (location_type l)
      {
        return symbol_type (token::TOK_RPAREN, std::move (l));
      }
#else
      static
      symbol_type
      make_RPAREN (const location_type& l)
      {
        return symbol_type (token::TOK_RPAREN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LBRACKET (location_type l)
      {
        return symbol_type (token::TOK_LBRACKET, std::move (l));
      }
#else
      static
      symbol_type
      make_LBRACKET (const location_type& l)
      {
        return symbol_type (token::TOK_LBRACKET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RBRACKET (location_type l)
      {
        return symbol_type (token::TOK_RBRACKET, std::move (l));
      }
#else
      static
      symbol_type
      make_RBRACKET (const location_type& l)
      {
        return symbol_type (token::TOK_RBRACKET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PLUS (location_type l)
      {
        return symbol_type (token::TOK_PLUS, std::move (l));
      }
#else
      static
      symbol_type
      make_PLUS (const location_type& l)
      {
        return symbol_type (token::TOK_PLUS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MINUS (location_type l)
      {
        return symbol_type (token::TOK_MINUS, std::move (l));
      }
#else
      static
      symbol_type
      make_MINUS (const location_type& l)
      {
        return symbol_type (token::TOK_MINUS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MULTIPLY (location_type l)
      {
        return symbol_type (token::TOK_MULTIPLY, std::move (l));
      }
#else
      static
      symbol_type
      make_MULTIPLY (const location_type& l)
      {
        return symbol_type (token::TOK_MULTIPLY, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DIVIDE (location_type l)
      {
        return symbol_type (token::TOK_DIVIDE, std::move (l));
      }
#else
      static
      symbol_type
      make_DIVIDE (const location_type& l)
      {
        return symbol_type (token::TOK_DIVIDE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OR (location_type l)
      {
        return symbol_type (token::TOK_OR, std::move (l));
      }
#else
      static
      symbol_type
      make_OR (const location_type& l)
      {
        return symbol_type (token::TOK_OR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AND (location_type l)
      {
        return symbol_type (token::TOK_AND, std::move (l));
      }
#else
      static
      symbol_type
      make_AND (const location_type& l)
      {
        return symbol_type (token::TOK_AND, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_XOR (location_type l)
      {
        return symbol_type (token::TOK_XOR, std::move (l));
      }
#else
      static
      symbol_type
      make_XOR (const location_type& l)
      {
        return symbol_type (token::TOK_XOR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SHL (location_type l)
      {
        return symbol_type (token::TOK_SHL, std::move (l));
      }
#else
      static
      symbol_type
      make_SHL (const location_type& l)
      {
        return symbol_type (token::TOK_SHL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SHR (location_type l)
      {
        return symbol_type (token::TOK_SHR, std::move (l));
      }
#else
      static
      symbol_type
      make_SHR (const location_type& l)
      {
        return symbol_type (token::TOK_SHR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_POST_DECREMENT (location_type l)
      {
        return symbol_type (token::TOK_POST_DECREMENT, std::move (l));
      }
#else
      static
      symbol_type
      make_POST_DECREMENT (const location_type& l)
      {
        return symbol_type (token::TOK_POST_DECREMENT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOT_EQUAL (location_type l)
      {
        return symbol_type (token::TOK_NOT_EQUAL, std::move (l));
      }
#else
      static
      symbol_type
      make_NOT_EQUAL (const location_type& l)
      {
        return symbol_type (token::TOK_NOT_EQUAL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOT (location_type l)
      {
        return symbol_type (token::TOK_NOT, std::move (l));
      }
#else
      static
      symbol_type
      make_NOT (const location_type& l)
      {
        return symbol_type (token::TOK_NOT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_REVERSE (location_type l)
      {
        return symbol_type (token::TOK_REVERSE, std::move (l));
      }
#else
      static
      symbol_type
      make_REVERSE (const location_type& l)
      {
        return symbol_type (token::TOK_REVERSE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ASSIGN (location_type l)
      {
        return symbol_type (token::TOK_ASSIGN, std::move (l));
      }
#else
      static
      symbol_type
      make_ASSIGN (const location_type& l)
      {
        return symbol_type (token::TOK_ASSIGN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LESSTHAN (location_type l)
      {
        return symbol_type (token::TOK_LESSTHAN, std::move (l));
      }
#else
      static
      symbol_type
      make_LESSTHAN (const location_type& l)
      {
        return symbol_type (token::TOK_LESSTHAN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PROGRAM (location_type l)
      {
        return symbol_type (token::TOK_PROGRAM, std::move (l));
      }
#else
      static
      symbol_type
      make_PROGRAM (const location_type& l)
      {
        return symbol_type (token::TOK_PROGRAM, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WRAP_TARGET (location_type l)
      {
        return symbol_type (token::TOK_WRAP_TARGET, std::move (l));
      }
#else
      static
      symbol_type
      make_WRAP_TARGET (const location_type& l)
      {
        return symbol_type (token::TOK_WRAP_TARGET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WRAP (location_type l)
      {
        return symbol_type (token::TOK_WRAP, std::move (l));
      }
#else
      static
      symbol_type
      make_WRAP (const location_type& l)
      {
        return symbol_type (token::TOK_WRAP, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DEFINE (location_type l)
      {
        return symbol_type (token::TOK_DEFINE, std::move (l));
      }
#else
      static
      symbol_type
      make_DEFINE (const location_type& l)
      {
        return symbol_type (token::TOK_DEFINE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SIDE_SET (location_type l)
      {
        return symbol_type (token::TOK_SIDE_SET, std::move (l));
      }
#else
      static
      symbol_type
      make_SIDE_SET (const location_type& l)
      {
        return symbol_type (token::TOK_SIDE_SET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WORD (location_type l)
      {
        return symbol_type (token::TOK_WORD, std::move (l));
      }
#else
      static
      symbol_type
      make_WORD (const location_type& l)
      {
        return symbol_type (token::TOK_WORD, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ORIGIN (location_type l)
      {
        return symbol_type (token::TOK_ORIGIN, std::move (l));
      }
#else
      static
      symbol_type
      make_ORIGIN (const location_type& l)
      {
        return symbol_type (token::TOK_ORIGIN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LANG_OPT (location_type l)
      {
        return symbol_type (token::TOK_LANG_OPT, std::move (l));
      }
#else
      static
      symbol_type
      make_LANG_OPT (const location_type& l)
      {
        return symbol_type (token::TOK_LANG_OPT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PIO_VERSION (location_type l)
      {
        return symbol_type (token::TOK_PIO_VERSION, std::move (l));
      }
#else
      static
      symbol_type
      make_PIO_VERSION (const location_type& l)
      {
        return symbol_type (token::TOK_PIO_VERSION, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CLOCK_DIV (location_type l)
      {
        return symbol_type (token::TOK_CLOCK_DIV, std::move (l));
      }
#else
      static
      symbol_type
      make_CLOCK_DIV (const location_type& l)
      {
        return symbol_type (token::TOK_CLOCK_DIV, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FIFO (location_type l)
      {
        return symbol_type (token::TOK_FIFO, std::move (l));
      }
#else
      static
      symbol_type
      make_FIFO (const location_type& l)
      {
        return symbol_type (token::TOK_FIFO, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MOV_STATUS (location_type l)
      {
        return symbol_type (token::TOK_MOV_STATUS, std::move (l));
      }
#else
      static
      symbol_type
      make_MOV_STATUS (const location_type& l)
      {
        return symbol_type (token::TOK_MOV_STATUS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DOT_SET (location_type l)
      {
        return symbol_type (token::TOK_DOT_SET, std::move (l));
      }
#else
      static
      symbol_type
      make_DOT_SET (const location_type& l)
      {
        return symbol_type (token::TOK_DOT_SET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DOT_OUT (location_type l)
      {
        return symbol_type (token::TOK_DOT_OUT, std::move (l));
      }
#else
      static
      symbol_type
      make_DOT_OUT (const location_type& l)
      {
        return symbol_type (token::TOK_DOT_OUT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DOT_IN (location_type l)
      {
        return symbol_type (token::TOK_DOT_IN, std::move (l));
      }
#else
      static
      symbol_type
      make_DOT_IN (const location_type& l)
      {
        return symbol_type (token::TOK_DOT_IN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_JMP (location_type l)
      {
        return symbol_type (token::TOK_JMP, std::move (l));
      }
#else
      static
      symbol_type
      make_JMP (const location_type& l)
      {
        return symbol_type (token::TOK_JMP, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WAIT (location_type l)
      {
        return symbol_type (token::TOK_WAIT, std::move (l));
      }
#else
      static
      symbol_type
      make_WAIT (const location_type& l)
      {
        return symbol_type (token::TOK_WAIT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IN (location_type l)
      {
        return symbol_type (token::TOK_IN, std::move (l));
      }
#else
      static
      symbol_type
      make_IN (const location_type& l)
      {
        return symbol_type (token::TOK_IN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OUT (location_type l)
      {
        return symbol_type (token::TOK_OUT, std::move (l));
      }
#else
      static
      symbol_type
      make_OUT (const location_type& l)
      {
        return symbol_type (token::TOK_OUT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PUSH (location_type l)
      {
        return symbol_type (token::TOK_PUSH, std::move (l));
      }
#else
      static
      symbol_type
      make_PUSH (const location_type& l)
      {
        return symbol_type (token::TOK_PUSH, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PULL (location_type l)
      {
        return symbol_type (token::TOK_PULL, std::move (l));
      }
#else
      static
      symbol_type
      make_PULL (const location_type& l)
      {
        return symbol_type (token::TOK_PULL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MOV (location_type l)
      {
        return symbol_type (token::TOK_MOV, std::move (l));
      }
#else
      static
      symbol_type
      make_MOV (const location_type& l)
      {
        return symbol_type (token::TOK_MOV, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IRQ (location_type l)
      {
        return symbol_type (token::TOK_IRQ, std::move (l));
      }
#else
      static
      symbol_type
      make_IRQ (const location_type& l)
      {
        return symbol_type (token::TOK_IRQ, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SET (location_type l)
      {
        return symbol_type (token::TOK_SET, std::move (l));
      }
#else
      static
      symbol_type
      make_SET (const location_type& l)
      {
        return symbol_type (token::TOK_SET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOP (location_type l)
      {
        return symbol_type (token::TOK_NOP, std::move (l));
      }
#else
      static
      symbol_type
      make_NOP (const location_type& l)
      {
        return symbol_type (token::TOK_NOP, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PIN (location_type l)
      {
        return symbol_type (token::TOK_PIN, std::move (l));
      }
#else
      static
      symbol_type
      make_PIN (const location_type& l)
      {
        return symbol_type (token::TOK_PIN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GPIO (location_type l)
      {
        return symbol_type (token::TOK_GPIO, std::move (l));
      }
#else
      static
      symbol_type
      make_GPIO (const location_type& l)
      {
        return symbol_type (token::TOK_GPIO, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OSRE (location_type l)
      {
        return symbol_type (token::TOK_OSRE, std::move (l));
      }
#else
      static
      symbol_type
      make_OSRE (const location_type& l)
      {
        return symbol_type (token::TOK_OSRE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_JMPPIN (location_type l)
      {
        return symbol_type (token::TOK_JMPPIN, std::move (l));
      }
#else
      static
      symbol_type
      make_JMPPIN (const location_type& l)
      {
        return symbol_type (token::TOK_JMPPIN, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PREV (location_type l)
      {
        return symbol_type (token::TOK_PREV, std::move (l));
      }
#else
      static
      symbol_type
      make_PREV (const location_type& l)
      {
        return symbol_type (token::TOK_PREV, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NEXT (location_type l)
      {
        return symbol_type (token::TOK_NEXT, std::move (l));
      }
#else
      static
      symbol_type
      make_NEXT (const location_type& l)
      {
        return symbol_type (token::TOK_NEXT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PINS (location_type l)
      {
        return symbol_type (token::TOK_PINS, std::move (l));
      }
#else
      static
      symbol_type
      make_PINS (const location_type& l)
      {
        return symbol_type (token::TOK_PINS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NULL (location_type l)
      {
        return symbol_type (token::TOK_NULL, std::move (l));
      }
#else
      static
      symbol_type
      make_NULL (const location_type& l)
      {
        return symbol_type (token::TOK_NULL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PINDIRS (location_type l)
      {
        return symbol_type (token::TOK_PINDIRS, std::move (l));
      }
#else
      static
      symbol_type
      make_PINDIRS (const location_type& l)
      {
        return symbol_type (token::TOK_PINDIRS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BLOCK (location_type l)
      {
        return symbol_type (token::TOK_BLOCK, std::move (l));
      }
#else
      static
      symbol_type
      make_BLOCK (const location_type& l)
      {
        return symbol_type (token::TOK_BLOCK, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOBLOCK (location_type l)
      {
        return symbol_type (token::TOK_NOBLOCK, std::move (l));
      }
#else
      static
      symbol_type
      make_NOBLOCK (const location_type& l)
      {
        return symbol_type (token::TOK_NOBLOCK, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IFEMPTY (location_type l)
      {
        return symbol_type (token::TOK_IFEMPTY, std::move (l));
      }
#else
      static
      symbol_type
      make_IFEMPTY (const location_type& l)
      {
        return symbol_type (token::TOK_IFEMPTY, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IFFULL (location_type l)
      {
        return symbol_type (token::TOK_IFFULL, std::move (l));
      }
#else
      static
      symbol_type
      make_IFFULL (const location_type& l)
      {
        return symbol_type (token::TOK_IFFULL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NOWAIT (location_type l)
      {
        return symbol_type (token::TOK_NOWAIT, std::move (l));
      }
#else
      static
      symbol_type
      make_NOWAIT (const location_type& l)
      {
        return symbol_type (token::TOK_NOWAIT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CLEAR (location_type l)
      {
        return symbol_type (token::TOK_CLEAR, std::move (l));
      }
#else
      static
      symbol_type
      make_CLEAR (const location_type& l)
      {
        return symbol_type (token::TOK_CLEAR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_REL (location_type l)
      {
        return symbol_type (token::TOK_REL, std::move (l));
      }
#else
      static
      symbol_type
      make_REL (const location_type& l)
      {
        return symbol_type (token::TOK_REL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_X (location_type l)
      {
        return symbol_type (token::TOK_X, std::move (l));
      }
#else
      static
      symbol_type
      make_X (const location_type& l)
      {
        return symbol_type (token::TOK_X, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_Y (location_type l)
      {
        return symbol_type (token::TOK_Y, std::move (l));
      }
#else
      static
      symbol_type
      make_Y (const location_type& l)
      {
        return symbol_type (token::TOK_Y, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_EXEC (location_type l)
      {
        return symbol_type (token::TOK_EXEC, std::move (l));
      }
#else
      static
      symbol_type
      make_EXEC (const location_type& l)
      {
        return symbol_type (token::TOK_EXEC, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PC (location_type l)
      {
        return symbol_type (token::TOK_PC, std::move (l));
      }
#else
      static
      symbol_type
      make_PC (const location_type& l)
      {
        return symbol_type (token::TOK_PC, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ISR (location_type l)
      {
        return symbol_type (token::TOK_ISR, std::move (l));
      }
#else
      static
      symbol_type
      make_ISR (const location_type& l)
      {
        return symbol_type (token::TOK_ISR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OSR (location_type l)
      {
        return symbol_type (token::TOK_OSR, std::move (l));
      }
#else
      static
      symbol_type
      make_OSR (const location_type& l)
      {
        return symbol_type (token::TOK_OSR, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OPTIONAL (location_type l)
      {
        return symbol_type (token::TOK_OPTIONAL, std::move (l));
      }
#else
      static
      symbol_type
      make_OPTIONAL (const location_type& l)
      {
        return symbol_type (token::TOK_OPTIONAL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SIDE (location_type l)
      {
        return symbol_type (token::TOK_SIDE, std::move (l));
      }
#else
      static
      symbol_type
      make_SIDE (const location_type& l)
      {
        return symbol_type (token::TOK_SIDE, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STATUS (location_type l)
      {
        return symbol_type (token::TOK_STATUS, std::move (l));
      }
#else
      static
      symbol_type
      make_STATUS (const location_type& l)
      {
        return symbol_type (token::TOK_STATUS, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PUBLIC (location_type l)
      {
        return symbol_type (token::TOK_PUBLIC, std::move (l));
      }
#else
      static
      symbol_type
      make_PUBLIC (const location_type& l)
      {
        return symbol_type (token::TOK_PUBLIC, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RP2040 (location_type l)
      {
        return symbol_type (token::TOK_RP2040, std::move (l));
      }
#else
      static
      symbol_type
      make_RP2040 (const location_type& l)
      {
        return symbol_type (token::TOK_RP2040, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RP2350 (location_type l)
      {
        return symbol_type (token::TOK_RP2350, std::move (l));
      }
#else
      static
      symbol_type
      make_RP2350 (const location_type& l)
      {
        return symbol_type (token::TOK_RP2350, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RXFIFO (location_type l)
      {
        return symbol_type (token::TOK_RXFIFO, std::move (l));
      }
#else
      static
      symbol_type
      make_RXFIFO (const location_type& l)
      {
        return symbol_type (token::TOK_RXFIFO, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TXFIFO (location_type l)
      {
        return symbol_type (token::TOK_TXFIFO, std::move (l));
      }
#else
      static
      symbol_type
      make_TXFIFO (const location_type& l)
      {
        return symbol_type (token::TOK_TXFIFO, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TXRX (location_type l)
      {
        return symbol_type (token::TOK_TXRX, std::move (l));
      }
#else
      static
      symbol_type
      make_TXRX (const location_type& l)
      {
        return symbol_type (token::TOK_TXRX, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TX (location_type l)
      {
        return symbol_type (token::TOK_TX, std::move (l));
      }
#else
      static
      symbol_type
      make_TX (const location_type& l)
      {
        return symbol_type (token::TOK_TX, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RX (location_type l)
      {
        return symbol_type (token::TOK_RX, std::move (l));
      }
#else
      static
      symbol_type
      make_RX (const location_type& l)
      {
        return symbol_type (token::TOK_RX, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TXPUT (location_type l)
      {
        return symbol_type (token::TOK_TXPUT, std::move (l));
      }
#else
      static
      symbol_type
      make_TXPUT (const location_type& l)
      {
        return symbol_type (token::TOK_TXPUT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TXGET (location_type l)
      {
        return symbol_type (token::TOK_TXGET, std::move (l));
      }
#else
      static
      symbol_type
      make_TXGET (const location_type& l)
      {
        return symbol_type (token::TOK_TXGET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_PUTGET (location_type l)
      {
        return symbol_type (token::TOK_PUTGET, std::move (l));
      }
#else
      static
      symbol_type
      make_PUTGET (const location_type& l)
      {
        return symbol_type (token::TOK_PUTGET, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LEFT (location_type l)
      {
        return symbol_type (token::TOK_LEFT, std::move (l));
      }
#else
      static
      symbol_type
      make_LEFT (const location_type& l)
      {
        return symbol_type (token::TOK_LEFT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_RIGHT (location_type l)
      {
        return symbol_type (token::TOK_RIGHT, std::move (l));
      }
#else
      static
      symbol_type
      make_RIGHT (const location_type& l)
      {
        return symbol_type (token::TOK_RIGHT, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AUTO (location_type l)
      {
        return symbol_type (token::TOK_AUTO, std::move (l));
      }
#else
      static
      symbol_type
      make_AUTO (const location_type& l)
      {
        return symbol_type (token::TOK_AUTO, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_MANUAL (location_type l)
      {
        return symbol_type (token::TOK_MANUAL, std::move (l));
      }
#else
      static
      symbol_type
      make_MANUAL (const location_type& l)
      {
        return symbol_type (token::TOK_MANUAL, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ID (std::string v, location_type l)
      {
        return symbol_type (token::TOK_ID, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_ID (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_ID, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_STRING (std::string v, location_type l)
      {
        return symbol_type (token::TOK_STRING, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_STRING (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_STRING, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NON_WS (std::string v, location_type l)
      {
        return symbol_type (token::TOK_NON_WS, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_NON_WS (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_NON_WS, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CODE_BLOCK_START (std::string v, location_type l)
      {
        return symbol_type (token::TOK_CODE_BLOCK_START, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CODE_BLOCK_START (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_CODE_BLOCK_START, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CODE_BLOCK_CONTENTS (std::string v, location_type l)
      {
        return symbol_type (token::TOK_CODE_BLOCK_CONTENTS, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_CODE_BLOCK_CONTENTS (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_CODE_BLOCK_CONTENTS, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_UNKNOWN_DIRECTIVE (std::string v, location_type l)
      {
        return symbol_type (token::TOK_UNKNOWN_DIRECTIVE, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_UNKNOWN_DIRECTIVE (const std::string& v, const location_type& l)
      {
        return symbol_type (token::TOK_UNKNOWN_DIRECTIVE, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_INT (int v, location_type l)
      {
        return symbol_type (token::TOK_INT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_INT (const int& v, const location_type& l)
      {
        return symbol_type (token::TOK_INT, v, l);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FLOAT (float v, location_type l)
      {
        return symbol_type (token::TOK_FLOAT, std::move (v), std::move (l));
      }
#else
      static
      symbol_type
      make_FLOAT (const float& v, const location_type& l)
      {
        return symbol_type (token::TOK_FLOAT, v, l);
      }
#endif


    class context
    {
    public:
      context (const parser& yyparser, const symbol_type& yyla);
      const symbol_type& lookahead () const { return yyla_; }
      symbol_kind_type token () const { return yyla_.kind (); }
      const location_type& location () const { return yyla_.location; }

      /// Put in YYARG at most YYARGN of the expected tokens, and return the
      /// number of tokens stored in YYARG.  If YYARG is null, return the
      /// number of expected tokens (guaranteed to be less than YYNTOKENS).
      int expected_tokens (symbol_kind_type yyarg[], int yyargn) const;

    private:
      const parser& yyparser_;
      const symbol_type& yyla_;
    };

  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    parser (const parser&);
    /// Non copyable.
    parser& operator= (const parser&);
#endif

    /// Check the lookahead yytoken.
    /// \returns  true iff the token will be eventually shifted.
    bool yy_lac_check_ (symbol_kind_type yytoken) const;
    /// Establish the initial context if no initial context currently exists.
    /// \returns  true iff the token will be eventually shifted.
    bool yy_lac_establish_ (symbol_kind_type yytoken);
    /// Discard any previous initial lookahead context because of event.
    /// \param event  the event which caused the lookahead to be discarded.
    ///               Only used for debbuging output.
    void yy_lac_discard_ (const char* event);

    /// Stored state numbers (used for stacks).
    typedef short state_type;

    /// The arguments of the error message.
    int yy_syntax_error_arguments_ (const context& yyctx,
                                    symbol_kind_type yyarg[], int yyargn) const;

    /// Generate an error message.
    /// \param yyctx     the context in which the error occurred.
    virtual std::string yysyntax_error_ (const context& yyctx) const;
    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue);

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue);

    static const signed char yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token kind \a t to a symbol kind.
    /// In theory \a t should be a token_kind_type, but character literals
    /// are valid, yet not members of the token_type enum.
    static symbol_kind_type yytranslate_ (int t);

    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    static std::string yytnamerr_ (const char *yystr);

    /// For a symbol, its name in clear.
    static const char* const yytname_[];


    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const short yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const unsigned char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const short yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const short yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const short yytable_[];

    static const short yycheck_[];

    // YYSTOS[STATE-NUM] -- The (internal number of the) accessing
    // symbol of state STATE-NUM.
    static const unsigned char yystos_[];

    // YYR1[YYN] -- Symbol number of symbol that rule YYN derives.
    static const unsigned char yyr1_[];

    // YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.
    static const signed char yyr2_[];


#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r) const;
    /// Print the state stack on the debug stream.
    virtual void yy_stack_print_ () const;

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol kind, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol kind as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_state& that);

      /// The symbol kind (corresponding to \a state).
      /// \a symbol_kind::S_YYEMPTY when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::iterator iterator;
      typedef typename S::const_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200)
        : seq_ (n)
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Non copyable.
      stack (const stack&) = delete;
      /// Non copyable.
      stack& operator= (const stack&) = delete;
#endif

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.begin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.end ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range)
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
#if YY_CPLUSPLUS < 201103L
      /// Non copyable.
      stack (const stack&);
      /// Non copyable.
      stack& operator= (const stack&);
#endif
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;
    /// The stack for LAC.
    /// Logically, the yy_lac_stack's lifetime is confined to the function
    /// yy_lac_check_. We just store it as a member of this class to hold
    /// on to the memory and to avoid frequent reallocations.
    /// Since yy_lac_check_ is const, this member must be mutable.
    mutable std::vector<state_type> yylac_stack_;
    /// Whether an initial LAC context was established.
    bool yy_lac_established_;


    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1);

    /// Constants.
    enum
    {
      yylast_ = 307,     ///< Last index in yytable_.
      yynnts_ = 32,  ///< Number of nonterminal symbols.
      yyfinal_ = 127 ///< Termination state number.
    };


    // User arguments.
    pio_assembler& pioasm;

  };

  inline
  parser::symbol_kind_type
  parser::yytranslate_ (int t)
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const signed char
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97
    };
    // Last valid token kind.
    const int code_max = 352;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return YY_CAST (symbol_kind_type, translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

  // basic_symbol.
  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value ()
    , location (that.location)
  {
    switch (this->kind ())
    {
      case symbol_kind::S_direction: // direction
      case symbol_kind::S_autop: // autop
      case symbol_kind::S_if_full: // if_full
      case symbol_kind::S_if_empty: // if_empty
      case symbol_kind::S_blocking: // blocking
        value.copy< bool > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_condition: // condition
        value.copy< enum condition > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_fifo_config: // fifo_config
        value.copy< enum fifo_config > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_in_source: // in_source
      case symbol_kind::S_out_target: // out_target
      case symbol_kind::S_set_target: // set_target
        value.copy< enum in_out_set > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
        value.copy< enum irq > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_mov_op: // mov_op
        value.copy< enum mov_op > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_mov_target: // mov_target
      case symbol_kind::S_mov_source: // mov_source
        value.copy< extended_mov > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FLOAT: // "float"
        value.copy< float > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_INT: // "integer"
        value.copy< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_instruction: // instruction
      case symbol_kind::S_base_instruction: // base_instruction
        value.copy< std::shared_ptr<instruction> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_value: // value
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_delay: // delay
      case symbol_kind::S_sideset: // sideset
      case symbol_kind::S_threshold: // threshold
        value.copy< std::shared_ptr<resolvable> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_label_decl: // label_decl
      case symbol_kind::S_symbol_def: // symbol_def
        value.copy< std::shared_ptr<symbol> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_wait_source: // wait_source
        value.copy< std::shared_ptr<wait_source> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ID: // "identifier"
      case symbol_kind::S_STRING: // "string"
      case symbol_kind::S_NON_WS: // "text"
      case symbol_kind::S_CODE_BLOCK_START: // "code block"
      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
        value.copy< std::string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_pio_version: // pio_version
        value.copy< uint > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

  }



  template <typename Base>
  parser::symbol_kind_type
  parser::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }

  template <typename Base>
  bool
  parser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    switch (this->kind ())
    {
      case symbol_kind::S_direction: // direction
      case symbol_kind::S_autop: // autop
      case symbol_kind::S_if_full: // if_full
      case symbol_kind::S_if_empty: // if_empty
      case symbol_kind::S_blocking: // blocking
        value.move< bool > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_condition: // condition
        value.move< enum condition > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_fifo_config: // fifo_config
        value.move< enum fifo_config > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_in_source: // in_source
      case symbol_kind::S_out_target: // out_target
      case symbol_kind::S_set_target: // set_target
        value.move< enum in_out_set > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_irq_modifiers: // irq_modifiers
        value.move< enum irq > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_mov_op: // mov_op
        value.move< enum mov_op > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_mov_target: // mov_target
      case symbol_kind::S_mov_source: // mov_source
        value.move< extended_mov > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_FLOAT: // "float"
        value.move< float > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_INT: // "integer"
        value.move< int > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_instruction: // instruction
      case symbol_kind::S_base_instruction: // base_instruction
        value.move< std::shared_ptr<instruction> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_value: // value
      case symbol_kind::S_expression: // expression
      case symbol_kind::S_delay: // delay
      case symbol_kind::S_sideset: // sideset
      case symbol_kind::S_threshold: // threshold
        value.move< std::shared_ptr<resolvable> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_label_decl: // label_decl
      case symbol_kind::S_symbol_def: // symbol_def
        value.move< std::shared_ptr<symbol> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_wait_source: // wait_source
        value.move< std::shared_ptr<wait_source> > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_ID: // "identifier"
      case symbol_kind::S_STRING: // "string"
      case symbol_kind::S_NON_WS: // "text"
      case symbol_kind::S_CODE_BLOCK_START: // "code block"
      case symbol_kind::S_CODE_BLOCK_CONTENTS: // "%}"
      case symbol_kind::S_UNKNOWN_DIRECTIVE: // UNKNOWN_DIRECTIVE
        value.move< std::string > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_pio_version: // pio_version
        value.move< uint > (YY_MOVE (s.value));
        break;

      default:
        break;
    }

    location = YY_MOVE (s.location);
  }

  // by_kind.
  inline
  parser::by_kind::by_kind ()
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  inline
  parser::by_kind::by_kind (by_kind&& that)
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  inline
  parser::by_kind::by_kind (const by_kind& that)
    : kind_ (that.kind_)
  {}

  inline
  parser::by_kind::by_kind (token_kind_type t)
    : kind_ (yytranslate_ (t))
  {}

  inline
  void
  parser::by_kind::clear ()
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  inline
  void
  parser::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  inline
  parser::symbol_kind_type
  parser::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }

  inline
  parser::symbol_kind_type
  parser::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }

} // yy




#endif // !YY_YY_HOME_GRAHAM_DEV_AMY_AMY_SDK_TOOLS_PIOASM_GEN_PARSER_HPP_INCLUDED
