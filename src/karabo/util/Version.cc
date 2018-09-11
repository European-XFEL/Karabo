/* 
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 10, 2014, 3:58 PM
 * 
 * Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iosfwd>
#include <fstream>
#include <regex>
#include <sstream>
#include <boost/filesystem/operations.hpp>
#include "Version.hh"
#include "Exception.hh"
#include "StringTools.hh"

namespace karabo {

    namespace util {


        Version::Version() : m_major(-1), m_minor(-1), m_patch(-1), m_postType(PostfixType::NONE), m_post(-1), m_dev(-1) {

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
                if (!m_versionString.empty()) {
                    std::vector<int> v = fromString<int, std::vector>(m_versionString, ".");
                    if (v.size() > 0) m_major = v[0];
                    if (v.size() > 1) m_minor = v[1];
                    if (v.size() > 2) m_patch = v[2];
                }
            }
        }

        Version::Version(const std::string &version) : m_major(-1), m_minor(-1), m_patch(-1), 
            m_postType(PostfixType::NONE), m_post(-1), m_dev(-1) {
            //                       MANDATORY FIELDS       |OPTIONAL FIELDS                        |
            //                       Major   .Minor   .Patch(suffix         )(suf_n )(dev_suf)(dev_n)
            std::regex versionRegex("(\\d+)\\.(\\d+)\\.(\\d+)(a|b|rc|\\.post)?(\\d+)?(\\.dev)?(\\d+)?");
            std::smatch versionParts;
            if (std::regex_search(version, versionParts, versionRegex)) {
                // versionParts[0] is the full matched string.
                m_major = fromString<unsigned int>(versionParts[1]);
                m_minor = fromString<unsigned int>(versionParts[2]);
                m_patch = fromString<unsigned int>(versionParts[3]);
                // the strings below here might be empty since they belong to optional fields
                const std::string &separator = versionParts[4];
                const std::string &postVersion = versionParts[5];
                const std::string &devSeparator = versionParts[6];
                const std::string &devVersion = versionParts[7];
                if (separator ==  "a" && postVersion.size() > 0){
                    m_postType = PostfixType::ALPHA;
                    m_post = fromString<unsigned int>(postVersion);
                } else if (separator ==  "b" && postVersion.size() > 0) {
                    m_postType = PostfixType::BETA; 
                    m_post = fromString<unsigned int>(postVersion);
                } else if (separator ==  "rc" && postVersion.size() > 0) {
                    m_postType = PostfixType::RC;
                    m_post = fromString<unsigned int>(postVersion);
                } else if (separator ==  ".post" && postVersion.size() > 0) {
                    m_postType = PostfixType::POST;
                    m_post = fromString<unsigned int>(postVersion);
                } else if (separator ==  "" || postVersion.size()==0) {
                    m_postType = PostfixType::NONE;
                    m_post = 0;
                }
                if (devSeparator ==  ".dev" && devVersion.size() > 0) {
                    // development release
                    m_dev = fromString<unsigned int>(devVersion);
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
            return m_major;
        }

        int Version::getMinor() {
            return m_minor;
        }

        int Version::getPatch() {
            return m_patch;
        }

        bool Version::isDevRelease(){
            return m_dev != -1;
        }

        bool Version::isPreRelease(){
            return (m_postType == PostfixType::ALPHA || m_postType == PostfixType::BETA || m_postType == PostfixType::RC);
        }

        bool Version::isPostRelease(){
            return m_postType == PostfixType::POST;
        }
        
        bool operator== (const karabo::util::Version &v1, const karabo::util::Version &v2){
            if ( v1.m_major == v2.m_major && v1.m_minor == v2.m_minor && \
                v1.m_patch == v2.m_patch && v1.m_postType == v2.m_postType && \
                v1.m_post == v2.m_post && v1.m_dev == v2.m_dev) {
                return true;
            }
            return false;
        }

        bool operator!= (const karabo::util::Version &v1, const karabo::util::Version &v2){
            return !( v1 == v2);
        }

        bool operator>= (const karabo::util::Version &v1, const karabo::util::Version &v2){
            if ( v1 == v2 ){
                return true;
            }
            if ( v1.m_major < v2.m_major ) {
                return false;
            } else if ( v1.m_major > v2.m_major ) {
                return true;
            } else if (v1.m_minor < v2.m_minor ) {
                return false;
            } else if (v1.m_minor > v2.m_minor ) {
                return true;
            } else if (v1.m_patch < v2.m_patch) {
                return false;
            } else if (v1.m_patch > v2.m_patch) {
                return true;
            } else if (v1.m_postType < v2.m_postType ) {
                return false;
            } else if (v1.m_postType > v2.m_postType ) {
                return true;
            } else if (v1.m_post < v2.m_post) {
                return false;
            } else if (v1.m_post > v2.m_post) {
                return true;
            } else if (v1.m_dev < v2.m_dev) {
                return false;
            } else {
                return true;
            }
        }

        bool operator< (const karabo::util::Version &v1, const karabo::util::Version &v2){
            return !(v1 >= v2);
        }

        bool operator<= (const karabo::util::Version &v1, const karabo::util::Version &v2){
            return (v2 >= v1);
        }

        bool operator> (const karabo::util::Version &v1, const karabo::util::Version &v2){
            return !(v1 <= v2);
        }
    }
}
