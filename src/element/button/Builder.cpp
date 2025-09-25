#include "Button.hpp"

using namespace Hyprtoolkit;

SP<CButtonBuilder> CButtonBuilder::begin() {
    SP<CButtonBuilder> p = SP<CButtonBuilder>(new CButtonBuilder());
    p->m_data            = makeUnique<SButtonData>();
    p->m_self            = p;
    return p;
}

SP<CButtonBuilder> CButtonBuilder::label(std::string&& s) {
    m_data->label = std::move(s);
    return m_self.lock();
}

SP<CButtonBuilder> CButtonBuilder::onMainClick(std::function<void(Hyprutils::Memory::CSharedPointer<CButtonElement>)>&& f) {
    m_data->onMainClick = std::move(f);
    return m_self.lock();
}

SP<CButtonBuilder> CButtonBuilder::onRightClick(std::function<void(Hyprutils::Memory::CSharedPointer<CButtonElement>)>&& f) {
    m_data->onRightClick = std::move(f);
    return m_self.lock();
}

SP<CButtonBuilder> CButtonBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CButtonElement> CButtonBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CButtonElement::create(*m_data);
}