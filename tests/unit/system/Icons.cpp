#include <gtest/gtest.h>
#include <system/Icons.hpp>
#include <hyprgraphics/resource/resources/ImageResource.hpp>
#include <hyprutils/utils/ScopeGuard.hpp>
#include <fstream>
#include <cstdlib>

using namespace Hyprtoolkit;

class CIconLookupTest : public testing::Test {
  protected:
    std::filesystem::path m_directory;

    void                  SetUp() override {
        auto       pattern = (std::filesystem::temp_directory_path() / "hyprtoolkit-icons-XXXXXX").string();
        const auto TEMP    = mkdtemp(pattern.data());
        ASSERT_NE(TEMP, nullptr);
        m_directory = TEMP;
    }
    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(m_directory, ec);
    }
    std::filesystem::path svg(const std::string& name, const std::string& color = "red") {
        const auto PATH = m_directory / name;
        std::filesystem::create_directories(PATH.parent_path());
        std::ofstream(PATH) << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\"><rect width=\"32\" height=\"32\" fill=\"" << color << "\"/></svg>";
        return PATH;
    }
    std::filesystem::path png(const std::string& name, int size) {
        const auto PATH = m_directory / name;
        std::filesystem::create_directories(PATH.parent_path());
        auto                          surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
        Hyprutils::Utils::CScopeGuard cleanup([surface] { cairo_surface_destroy(surface); });
        EXPECT_EQ(cairo_surface_write_to_png(surface, PATH.c_str()), CAIRO_STATUS_SUCCESS);
        return PATH;
    }
};

TEST_F(CIconLookupTest, selectsResolutionAndKeepsThemePriority) {
    const auto SMALL = png("theme/16/app.png", 16);
    const auto LARGE = png("theme/64/app.png", 64);
    svg("parent/scalable/app.svg");
    const std::vector<SIconDirectory> DIRS{{SMALL.parent_path(), "theme"}, {LARGE.parent_path(), "theme"}, {m_directory / "parent/scalable", "parent"}};
    const auto                        CANDIDATES = findIconCandidates(DIRS, "app");
    ASSERT_EQ(CANDIDATES.size(), 2U);
    EXPECT_EQ(selectIconPath(CANDIDATES, 16), SMALL);
    EXPECT_EQ(selectIconPath(CANDIDATES, 37), LARGE);
    EXPECT_EQ(selectIconPath(CANDIDATES, 74), LARGE);
    EXPECT_EQ(selectIconPath(CANDIDATES, 16), SMALL);
}

TEST_F(CIconLookupTest, scalableAssetWinsWithinTheme) {
    const auto SMALL      = png("16/app.png", 16);
    const auto VECTOR     = svg("scalable/app.svg");
    const auto CANDIDATES = findIconCandidates({{SMALL.parent_path(), "theme"}, {VECTOR.parent_path(), "theme"}}, "app");
    for (int size : {16, 37, 74}) {
        EXPECT_EQ(selectIconPath(CANDIDATES, size), VECTOR);
        Hyprgraphics::CImageResource resource(VECTOR.string(), {double(size), double(size)});
        resource.render();
        ASSERT_TRUE(resource.m_asset.cairoSurface);
        EXPECT_EQ(resource.m_asset.pixelSize.x, size);
        EXPECT_EQ(resource.m_asset.pixelSize.y, size);
    }
}

TEST_F(CIconLookupTest, absoluteMissingAndInvalidArtwork) {
    const auto VALID = svg("valid.svg");
    EXPECT_EQ(selectIconPath(findIconCandidates({}, VALID.string()), 37), VALID);
    EXPECT_TRUE(findIconCandidates({}, (m_directory / "absent.svg").string()).empty());
    const auto BROKEN = m_directory / "invalid.svg";
    std::ofstream(BROKEN) << "not an SVG";
    EXPECT_TRUE(findIconCandidates({}, BROKEN.string()).empty());
    EXPECT_TRUE(findIconCandidates({{m_directory, "theme"}}, "../valid").empty());
}

TEST_F(CIconLookupTest, invalidThemeAssetFallsBackToParent) {
    std::filesystem::create_directories(m_directory / "theme");
    std::ofstream(m_directory / "theme/app.svg") << "not an SVG";
    const auto VALID      = svg("parent/app.svg");
    const auto CANDIDATES = findIconCandidates({{m_directory / "theme", "theme"}, {VALID.parent_path(), "parent"}}, "app");
    EXPECT_EQ(selectIconPath(CANDIDATES, 37), VALID);
}

TEST_F(CIconLookupTest, differentIconsAndSizesKeepTheirIdentity) {
    const auto             RED  = svg("red.svg");
    const auto             BLUE = svg("blue.svg", "blue");
    CSystemIconDescription red, blue;
    red.m_candidates  = findIconCandidates({}, RED.string());
    blue.m_candidates = findIconCandidates({}, BLUE.string());
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(red.pathForSize(i % 2 ? 37 : 74), RED);
        EXPECT_EQ(blue.pathForSize(i % 2 ? 74 : 37), BLUE);
    }
}
