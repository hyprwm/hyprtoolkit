#include <helpers/UTF8.hpp>

#include <gtest/gtest.h>

using namespace Hyprtoolkit;

TEST(UTF8, Length) {
    EXPECT_EQ(UTF8::length(""), 0);
    EXPECT_EQ(UTF8::length("Hello"), 5);
    EXPECT_EQ(UTF8::length("世界"), 2);
    EXPECT_EQ(UTF8::length("世界is酷薄"), 6);
}

TEST(UTF8, Substr) {
    EXPECT_EQ(UTF8::substr("", 0, 0), "");
    EXPECT_EQ(UTF8::substr("死", 1), "");
    EXPECT_EQ(UTF8::substr("死", 0, 0), "");
    EXPECT_EQ(UTF8::substr("Hello", 0, 3), "Hel");
    EXPECT_EQ(UTF8::substr("世界", 1, 1), "界");
    EXPECT_EQ(UTF8::substr("世界is酷薄", 1), "界is酷薄");
    EXPECT_EQ(UTF8::substr("ハイパーランド", 1, 2), "イパ");
}

TEST(UTF8, UTF8ToOffset) {
    EXPECT_EQ(UTF8::utf8ToOffset("", 0), 0);
    EXPECT_EQ(UTF8::utf8ToOffset("Hello", 20000), 5);
    EXPECT_EQ(UTF8::utf8ToOffset("Hello", 3), 3);
    EXPECT_EQ(UTF8::utf8ToOffset("魑魅魍魎", 3), 9);
    EXPECT_EQ(UTF8::utf8ToOffset("a魑魅魍魎", 3), 7);
}

TEST(UTF8, offsetToUTF8Len) {
    EXPECT_EQ(UTF8::offsetToUTF8Len("", 0), 0);
    EXPECT_EQ(UTF8::offsetToUTF8Len("Hello", 3), 3);
    EXPECT_EQ(UTF8::offsetToUTF8Len("魑魅魍魎", 3), 1);
    EXPECT_EQ(UTF8::offsetToUTF8Len("a魑魅魍魎", 4), 2);
}