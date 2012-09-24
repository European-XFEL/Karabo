/*
 * $Id: ClassInfo.hh 4978 2012-01-18 10:43:06Z wegerk@DESY.DE $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_CLASSINFO_HH
#define	EXFEL_UTIL_CLASSINFO_HH

#include <string>
#include <boost/current_function.hpp>

#include "utildll.hh"

namespace exfel {
  namespace util {

    class DECLSPEC_UTIL ClassInfo {
    public:

      ClassInfo(const std::string& classId, const std::string& signature, const std::string& classVersion);
      const std::string& getClassName() const;
      const std::string& getNamespace() const;
      const std::string& getClassId() const;
      const std::string& getLogCategory() const;
      const std::string& getConfigVersion() const;


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

// This macro should be called in header file, inside public section of the class declaration (see tests/BobbyCar.hh)
// It provides two methods which can be used for type introspection. The macro can be used in any class.
#define EXFEL_CLASSINFO(className, classId, classVersion)\
static exfel::util::ClassInfo classInfo() { return exfel::util::ClassInfo(classId, BOOST_CURRENT_FUNCTION, classVersion); } \
virtual exfel::util::ClassInfo getClassInfo() const { return classInfo(); } \
typedef className Self;


  }
}


#endif	/* EXFEL_UTIL_CLASSINFO_HH */

