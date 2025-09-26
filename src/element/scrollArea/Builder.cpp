#include "ScrollArea.hpp"

using namespace Hyprtoolkit;

SP<CScrollAreaBuilder> CScrollAreaBuilder::begin() {
    SP<CScrollAreaBuilder> p = SP<CScrollAreaBuilder>(new CScrollAreaBuilder());
    p->m_data                = makeUnique<SScrollAreaData>();
    p->m_self                = p;
    return p;
}

SP<CScrollAreaBuilder> CScrollAreaBuilder::scrollX(bool x) {
    m_data->scrollX = x;
    return m_self.lock();
}

SP<CScrollAreaBuilder> CScrollAreaBuilder::scrollY(bool x) {
    m_data->scrollY = x;
    return m_self.lock();
}

SP<CScrollAreaBuilder> CScrollAreaBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CScrollAreaElement> CScrollAreaBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CScrollAreaElement::create(*m_data);
}