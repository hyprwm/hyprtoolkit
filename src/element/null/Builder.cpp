#include "Null.hpp"

using namespace Hyprtoolkit;

SP<CNullBuilder> CNullBuilder::begin() {
    SP<CNullBuilder> p = SP<CNullBuilder>(new CNullBuilder());
    p->m_data          = makeUnique<SNullData>();
    p->m_self          = p;
    return p;
}

SP<CNullBuilder> CNullBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CNullElement> CNullBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CNullElement::create(*m_data);
}