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

SP<CWindowBuilder> CWindowBuilder::prefferedOutput(uint32_t x) {
    m_data->prefferedOutputId = x;
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

SP<CWindowBuilder> CWindowBuilder::marginTopLeft(const Hyprutils::Math::Vector2D& x) {
    m_data->marginTopLeft = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::marginBottomRight(const Hyprutils::Math::Vector2D& x) {
    m_data->marginBottomRight = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::layer(uint32_t x) {
    m_data->layer = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::anchor(uint32_t x) {
    m_data->anchor = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::exclusiveEdge(uint32_t x) {
    m_data->exclusiveEdge = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::exclusiveZone(int32_t x) {
    m_data->exclusiveZone = x;
    return m_self.lock();
}

SP<CWindowBuilder> CWindowBuilder::kbInteractive(uint32_t x) {
    m_data->kbInteractive = x;
    return m_self.lock();
}

SP<IWindow> CWindowBuilder::commence() {
    switch (m_data->type) {
        case HT_WINDOW_POPUP:
            if (!m_data->parent)
                return nullptr;

            return reinterpretPointerCast<IToolkitWindow>(m_data->parent)->openPopup(*m_data);
        case HT_WINDOW_TOPLEVEL:
        case HT_WINDOW_LAYER:
        case HT_WINDOW_LOCK_SURFACE: return g_backend->openWindow(*m_data);
    }
    return nullptr;
}
