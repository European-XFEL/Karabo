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

#ifndef EXFEL_UTIL_PLUGINLOADER_HH
#define	EXFEL_UTIL_PLUGINLOADER_HH

#include "Schema.hh"


/**
 * The main European XFEL namespace
 */
namespace exfel {

  /**
   * Namespace for package packageName
   */
  namespace util {

    /**
     * The PluginLoader class.
     */
    class PluginLoader {
    public:

      EXFEL_CLASSINFO(PluginLoader, "PluginLoader", "1.0")
      EXFEL_FACTORY_BASE_CLASS

      PluginLoader() {
      };

      PluginLoader(const boost::filesystem::path& pluginDirectory) : m_pluginDirectory(pluginDirectory) {
      }

      static void expectedParameters(exfel::util::Schema& expected);

      void configure(const exfel::util::Hash& input);

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
} // namespace exfel

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::util::PluginLoader, TEMPLATE_UTIL, DECLSPEC_UTIL)

#endif	/* EXFEL_PACKAGENAME_PLUGINLOADER_HH */
