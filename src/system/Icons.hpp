#pragma once

#include <hyprtoolkit/system/Icons.hpp>

#include <optional>
#include <filesystem>
#include <vector>
#include <unordered_map>

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

        struct SIconCacheResult {
            bool                  badIcon = true;
            std::filesystem::path path;
        };

        /*
            Lookup an icon. If the icon is found, will return associated data.
            This object can be used to create an ImageElement
        */
        virtual Hyprutils::Memory::CSharedPointer<ISystemIconDescription> lookupIcon(const std::string& iconName);

        /*
            A cache is kept: loopkup paths can grow large, this is to avoid tons of disk I/O
        */
        void                            cacheEntry(const std::string& iconName, SIconCacheResult&& result);
        std::optional<SIconCacheResult> getCached(const std::string& name);

        bool                            m_hicolorAdded = false;

      private:
        void                     parseThemes(const std::vector<std::string>& themeDirs);
        void                     parseTheme(const std::string& themeDir);

        std::vector<std::string> m_lookupPaths;

        // TODO: stdlib's map is SLOW
        std::unordered_map<std::string, SIconCacheResult> m_pathCache;

        friend class CSystemIconDescription;
    };
};
