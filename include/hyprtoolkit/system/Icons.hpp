#pragma once

#include <hyprutils/memory/SharedPtr.hpp>

namespace Hyprtoolkit {

    class ISystemIconDescription {
      public:
        virtual ~ISystemIconDescription() = default;

        virtual bool exists()   = 0;
        virtual bool scalable() = 0;

      protected:
        ISystemIconDescription() = default;
    };

    class ISystemIconFactory {
      public:
        virtual ~ISystemIconFactory() = default;

        /*
            Lookup an icon. If the icon is found, will return associated data.
            This object can be used to create an ImageElement
        */
        virtual Hyprutils::Memory::CSharedPointer<ISystemIconDescription> lookupIcon(const std::string& iconName) = 0;

      protected:
        ISystemIconFactory() = default;
    };
}
