#include "Window.hpp"
#include "../core/InternalBackend.hpp"
#include "ToolkitWindow.hpp"

using namespace Hyprtoolkit;

SP<CWindowBuilder> CWindowBuilder::begin() {
    SP<CWindowBuilder> p = SP<CWindowBuilder>(new CWindowBuilder());
    p->m_data            = makeUnique<SWindowCreationData>();
    p->m_self            = p;
    return p;
}

SP<CWindowBuilder> CWindowBuilder::type(eWindowType x) {
    m_data->type = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::appTitle(std::string&& x) {
    m_data->title = std::move(x);
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::appClass(std::string&& x) {
    m_data->class_ = std::move(x);
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::preferredSize(const Hyprutils::Math::Vector2D& x) {
    m_data->preferredSize = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::minSize(const Hyprutils::Math::Vector2D& x) {
    m_data->minSize = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::maxSize(const Hyprutils::Math::Vector2D& x) {
    m_data->maxSize = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::parent(const SP<IWindow>& x) {
    m_data->parent = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::pos(const Hyprutils::Math::Vector2D& x) {
    m_data->pos = x;
    return m_self.lock();
}

SP<IWindow> CWindowBuilder::commence() {
    switch (m_data->type) {
        case HT_WINDOW_POPUP:
            if (!m_data->parent)
                return nullptr;

            return reinterpretPointerCast<IToolkitWindow>(m_data->parent)->openPopup(*m_data);
        case HT_WINDOW_TOPLEVEL: return g_backend->openWindow(*m_data);
    }
    return nullptr;
}