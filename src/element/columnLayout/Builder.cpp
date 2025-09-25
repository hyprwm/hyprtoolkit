#include "ColumnLayout.hpp"

using namespace Hyprtoolkit;

SP<CColumnLayoutBuilder> CColumnLayoutBuilder::begin() {
    SP<CColumnLayoutBuilder> p = SP<CColumnLayoutBuilder>(new CColumnLayoutBuilder());
    p->m_data                  = makeUnique<SColumnLayoutData>();
    p->m_self                  = p;
    return p;
}

SP<CColumnLayoutBuilder> CColumnLayoutBuilder::gap(size_t x) {
    m_data->gap = x;
    return m_self.lock();
}

SP<CColumnLayoutBuilder> CColumnLayoutBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CColumnLayoutElement> CColumnLayoutBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CColumnLayoutElement::create(*m_data);
}