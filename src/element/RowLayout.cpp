#include <hyprtoolkit/element/RowLayout.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../core/InternalBackend.hpp"

#include "Element.hpp"

using namespace Hyprtoolkit;

SP<CRowLayoutElement> CRowLayoutElement::create(const SRowLayoutData& data) {
    auto p        = SP<CRowLayoutElement>(new CRowLayoutElement(data));
    p->impl->self = p;
    return p;
}

CRowLayoutElement::CRowLayoutElement(const SRowLayoutData& data) : IElement(), m_data(data) {
    ;
}

void CRowLayoutElement::paint() {
    ; // no-op
}

// FIXME: de-dup with columnlayout?
void CRowLayoutElement::reposition(const Hyprutils::Math::CBox& sbox, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(sbox);

    const auto& box = impl->position;
    const auto  C   = impl->children;

    // position children in this layout.

    size_t              usedX = 0;
    const size_t        MAX_X = (uint64_t)impl->position.size().x;

    size_t              i = 0;

    std::vector<size_t> widths;
    widths.resize(impl->children.size());

    for (i = 0; i < C.size(); ++i) {
        const auto& child = C.at(i);

        if (MAX_X <= usedX) {
            // no more space
            child->impl->failedPositioning = true;
            continue;
        }

        Vector2D cSize = childSize(child);
        if (cSize == Vector2D{-1, -1})
            cSize = {1.F, box.h};

        if (usedX + cSize.x > MAX_X) {
            // we exceeded our available space.
            if (!child->minimumSize(box.size())) {
                // doesn't fit: disable
                child->impl->failedPositioning = true;
                continue;
            }

            cSize = *child->minimumSize(box.size());

            if (usedX + cSize.x > MAX_X) {
                // doesn't fit: disable and expand the last if possible
                child->impl->failedPositioning = true;
                if (i != 0) {
                    // try to expand last child
                    const auto& lastChild = C.at(i - 1);

                    if (lastChild->maximumSize(box.size()) && widths.at(i - 1) + MAX_X - usedX > lastChild->maximumSize(box.size())->x)
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
        widths.at(i)                   = cSize.x;
        child->impl->failedPositioning = false;
        usedX += cSize.x + m_data.gap;
    }

    if (!C.empty())
        usedX -= m_data.gap;

    // check if any element grows and grow if applicable
    if (usedX < MAX_X) {
        for (i = 0; i < C.size(); ++i) {
            const auto& child = C.at(i);

            if (!child->impl->growH)
                continue;

            widths.at(i) += MAX_X - usedX;
            break;
        }
    }

    // widths done: lay out elements
    size_t currentX = 0;

    for (i = 0; i < C.size(); ++i) {
        const auto& child = C.at(i);

        if (child->impl->failedPositioning)
            continue;

        Vector2D   cSize   = childSize(child);
        const auto maxSize = child->maximumSize(box.size());

        bool       EXPANDS_H = box.h > cSize.y;

        if (maxSize) {
            cSize.y   = maxSize->y;
            EXPANDS_H = box.w > cSize.y;
        } else if (EXPANDS_H) {
            cSize.y   = box.h;
            EXPANDS_H = false;
        }

        CBox childBox = CBox{box.x + currentX, EXPANDS_H ? box.y + ((box.h - cSize.y) / 2) : box.y, (double)widths.at(i), cSize.y};

        g_positioner->position(child, childBox, Vector2D{-1.F, box.h});

        currentX += childBox.w + m_data.gap;
    }
}

Hyprutils::Math::Vector2D CRowLayoutElement::childSize(Hyprutils::Memory::CSharedPointer<IElement> child) {
    if (child->preferredSize(impl->position.size()))
        return *child->preferredSize(impl->position.size());
    else if (child->minimumSize(impl->position.size()))
        return *child->minimumSize(impl->position.size());
    return {-1, -1};
}

Hyprutils::Math::Vector2D CRowLayoutElement::size() {
    return impl->position.size();
}

std::optional<Hyprutils::Math::Vector2D> CRowLayoutElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    auto calc = m_data.size.calculate(parent);

    if (calc.x != -1 && calc.y != -1)
        return calc;

    Vector2D max;
    for (const auto& child : impl->children) {
        max.x += childSize(child).x;
        max.y = std::max(max.y, childSize(child).y);
    }

    if (calc.x == -1)
        calc.x = max.x;
    if (calc.y == -1)
        calc.y = max.y;

    return calc;
}

std::optional<Hyprutils::Math::Vector2D> CRowLayoutElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    Vector2D min;
    for (const auto& child : impl->children) {
        min.y = std::max(min.y, childSize(child).y);
    }

    return min;
}
