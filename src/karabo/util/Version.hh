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

        /**
         * @class Version
         * @brief A class providing versioning information for the Karabo framework
         */
        class Version {

        private:

            enum PostfixType { ALPHA=-3, BETA=-2, RC=-1 , NONE=0, POST=1};

            std::string m_versionString;

            int m_major;

            int m_minor;

            int m_patch;

            PostfixType m_postType;

            int m_post;

            int m_dev;

            static Version& getInstance();

            static std::string getPathToVersionFile();

        public:

            Version();

            Version(const std::string &version);

            virtual ~Version() {
            };

            static std::string getPathToKaraboInstallation();

            static std::string getVersion();

            int getMajor();

            int getMinor();

            int getPatch();

            bool isDevRelease();
            
            bool isPreRelease();
            
            bool isPostRelease();

            friend bool operator== (const Version &v1, const Version &v2);
            friend bool operator!= (const Version &v1, const Version &v2);
            friend bool operator> (const Version &v1, const Version &v2);
            friend bool operator<= (const Version &v1, const Version &v2);
            friend bool operator< (const Version &v1, const Version &v2);
            friend bool operator>= (const Version &v1, const Version &v2);

        };
    }    
}
#endif
