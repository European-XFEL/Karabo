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

#include "Schema.hh"


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
      KARABO_FACTORY_BASE_CLASS

      PluginLoader() {
      };

      PluginLoader(const boost::filesystem::path& pluginDirectory) : m_pluginDirectory(pluginDirectory) {
      }

      static void expectedParameters(karabo::util::Schema& expected);

      void configure(const karabo::util::Hash& input);

      virtual ~PluginLoader() {
      };

      bool update();

      const boost::filesystem::path& getPluginDirectory() const;

    protected:

    private:

      boost::filesystem::path m_pluginDirectory;

      static std::map<boost::filesystem::path, void*> m_loadedPlugins;

    };
  } // namespace util
} // namespace karabo

KARABO_REGISTER_FACTORY_BASE_HH(karabo::util::PluginLoader, TEMPLATE_UTIL, DECLSPEC_UTIL)

#endif	/* KARABO_PACKAGENAME_PLUGINLOADER_HH */
