#pragma once

#include <hyprtoolkit/system/Icons.hpp>

#include <optional>
#include <vector>

namespace Hyprtoolkit {
    class CSystemIconDescription : public ISystemIconDescription {
      public:
        // invalid icon
        CSystemIconDescription();
        // lookup icon
        CSystemIconDescription(const std::string& name);
        virtual ~CSystemIconDescription() = default;

        virtual bool exists();
        virtual bool scalable();

        std::string  m_bestPath = "";
        bool         m_scalable = false;
    };

    class CSystemIconFactory : public ISystemIconFactory {
      public:
        CSystemIconFactory();
        virtual ~CSystemIconFactory() = default;

        /*
            Lookup an icon. If the icon is found, will return associated data.
            This object can be used to create an ImageElement
        */
        virtual Hyprutils::Memory::CSharedPointer<ISystemIconDescription> lookupIcon(const std::string& iconName);

      private:
        std::optional<std::string> m_themeDir;
        std::vector<std::string>   m_iconDirs;

        friend class CSystemIconDescription;
    };
};
