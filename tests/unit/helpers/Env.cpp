#include <gtest/gtest.h>

#include <helpers/Env.hpp>

using namespace Hyprtoolkit;

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