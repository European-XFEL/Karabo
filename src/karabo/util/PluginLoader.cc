/*
 * $Id: PluginLoader.cc 5211 2012-02-21 20:25:17Z heisenb $
 *
 * File:   PluginLoader.cc
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

#ifndef _WIN32
#include <dlfcn.h>
#endif

#include <filesystem>

#include "PluginLoader.hh"
#include "SimpleElement.hh"
#include "VectorElement.hh"
#include "Version.hh"


namespace karabo {
    namespace util {

        KARABO_REGISTER_FOR_CONFIGURATION(PluginLoader)

        using namespace std;
        using namespace boost;
        using namespace std::filesystem;

        // Static initialization
        map<path, void*> PluginLoader::m_loadedPlugins = map<path, void*>();
        vector<string> PluginLoader::m_failedPlugins = vector<string>();
        std::set<std::string> PluginLoader::m_knownPlugins = std::set<std::string>();


        std::string PluginLoader::defaultPluginPath() {
            return Version::getPathToKaraboInstallation() + "/plugins";
        }


        void PluginLoader::expectedParameters(Schema& expected) {
            STRING_ELEMENT(expected)
                  .key("pluginDirectory")
                  .displayedName("Plugin Directory")
                  .description("Directory to search for plugins")
                  .assignmentOptional()
                  .defaultValue(defaultPluginPath())
                  .expertAccess()
                  .commit();

            VECTOR_STRING_ELEMENT(expected)
                  .key("pluginsToLoad")
                  .displayedName("Plugins to load")
                  .assignmentOptional()
                  .defaultValue(std::vector<std::string>(1, "*"))
                  .expertAccess()
                  .commit();
        }


        PluginLoader::PluginLoader(const Hash& input) {
            m_pluginDirectory = std::filesystem::path(input.get<string>("pluginDirectory"));
            const std::vector<std::string>& pluginsToLoad = input.get<std::vector<std::string> >("pluginsToLoad");
            m_pluginsToLoad.insert(pluginsToLoad.cbegin(), pluginsToLoad.cend());
        }


        const path& PluginLoader::getPluginDirectory() const {
            return m_pluginDirectory;
        }


        std::vector<std::string> PluginLoader::getKnownPlugins() const {
            return std::vector<std::string>(m_knownPlugins.cbegin(), m_knownPlugins.cbegin());
        }


        void PluginLoader::updatePluginsToLoad(const std::vector<std::string>& pluginsToLoad) {
            m_pluginsToLoad.clear();
            m_pluginsToLoad.insert(pluginsToLoad.cbegin(), pluginsToLoad.cend());
        }


        bool PluginLoader::update() {
            bool hasNewPlugins = false;

#ifndef _WIN32

            if (exists(m_pluginDirectory)) {
                unsigned long fileCount = 0;
                unsigned long dirCount = 0;
                unsigned long otherCount = 0;

                directory_iterator endIt; // default construction yields past-the-end
                for (directory_iterator it(m_pluginDirectory); it != endIt; ++it) {
                    try {
                        if (is_directory(it->status())) {
                            ++dirCount;
                            // cout << it->path().filename() << " [directory]\n";
                        } else if (is_regular_file(it->status())) {
                            ++fileCount;
                            // cout << it->path().filename() << "\n";

                            string plugin = it->path().string();
                            bool faultyPlugin = false;
                            for (size_t i = 0; i < m_failedPlugins.size(); i++) {
                                if (m_failedPlugins[i] == plugin) {
                                    faultyPlugin = true;
                                    break;
                                }
                            }
                            // add to known plugins
                            m_knownPlugins.insert(it->path().stem().string());

                            if (faultyPlugin) continue;
                            if (m_pluginsToLoad.find(it->path().stem().string()) == m_pluginsToLoad.end() &&
                                m_pluginsToLoad.find("*") == m_pluginsToLoad.end()) {
                                continue;
                            }
                            if (m_loadedPlugins.find(plugin) == m_loadedPlugins.end()) {
                                void* libHandle = dlopen(plugin.c_str(), RTLD_NOW);
                                if (libHandle == 0) {
                                    const string loadingError =
                                          string(dlerror()); // dlerror() != 0 since dlopen above failed
                                    if (loadingError.find("invalid ELF header") == string::npos &&
                                        loadingError.find("position independent executable") == string::npos) {
                                        // The file whose loading failed was a valid ELF file and was not an executable;
                                        // most likely it was a valid shared library and the failure was unexpected. The
                                        // error should be communicated.
                                        // Using plain output here as KARABO_LOG_[...] is potentially
                                        // not active at the time this message is generated and anyway comes from
                                        // karabo/log/... which should not be included here in karabo/util/...
                                        cerr << "ERROR Trouble loading plugin " << it->path().filename() << ":\n\t"
                                             << loadingError << endl;
                                    }
                                    // Log the failure to load the file to avoid any further retry that would also
                                    // fail.
                                    m_failedPlugins.push_back(plugin);
                                } else {
                                    m_loadedPlugins[it->path()] = libHandle;
                                    cerr << "INFO  Successfully loaded plugin: " << it->path().filename() << endl;
                                    hasNewPlugins = true;
                                }
                            } else {
                                // cout << "has already been loaded, skipping" << endl;><>
                            }
                        } else {
                            ++otherCount;
                            // cout << it->path().filename() << " [other]\n";
                        }
                    } catch (...) {
                        KARABO_RETHROW;
                    }
                }
            } else {
                throw KARABO_INIT_EXCEPTION("Could not find plugin directory:" + m_pluginDirectory.string());
            }

#endif

            return hasNewPlugins;
        }
    } // namespace util
} // namespace karabo
