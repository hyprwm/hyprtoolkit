#include "Checkbox.hpp"

using namespace Hyprtoolkit;

SP<CCheckboxBuilder> CCheckboxBuilder::begin() {
    SP<CCheckboxBuilder> p = SP<CCheckboxBuilder>(new CCheckboxBuilder());
    p->m_data              = makeUnique<SCheckboxData>();
    p->m_self              = p;
    return p;
}

SP<CCheckboxBuilder> CCheckboxBuilder::label(std::string&& s) {
    m_data->label = std::move(s);
    return m_self.lock();
}

SP<CCheckboxBuilder> CCheckboxBuilder::onToggled(std::function<void(Hyprutils::Memory::CSharedPointer<CCheckboxElement>, bool)>&& f) {
    m_data->onToggled = std::move(f);
    return m_self.lock();
}

SP<CCheckboxBuilder> CCheckboxBuilder::toggled(bool x) {
    m_data->toggled = x;
    return m_self.lock();
}

SP<CCheckboxBuilder> CCheckboxBuilder::fill(bool x) {
    m_data->fill = x;
    return m_self.lock();
}

SP<CCheckboxBuilder> CCheckboxBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CCheckboxElement> CCheckboxBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CCheckboxElement::create(*m_data);
}