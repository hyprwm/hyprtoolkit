#include "Spinbox.hpp"

using namespace Hyprtoolkit;

SP<CSpinboxBuilder> CSpinboxBuilder::begin() {
    SP<CSpinboxBuilder> p = SP<CSpinboxBuilder>(new CSpinboxBuilder());
    p->m_data             = makeUnique<SSpinboxData>();
    p->m_self             = p;
    return p;
}

SP<CSpinboxBuilder> CSpinboxBuilder::label(std::string&& x) {
    m_data->label = std::move(x);
    return m_self.lock();
}

SP<CSpinboxBuilder> CSpinboxBuilder::items(std::vector<std::string>&& x) {
    m_data->items = std::move(x);
    return m_self.lock();
}

SP<CSpinboxBuilder> CSpinboxBuilder::currentItem(size_t x) {
    m_data->currentItem = x;
    return m_self.lock();
}

SP<CSpinboxBuilder> CSpinboxBuilder::fill(bool x) {
    m_data->fill = x;
    return m_self.lock();
}

SP<CSpinboxBuilder> CSpinboxBuilder::onChanged(std::function<void(Hyprutils::Memory::CSharedPointer<CSpinboxElement>, size_t)>&& x) {
    m_data->onChanged = std::move(x);
    return m_self.lock();
}

SP<CSpinboxBuilder> CSpinboxBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CSpinboxElement> CSpinboxBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CSpinboxElement::create(*m_data);
}