#!/usr/bin/env python3

import os
import re
import sys
from collections import namedtuple
from dataclasses import dataclass

ENFORCE_SEQUENTIAL_SUFFIXES = True # if False, allow "double2ufix8" to be followed by "double2ufix12"
ENFORCE_UNDERSCORE_IN_SUFFIX_IF_FUNCTION_ENDS_WITH_NUMBER = True # if False, allow "float2fix641"
ALLOW_TEST_SUFFIXES_TO_START_AT_ZERO = True # if False, don't allow "float2fix0"
ALLOW_TEST_SUFFIXES_TO_SKIP_A = True # if False, don't allow "float2int1b" to follow "float2int1"

Check = namedtuple("Check", ("test_type", "header_file", "tests_file"))

CHECKS = (
    Check("float", "src/rp2_common/pico_float/include/pico/float.h", "test/pico_float_test/custom_float_funcs_test.c"),
    Check("double", "src/rp2_common/pico_double/include/pico/double.h", "test/pico_float_test/custom_double_funcs_test.c"),
)

CONVERSION_TYPES = set((
    # integral types
    "int", "uint", "int64", "uint64",
    # floating-point types
    "float", "double",
    # fixed-point types
    "fix", "ufix", "fix64", "ufix64",
    # integral types that round towards zero
    "int_z", "uint_z", "int64_z", "uint64_z",
    # fixed-point types that round towards zero
    "fix_z", "ufix_z", "fix64_z", "ufix64_z",
    # other "types" used in the test functions
    "float_8", "float_12", "float_16", "float_24", "float_28", "float_32",
    "double_8", "double_12", "double_16", "double_24", "double_28", "double_32",
    "fix_12", "ufix_12",
))

if ENFORCE_SEQUENTIAL_SUFFIXES:
    @dataclass
    class ConversionFunc:
        name: str
        from_type: str
        to_type: str
        num_input_args: int
        tested: bool = False
        last_test: str = ""
        last_test_suffix: str = ""
else:
    @dataclass
    class ConversionFunc:
        name: str
        from_type: str
        to_type: str
        num_input_args: int
        tested: bool = False

def add_conversion_function(conversion_functions, conversion_function, from_type, to_type, num_input_args, filename, lineno):
    if conversion_function in conversion_functions:
        raise Exception(f"{filename}:{lineno} Conversion function {conversion_function} appears twice")
    else:
        if from_type not in CONVERSION_TYPES:
            raise Exception(f"{filename}:{lineno} Conversion function {conversion_function} converts from unknown type {from_type}")
        if to_type not in CONVERSION_TYPES:
            raise Exception(f"{filename}:{lineno} Conversion function {conversion_function} converts to unknown type {to_type}")
        conversion_functions[conversion_function] = ConversionFunc(conversion_function, from_type, to_type, num_input_args)

@dataclass
class TestMacro:
    name: str
    short_type: str
    used: bool = False

def add_test_macro(test_macros, test_macro, short_type, filename, lineno):
    if test_macro in test_macros:
        raise Exception(f"{filename}:{lineno} Test macro {test_macro} defined twice")
    else:
        test_macros[test_macro] = TestMacro(test_macro, short_type)

@dataclass
class FunctionGroup:
    name: str
    lineno: int
    from_type: str
    to_type: str
    used: bool = False

def add_function_group(function_groups, function_group, from_type, to_type, filename, lineno):
    if function_group in function_groups:
        raise Exception(f"{filename}:{lineno} Function group {function_group} appears twice")
    else:
        function_groups[function_group] = FunctionGroup(function_group, lineno, from_type, to_type)

def suffix_greater(suffix1, suffix2):
    if suffix1 == suffix2:
        raise Exception(f"Identical suffixes {suffix1}")
    if suffix2 == "":
        return True
    else:
        m1 = re.match(r"^(\d+)([a-z])?$", suffix1)
        num1 = int(m1.group(1))
        alpha1 = m1.group(2)
        m2 = re.match(r"^(\d+)([a-z])?$", suffix2)
        num2 = int(m2.group(1))
        alpha2 = m2.group(2)
        if num1 > num2:
            return True
        elif num1 < num2:
            return False
        else: # num1 == num2
            if alpha2 is None and alpha1 is not None:
                return True
            else:
                return alpha1 > alpha2

