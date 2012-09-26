/* 
 * $Id$
 * 
 * Author: <irina.kozlova@xfel.eu>
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef PYTHONFACTORYMACROS_HH
#define	PYTHONFACTORYMACROS_HH

#include <boost/python.hpp>

namespace bp = boost::python;

#define KARABO_PYTHON_FACTORY_TYPEDEFS(baseClass) \
typedef boost::shared_ptr< baseClass > (*create_function_type1)(karabo::util::Hash const &);\
typedef boost::shared_ptr< baseClass > (*create_function_type2)(std::string const &, karabo::util::Hash const &);\
typedef boost::shared_ptr< baseClass > (*createDefault_function_type)(std::string const &);\
typedef boost::shared_ptr< baseClass > (*createChoice_function_type)(std::string const &, karabo::util::Hash const &);\
typedef vector<boost::shared_ptr< baseClass > > (*createList_function_type)(std::string const &, karabo::util::Hash const &);\
typedef boost::shared_ptr< baseClass > (*createSingle_function_type)(std::string const &, std::string const &, karabo::util::Hash const &);\
typedef karabo::util::ClassInfo(*classInfo_function_type)();\
typedef karabo::util::ClassInfo(baseClass::*getClassInfo_function_type)() const;\
typedef karabo::util::Schema(*expectedParameters_function_type1)(karabo::util::AccessType, std::string const &);\
typedef karabo::util::Schema(*expectedParameters_function_type2)(std::string const &, karabo::util::AccessType, std::string const &, std::string const &);\
typedef karabo::util::Schema(*initialParameters_function_type1)(std::string const &, std::string const &, std::string const &);\
typedef karabo::util::Schema(*initialParameters_function_type2)();\
typedef karabo::util::Schema(*monitorableParameters_function_type1)(std::string const &, std::string const &, std::string const &);\
typedef karabo::util::Schema(*monitorableParameters_function_type2)();\
typedef karabo::util::Schema(*reconfigurableParameters_function_type1)(std::string const &, std::string const &, std::string const &);\
typedef karabo::util::Schema(*reconfigurableParameters_function_type2)();\
typedef void (*help_function_type)(std::string const &)

#define KARABO_PYTHON_FACTORY_BINDING_BASE(baseClass) \
.def("create", create_function_type1(&baseClass::create), (bp::arg("config")))\
.def("create", create_function_type2(&baseClass::create), (bp::arg("classId"), bp::arg("parameters")=karabo::util::Hash() )).staticmethod("create")\
.def("createDefault", createDefault_function_type(&baseClass::createDefault), (bp::arg("classId"))).staticmethod("createDefault")\
.def("createChoice", createChoice_function_type(&baseClass::createChoice), (bp::arg("key"), bp::arg("input"))).staticmethod("createChoice")\
.def("createList", createList_function_type(&baseClass::createList), (bp::arg("key"), bp::arg("input"))).staticmethod("createList")\
.def("createSingle", createSingle_function_type(&baseClass::createSingle), (bp::arg("key"), bp::arg("classId"), bp::arg("input"))).staticmethod("createSingle")\
.def("classInfo", classInfo_function_type(&baseClass::classInfo)).staticmethod("classInfo")\
.def("getClassInfo", getClassInfo_function_type(&baseClass::getClassInfo))\
.def("expectedParameters", expectedParameters_function_type1(&baseClass::expectedParameters), (bp::arg("at")=karabo::util::INIT|karabo::util::WRITE, bp::arg("currentState")=""))\
.def("expectedParameters", expectedParameters_function_type2(&baseClass::expectedParameters), (bp::arg("classId"), bp::arg("at")=karabo::util::INIT|karabo::util::WRITE, bp::arg("currentState")="", bp::arg("displayedClassId")="")).staticmethod("expectedParameters")\
.def("initialParameters", initialParameters_function_type1(&baseClass::initialParameters), (bp::arg("classId"), bp::arg("currentState")="", bp::arg("displayedClassId")=""))\
.def("initialParameters", initialParameters_function_type2(&baseClass::initialParameters)).staticmethod("initialParameters")\
.def("monitorableParameters", monitorableParameters_function_type1(&baseClass::monitorableParameters), (bp::arg("classId"), bp::arg("currentState")="", bp::arg("displayedClassId")=""))\
.def("monitorableParameters", monitorableParameters_function_type2(&baseClass::monitorableParameters)).staticmethod("monitorableParameters")\
.def("reconfigurableParameters", reconfigurableParameters_function_type1(&baseClass::reconfigurableParameters), (bp::arg("classId"), bp::arg("currentState")="", bp::arg("displayedClassId")=""))\
.def("reconfigurableParameters", reconfigurableParameters_function_type2(&baseClass::reconfigurableParameters)).staticmethod("reconfigurableParameters")\
.def("help", help_function_type(&baseClass::help), ( bp::arg("classId")="" )).staticmethod("help")

#endif	/* PYTHONFACTORYMACROS_HH */