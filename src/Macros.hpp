#pragma once

#include <iostream>
#include <csignal>

#include "./helpers/Env.hpp"

#define TRACE(expr)                                                                                                                                                                \
    {                                                                                                                                                                              \
        if (Hyprtoolkit::Env::isTrace()) {                                                                                                                                         \
            expr;                                                                                                                                                                  \
        }                                                                                                                                                                          \
    }

#define RASSERT(expr, reason, ...)                                                                                                                                                 \
    if (!(expr)) {                                                                                                                                                                 \
        std::cout << std::format("\n==========================================================================================\nASSERTION FAILED! \n\n{}\n\nat: line {} in {}",    \
                                 std::format(reason, ##__VA_ARGS__), __LINE__,                                                                                                     \
                                 ([]() constexpr -> std::string { return std::string(__FILE__).substr(std::string(__FILE__).find_last_of('/') + 1); })());                         \
        std::cout << "[HT] Assertion failed!";                                                                                                                                     \
        std::fflush(stdout);                                                                                                                                                       \
        raise(SIGABRT);                                                                                                                                                            \
    }

#define ASSERT(expr) RASSERT(expr, "?")