def suffix_one_more_than(suffix1, suffix2):
    if suffix1 == suffix2:
        raise Exception(f"Identical suffixes {suffix1}")
    m1 = re.match(r"^(\d+)([a-z])?$", suffix1)
    num1 = int(m1.group(1))
    alpha1 = m1.group(2)
    if suffix2 == "":
        if ALLOW_TEST_SUFFIXES_TO_START_AT_ZERO:
            return num1 in (0, 1) and (alpha1 is None or alpha1 == "a")
        else:
            return num1 == 1 and (alpha1 is None or alpha1 == "a")
    else:
        m2 = re.match(r"^(\d+)([a-z])?$", suffix2)
        num2 = int(m2.group(1))
        alpha2 = m2.group(2)
        if num1 > num2:
            return num1 - num2 == 1 and (alpha1 is None or alpha1 == "a")
        elif num1 < num2:
            return False
        else: # num1 == num2
            if alpha2 is None and alpha1 is not None:
                if ALLOW_TEST_SUFFIXES_TO_SKIP_A:
                    return alpha1 == "b"
                else:
                    return False
            else:
                return ord(alpha1) - ord(alpha2) == 1

def type_to_short_type(t):
    assert(t in CONVERSION_TYPES)
    if t in ("int", "fix", "int_z", "fix_z") or t.startswith("fix_"):
        return "i"
    elif t in ("uint", "ufix", "uint_z", "ufix_z") or t.startswith("ufix_"):
        return "u"
    elif t in ("int64", "fix64", "int64_z", "fix64_z"):
        return "i64"
    elif t in ("uint64", "ufix64", "uint64_z", "ufix64_z"):
        return "u64"
    elif t == "float" or t.startswith("float_"):
        return "f"
    elif t == "double" or t.startswith("double_"):
        return "d"
    else:
        raise Exception(f"Couldn't determine short_type for {t}")


