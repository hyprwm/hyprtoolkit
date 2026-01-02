#include <helpers/UTF8.hpp>

#include <gtest/gtest.h>

using namespace Hyprtoolkit;

TEST(UTF8, codepointLen) {
    const auto str = std::string("世 hello 🧑‍🌾");
    EXPECT_EQ(UTF8::codepointLen(&str[0], str.length()), 3);
    EXPECT_EQ(UTF8::codepointLen(&str[3], str.length() - 3), 1);
    EXPECT_EQ(UTF8::codepointLen(&str[10], str.length() - 10), 4);
}

TEST(UTF8, codepointLenBefore) {
    const auto str = std::string("世 hello 🧑‍🌾");
    EXPECT_EQ(UTF8::codepointLenBefore(str, 0), 0);
    EXPECT_EQ(UTF8::codepointLenBefore(str, 3), 3);
    EXPECT_EQ(UTF8::codepointLenBefore(str, 10), 1);
    EXPECT_EQ(UTF8::codepointLenBefore(str, 14), 4);
}

TEST(UTF8, length) {
    EXPECT_EQ(UTF8::length(""), 0);
    EXPECT_EQ(UTF8::length("Hello"), 5);
    EXPECT_EQ(UTF8::length("世界"), 2);
    EXPECT_EQ(UTF8::length("世界is酷薄"), 6);
}

TEST(UTF8, substr) {
    EXPECT_EQ(UTF8::substr("", 0, 0), "");
    EXPECT_EQ(UTF8::substr("死", 1), "");
    EXPECT_EQ(UTF8::substr("死", 0, 0), "");
    EXPECT_EQ(UTF8::substr("Hello", 0, 3), "Hel");
    EXPECT_EQ(UTF8::substr("世界", 1, 1), "界");
    EXPECT_EQ(UTF8::substr("世界is酷薄", 1), "界is酷薄");
    EXPECT_EQ(UTF8::substr("ハイパーランド", 1, 2), "イパ");
}

TEST(UTF8, utf8ToOffset) {
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
    EXPECT_EQ(UTF8::offsetToUTF8Len("魑a魅", 1), 1);
}

TEST(UTF8, findFirstOf) {
    // weird values
    EXPECT_EQ(UTF8::findFirstOf("", ""), std::string::npos);
    EXPECT_EQ(UTF8::findFirstOf("", "aba"), std::string::npos);
    EXPECT_EQ(UTF8::findFirstOf("", "aba", 1000), std::string::npos);
    EXPECT_EQ(UTF8::findFirstOf("aba", ""), std::string::npos);
    EXPECT_EQ(UTF8::findFirstOf("aba", "", 1000), std::string::npos);
    EXPECT_EQ(UTF8::findFirstOf("aba", "a", 1000), std::string::npos);

    const auto str = "is 🧑‍🌾 the same as 🧑🌾?";
    EXPECT_EQ(UTF8::findFirstOf(str, "i", 2), std::string::npos);
    EXPECT_EQ(UTF8::findFirstOf(str, "I"), std::string::npos);
    EXPECT_EQ(UTF8::findFirstOf(str, "i"), 0);
    EXPECT_EQ(UTF8::findFirstOf(str, "?"), 35);
    EXPECT_EQ(UTF8::findFirstOf(str, "🧑"), 3);
}

TEST(UTF8, findLastOf) {
    // weird values
    EXPECT_EQ(UTF8::findLastOf("", ""), std::string::npos);
    EXPECT_EQ(UTF8::findLastOf("", "aba"), std::string::npos);
    EXPECT_EQ(UTF8::findLastOf("", "aba", 1000), std::string::npos);
    EXPECT_EQ(UTF8::findLastOf("aba", ""), std::string::npos);
    EXPECT_EQ(UTF8::findLastOf("aba", "", 1000), std::string::npos);
    EXPECT_EQ(UTF8::findLastOf("aba", "a", 1000), 2);

    const auto str = "is 🧑‍🌾 the same as 🧑🌾?";
    EXPECT_EQ(UTF8::findLastOf(str, "i", 2), 0);
    EXPECT_EQ(UTF8::findLastOf(str, "I"), std::string::npos);
    EXPECT_EQ(UTF8::findLastOf(str, "i"), 0);
    EXPECT_EQ(UTF8::findLastOf(str, "?"), 35);
    EXPECT_EQ(UTF8::findLastOf(str, "🧑"), 27);
}

TEST(UTF8, findFirstNotOf) {
    // weird values
    EXPECT_EQ(UTF8::findFirstNotOf("", ""), std::string::npos);
    EXPECT_EQ(UTF8::findFirstNotOf("", "aba"), std::string::npos);
    EXPECT_EQ(UTF8::findFirstNotOf("", "aba", 1000), std::string::npos);
    EXPECT_EQ(UTF8::findFirstNotOf("aba", "", 1000), std::string::npos);
    EXPECT_EQ(UTF8::findFirstNotOf("aba", "a", 1000), std::string::npos);

    EXPECT_EQ(UTF8::findFirstNotOf("aba", ""), 0);
    EXPECT_EQ(UTF8::findFirstNotOf("is?", "i", 2), 2);
    EXPECT_EQ(UTF8::findFirstNotOf("is?", "i"), 1);
    EXPECT_EQ(UTF8::findFirstNotOf("is?", "?", 2), std::string::npos);
    EXPECT_EQ(UTF8::findFirstNotOf("is?", "?"), 0);
    EXPECT_EQ(UTF8::findFirstNotOf("bbbbb", "b"), std::string::npos);
    EXPECT_EQ(UTF8::findFirstNotOf("🧑🧑🌾🌾🧑", "🧑"), 8);
}

TEST(UTF8, findLastNotOf) {
    // weird values
    EXPECT_EQ(UTF8::findLastNotOf("", ""), std::string::npos);
    EXPECT_EQ(UTF8::findLastNotOf("", "aba"), std::string::npos);
    EXPECT_EQ(UTF8::findLastNotOf("", "aba", 1000), std::string::npos);
    EXPECT_EQ(UTF8::findLastNotOf("aba", "", 1000), 2);
    EXPECT_EQ(UTF8::findLastNotOf("aba", "a", 1000), 1);

    EXPECT_EQ(UTF8::findLastNotOf("aba", ""), 2);
    EXPECT_EQ(UTF8::findLastNotOf("is?", "i", 3), 2);
    EXPECT_EQ(UTF8::findLastNotOf("is?", "i"), 2);
    EXPECT_EQ(UTF8::findLastNotOf("is?", "?", 2), 1);
    EXPECT_EQ(UTF8::findLastNotOf("is?", "?"), 1);
    EXPECT_EQ(UTF8::findLastNotOf("bbbbb", "b"), std::string::npos);
    EXPECT_EQ(UTF8::findLastNotOf("🧑🧑🌾🌾🧑", "🧑"), 12);
}
