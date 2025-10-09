#include "Textbox.hpp"

using namespace Hyprtoolkit;

SP<CTextboxBuilder> CTextboxBuilder::begin() {
    SP<CTextboxBuilder> p = SP<CTextboxBuilder>(new CTextboxBuilder());
    p->m_data             = makeUnique<STextboxData>();
    p->m_self             = p;
    return p;
}

SP<CTextboxBuilder> CTextboxBuilder::placeholder(std::string&& x) {
    m_data->placeholder = std::move(x);
    return m_self.lock();
}

SP<CTextboxBuilder> CTextboxBuilder::defaultText(std::string&& x) {
    m_data->text = std::move(x);
    return m_self.lock();
}

SP<CTextboxBuilder> CTextboxBuilder::onTextEdited(std::function<void(SP<CTextboxElement>, const std::string&)>&& x) {
    m_data->onTextEdited = std::move(x);
    return m_self.lock();
}

SP<CTextboxBuilder> CTextboxBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CTextboxBuilder> CTextboxBuilder::multiline(bool x) {
    m_data->multiline = x;
    return m_self.lock();
}

SP<CTextboxElement> CTextboxBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CTextboxElement::create(*m_data);
}