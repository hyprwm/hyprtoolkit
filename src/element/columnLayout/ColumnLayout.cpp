#include "ColumnLayout.hpp"
#include <cmath>

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../window/ToolkitWindow.hpp"

#include "../Element.hpp"

using namespace Hyprtoolkit;

SP<CColumnLayoutElement> CColumnLayoutElement::create(const SColumnLayoutData& data) {
    auto p          = SP<CColumnLayoutElement>(new CColumnLayoutElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CColumnLayoutElement::CColumnLayoutElement(const SColumnLayoutData& data) : IElement(), m_impl(makeUnique<SColumnLayoutImpl>()) {
    m_impl->data = data;
}

void CColumnLayoutElement::paint() {
    ; // no-op
}

SP<CColumnLayoutBuilder> CColumnLayoutElement::rebuild() {
    auto p       = SP<CColumnLayoutBuilder>(new CColumnLayoutBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SColumnLayoutData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CColumnLayoutElement::replaceData(const SColumnLayoutData& data) {
    m_impl->data = data;

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

// FIXME: de-dup with rowlayout?
void CColumnLayoutElement::reposition(const Hyprutils::Math::CBox& sbox, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(sbox);

    const auto& box = impl->position;

    const auto  C = impl->children;

    // position children in this layout.

    size_t              usedY = 0;
    const size_t        MAX_Y = (uint64_t)impl->position.size().y;

    size_t              i = 0;

    std::vector<size_t> heights;
    heights.resize(impl->children.size());

    for (i = 0; i < C.size(); ++i) {
        const auto& child = C.at(i);

        if (MAX_Y <= usedY) {
            // no more space
            child->impl->setFailedPositioning(true);
            continue;
        }

        Vector2D cSize = childSize(child);
        if (cSize == Vector2D{-1, -1})
            cSize = {box.w, 1.F};

        if (usedY + cSize.y > MAX_Y + 2 /* FIXME: WHERE THE FUCK IS THIS LOST??? */) {
            // we exceeded our available space.
            if (!child->minimumSize(box.size())) {
                // doesn't fit: disable
                child->impl->setFailedPositioning(true);
                continue;
            }

            cSize = *child->minimumSize(box.size());

            if (usedY + cSize.y > MAX_Y) {
                // doesn't fit: disable and expand the last if possible
                child->impl->setFailedPositioning(true);
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
        heights.at(i) = cSize.y;
        child->impl->setFailedPositioning(false);
        usedY += cSize.y + m_impl->data.gap;
    }

    if (!C.empty())
        usedY -= m_impl->data.gap;

    // check if any element grows and grow if applicable
    if (usedY < MAX_Y) {
        for (i = 0; i < C.size(); ++i) {
            const auto& child = C.at(i);

            if (!child->impl->growV)
                continue;

            heights.at(i) += MAX_Y - usedY;
            break;
        }
    }

    // widths done: lay out elements
    size_t currentY = 0;

    for (i = 0; i < C.size(); ++i) {
        const auto& child = C.at(i);

        if (child->impl->failedPositioning)
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

        g_positioner->position(child, childBox, Vector2D{box.w, -1.F});

        currentY += childBox.h + m_impl->data.gap;
    }
}

Hyprutils::Math::Vector2D CColumnLayoutElement::size() {
    return impl->position.size();
}

Hyprutils::Math::Vector2D CColumnLayoutElement::childSize(Hyprutils::Memory::CSharedPointer<IElement> child) {
    if (child->preferredSize(impl->position.size()))
        return *child->preferredSize(impl->position.size());
    else if (child->minimumSize(impl->position.size()))
        return *child->minimumSize(impl->position.size());
    return {-1, -1};
}

std::optional<Hyprutils::Math::Vector2D> CColumnLayoutElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    auto calc = m_impl->data.size.calculate(parent);

    if (calc.x != -1 && calc.y != -1)
        return calc;

    Vector2D max;
    for (const auto& child : impl->children) {
        max.x = std::max(childSize(child).x, max.x);
        max.y += childSize(child).y + m_impl->data.gap;
    }

    if (!impl->children.empty())
        max.y -= m_impl->data.gap;

    max.x += impl->margin * 2;
    max.y += impl->margin * 2;

    max.x = std::ceil(max.x);
    max.y = std::ceil(max.y);

    if (calc.x == -1)
        calc.x = max.x;
    if (calc.y == -1)
        calc.y = max.y;

    return calc;
}

std::optional<Hyprutils::Math::Vector2D> CColumnLayoutElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    Vector2D min;
    for (const auto& child : impl->children) {
        min.x = std::max(min.x, childSize(child).x);
    }

    return min;
}

bool CColumnLayoutElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
