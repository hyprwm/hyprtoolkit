#include <gtest/gtest.h>

#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/ProgressBar.hpp>

#include <layout/Positioner.hpp>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

TEST(Element, progressBarRebuildSizesIndeterminatePulse) {
    Tests::Tricks::createBackendSupport();

    const CBox area     = {{}, {200, 20}};
    const auto root     = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, area.size()})->commence();
    const auto progress = CProgressBarBuilder::begin()->value(0.8F)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->commence();
    root->addChild(progress);

    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    const auto foreground = progress->impl->children.at(0)->impl->children.at(0);
    EXPECT_NEAR(foreground->impl->position.w, 160.F, 0.1F);

    progress->rebuild()->indeterminate(true)->commence();
    g_positioner->position(root, area);
    g_positioner->positionChildren(root);
    EXPECT_NEAR(foreground->impl->position.w, 60.F, 0.1F);
}
