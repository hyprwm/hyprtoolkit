#pragma once

#include "wayland.hpp"

#include <hyprtoolkit/core/Output.hpp>

#include <hyprutils/math/Misc.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    class CWaylandOutput : public IOutput {
      public:
        CWaylandOutput(wl_proxy* wlResource, uint32_t id);
        ~CWaylandOutput() = default;

        virtual uint32_t                              handle();

        uint32_t                                      m_id       = 0;
        bool                                          m_focused  = false;
        Hyprutils::Memory::CSharedPointer<CCWlOutput> m_wlOutput = nullptr;

        struct {
            bool                        done      = false;
            Hyprutils::Math::eTransform transform = Hyprutils::Math::HYPRUTILS_TRANSFORM_NORMAL;
            Hyprutils::Math::Vector2D   size;
            int                         scale = 1;
            std::string                 name  = "";
            std::string                 port  = "";
            std::string                 desc  = "";
        } m_configuration;
    };
}
