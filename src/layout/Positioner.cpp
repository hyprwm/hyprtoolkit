#include "Positioner.hpp"

#include "../element/Element.hpp"
#include "../window/ToolkitWindow.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

void CPositioner::position(SP<IElement> element, const CBox& box, const Hyprutils::Math::Vector2D& maxSize) {

    initElementIfNeeded(element);

    // damage old box
    if (element->impl->window)
        element->impl->window->damage(element->impl->positionerData->baseBox);

    element->impl->positionerData->baseBox = box;
    element->reposition(box, maxSize);

    if (element->impl->window)
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

        if (c->impl->positionMode == IElement::HT_POSITION_ABSOLUTE) {

            // apply position first, then centering

            if (c->impl->positionFlags & IElement::HT_POSITION_FLAG_LEFT)
                ;
            else if (c->impl->positionFlags & IElement::HT_POSITION_FLAG_RIGHT) {
                if (BOX.size().x > itemBox.size().x)
                    itemBox.translate({(BOX.size() - itemBox.size()).x, 0.F});
            }
            if (c->impl->positionFlags & IElement::HT_POSITION_FLAG_TOP)
                ;
            else if (c->impl->positionFlags & IElement::HT_POSITION_FLAG_BOTTOM) {
                if (BOX.size().y > itemBox.size().y)
                    itemBox.translate({0.F, (BOX.size() - itemBox.size()).y});
            }

            if (c->impl->positionFlags & IElement::HT_POSITION_FLAG_HCENTER)
                itemBox.translate(Vector2D{((BOX.size() - itemBox.size()) / 2.F).x, 0.F});
            if (c->impl->positionFlags & IElement::HT_POSITION_FLAG_VCENTER)
                itemBox.translate(Vector2D{0.F, ((BOX.size() - itemBox.size()) / 2.F).y});

            itemBox.translate(c->impl->absoluteOffset);
        }

        if (data.growX)
            itemBox.w = 99999999;
        if (data.growY)
            itemBox.h = 99999999;

        position(c, itemBox);
    }
}

void CPositioner::repositionNeeded(SP<IElement> element, bool force) {
    if (!element->impl->parent) {
        // root el likely, check
        if (!element->impl->window || element != element->impl->window->m_rootElement) {
            initElementIfNeeded(element);
            position(element, element->impl->positionerData->baseBox);
            return;
        }

        // otherwise, max box
        position(element, {{}, (element->impl->window->pixelSize() / element->impl->window->scale()).round()});
        return;
    }

    if (!element->impl->parent->impl->positionerData || element->impl->parent->impl->positionerData->baseBox.empty()) {
        if (force) {
            initElementIfNeeded(element);
            position(element, CBox{Vector2D{}, element->preferredSize(Vector2D{}).value_or({})});
        } else if (element->impl->window) // full reflow needed
            element->impl->window->scheduleReposition(element->impl->window->m_rootElement);
        return;
    }

    position(element->impl->parent.lock(), element->impl->parent->impl->positionerData->baseBox);
}

void CPositioner::initElementIfNeeded(SP<IElement> el) {
    if (el->impl->positionerData)
        return;

    el->impl->positionerData = makeUnique<Hyprtoolkit::SPositionerData>();
}
