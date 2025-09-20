#include <hyprtoolkit/element/ColumnLayout.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../core/InternalBackend.hpp"

#include "Element.hpp"

using namespace Hyprtoolkit;

SP<CColumnLayoutElement> CColumnLayoutElement::create(const SColumnLayoutData& data) {
    auto p = SP<CColumnLayoutElement>(new CColumnLayoutElement(data));
    p->m_self = p;
    return p;
}

CColumnLayoutElement::CColumnLayoutElement(const SColumnLayoutData& data) : IElement(), m_data(data) {
    ;
}

void CColumnLayoutElement::paint() {
    ; // no-op
}

// FIXME: de-dup with rowlayout?
void CColumnLayoutElement::reposition(const Hyprutils::Math::CBox& box) {
    m_position = box;

    const auto C = m_elementData->children;

    // position children in this layout.

    size_t              usedY = 0;
    const size_t        MAX_Y = (uint64_t)m_position.size().y;

    size_t              i = 0;

    std::vector<size_t> heights;
    heights.resize(m_elementData->children.size());

    for (i = 0; i < C.size(); ++i) {
        const auto& child = C.at(i);

        if (MAX_Y <= usedY) {
            // no more space
            child->m_elementData->failedPositioning = true;
            continue;
        }

        Vector2D cSize = childSize(child);
        if (cSize == Vector2D{-1, -1}) {
            g_logger->log(HT_LOG_ERROR, "child {:x} of ColumnLayout has no preferred or minimum size: positioning will fail", (uintptr_t)child.get());
            child->m_elementData->failedPositioning = true;
            continue;
        }

        if (usedY + cSize.y > MAX_Y) {
            // we exceeded our available space.
            if (!child->minimumSize(box.size())) {
                // doesn't fit: disable
                child->m_elementData->failedPositioning = true;
                continue;
            }

            cSize = *child->minimumSize(box.size());

            if (usedY + cSize.y > MAX_Y) {
                // doesn't fit: disable and expand the last if possible
                child->m_elementData->failedPositioning = true;
                if (i != 0) {
                    // try to expand last child
                    const auto& lastChild = C.at(i - 1);

                    if (lastChild->maximumSize(box.size()) && heights.at(i - 1) + MAX_Y - usedY > lastChild->maximumSize(box.size())->y)
                        continue; // too bad, we'll have a gap

                    heights.at(i - 1) += MAX_Y - usedY;
                }
                continue;
            }

            // squeeze the last element in
            heights.at(i) = MAX_Y - usedY;
            continue;
        }

        // can fit: use preferred
        heights.at(i)                           = cSize.y;
        child->m_elementData->failedPositioning = false;
        usedY += cSize.y;
    }

    // check if any element grows and grow if applicable
    if (usedY < MAX_Y) {
        for (i = 0; i < C.size(); ++i) {
            const auto& child = C.at(i);

            if (!child->m_elementData->grow)
                continue;

            heights.at(i) += MAX_Y - usedY;
            break;
        }
    }

    // widths done: lay out elements
    size_t currentY = 0;

    for (i = 0; i < C.size(); ++i) {
        const auto& child = C.at(i);

        if (child->m_elementData->failedPositioning)
            continue;

        Vector2D   cSize   = childSize(child);
        const auto maxSize = child->maximumSize(box.size());

        bool       EXPANDS_W = box.w > cSize.x;

        if (maxSize) {
            cSize.x   = maxSize->x;
            EXPANDS_W = box.w > cSize.x;
        } else if (EXPANDS_W) {
            cSize.x   = box.w;
            EXPANDS_W = false;
        }

        CBox childBox = CBox{EXPANDS_W ? box.x + ((box.w - cSize.x) / 2) : box.x, box.y + currentY, cSize.x, (double)heights.at(i)};

        g_positioner->position(child, childBox);

        currentY += childBox.h;
    }
}

Hyprutils::Math::Vector2D CColumnLayoutElement::size() {
    return m_position.size();
}

Hyprutils::Math::Vector2D CColumnLayoutElement::childSize(Hyprutils::Memory::CSharedPointer<IElement> child) {
    if (child->preferredSize(m_position.size()))
        return *child->preferredSize(m_position.size());
    else if (child->minimumSize(m_position.size()))
        return *child->minimumSize(m_position.size());
    return {-1, -1};
}

std::optional<Hyprutils::Math::Vector2D> CColumnLayoutElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    auto calc = m_data.size.calculate(parent);

    if (calc.x != -1 && calc.y != -1)
        return calc;

    Vector2D max;
    for (const auto& child : m_elementData->children) {
        max.x = std::max(childSize(child).x, max.x);
        max.y += childSize(child).y;
    }

    if (calc.x == -1)
        calc.x = max.x;
    if (calc.y == -1)
        calc.y = max.y;

    return calc;
}

std::optional<Hyprutils::Math::Vector2D> CColumnLayoutElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    Vector2D min;
    for (const auto& child : m_elementData->children) {
        min.x = std::max(min.x, childSize(child).x);
    }

    return min;
}
