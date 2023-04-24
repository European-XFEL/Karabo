/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 10, 2014, 3:58 PM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_UTIL_VERSION_HH
#define KARABO_UTIL_VERSION_HH

#include <string>

namespace karabo {

    namespace util {

        /**
         * @class Version
         * @brief A class providing versioning information for the Karabo framework
         */
        class Version {
           private:
            enum PostfixType { ALPHA = -3, BETA = -2, RC = -1, NONE = 0, POST = 1 };

            std::string m_versionString;

            int m_major;

            int m_minor;

            int m_patch;

            PostfixType m_postType;

            int m_post;

            int m_dev;

            static Version &getInstance();

            static std::string getPathToVersionFile();

            void processString(const std::string &version);

            Version();

           public:
            /**
             * Gets a Version object of the curent Karabo's Framework
             *
             * @return Version object
             */
            static const Version &getKaraboVersion();

            /**
             * Creates an Version object from a string.
             *
             * The version string should match a Major.Minor.Patch flavor
             * Alpha, Beta, Release Candidates and Post-releases should be labeled
             * following the PEP440 guidelines.
             *
             * @param version
             */
            Version(const std::string &version);

            virtual ~Version(){};

            static std::string getPathToKaraboInstallation();

            /**
             * Returns a string describing the current version of the Framework
             * Equivalent of calling.
             * karabo::util::Version::getKaraboVersion().getString();
             *
             * @returns std::string
             */
            static std::string getVersion();

            int getMajor() const;

            int getMinor() const;

            int getPatch() const;

            const std::string &getString() const;

            bool isDevRelease() const;

            bool isPreRelease() const;

            bool isPostRelease() const;

            // the comparison operators implemented follow the guidelines of
            // PEP440 https://www.python.org/dev/peps/pep-0440/
            // When in doubt, the implementation of `distutils.version.LooseVersion`
            // was followed.
            friend bool operator==(const Version &v1, const Version &v2);
            friend bool operator!=(const Version &v1, const Version &v2);
            friend bool operator>(const Version &v1, const Version &v2);
            friend bool operator<=(const Version &v1, const Version &v2);
            friend bool operator<(const Version &v1, const Version &v2);
            friend bool operator>=(const Version &v1, const Version &v2);
        };
    } // namespace util
} // namespace karabo
#endif
