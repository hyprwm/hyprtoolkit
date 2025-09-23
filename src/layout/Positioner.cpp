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

    CBox newBox = box.copy();

    if (element->impl->positionMode == IElement::HT_POSITION_ABSOLUTE)
        newBox.translate(element->impl->absoluteOffset);
    else if (element->impl->positionMode == IElement::HT_POSITION_CENTER)
        newBox.translate((box.size() - element->size()) / 2.F);

    element->impl->positionerData->baseBox = newBox;
    element->reposition(newBox, maxSize);

    element->impl->window->damage(newBox);
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
