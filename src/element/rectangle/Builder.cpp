#include "Rectangle.hpp"

using namespace Hyprtoolkit;

SP<CRectangleBuilder> CRectangleBuilder::begin() {
    SP<CRectangleBuilder> p = SP<CRectangleBuilder>(new CRectangleBuilder());
    p->m_data               = makeUnique<SRectangleData>();
    p->m_self               = p;
    return p;
}

SP<CRectangleBuilder> CRectangleBuilder::color(colorFn&& f) {
    m_data->color = std::move(f);
    return m_self.lock();
}

SP<CRectangleBuilder> CRectangleBuilder::borderColor(colorFn&& f) {
    m_data->borderColor = std::move(f);
    return m_self.lock();
}

SP<CRectangleBuilder> CRectangleBuilder::rounding(int x) {
    m_data->rounding = x;
    return m_self.lock();
}

SP<CRectangleBuilder> CRectangleBuilder::borderThickness(int x) {
    m_data->borderThickness = x;
    return m_self.lock();
}

SP<CRectangleBuilder> CRectangleBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CRectangleElement> CRectangleBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CRectangleElement::create(*m_data);
}