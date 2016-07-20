/* 
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 10, 2014, 3:58 PM
 * 
 * Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iosfwd>
#include <fstream>
#include <sstream>
#include <boost/filesystem/operations.hpp>
#include "Version.hh"
#include "Exception.hh"
#include "StringTools.hh"

namespace karabo {

    namespace util {


        Version::Version() : m_major(-1), m_minor(-1), m_patch(-1), m_revision(-1) {

            boost::filesystem::path versionFile = getPathToVersionFile();
            if (boost::filesystem::exists(versionFile)) {
                std::stringstream buffer;
                std::ifstream file(versionFile.string().c_str());
                if (file) {
                    buffer << file.rdbuf();
                    file.close();
                } else {
                    throw KARABO_IO_EXCEPTION("Cannot open file: " + versionFile.string());
                }
                m_versionString = buffer.str();
                m_versionString = m_versionString.substr(0, m_versionString.size() - 1); // Cut away newline character
                if (m_versionString.empty()) {
                    if (m_versionString[0] == 'r') {
                        m_revision = fromString<int>(m_versionString.substr(1));
                    } else {
                        std::vector<int> v = fromString<int, std::vector>(m_versionString, ".");
                        if (v.size() > 0) m_major = v[0];
                        if (v.size() > 1) m_minor = v[1];
                        if (v.size() > 2) m_patch = v[2];
                    }
                }
            }
        }


        std::string Version::getPathToVersionFile() {
            return getPathToKaraboInstallation() += "/VERSION";
        }


        std::string Version::getPathToKaraboInstallation() {
            std::string karabo;
            const char* tmp = getenv("KARABO");
            if (tmp) karabo = tmp;
            if (karabo.empty()) {
                // Check whether we are acting as framework developers
                // In this case the path to the installed framework is like that 
                // (as we are sitting in build/netbeans/karabo)
                karabo = "../../../karabo";
                if (boost::filesystem::exists(karabo)) {
                    return karabo;
                }
                throw KARABO_INIT_EXCEPTION("$KARABO environment variable is not defined but needed to get the path to the Karabo installation.");
            }
            return karabo;
        }


        Version& Version::getInstance() {
            static Version v;
            return v;
        }


        std::string Version::getVersion() {
            return getInstance().m_versionString;
        }


        int Version::getMajor() {
            return getInstance().m_major;
        }


        int Version::getMinor() {
            return getInstance().m_minor;
        }


        int Version::getPatch() {
            return getInstance().m_patch;
        }


        int Version::getRevision() {
            return getInstance().m_revision;
        }

    }
}