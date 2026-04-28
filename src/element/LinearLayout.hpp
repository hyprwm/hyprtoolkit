#pragma once

#include "../helpers/Memory.hpp"
#include "../layout/Positioner.hpp"
#include "Element.hpp"

#include <hyprutils/math/Box.hpp>
#include <hyprutils/math/Vector2D.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace Hyprtoolkit::LinearLayout {
    // axis-agnostic reposition for row / column layouts. Horizontal=true means
    // children flow along x (row layout); false means they flow along y (column).
    // childSize must return the layout's preferred-or-minimum size for a child.
    template <bool Horizontal>
    inline void reposition(IElement* self, const Hyprutils::Math::CBox& box, const std::vector<SP<IElement>>& C, float gap, const std::function<Hyprutils::Math::Vector2D(SP<IElement>)>& childSize) {
        using Vector2D = Hyprutils::Math::Vector2D;
        using CBox     = Hyprutils::Math::CBox;

        auto axisPrimary = [](const Vector2D& v) -> double { return Horizontal ? v.x : v.y; };
        auto axisCross   = [](const Vector2D& v) -> double { return Horizontal ? v.y : v.x; };
        auto boxPrimary  = [](const CBox& b) -> double { return Horizontal ? b.w : b.h; };
        auto grows       = [](const SP<IElement>& e) -> bool { return Horizontal ? e->impl->growH : e->impl->growV; };

        const double        MAX = (double)(uint64_t)boxPrimary(box); // floor to match upstream behavior
        double              used = 0;

        std::vector<size_t> sizes;
        sizes.resize(C.size());

        size_t i = 0;
        for (i = 0; i < C.size(); ++i) {
            const auto& child = C.at(i);

            Vector2D    cSize = childSize(child);
            if (cSize == Vector2D{-1, -1})
                cSize = Horizontal ? Vector2D{1.F, box.h} : Vector2D{box.w, 1.F};

            if (used + axisPrimary(cSize) > MAX + 1) {
                // we exceeded our available space.
                if (!child->minimumSize(box.size())) {
                    // doesn't fit: disable
                    child->impl->setFailedPositioning(true);
                    continue;
                }

                cSize = *child->minimumSize(box.size());

                if (used + axisPrimary(cSize) > MAX + 1) {
                    // doesn't fit: try to shrink any previous element if it allows to do so
                    // FIXME: if we resize and fail to fit, it will mess up the layout a bit
                    float needs = (used + axisPrimary(cSize)) - (MAX + 1);
                    for (int j = (int)i - 1; j >= 0; --j) {
                        const auto& prevChild = C.at(j);
                        const auto  MIN       = prevChild->minimumSize(box.size());
                        if (!MIN.has_value()) {
                            // can shrink to zero. give all of it, continue if not enough.
                            if (needs > sizes.at(j)) {
                                needs -= sizes.at(j);
                                sizes.at(j) = 0;
                                continue;
                            }

                            sizes.at(j) -= needs;
                            needs = 0;
                            break;
                        } else if (axisPrimary(*MIN) < sizes.at(j)) {
                            // can shrink down to MIN. give all available slack, continue if not enough.
                            const double slack = sizes.at(j) - axisPrimary(*MIN);
                            if (needs > slack) {
                                sizes.at(j) -= slack;
                                needs -= slack;
                                continue;
                            }

                            sizes.at(j) -= needs;
                            needs = 0;
                            break;
                        }
                    }

                    if (needs > 0) {
                        // doesn't fit: disable and expand the last if possible
                        child->impl->setFailedPositioning(true);
                        if (i != 0) {
                            const auto& lastChild = C.at(i - 1);

                            if (lastChild->maximumSize(box.size()) && sizes.at(i - 1) + MAX - used > axisPrimary(*lastChild->maximumSize(box.size())))
                                continue; // too bad, we'll have a gap

                            sizes.at(i - 1) += MAX - used;
                        }
                        continue;
                    } else {
                        child->impl->setFailedPositioning(false);
                        sizes.at(i) = axisPrimary(cSize);

                        // recalc used, we changed prior sizes
                        used = 0;
                        for (const auto& s : sizes)
                            used += s + gap;

                        continue;
                    }
                }

                // squeeze the last element in
                sizes.at(i) = MAX - used;
                continue;
            }

            // can fit: use preferred
            sizes.at(i) = axisPrimary(cSize);
            child->impl->setFailedPositioning(false);
            used += axisPrimary(cSize) + gap;
        }

        if (!C.empty())
            used -= gap;

        // grow the first element marked as growing on this axis to fill remaining space
        if (used < MAX) {
            for (i = 0; i < C.size(); ++i) {
                if (!grows(C.at(i)))
                    continue;
                sizes.at(i) += MAX - used;
                break;
            }
        }

        // sizes resolved: place each child
        size_t cursor = 0;
        for (i = 0; i < C.size(); ++i) {
            const auto& child = C.at(i);
            if (child->impl->failedPositioning)
                continue;

            Vector2D cSize = childSize(child);

            CBox childBox;
            if constexpr (Horizontal) {
                cSize.y       = std::clamp(cSize.y, 0.0, box.h);
                childBox      = CBox{box.x + (double)cursor, box.y + ((box.h - cSize.y) / 2), (double)sizes.at(i), cSize.y};
                g_positioner->position(child, childBox, Vector2D{childBox.w + (MAX - used), box.h});
            } else {
                cSize.x       = std::clamp(cSize.x, 0.0, box.w);
                childBox      = CBox{box.x + ((box.w - cSize.x) / 2), box.y + (double)cursor, cSize.x, (double)sizes.at(i)};
                g_positioner->position(child, childBox, Vector2D{box.w, childBox.h + (MAX - used)});
            }

            cursor += (size_t)((Horizontal ? childBox.w : childBox.h) + gap);
        }
    }
}
