#include "WaylandOutput.hpp"
#include "../core/InternalBackend.hpp"
#include "../helpers/Memory.hpp"

using namespace Hyprtoolkit;

static Hyprutils::Math::eTransform wlTransformToHyprutils(wl_output_transform t) {
    switch (t) {
        case WL_OUTPUT_TRANSFORM_NORMAL: return Hyprutils::Math::eTransform::HYPRUTILS_TRANSFORM_NORMAL;
        case WL_OUTPUT_TRANSFORM_180: return Hyprutils::Math::eTransform::HYPRUTILS_TRANSFORM_180;
        case WL_OUTPUT_TRANSFORM_90: return Hyprutils::Math::eTransform::HYPRUTILS_TRANSFORM_90;
        case WL_OUTPUT_TRANSFORM_270: return Hyprutils::Math::eTransform::HYPRUTILS_TRANSFORM_270;
        case WL_OUTPUT_TRANSFORM_FLIPPED: return Hyprutils::Math::eTransform::HYPRUTILS_TRANSFORM_FLIPPED;
        case WL_OUTPUT_TRANSFORM_FLIPPED_180: return Hyprutils::Math::eTransform::HYPRUTILS_TRANSFORM_FLIPPED_180;
        case WL_OUTPUT_TRANSFORM_FLIPPED_270: return Hyprutils::Math::eTransform::HYPRUTILS_TRANSFORM_FLIPPED_270;
        case WL_OUTPUT_TRANSFORM_FLIPPED_90: return Hyprutils::Math::eTransform::HYPRUTILS_TRANSFORM_FLIPPED_90;
        default: break;
    }
    return Hyprutils::Math::eTransform::HYPRUTILS_TRANSFORM_NORMAL;
}

CWaylandOutput::CWaylandOutput(wl_proxy* wlResource, uint32_t id) : m_id(id), m_wlOutput(wlResource) {
    m_wlOutput.setDescription([this](CCWlOutput* r, const char* description) {
        m_configuration.desc = description ? std::string{description} : "";
        g_logger->log(HT_LOG_DEBUG, "wayland output {}: description {}", m_id, m_configuration.desc);
    });

    m_wlOutput.setName([this](CCWlOutput* r, const char* name) {
        m_configuration.name = std::string{name} + m_configuration.name;
        m_configuration.port = std::string{name};
        g_logger->log(HT_LOG_DEBUG, "wayland output {}: name {}", name, name);
    });

    m_wlOutput.setScale([this](CCWlOutput* r, int32_t sc) { m_configuration.scale = sc; });

    m_wlOutput.setDone([this](CCWlOutput* r) {
        m_configuration.done = true;
        g_logger->log(HT_LOG_DEBUG, "wayland output {} done", m_id);
    });

    m_wlOutput.setMode([this](CCWlOutput* r, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
        // handle portrait mode and flipped cases
        if (m_configuration.transform % 2 == 1)
            m_configuration.size = {height, width};
        else
            m_configuration.size = {width, height};

        g_logger->log(HT_LOG_DEBUG, "wayland output {} dimensions {}", m_id);
    });

    m_wlOutput.setGeometry(
        [this](CCWlOutput* r, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, const char* make, const char* model, int32_t transform_) {
            m_configuration.transform = wlTransformToHyprutils((wl_output_transform)transform_);

            g_logger->log(HT_LOG_DEBUG, "wayland output {} make {} model {}", m_id, make ? make : "", model ? model : "");
        });
}
