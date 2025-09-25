#include "RowLayout.hpp"

using namespace Hyprtoolkit;

SP<CRowLayoutBuilder> CRowLayoutBuilder::begin() {
    SP<CRowLayoutBuilder> p = SP<CRowLayoutBuilder>(new CRowLayoutBuilder());
    p->m_data               = makeUnique<SRowLayoutData>();
    p->m_self               = p;
    return p;
}

SP<CRowLayoutBuilder> CRowLayoutBuilder::gap(size_t x) {
    m_data->gap = x;
    return m_self.lock();
}

SP<CRowLayoutBuilder> CRowLayoutBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CRowLayoutElement> CRowLayoutBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CRowLayoutElement::create(*m_data);
}