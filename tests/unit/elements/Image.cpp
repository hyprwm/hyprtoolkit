#include <gtest/gtest.h>

#include <element/image/Image.hpp>
#include <hyprgraphics/resource/resources/ImageResource.hpp>

using namespace Hyprtoolkit;

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