if __name__ == "__main__":
    for check in CHECKS:
        conversion_functions = dict()
        with open(check.header_file) as fh:
            #print(f"Reading {check.header_file}")
            for lineno, line in enumerate(fh.readlines()):
                lineno += 1
                line = line.strip()
                # strip trailing comments
                line = re.sub(r"\s*//.*$", "", line)
                if line:
                    if m := re.match(r"^\w+ ((\w+)2(\w+))\(([^\)]+)\);$", line):
                        conversion_function = m.group(1)
                        from_type = m.group(2)
                        to_type = m.group(3)
                        num_input_args = len(m.group(4).split(","))
                        #print(lineno, line, conversion_function)
                        add_conversion_function(conversion_functions, conversion_function, from_type, to_type, num_input_args, check.header_file, lineno)

        test_macros = dict()
        function_groups = dict()
        last_function_group = None
        test_names = set()

        def _check_test(filename, lineno, line, test_macro, function, num_input_args, compare_val, to_type, test_name):
            #print(lineno, line, test_macro, function, num_input_args, compare_val, test_name)
            if test_macro not in test_macros:
                raise Exception(f"{filename}:{lineno} Trying to use unknown test macro {test_macro}")
            else:
                test_macros[test_macro].used = True
                short_type = type_to_short_type(to_type)
                expected_macro = "test_check" + short_type
                if test_macro != expected_macro:
                    raise Exception(f"{filename}:{lineno} {test_name} tests {function} which returns {to_type}, so expected it to be checked with {expected_macro} (not {test_macro})")
            if function not in conversion_functions:
                raise Exception(f"{filename}:{lineno} Trying to use unknown conversion function {function}")
            else:
                conversion_functions[function].tested = True
                if num_input_args != conversion_functions[function].num_input_args:
                    raise Exception(f"{filename}:{lineno} {num_input_args} arguments were passed to {function} which only expects {conversion_functions[function].num_input_args} arguments")
            function_group = re.sub(r"_(\d+)$", "_N", function)
            if function_group not in function_groups:
                raise Exception(f"{filename}:{lineno} Unexpected function group {function_group}")
            else:
                function_groups[function_group].used = True
                if function_group != last_function_group:
                    raise Exception(f"{filename}:{lineno} Function group {function_group} doesn't match {last_function_group} on line {function_groups[last_function_group].lineno}")
            expected_prefix = function
            if not test_name.startswith(expected_prefix):
                raise Exception(f"{filename}:{lineno} {test_name} tests {function}, so expected it to start with {expected_prefix}")
            if ENFORCE_UNDERSCORE_IN_SUFFIX_IF_FUNCTION_ENDS_WITH_NUMBER:
                if re.search(r"\d$", function):
                    expected_prefix += "_"
                    if not test_name.startswith(expected_prefix):
                        raise Exception(f"{filename}:{lineno} {function} ends with a number, so expected the test name ({test_name}) to start with {expected_prefix}")
            if test_name in test_names:
                raise Exception(f"{filename}:{lineno} Duplicate test name {test_name}")
            test_names.add(test_name)
            if ENFORCE_UNDERSCORE_IN_SUFFIX_IF_FUNCTION_ENDS_WITH_NUMBER:
                test_suffix = re.sub(f"^{re.escape(expected_prefix)}", "", test_name)
            else:
                test_suffix = re.sub(f"^{re.escape(expected_prefix)}_?", "", test_name)
            if not re.match(r"^\d+([a-z])?$", test_suffix):
                raise Exception(f"{filename}:{lineno} {test_name} has suffix {test_suffix} which doesn't match the expected pattern of a number followed by an optional letter")
            if ENFORCE_SEQUENTIAL_SUFFIXES:
                if not suffix_greater(test_suffix, conversion_functions[function].last_test_suffix):
                    raise Exception(f"{filename}:{lineno} {test_name} follows {conversion_functions[function].last_test} but has a smaller suffix")
                if not suffix_one_more_than(test_suffix, conversion_functions[function].last_test_suffix):
                    if conversion_functions[function].last_test_suffix == "":
                        raise Exception(f"{filename}:{lineno} {test_name} is the first test in group {function_group} so expected a suffix of 1 (or 1a), not {test_suffix}")
                    elif test_suffix == conversion_functions[function].last_test_suffix + "a":
                        raise Exception(f"{filename}:{lineno} {test_name} uses suffix {test_suffix} which can't follow suffix {conversion_functions[function].last_test_suffix}")
                    else:
                        raise Exception(f"{filename}:{lineno} {test_name} follows {conversion_functions[function].last_test} but the jump from {conversion_functions[function].last_test_suffix} to {test_suffix} is bigger than one")
                conversion_functions[function].last_test = test_name
                conversion_functions[function].last_test_suffix = test_suffix

        with open(check.tests_file) as fh:
            #print(f"Reading {check.tests_file}")
            for lineno, line in enumerate(fh.readlines()):
                lineno += 1
                line = line.strip()
                # strip trailing comments
                line = re.sub(r"\s*//.*$", "", line)
                if line:
                    if m := re.match(r"^#define (test_check(\w+))\(", line):
                        test_macro = m.group(1)
                        short_type = m.group(2)
                        #print(lineno, line, test_macro)
                        add_test_macro(test_macros, test_macro, short_type, check.tests_file, lineno)
                    elif m := re.match(r"^#define ((\w+)2(\w+))\(([^\)]+)\)", line):
                        conversion_function = m.group(1)
                        from_type = m.group(2)
                        to_type = m.group(3)
                        num_input_args = len(m.group(4).split(","))
                        #print(lineno, line, conversion_function)
                        if conversion_function not in conversion_functions:
                            raise Exception(f"{check.tests_file}:{lineno} {conversion_function} has no counterpart in {check.header_file}")
                        else:
                            if num_input_args != conversion_functions[conversion_function].num_input_args:
                                raise Exception(f"{check.tests_file}:{lineno} {conversion_function} has a different number of arguments to the counterpart in {check.header_file}")
                    elif m := re.match(r"^\w+ __attribute__\(\(naked\)\) (call_((\w+)2(\w+)))\(([^\)]+)\)", line):
                        conversion_function = m.group(1)
                        base_function = m.group(2)
                        from_type = m.group(3)
                        to_type = m.group(4)
                        num_input_args = len(m.group(5).split(","))
                        #print(lineno, line, conversion_function)
                        if base_function not in conversion_functions:
                            raise Exception(f"{check.tests_file}:{lineno} {conversion_function} exists but {base_function} doesn't exist")
                        else:
                            if num_input_args != conversion_functions[base_function].num_input_args:
                                raise Exception(f"{check.tests_file}:{lineno} {conversion_function} has a different number of arguments to {base_function}")
                        add_conversion_function(conversion_functions, conversion_function, from_type, to_type, num_input_args, check.tests_file, lineno)
                    elif m := re.match(r"^static inline (?:float|double) ((\w+)2(\w+_\d+))\(([^\)]+)\)", line):
                        conversion_function = m.group(1)
                        from_type = m.group(2)
                        to_type = m.group(3)
                        num_input_args = len(m.group(4).split(","))
                        #print(lineno, line, conversion_function)
                        m = re.match(r"^static inline (?:float|double) (\w+2\w+)_\d+\(", line)
                        base_function = m.group(1)
                        if base_function not in conversion_functions:
                            raise Exception(f"{check.tests_file}:{lineno} {conversion_function} exists but {base_function} doesn't exist")
                        add_conversion_function(conversion_functions, conversion_function, from_type, to_type, num_input_args, check.tests_file, lineno)
                    elif m := re.match(r"^printf\(\"((\w+)2(\w+))\\n\"\);$", line):
                        function_group = m.group(1)
                        from_type = m.group(2)
                        to_type = m.group(3)
                        #print(lineno, line, function_group)
                        if not function_group.endswith("_N"):
                            if function_group not in conversion_functions:
                                raise Exception(f"{check.tests_file}:{lineno} Function group {function_group} has no corresponding conversion function")
                        add_function_group(function_groups, function_group, from_type, to_type, check.tests_file, lineno)
                        last_function_group = function_group
                    elif m := re.match(r"^(test_\w+)\(((\w+?)2(\w+))\(([^\)]+(?:\(.*?\))?)\), ([^,]+), \"(\w+)\"\);$", line):
                        test_macro = m.group(1)
                        function = m.group(2)
                        from_type = m.group(3)
                        to_type = m.group(4)
                        num_input_args = len(m.group(5).split(","))
                        compare_val = m.group(6)
                        test_name = m.group(7)
                        _check_test(check.tests_file, lineno, line, test_macro, function, num_input_args, compare_val, to_type, test_name)
                    elif m:= re.match(r"^(test_\w+)\(((double)2(int))20, ([^,]+), \"(\w+)\"\);$", line):
                        # special-case, because it uses a stored value rather than an inline conversion
                        test_macro = m.group(1)
                        function = m.group(2)
                        from_type = m.group(3)
                        to_type = m.group(4)
                        num_input_args = 1
                        compare_val = m.group(5)
                        test_name = m.group(6)
                        _check_test(check.tests_file, lineno, line, test_macro, function, num_input_args, compare_val, to_type, test_name)
                    elif line.startswith("test_"):
                        raise Exception(f"{check.tests_file}:{lineno} It looks like '{line}' wasn't checked by {os.path.basename(sys.argv[0])}")
                    #else:
                    #    print(lineno, line)
        #print(sorted(conversion_functions.keys()))
        #print(sorted(test_macros.keys()))
        untested_conversion_functions = list(filter(lambda f: f.tested == False, conversion_functions.values()))
        if untested_conversion_functions:
            print(f"The following {check.test_type} functions are untested: {sorted(f.name for f in untested_conversion_functions)}")
        unused_test_macros = list(filter(lambda m: m.used == False, test_macros.values()))
        if unused_test_macros:
            print(f"The following {check.test_type} test macros weren't used: {sorted(m.name for m in unused_test_macros)}")
        unused_function_groups = list(filter(lambda g: g.used == False, function_groups.values()))
        if unused_function_groups:
            print(f"The following {check.test_type} function groups didn't contain any tests: {sorted(m.name for m in unused_function_groups)}")

