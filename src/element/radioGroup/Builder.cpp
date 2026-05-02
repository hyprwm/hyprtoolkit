#include "RadioGroup.hpp"

using namespace Hyprtoolkit;

SP<CRadioGroupBuilder> CRadioGroupBuilder::begin() {
    SP<CRadioGroupBuilder> p = SP<CRadioGroupBuilder>(new CRadioGroupBuilder());
    p->m_data                = makeUnique<SRadioGroupData>();
    p->m_self                = p;
    return p;
}

SP<CRadioGroupBuilder> CRadioGroupBuilder::items(std::vector<std::string>&& v) {
    m_data->items = std::move(v);
    return m_self.lock();
}

SP<CRadioGroupBuilder> CRadioGroupBuilder::selected(int idx) {
    m_data->selected = idx;
    return m_self.lock();
}

SP<CRadioGroupBuilder> CRadioGroupBuilder::gap(float g) {
    m_data->gap = g;
    return m_self.lock();
}

SP<CRadioGroupBuilder> CRadioGroupBuilder::onSelected(std::function<void(SP<CRadioGroupElement>, int)>&& cb) {
    m_data->onSelected = std::move(cb);
    return m_self.lock();
}

SP<CRadioGroupBuilder> CRadioGroupBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CRadioGroupElement> CRadioGroupBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }
    return CRadioGroupElement::create(*m_data);
}
