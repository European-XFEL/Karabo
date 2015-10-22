/* 
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 10, 2014, 3:58 PM
 * 
 * Copyright (c) 2010-2015 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_VERSION_HH
#define	KARABO_UTIL_VERSION_HH

namespace karabo {

    namespace util {

        class Version {

            std::string m_versionString;

            int m_major;

            int m_minor;

            int m_patch;

            int m_revision;

            Version();

            virtual ~Version() {
            };

            static Version& getInstance();
            
            static std::string getPathToVersionFile();

        public:

            static std::string getPathToKaraboInstallation();

            static std::string getVersion();

            static int getMajor();

            static int getMinor();

            static int getPatch();

            static int getRevision();

        };
    }
}
#endif

