#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include "pio_assembler.h"
#include "output_format.h"

namespace {

struct ScopedRedirect {
    int stdout_fd = -1;
    int stderr_fd = -1;
    FILE *null_out = nullptr;
    FILE *null_err = nullptr;

    ScopedRedirect() {
        stdout_fd = dup(STDOUT_FILENO);
        stderr_fd = dup(STDERR_FILENO);
        null_out = freopen("/dev/null", "w", stdout);
        null_err = freopen("/dev/null", "w", stderr);
    }

    ~ScopedRedirect() {
        fflush(stdout);
        fflush(stderr);
        if (stdout_fd >= 0) {
            dup2(stdout_fd, STDOUT_FILENO);
            close(stdout_fd);
        }
        if (stderr_fd >= 0) {
            dup2(stderr_fd, STDERR_FILENO);
            close(stderr_fd);
        }
        if (null_out) {
            fclose(null_out);
        }
        if (null_err) {
            fclose(null_err);
        }
    }
};

std::string test_root() {
    const char *srcdir = std::getenv("TEST_SRCDIR");
    const char *workspace = std::getenv("TEST_WORKSPACE");
    if (srcdir && workspace) {
        std::filesystem::path root = std::filesystem::path(srcdir) / workspace / "tools" / "pioasm" / "test";
        return root.string();
    }
    return (std::filesystem::current_path() / "test").string();
}

std::vector<std::string> read_commands(const std::filesystem::path &path) {
    std::ifstream file(path);
    std::vector<std::string> commands;
    std::string line;

    while (std::getline(file, line)) {
        const std::string prefix = "// run: ";
        if (line.rfind(prefix, 0) == 0) {
            commands.push_back(line.substr(prefix.size()));
        } else {
            break;
        }
    }

    return commands;
}

struct CommandOptions {
    int default_pio_version = 0;
    std::string format = output_format::default_name;
    std::vector<std::string> output_options;
};

CommandOptions parse_command(const std::string &command) {
    CommandOptions options;
    std::istringstream input(command);
    std::string token;
    std::vector<std::string> tokens;

    while (input >> token) {
        tokens.push_back(token);
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == "pioasm") {
            continue;
        }
        if (tokens[i] == "-v" && i + 1 < tokens.size()) {
            options.default_pio_version = std::stoi(tokens[++i]);
            continue;
        }
        if (tokens[i] == "-o" && i + 1 < tokens.size()) {
            options.format = tokens[++i];
            continue;
        }
        if (tokens[i] == "-p" && i + 1 < tokens.size()) {
            options.output_options.push_back(tokens[++i]);
            continue;
        }
    }

    return options;
}

std::shared_ptr<output_format> find_format(const std::string &name) {
    const auto &formats = output_format::all();
    auto it = std::find_if(formats.begin(), formats.end(), [&](const auto &format) {
        return format->name == name;
    });
    if (it == formats.end()) {
        return nullptr;
    }
    return *it;
}

bool run_command(const std::filesystem::path &source, const CommandOptions &options, bool expect_failure) {
    auto format = find_format(options.format);
    if (!format) {
        return expect_failure;
    }

    pio_assembler assembler;
    assembler.default_pio_version = options.default_pio_version;

    ScopedRedirect redirect;

    int result = 1;
    try {
        result = assembler.generate(format, source.string(), "-", options.output_options);
    } catch (const std::exception &) {
        result = 1;
    }

    bool success = (result == 0 && assembler.error_count == 0);
    return expect_failure ? !success : success;
}

bool should_expect_failure(const std::filesystem::path &path) {
    std::string path_str = path.generic_string();
    return path_str.find("/errors/") != std::string::npos;
}

} // namespace

int main() {
    const char *outputs_dir = std::getenv("TEST_UNDECLARED_OUTPUTS_DIR");
    if (outputs_dir) {
        if (!std::getenv("LLVM_PROFILE_FILE")) {
            std::string profile_path = std::string(outputs_dir) + "/pioasm-%p.profraw";
            setenv("LLVM_PROFILE_FILE", profile_path.c_str(), 1);
        }
        if (!std::getenv("GCOV_PREFIX")) {
            setenv("GCOV_PREFIX", outputs_dir, 1);
        }
        if (!std::getenv("GCOV_PREFIX_STRIP")) {
            setenv("GCOV_PREFIX_STRIP", "0", 1);
        }
    }

    std::filesystem::path root = test_root();
    if (!std::filesystem::exists(root)) {
        std::cerr << "Test root not found: " << root << "\n";
        return 1;
    }

    std::vector<std::filesystem::path> tests;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto &path = entry.path();
        if (path.extension() == ".pio" && path.filename().string().rfind("test_", 0) == 0) {
            tests.push_back(path);
        }
    }

    std::sort(tests.begin(), tests.end());

    int failures = 0;
    for (const auto &test : tests) {
        auto commands = read_commands(test);
        bool expect_failure = should_expect_failure(test);
        for (const auto &command : commands) {
            auto options = parse_command(command);
            bool ok = run_command(test, options, expect_failure);
            if (!ok) {
                std::cerr << "Coverage test failed: " << test << " with command: " << command << "\n";
                failures++;
            }
        }
    }

    if (failures > 0) {
        std::cerr << failures << " coverage test command(s) failed\n";
        return 1;
    }

    return 0;
}
