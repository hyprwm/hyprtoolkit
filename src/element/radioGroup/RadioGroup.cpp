#include <hyprtoolkit/element/RadioGroup.hpp>

#include "../checkbox/Checkbox.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

SP<CRadioGroup> CRadioGroup::create() {
    auto p    = SP<CRadioGroup>(new CRadioGroup());
    p->m_self = p;
    return p;
}

void CRadioGroup::add(SP<CCheckboxElement> radio) {
    if (!radio)
        return;

    // wrap any existing onToggled so the user's callback still fires, and
    // tack on the exclusivity + group-level notification afterwards.
    auto userCb = std::move(radio->m_impl->data.onToggled);

    radio->m_impl->data.onToggled = [userCb = std::move(userCb), groupWp = m_self](SP<CCheckboxElement> elem, bool on) {
        if (userCb)
            userCb(elem, on);

        auto group = groupWp.lock();
        if (!group || !on)
            return;

        for (const auto& wp : group->m_radios) {
            auto other = wp.lock();
            if (!other || other == elem)
                continue;
            other->setState(false);
        }

        if (group->m_onSelected)
            group->m_onSelected(elem);
    };

    m_radios.emplace_back(radio);
}

SP<CCheckboxElement> CRadioGroup::selected() {
    for (const auto& wp : m_radios) {
        auto p = wp.lock();
        if (p && p->state())
            return p;
    }
    return nullptr;
}

void CRadioGroup::setSelected(SP<CCheckboxElement> radio) {
    for (const auto& wp : m_radios) {
        auto p = wp.lock();
        if (!p)
            continue;
        p->setState(p == radio);
    }

    if (m_onSelected && radio)
        m_onSelected(radio);
}

void CRadioGroup::onSelected(std::function<void(SP<CCheckboxElement>)>&& fn) {
    m_onSelected = std::move(fn);
}
