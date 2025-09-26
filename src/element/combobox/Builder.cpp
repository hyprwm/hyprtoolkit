#include "Combobox.hpp"

using namespace Hyprtoolkit;

SP<CComboboxBuilder> CComboboxBuilder::begin() {
    SP<CComboboxBuilder> p = SP<CComboboxBuilder>(new CComboboxBuilder());
    p->m_data              = makeUnique<SComboboxData>();
    p->m_self              = p;
    return p;
}

SP<CComboboxBuilder> CComboboxBuilder::label(std::string&& x) {
    m_data->label = std::move(x);
    return m_self.lock();
}

SP<CComboboxBuilder> CComboboxBuilder::items(std::vector<std::string>&& x) {
    m_data->items = std::move(x);
    return m_self.lock();
}

SP<CComboboxBuilder> CComboboxBuilder::currentItem(size_t x) {
    m_data->currentItem = x;
    return m_self.lock();
}

SP<CComboboxBuilder> CComboboxBuilder::fill(bool x) {
    m_data->fill = x;
    return m_self.lock();
}

SP<CComboboxBuilder> CComboboxBuilder::onChanged(std::function<void(Hyprutils::Memory::CSharedPointer<CComboboxElement>, size_t)>&& x) {
    m_data->onChanged = std::move(x);
    return m_self.lock();
}

SP<CComboboxBuilder> CComboboxBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CComboboxElement> CComboboxBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CComboboxElement::create(*m_data);
}