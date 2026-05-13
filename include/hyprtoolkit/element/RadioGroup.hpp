#pragma once

#include "Checkbox.hpp"

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>

#include <functional>
#include <vector>

namespace Hyprtoolkit {

    // Coordinates exclusivity across a set of radio-styled CCheckboxElements.
    // Layout and sizing are left to the caller; the group only enforces that
    // toggling one radio on turns the others off.
    class CRadioGroup {
      public:
        static Hyprutils::Memory::CSharedPointer<CRadioGroup> create();
        ~CRadioGroup()                                        = default;

        void                                                  add(Hyprutils::Memory::CSharedPointer<CCheckboxElement> radio);
        Hyprutils::Memory::CSharedPointer<CCheckboxElement>   selected();
        void                                                  setSelected(Hyprutils::Memory::CSharedPointer<CCheckboxElement> radio);
        void                                                  onSelected(std::function<void(Hyprutils::Memory::CSharedPointer<CCheckboxElement>)>&&);

      private:
        CRadioGroup() = default;

        std::vector<Hyprutils::Memory::CWeakPointer<CCheckboxElement>>      m_radios;
        std::function<void(Hyprutils::Memory::CSharedPointer<CCheckboxElement>)> m_onSelected;
        Hyprutils::Memory::CWeakPointer<CRadioGroup>                        m_self;
    };
};
