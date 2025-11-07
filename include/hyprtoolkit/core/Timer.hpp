#pragma once

#include <chrono>
#include <functional>
#include <hyprutils/memory/Atomic.hpp>

namespace Hyprtoolkit {
    class CTimer {
      public:
        CTimer(std::chrono::steady_clock::duration timeout, std::function<void(Hyprutils::Memory::CAtomicSharedPointer<CTimer> self, void* data)> cb_, void* data_, bool force);

        void  cancel();
        bool  passed();
        bool  canForceUpdate();
        void  updateTimeout(std::chrono::steady_clock::duration timeout);

        float leftMs();

        bool  cancelled();
        void  call(Hyprutils::Memory::CAtomicSharedPointer<CTimer> self);

      private:
        std::function<void(Hyprutils::Memory::CAtomicSharedPointer<CTimer> self, void* data)> m_cb;
        void*                                                                                 m_data = nullptr;
        std::chrono::steady_clock::time_point                                                 m_expires;
        bool                                                                                  m_wasCancelled     = false;
        bool                                                                                  m_allowForceUpdate = false;
    };
}
