/*
 * $Id: PluginLoader.cc 5211 2012-02-21 20:25:17Z heisenb $
 *
 * File:   PluginLoader.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on October 4, 2010, 6:16 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef _WIN32
#include <dlfcn.h>
#endif

#include "PathElement.hh"
#include "PluginLoader.hh"
#include "karabo/log/Logger.hh"


namespace karabo {
    namespace util {

        KARABO_REGISTER_FOR_CONFIGURATION(PluginLoader)

        using namespace std;
        using namespace boost;
        using namespace boost::filesystem;

        // Static initialization
        map<path, void*> PluginLoader::m_loadedPlugins = map<path, void*>();
        vector<string> PluginLoader::m_failedPlugins = vector<string>();


        void PluginLoader::expectedParameters(Schema& expected) {

            PATH_ELEMENT(expected)
                    .key("pluginDirectory")
                    .displayedName("Plugin Directory")
                    .description("Directory to search for plugins")
                    .assignmentOptional().defaultValue("plugins")
                    .isDirectory()
                    .expertAccess()
                    .commit();

        }


        PluginLoader::PluginLoader(const Hash& input) {
            m_pluginDirectory = boost::filesystem::path(input.get<string>("pluginDirectory"));
        }


        const path& PluginLoader::getPluginDirectory() const {
            return m_pluginDirectory;
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
                            //cout << it->path().filename() << " [directory]\n";
                        } else if (is_regular_file(it->status())) {
                            ++fileCount;
                            //cout << it->path().filename() << "\n";

                            string plugin = it->path().string();
                            bool faultyPlugin = false;
                            for (size_t i = 0; i < m_failedPlugins.size(); i++)
                                if (m_failedPlugins[i] == plugin) {
                                    faultyPlugin = true;
                                    break;
                                }
                            if (faultyPlugin) continue;
                            if (m_loadedPlugins.find(plugin) == m_loadedPlugins.end()) {
                                void* libHandle = dlopen(plugin.c_str(), RTLD_NOW);
                                if (libHandle == 0) {
                                    KARABO_LOG_FRAMEWORK_ERROR << "Trouble loading plugin "
                                            << it->path().filename()
                                            << ":\n\t" << string(dlerror());
                                    m_failedPlugins.push_back(plugin);
                                    //throw KARABO_INIT_EXCEPTION("Failed to load plugin " + string(it->path().filename().string()) + ": " + string(dlerror()));
                                } else {
                                    m_loadedPlugins[it->path()] = libHandle;
                                    KARABO_LOG_FRAMEWORK_INFO << "Successfully loaded plugin: "
                                            << it->path().filename();
                                    hasNewPlugins = true;
                                }
                            } else {
                                //cout << "has already been loaded, skipping" << endl;><>
                            }
                        } else {
                            ++otherCount;
                            //cout << it->path().filename() << " [other]\n";
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
