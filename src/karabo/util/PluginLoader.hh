/*
 * $Id: PluginLoader.hh 5394 2012-03-07 16:09:30Z wegerk $
 *
 * File:   PluginLoader.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2010, 6:16 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef KARABO_UTIL_PLUGINLOADER_HH
#define KARABO_UTIL_PLUGINLOADER_HH

#include <filesystem>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Schema.hh"


/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package packageName
     */
    namespace util {

        /**
         * The PluginLoader class.
         */
        class PluginLoader {
           public:
            KARABO_CLASSINFO(PluginLoader, "PluginLoader", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            PluginLoader(const std::string& pluginDirectory) : m_pluginDirectory(pluginDirectory) {}

            static std::string defaultPluginPath();

            static void expectedParameters(karabo::data::Schema& expected);

            PluginLoader(const karabo::data::Hash& input);

            virtual ~PluginLoader(){};

            bool update();

            const std::filesystem::path& getPluginDirectory() const;

            std::vector<std::string> getKnownPlugins() const;

            void updatePluginsToLoad(const std::vector<std::string>& pluginsToLoad);

           protected:
           private:
            std::filesystem::path m_pluginDirectory;

            static std::map<std::filesystem::path, void*> m_loadedPlugins;
            static std::vector<std::string> m_failedPlugins;
            static std::set<std::string> m_knownPlugins;
            std::set<std::string> m_pluginsToLoad;
        };
    } // namespace util
} // namespace karabo

#endif /* KARABO_PACKAGENAME_PLUGINLOADER_HH */
