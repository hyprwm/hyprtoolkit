#include "Line.hpp"

using namespace Hyprtoolkit;

SP<CLineBuilder> CLineBuilder::begin() {
    SP<CLineBuilder> p = SP<CLineBuilder>(new CLineBuilder());
    p->m_data          = makeUnique<SLineData>();
    p->m_self          = p;
    return p;
}

SP<CLineBuilder> CLineBuilder::color(colorFn&& f) {
    m_data->color = std::move(f);
    return m_self.lock();
}

SP<CLineBuilder> CLineBuilder::points(std::vector<Hyprutils::Math::Vector2D>&& f) {
    m_data->points = std::move(f);
    return m_self.lock();
}

SP<CLineBuilder> CLineBuilder::thick(int x) {
    m_data->thick = x;
    return m_self.lock();
}

SP<CLineBuilder> CLineBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CLineElement> CLineBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CLineElement::create(*m_data);
}