#pragma once

#include <optional>
#include <unordered_map>

#include <hyprtoolkit/window/EmbeddedSurface.hpp>

#include "ToolkitWindow.hpp"

namespace Hyprtoolkit {
    class CEmbeddedBackend;

    class CEmbeddedSurface final : public IToolkitWindow, public IEmbeddedSurface {
      public:
        CEmbeddedSurface();
        ~CEmbeddedSurface();

        void                      resize(const Hyprutils::Math::Vector2D& logicalSize, const Hyprutils::Math::Vector2D& pixelSize, float scale) override;
        void                      render(uint32_t bufferAge) override;

        void                      pointerEnter(const Hyprutils::Math::Vector2D& local) override;
        void                      pointerMotion(const Hyprutils::Math::Vector2D& local) override;
        void                      pointerButton(Input::eMouseButton button, bool pressed) override;
        void                      pointerAxis(Input::eAxisAxis axis, float delta) override;
        void                      pointerLeave() override;
        void                      keyboardEnter() override;
        void                      keyboardKey(const Input::SKeyboardKeyEvent& event) override;
        void                      keyboardLeave() override;
        void                      imCommit(const std::string& text) override;
        void                      imApply() override;
        void                      touchDown(int32_t id, const Hyprutils::Math::Vector2D& local, uint32_t timeMs) override;
        void                      touchMotion(int32_t id, const Hyprutils::Math::Vector2D& local, uint32_t timeMs) override;
        void                      touchUp(int32_t id, uint32_t timeMs) override;
        void                      touchCancel(int32_t id, uint32_t timeMs) override;

        SP<IElement>              rootElement() override;
        EmbeddedSurfaceID         id() override;

        Hyprutils::Math::Vector2D pixelSize() override;
        float                     scale() override;
        void                      open() override;
        void                      close() override;
        void                      scheduleFrame() override;
        void                      damage(Hyprutils::Math::CRegion&& region) override;
        void                      damageEntire() override;
        void                      setCursor(ePointerShape shape) override;
        void                      setIMTo(const Hyprutils::Math::CBox& box, const std::string& text, size_t cursor) override;
        void                      resetIM() override;
        SP<IWindow>               openPopup(const SWindowCreationData& data) override;

        void                      setSelf(const SP<CEmbeddedSurface>& self, const SP<CEmbeddedBackend>& backend);
        void                      invalidate();

      private:
        void                      render() override;
        SP<IElement>              touchTarget(const Hyprutils::Math::Vector2D& local);
        bool                      clippedAt(SP<IElement> element, const Hyprutils::Math::Vector2D& local);

        Hyprutils::Math::Vector2D m_logicalSize;
        Hyprutils::Math::Vector2D m_pixelSize;
        float                     m_scale = 1.F;
        bool                      m_open  = false;
        bool                      m_valid = true;
        WP<CEmbeddedSurface>      m_embeddedSelf;
        WP<CEmbeddedBackend>      m_backend;
        EmbeddedSurfaceID         m_id = 0;

        struct STouchFocus {
            WP<IElement>              element;
            Hyprutils::Math::Vector2D lastLocal;
        };

        std::unordered_map<int32_t, STouchFocus> m_touchFocus;
        std::optional<int32_t>                   m_primaryTouch;
    };
}
