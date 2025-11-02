#include "Env.hpp"

#include <cstdlib>
#include <string_view>

using namespace Hyprtoolkit;
using namespace Hyprtoolkit::Env;

bool Hyprtoolkit::Env::envEnabled(const std::string& env) {
    auto ret = getenv(env.c_str());
    if (!ret)
        return false;

    const std::string_view sv = ret;

    return !sv.empty() && sv != "0";
}

bool Hyprtoolkit::Env::isTrace() {
    static bool TRACE = envEnabled("HT_TRACE");
    return TRACE;
}

#ifdef HT_UNIT_TESTS

#include <gtest/gtest.h>

TEST(Env, enabled) {
    setenv("HT_INTERNAL_TEST1", "1", 1);
    setenv("HT_INTERNAL_TEST2", "SUS", 1);
    setenv("HT_INTERNAL_TEST3", "0", 1);
    setenv("HT_INTERNAL_TEST4", "", 1);

    EXPECT_EQ(Env::envEnabled("HT_INTERNAL_TEST1"), true);
    EXPECT_EQ(Env::envEnabled("HT_INTERNAL_TEST2"), true);
    EXPECT_EQ(Env::envEnabled("HT_INTERNAL_TEST3"), false);
    EXPECT_EQ(Env::envEnabled("HT_INTERNAL_TEST4"), false);
}

#endif
