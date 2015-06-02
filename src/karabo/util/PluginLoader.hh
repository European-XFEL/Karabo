/*
 * $Id: PluginLoader.hh 5394 2012-03-07 16:09:30Z wegerk $
 *
 * File:   PluginLoader.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2010, 6:16 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_PLUGINLOADER_HH
#define	KARABO_UTIL_PLUGINLOADER_HH

#include <boost/filesystem.hpp>

#include "Schema.hh"
#include "Configurator.hh"


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

            PluginLoader(const std::string& pluginDirectory) : m_pluginDirectory(pluginDirectory) {
            }

            static void expectedParameters(karabo::util::Schema& expected);

            PluginLoader(const karabo::util::Hash& input);

            virtual ~PluginLoader() {
            };

            bool update();

            const boost::filesystem::path& getPluginDirectory() const;

        protected:

        private:

            boost::filesystem::path m_pluginDirectory;

            static std::map<boost::filesystem::path, void*> m_loadedPlugins;
            static std::vector<std::string> m_failedPlugins;

        };
    } // namespace util
} // namespace karabo

#endif	/* KARABO_PACKAGENAME_PLUGINLOADER_HH */
