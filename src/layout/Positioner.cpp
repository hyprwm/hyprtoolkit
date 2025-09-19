#include "Positioner.hpp"

#include <hyprtoolkit/element/Element.hpp>

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

void CPositioner::position(SP<IElement> element, const CBox& box) {
    if (element->m_positionMode == IElement::HT_POSITION_ABSOLUTE)
        element->reposition(box.copy().translate(element->m_absoluteOffset));
    else if (element->m_positionMode == IElement::HT_POSITION_CENTER)
        element->reposition(box.copy().translate((box.size() - element->size()) / 2.F));
    else
        element->reposition(box);
}