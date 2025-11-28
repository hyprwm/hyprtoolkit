#include "Image.hpp"

using namespace Hyprtoolkit;

SP<CImageBuilder> CImageBuilder::begin() {
    SP<CImageBuilder> p = SP<CImageBuilder>(new CImageBuilder());
    p->m_data           = makeUnique<SImageData>();
    p->m_self           = p;
    return p;
}

SP<CImageBuilder> CImageBuilder::path(std::string&& s) {
    m_data->path = std::move(s);
    return m_self.lock();
}

SP<CImageBuilder> CImageBuilder::a(float a) {
    m_data->a = a;
    return m_self.lock();
}

Hyprutils::Memory::CSharedPointer<CImageBuilder> CImageBuilder::sync(bool x) {
    m_data->sync = x;
    return m_self.lock();
}

SP<CImageBuilder> CImageBuilder::fitMode(eImageFitMode x) {
    m_data->fitMode = x;
    return m_self.lock();
}

SP<CImageBuilder> CImageBuilder::rounding(int x) {
    m_data->rounding = x;
    return m_self.lock();
}

SP<CImageBuilder> CImageBuilder::icon(const SP<ISystemIconDescription>& x) {
    m_data->icon = x;
    return m_self.lock();
}

SP<CImageBuilder> CImageBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CImageElement> CImageBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CImageElement::create(*m_data);
}