#include "Positioner.hpp"

#include "../element/Element.hpp"
#include "../window/ToolkitWindow.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

void CPositioner::position(SP<IElement> element, const CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    if (!element->impl->window)
        return;

    initElementIfNeeded(element);

    // damage old box
    element->impl->window->damage(element->impl->positionerData->baseBox);

    element->impl->positionerData->baseBox = box;
    element->reposition(box, maxSize);

    element->impl->window->damage(box);
}

void CPositioner::positionChildren(SP<IElement> element, const SRepositionData& data) {
    const auto C   = element->impl->children;
    const auto BOX = element->impl->position.copy().translate(data.offset);

    // position children according to how they wanna be positioned

    for (const auto& c : C) {
        auto itemSize = c->preferredSize(BOX.size());

        if (!itemSize) {
            // no size to base off of, just position
            CBox itemBox = BOX;
            if (data.growX)
                itemBox.w = 99999999;
            if (data.growY)
                itemBox.h = 99999999;

            position(c, itemBox);
            continue;
        }

        // it has a size, let's see what it wants.
        CBox itemBox = {BOX.pos(), *itemSize};
        if (c->impl->positionMode == IElement::HT_POSITION_CENTER)
            itemBox.translate((BOX.size() - itemBox.size()) / 2.F);
        else if (c->impl->positionMode == IElement::HT_POSITION_HCENTER)
            itemBox.translate(Vector2D{((BOX.size() - itemBox.size()) / 2.F).x, 0.F});
        else if (c->impl->positionMode == IElement::HT_POSITION_VCENTER || c->impl->positionMode == IElement::HT_POSITION_LEFT ||
                 c->impl->positionMode == IElement::HT_POSITION_RIGHT)
            itemBox.translate(Vector2D{0.F, ((BOX.size() - itemBox.size()) / 2.F).y});

        if (c->impl->positionMode == IElement::HT_POSITION_RIGHT && BOX.w > itemBox.w)
            itemBox.translate(Vector2D{(BOX.size() - itemBox.size()).x, 0.F});

        itemBox.translate(c->impl->absoluteOffset);

        if (data.growX)
            itemBox.w = 99999999;
        if (data.growY)
            itemBox.h = 99999999;

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
