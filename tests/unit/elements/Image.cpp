#include <gtest/gtest.h>
#include "../tricks/Tricks.hpp"
#include <system/Icons.hpp>
#include <element/Element.hpp>

#include <element/image/Image.hpp>
#include <hyprgraphics/resource/resources/ImageResource.hpp>
#include <hyprtoolkit/system/Icons.hpp>

using namespace Hyprtoolkit;

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

// Reusing rows must not reuse another application's texture, and changing the
// physical size must select a different bitmap when one is available.
TEST(Element, iconCacheTracksArtworkAndDisplayScale) {
    Tests::Tricks::createBackendSupport();
    const auto ICON    = makeShared<CSystemIconDescription>();
    ICON->m_candidates = {{"small.png", 16, false}, {"large.png", 64, false}};
    SImageImpl image;
    image.data.icon       = ICON;
    const auto IMAGE      = CImageBuilder::begin()->icon(ICON)->commence();
    image.self            = IMAGE;
    IMAGE->impl->position = {0, 0, 16, 16};
    const auto SMALL      = image.getCacheString();
    EXPECT_NE(SMALL.find("small.png"), std::string::npos);
    image.lastScale  = 2.F;
    const auto LARGE = image.getCacheString();
    EXPECT_NE(LARGE.find("large.png"), std::string::npos);
    EXPECT_NE(SMALL, LARGE);
    image.lastScale = 1.F;
    EXPECT_EQ(image.getCacheString(), SMALL);
    ICON->m_candidates = {{"other.png", 16, false}};
    EXPECT_NE(image.getCacheString(), SMALL);
}
