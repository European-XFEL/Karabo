/*
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 10, 2014, 3:58 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "Version.hh"

#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <iosfwd>
#include <sstream>

#include "Exception.hh"
#include "StringTools.hh"
#include "repositoryVersion"

// Temporary change to trigger CI build.

namespace karabo {

    namespace util {


        Version::Version()
            : m_major(-1), m_minor(-1), m_patch(-1), m_postType(PostfixType::NONE), m_post(-1), m_dev(-1) {
            processString(KARABO_VERSION); // from repositoryVersion
        }

        Version::Version(const std::string &version)
            : m_major(-1), m_minor(-1), m_patch(-1), m_postType(PostfixType::NONE), m_post(-1), m_dev(-1) {
            processString(version);
        }

        void Version::processString(const std::string &version) {
            //                       MANDATORY FIELDS       |OPTIONAL FIELDS                        |
            //                         Major   .Minor   .Patch(suffix         )(suf_n )(dev_suf)(dev_n)
            m_versionString = version;
            boost::regex versionRegex("(\\d+)\\.(\\d+)\\.(\\d+)(a|b|rc|\\.post)?(\\d+)?(\\.dev)?(\\d+)?");
            boost::smatch versionParts;
            // could not use std::regex because some legacy hardware uses gcc 4.8
            bool result = boost::regex_search(version, versionParts, versionRegex);
            if (result && versionParts.size() == 8) {
                // versionParts[0] is the full matched string.
                m_major = fromString<unsigned int>(versionParts.str(1));
                m_minor = fromString<unsigned int>(versionParts.str(2));
                m_patch = fromString<unsigned int>(versionParts.str(3));
                // the strings below here might be empty since they belong to optional fields
                const std::string &separator = versionParts.str(4);
                const std::string &postVersion = versionParts.str(5);
                const std::string &devSeparator = versionParts.str(6);
                const std::string &devVersion = versionParts.str(7);
                if (separator == "a" && postVersion.size() > 0) {
                    m_postType = PostfixType::ALPHA;
                    m_post = fromString<unsigned int>(postVersion);
                } else if (separator == "b" && postVersion.size() > 0) {
                    m_postType = PostfixType::BETA;
                    m_post = fromString<unsigned int>(postVersion);
                } else if (separator == "rc" && postVersion.size() > 0) {
                    m_postType = PostfixType::RC;
                    m_post = fromString<unsigned int>(postVersion);
                } else if (separator == ".post" && postVersion.size() > 0) {
                    m_postType = PostfixType::POST;
                    m_post = fromString<unsigned int>(postVersion);
                } else if (separator == "" || postVersion.size() == 0) {
                    m_postType = PostfixType::NONE;
                    m_post = 0;
                }
                if (devSeparator == ".dev" && devVersion.size() > 0) {
                    // development release
                    m_dev = fromString<unsigned int>(devVersion);
                }
            }
        }

        std::string Version::getPathToVersionFile() {
            return getPathToKaraboInstallation() += "/VERSION";
        }


        std::string Version::getPathToKaraboInstallation() {
            const char *envKarabo = getenv("KARABO");
            if (envKarabo) {
                // The KARABO env var is defined; we're done.
                return std::string(envKarabo);
            }

            // TODO: remove this exceptional handling for older Karabo versions.
            //       suggested time-frame: Summer 2022.

            // Check whether we are acting as framework developers with the build
            // directory located inside the source tree (at build/netbeans/karabo).
            // This is the directory structure used by the deprecated Netbeans
            // based build system and is checked for backward compatibility
            // reasons - Karabo versions released before Summer 2021 used the
            // Netbeans based build.
            const std::string karaboParentDir("../../../karabo");
            boost::system::error_code ec;
            bool karaboParentDirFound = boost::filesystem::exists(karaboParentDir, ec);
            if (!ec && karaboParentDirFound) {
                // Yes, we are using the in source build tree of
                // the Netbeans based build environment.
                return karaboParentDir;
            }

            // All the attempts to obtain a path for the Karabo installation failed.
            throw KARABO_INIT_EXCEPTION(
                  "$KARABO environment variable is not defined but needed to get the path to the Karabo installation.");
        }


        const Version &Version::getKaraboVersion() {
            static const Version v;
            return v;
        }


        std::string Version::getVersion() {
            return getKaraboVersion().m_versionString;
        }

        const std::string &Version::getString() const {
            return m_versionString;
        }

        int Version::getMajor() const {
            return m_major;
        }

        int Version::getMinor() const {
            return m_minor;
        }

        int Version::getPatch() const {
            return m_patch;
        }

        bool Version::isDevRelease() const {
            return m_dev != -1;
        }

        bool Version::isPreRelease() const {
            return (m_postType == PostfixType::ALPHA || m_postType == PostfixType::BETA ||
                    m_postType == PostfixType::RC);
        }

        bool Version::isPostRelease() const {
            return m_postType == PostfixType::POST;
        }

        bool operator==(const karabo::util::Version &v1, const karabo::util::Version &v2) {
            if (v1.m_major == v2.m_major && v1.m_minor == v2.m_minor && v1.m_patch == v2.m_patch &&
                v1.m_postType == v2.m_postType && v1.m_post == v2.m_post && v1.m_dev == v2.m_dev) {
                return true;
            }
            return false;
        }

        bool operator>(const karabo::util::Version &v1, const karabo::util::Version &v2) {
            if (v1.m_major < v2.m_major) {
                return false;
            } else if (v1.m_major > v2.m_major) {
                return true;
            } else if (v1.m_minor < v2.m_minor) {
                return false;
            } else if (v1.m_minor > v2.m_minor) {
                return true;
            } else if (v1.m_patch < v2.m_patch) {
                return false;
            } else if (v1.m_patch > v2.m_patch) {
                return true;
            } else if (v1.m_postType < v2.m_postType) {
                return false;
            } else if (v1.m_postType > v2.m_postType) {
                return true;
            } else if (v1.m_post < v2.m_post) {
                return false;
            } else if (v1.m_post > v2.m_post) {
                return true;
            } else if (v1.m_dev < v2.m_dev) {
                return false;
            } else if (v1.m_dev > v2.m_dev) {
                return true;
            }
            // here is the == case
            return false;
        }

        bool operator!=(const karabo::util::Version &v1, const karabo::util::Version &v2) {
            return !(v1 == v2);
        }

        bool operator>=(const karabo::util::Version &v1, const karabo::util::Version &v2) {
            return (v1 > v2) || (v1 == v2);
        }

        bool operator<(const karabo::util::Version &v1, const karabo::util::Version &v2) {
            return (v2 > v1);
        }

        bool operator<=(const karabo::util::Version &v1, const karabo::util::Version &v2) {
            return (v2 > v1) || (v1 == v2);
        }

    } // namespace util
} // namespace karabo
