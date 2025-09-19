#include <hyprtoolkit/element/RowLayout.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../core/InternalBackend.hpp"

using namespace Hyprtoolkit;

void CRowLayoutElement::paint() {
    ; // no-op
}

// FIXME: de-dup with columnlayout?
void CRowLayoutElement::reposition(const Hyprutils::Math::CBox& box) {
    m_position = box;

    const auto C = m_children;

    // position children in this layout.

    size_t              usedX = 0;
    const size_t        MAX_X = (uint64_t)m_position.size().x;

    size_t              i = 0;

    std::vector<size_t> widths;
    widths.resize(m_children.size());

    for (i = 0; i < C.size(); ++i) {
        const auto& child = C.at(i);

        if (MAX_X <= usedX) {
            // no more space
            child->m_failedPositioning = true;
            continue;
        }

        Vector2D cSize = childSize(child);
        if (cSize == Vector2D{-1, -1}) {
            g_logger->log(HT_LOG_ERROR, "child {:x} of RowLayout has no preferred or minimum size: positioning will fail", (uintptr_t)child.get());
            child->m_failedPositioning = true;
            continue;
        }

        if (usedX + cSize.x > MAX_X) {
            // we exceeded our available space.
            if (!child->minimumSize()) {
                // doesn't fit: disable
                child->m_failedPositioning = true;
                continue;
            }

            cSize = *child->minimumSize();

            if (usedX + cSize.x > MAX_X) {
                // doesn't fit: disable and expand the last if possible
                child->m_failedPositioning = true;
                if (i != 0) {
                    // try to expand last child
                    const auto& lastChild = C.at(i - 1);

                    if (lastChild->maximumSize() && widths.at(i - 1) + MAX_X - usedX > lastChild->maximumSize()->x)
                        continue; // too bad, we'll have a gap

                    widths.at(i - 1) += MAX_X - usedX;
                }
                continue;
            }

            // squeeze the last element in
            widths.at(i) = MAX_X - usedX;
            continue;
        }

        // can fit: use preferred
        widths.at(i)               = cSize.x;
        child->m_failedPositioning = false;
        usedX += cSize.x;
    }

    // check if any element grows and grow if applicable
    if (usedX < MAX_X) {
        for (i = 0; i < C.size(); ++i) {
            const auto& child = C.at(i);

            if (!child->m_grow)
                continue;

            widths.at(i) += MAX_X - usedX;
            break;
        }
    }

    // widths done: lay out elements
    size_t currentX = 0;

    for (i = 0; i < C.size(); ++i) {
        const auto& child = C.at(i);

        if (child->m_failedPositioning)
            continue;

        Vector2D   cSize   = childSize(child);
        const auto maxSize = child->maximumSize();

        bool       EXPANDS_H = box.h > cSize.y;

        if (maxSize) {
            cSize.y   = maxSize->y;
            EXPANDS_H = box.w > cSize.y;
        } else if (EXPANDS_H) {
            cSize.y   = box.h;
            EXPANDS_H = false;
        }

        CBox childBox = CBox{box.x + currentX, EXPANDS_H ? box.y + ((box.h - cSize.y) / 2) : box.y, (double)widths.at(i), cSize.y};

        g_positioner->position(child, childBox);

        currentX += childBox.w;
    }
}

Hyprutils::Math::Vector2D CRowLayoutElement::childSize(Hyprutils::Memory::CSharedPointer<IElement> child) {
    if (child->preferredSize())
        return *child->preferredSize();
    else if (child->minimumSize())
        return *child->minimumSize();
    return {-1, -1};
}

Hyprutils::Math::Vector2D CRowLayoutElement::size() {
    return m_position.size();
}

std::optional<Hyprutils::Math::Vector2D> CRowLayoutElement::preferredSize() {
    Vector2D max;
    for (const auto& child : m_children) {
        max.x += childSize(child).x;
        max.y = std::max(max.y, childSize(child).y);
    }

    return max;
}

std::optional<Hyprutils::Math::Vector2D> CRowLayoutElement::minimumSize() {
    Vector2D min;
    for (const auto& child : m_children) {
        min.y = std::max(min.y, childSize(child).y);
    }

    return min;
}
