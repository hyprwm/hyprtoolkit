#include "ProgressBar.hpp"

using namespace Hyprtoolkit;

SP<CProgressBarBuilder> CProgressBarBuilder::begin() {
    SP<CProgressBarBuilder> p = SP<CProgressBarBuilder>(new CProgressBarBuilder());
    p->m_data                 = makeUnique<SProgressBarData>();
    p->m_self                 = p;
    return p;
}

SP<CProgressBarBuilder> CProgressBarBuilder::value(float v) {
    m_data->value = v;
    return m_self.lock();
}

SP<CProgressBarBuilder> CProgressBarBuilder::indeterminate(bool v) {
    m_data->indeterminate = v;
    return m_self.lock();
}

SP<CProgressBarBuilder> CProgressBarBuilder::size(CDynamicSize&& s) {
    m_data->size = std::move(s);
    return m_self.lock();
}

SP<CProgressBarElement> CProgressBarBuilder::commence() {
    if (m_element) {
        m_element->replaceData(*m_data);
        return m_element.lock();
    }

    return CProgressBarElement::create(*m_data);
}
