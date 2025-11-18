#include "Text.hpp"

using namespace Hyprtoolkit;

SP<CTextBuilder> CTextBuilder::begin() {
    SP<CTextBuilder> p = SP<CTextBuilder>(new CTextBuilder());
    p->m_data          = makeUnique<STextData>();
    p->m_self          = p;
    return p;
}

SP<CTextBuilder> CTextBuilder::color(colorFn&& f) {
    m_data->color = std::move(f);
    return m_self.lock();
}

SP<CTextBuilder> CTextBuilder::a(float a) {
    m_data->a = a;
    return m_self.lock();
}

SP<CTextBuilder> CTextBuilder::fontSize(CFontSize&& x) {
    //NOLINTNEXTLINE
    m_data->fontSize = std::move(x);
    return m_self.lock();
}

SP<CTextBuilder> CTextBuilder::text(std::string&& x) {
    m_data->text = std::move(x);
    return m_self.lock();
}

SP<CTextBuilder> CTextBuilder::align(eFontAlignment x) {
    m_data->align = x;
    return m_self.lock();
}

SP<CTextBuilder> CTextBuilder::fontFamily(std::string&& x) {
    m_data->fontFamily = std::move(x);
    return m_self.lock();
}

SP<CTextBuilder> CTextBuilder::clampSize(Hyprutils::Math::Vector2D&& x) {
    m_data->clampSize = std::move(x);
    return m_self.lock();
}

SP<CTextBuilder> CTextBuilder::noEllipsize(bool x) {
    m_data->noEllipsize = std::move(x);
    return m_self.lock();
}

SP<CTextBuilder> CTextBuilder::callback(std::function<void()>&& x) {
    m_data->callback = std::move(x);
    return m_self.lock();
}

SP<CTextBuilder> CTextBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

Hyprutils::Memory::CSharedPointer<CTextBuilder> CTextBuilder::async(bool x) {
    m_data->async = x;
    return m_self.lock();
}

SP<CTextElement> CTextBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CTextElement::create(*m_data);
}