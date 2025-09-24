#include "Positioner.hpp"

#include "../element/Element.hpp"
#include "../window/ToolkitWindow.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

struct Hyprtoolkit::SPositionerData {
    CBox baseBox;
};

void CPositioner::position(SP<IElement> element, const CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    if (!element->impl->window)
        return;

    initElementIfNeeded(element);

    element->impl->positionerData->baseBox = box;
    element->reposition(box, maxSize);

    element->impl->window->damage(box);
}

void CPositioner::positionChildren(SP<IElement> element) {
    const auto C   = element->impl->children;
    const auto BOX = element->impl->position;

    // position children according to how they wanna be positioned

    for (const auto& c : C) {
        auto itemSize = c->preferredSize(BOX.size());

        if (!itemSize) {
            // no size to base off of, just position
            position(c, BOX);
            continue;
        }

        // it has a size, let's see what it wants.
        CBox itemBox = {BOX.pos(), *itemSize};
        if (c->impl->positionMode == IElement::HT_POSITION_ABSOLUTE)
            itemBox.translate(c->impl->absoluteOffset);
        else if (c->impl->positionMode == IElement::HT_POSITION_CENTER)
            itemBox.translate((BOX.size() - itemBox.size()) / 2.F);

        position(c, itemBox);
    }
}

void CPositioner::repositionNeeded(SP<IElement> element) {
    if (!element->impl->parent || !element->impl->parent->impl->positionerData || element->impl->parent->impl->positionerData->baseBox.empty())
        return;

    position(element->impl->parent.lock(), element->impl->parent->impl->positionerData->baseBox);
}

void CPositioner::initElementIfNeeded(SP<IElement> el) {
    if (el->impl->positionerData)
        return;

    el->impl->positionerData = makeUnique<Hyprtoolkit::SPositionerData>();
}
