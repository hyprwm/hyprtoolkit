#include "Positioner.hpp"

#include "../element/Element.hpp"
#include "../window/ToolkitWindow.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

void CPositioner::position(SP<IElement> element, const CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    if (!element->impl->window)
        return;

    CBox newBox = box.copy();

    if (element->impl->positionMode == IElement::HT_POSITION_ABSOLUTE)
        newBox.translate(element->impl->absoluteOffset);
    else if (element->impl->positionMode == IElement::HT_POSITION_CENTER)
        newBox.translate((box.size() - element->size()) / 2.F);

    element->reposition(newBox, maxSize);

    element->impl->window->damage(newBox);
}

void CPositioner::repositionNeeded(SP<IElement> element) {
    if (!element->impl->parent)
        return;

    position(element->impl->parent.lock(), element->impl->parent->impl->position);
}
