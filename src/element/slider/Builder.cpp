#include "Slider.hpp"

using namespace Hyprtoolkit;

SP<CSliderBuilder> CSliderBuilder::begin() {
    SP<CSliderBuilder> p = SP<CSliderBuilder>(new CSliderBuilder());
    p->m_data            = makeUnique<SSliderData>();
    p->m_self            = p;
    return p;
}

SP<CSliderBuilder> CSliderBuilder::min(float x) {
    m_data->min = x;
    return m_self.lock();
}

SP<CSliderBuilder> CSliderBuilder::max(float x) {
    m_data->max = x;
    return m_self.lock();
}

SP<CSliderBuilder> CSliderBuilder::val(float x) {
    m_data->current = x;
    return m_self.lock();
}

SP<CSliderBuilder> CSliderBuilder::fill(bool x) {
    m_data->fill = x;
    return m_self.lock();
}

SP<CSliderBuilder> CSliderBuilder::snapInt(bool x) {
    m_data->snapInt = x;
    return m_self.lock();
}

SP<CSliderBuilder> CSliderBuilder::label(std::string&& x) {
    m_data->label = std::move(x);
    return m_self.lock();
}

SP<CSliderBuilder> CSliderBuilder::onChanged(std::function<void(Hyprutils::Memory::CSharedPointer<CSliderElement>, float)>&& x) {
    m_data->onChanged = std::move(x);
    return m_self.lock();
}

SP<CSliderBuilder> CSliderBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CSliderElement> CSliderBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CSliderElement::create(*m_data);
}