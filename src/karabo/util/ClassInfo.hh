/*
 * $Id: ClassInfo.hh 4978 2012-01-18 10:43:06Z wegerk@DESY.DE $
 *
 * Author: <krzysztof.wrona@xfel.eu>
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

#ifndef KARABO_UTIL_CLASSINFO_HH
#define KARABO_UTIL_CLASSINFO_HH

#include <boost/current_function.hpp>
#include <memory>
#include <string>

#include "karaboDll.hh"

namespace karabo {
    namespace util {

        /**
         * @class ClassInfo
         * @brief ClassInfo holds meta-data to a factorized class
         */
        class KARABO_DECLSPEC ClassInfo {
           public:
            ClassInfo(const std::string& classId, const std::string& signature, const std::string& classVersion);
            /**
             * Return the C++ class name of this class
             * @return
             */
            const std::string& getClassName() const;

            /**
             * Return the C++ name space of this class
             * @return
             */
            const std::string& getNamespace() const;

            /**
             * Return the Karabo ClassId of this class
             * @return
             */
            const std::string& getClassId() const;

            /**
             * Return the LogCategory for this class
             * @return
             */
            const std::string& getLogCategory() const;

            /**
             * Return the version number of this class - currently does not
             * convey much meaning.
             * @return
             */
            const std::string& getVersion() const;

           private: // Functions
            void initClassNameAndSpace(const std::string& signature);
            void initLogCategory();

           private: // Members
            std::string m_classId;
            std::string m_namespace;
            std::string m_className;
            std::string m_logCategory;
            std::string m_configVersion;
        };
    } // namespace util
} // namespace karabo

// This macro should be called in header file, inside public section of the class declaration (see tests/BobbyCar.hh)
// It provides two methods which can be used for type introspection. The macro can be used in any class.
#define KARABO_CLASSINFO(className, classId, classVersion)                             \
    static karabo::util::ClassInfo classInfo() {                                       \
        return karabo::util::ClassInfo(classId, BOOST_CURRENT_FUNCTION, classVersion); \
    }                                                                                  \
    virtual karabo::util::ClassInfo getClassInfo() const {                             \
        return classInfo();                                                            \
    }                                                                                  \
    typedef className Self;                                                            \
    typedef std::shared_ptr<className> Pointer;                                        \
    typedef std::shared_ptr<const className> ConstPointer;                             \
    typedef std::weak_ptr<className> WeakPointer;


#endif
