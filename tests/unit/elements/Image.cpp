#include <gtest/gtest.h>

#include <element/image/Image.hpp>

#include <chrono>
#include <hyprgraphics/resource/resources/ImageResource.hpp>
#include <hyprtoolkit/system/Icons.hpp>

using namespace Hyprtoolkit;
using namespace std::chrono_literals;

class CDummyIcon final : public ISystemIconDescription {
  public:
    bool exists() override {
        return true;
    }

    bool scalable() override {
        return true;
    }
};

// the cache key has to encode the fit mode, otherwise two elements sharing a path but rendering
// with different fit modes collide on one cached texture (the texture bakes in the fit mode). this
// is the hyprpaper "same image, two monitors, different fit_mode" bug.
TEST(Element, imageCacheStringEncodesFitMode) {
    SImageImpl a, b;
    a.data.path = b.data.path = "wallpaper.png";

    a.data.fitMode = IMAGE_FIT_MODE_COVER;
    b.data.fitMode = IMAGE_FIT_MODE_STRETCH;
    EXPECT_NE(a.getCacheString(), b.getCacheString());

    b.data.fitMode = IMAGE_FIT_MODE_COVER;
    EXPECT_EQ(a.getCacheString(), b.getCacheString());
}

TEST(Element, imageSourcesAreExclusive) {
    SImageData data;
    const auto icon = makeShared<CDummyIcon>();

    data.setData({1, 2, 3});
    EXPECT_FALSE(data.data.empty());
    EXPECT_TRUE(data.path.empty());
    EXPECT_FALSE(data.icon);

    data.setPath("image.png");
    EXPECT_TRUE(data.data.empty());
    EXPECT_EQ(data.path, "image.png");
    EXPECT_FALSE(data.icon);

    data.setIcon(icon);
    EXPECT_TRUE(data.data.empty());
    EXPECT_TRUE(data.path.empty());
    EXPECT_EQ(data.icon, icon);
}

TEST(Element, imageCacheEntryReportsFailure) {
    const auto entry = makeShared<Asset::CAssetCacheEntry>("broken-image");
    int        done  = 0;
    entry->m_events.done.listenStatic([&done] { ++done; });

    entry->fail();

    EXPECT_EQ(entry->status(), Asset::CACHE_ENTRY_FAILED);
    EXPECT_EQ(done, 1);
}

TEST(Element, imageRecognizesScalablePaths) {
    SImageImpl image;

    image.data.path = "image.png";
    EXPECT_FALSE(image.scalable());

    image.data.path = "image.svg";
    EXPECT_TRUE(image.scalable());
}

TEST(Element, imageTransitionDefaults) {
    SImageTransition t;
    EXPECT_EQ(t.duration.count(), 0);
    EXPECT_TRUE(t.shader.empty());
}

TEST(Element, imageTransitionWantedOnlyForChangedSourceWithDurationAndDisplay) {
    SImageImpl impl;
    impl.data.path = "a.png";

    SImageData next;
    next.setPath("b.png");
    EXPECT_FALSE(impl.transitionWanted(next)); // duration 0: upstream swap

    next.transition.duration = 200ms;
    EXPECT_FALSE(impl.transitionWanted(next)); // nothing displayed yet

    SImageData same;
    same.setPath("a.png");
    same.transition.duration = 200ms;
    EXPECT_FALSE(impl.transitionWanted(same)); // same source
}

TEST(Element, imageTransitionNotWantedForIdenticalRawData) {
    SImageImpl impl;
    impl.data.data          = {1, 2, 3, 4};
    impl.transition.pending = true;

    SImageData next;
    next.data                = impl.data.data; // same bytes, distinct copy
    next.transition.duration = 200ms;

    EXPECT_FALSE(impl.transitionWanted(next));

    next.data.push_back(5);
    EXPECT_TRUE(impl.transitionWanted(next));
}
