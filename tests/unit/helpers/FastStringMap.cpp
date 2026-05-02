#include <helpers/FastStringMap.hpp>

#include <gtest/gtest.h>

#include <string>

using namespace Hyprtoolkit;

TEST(FastStringMap, emptyOnConstruction) {
    CFastStringMap<int> m;
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0u);
    EXPECT_EQ(m.get("anything"), nullptr);
}

TEST(FastStringMap, setAndGet) {
    CFastStringMap<int> m;
    m.set("foo", 1);
    m.set("bar", 2);

    EXPECT_FALSE(m.empty());
    EXPECT_EQ(m.size(), 2u);

    const auto* a = m.get("foo");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(*a, 1);

    const auto* b = m.get("bar");
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(*b, 2);

    EXPECT_EQ(m.get("missing"), nullptr);
}

TEST(FastStringMap, overwriteSameKey) {
    CFastStringMap<int> m;
    m.set("k", 1);
    m.set("k", 42);
    EXPECT_EQ(m.size(), 1u);
    ASSERT_NE(m.get("k"), nullptr);
    EXPECT_EQ(*m.get("k"), 42);
}

TEST(FastStringMap, growsBeyondInitialCapacity) {
    CFastStringMap<int> m;
    for (int i = 0; i < 200; ++i)
        m.set("key" + std::to_string(i), i);

    EXPECT_EQ(m.size(), 200u);

    for (int i = 0; i < 200; ++i) {
        const auto* v = m.get("key" + std::to_string(i));
        ASSERT_NE(v, nullptr) << "missing key " << i;
        EXPECT_EQ(*v, i);
    }
}

TEST(FastStringMap, emptyStringKey) {
    CFastStringMap<int> m;
    m.set("", 7);
    EXPECT_EQ(m.size(), 1u);
    ASSERT_NE(m.get(""), nullptr);
    EXPECT_EQ(*m.get(""), 7);
}

TEST(FastStringMap, valueLifetime) {
    CFastStringMap<std::string> m;
    m.set("a", std::string{"hello"});
    m.set("b", std::string{"world"});

    // force grow to trigger rehash + move
    for (int i = 0; i < 50; ++i)
        m.set("x" + std::to_string(i), "v" + std::to_string(i));

    ASSERT_NE(m.get("a"), nullptr);
    EXPECT_EQ(*m.get("a"), "hello");
    ASSERT_NE(m.get("b"), nullptr);
    EXPECT_EQ(*m.get("b"), "world");
}

TEST(FastStringMap, reserveDoesNotShrink) {
    CFastStringMap<int> m;
    for (int i = 0; i < 30; ++i)
        m.set("k" + std::to_string(i), i);

    m.reserve(0);
    m.reserve(4);

    EXPECT_EQ(m.size(), 30u);
    for (int i = 0; i < 30; ++i)
        ASSERT_NE(m.get("k" + std::to_string(i)), nullptr);
}
