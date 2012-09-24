/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_PYEXFEL_PYTHONLOADER_HH
#define	EXFEL_PYEXFEL_PYTHONLOADER_HH

#include <string>
#include <boost/shared_ptr.hpp>
// This is some stuff special to python
// TODO Double-check whether this approach here is correct
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif

#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#include <Python.h>
#include <boost/python.hpp>
#include <exfel/util/Hash.hh>

#ifdef EXFEL_TRACE_FLAG
#define TRACE(details) \
PythonLoader::trace(details);
#else
#define TRACE(details)
#endif

namespace exfel {
    namespace pyexfel {

        class PythonLoader {
        public:

            static void expectedParameters(exfel::util::Schema& expected, const std::string& interface);
            static std::string retrievePythonError();
            // just for internal tracing: trace
            static void trace(const std::string&);

            template <class T>
            static boost::shared_ptr<T> createInstance(const std::string& className) {

                PyObject* pName = PyString_FromString(className.c_str());
                PyObject* pModule = PyImport_Import(pName);

                TRACE("before Py_DECREF(pName)");
                Py_DECREF(pName);
                TRACE("after Py_DECREF(pName)");

                PyObject* pDict = PyModule_GetDict(pModule);
                PyObject* pClass = PyDict_GetItemString(pDict, className.c_str());
                TRACE("class " + className + " loaded");
                try {
                    return boost::python::call_method< boost::shared_ptr<T> > (pClass, "create");
                } catch (boost::python::error_already_set const &) {
                    std::string error = PythonLoader::retrievePythonError();
                    throw PYTHON_EXCEPTION("Failure in Python code (" + className + "). " + error);
                }
            }

        private:


        };
        
    }
}

#define EXFEL_PYTHON_FACTORY_TYPEDEFS(baseClass) \
typedef boost::shared_ptr< baseClass > (*create_function_type1)(Hash const &);\
typedef boost::shared_ptr< baseClass > (*create_function_type2)(string const &, Hash const &);\
typedef boost::shared_ptr< baseClass > (*createDefault_function_type)(string const &);\
typedef boost::shared_ptr< baseClass > (*createChoice_function_type)(string const &, Hash const &);\
typedef vector<boost::shared_ptr< baseClass > > (*createList_function_type)(string const &, Hash const &);\
typedef boost::shared_ptr< baseClass > (*createSingle_function_type)(string const &, string const &, Hash const &);\
typedef Schema(*expectedParameters_function_type1)(exfel::util::AccessType, string const &);\
typedef Schema(*expectedParameters_function_type2)(string const &, exfel::util::AccessType, string const &, string const &);\
typedef ClassInfo(*classInfo_function_type)();\
typedef ClassInfo(baseClass::*getClassInfo_function_type)() const;\
typedef Schema(*initialParameters_function_type1)(string const &, string const &, string const &);\
typedef Schema(*initialParameters_function_type2)();\
typedef Schema(*monitorableParameters_function_type1)(string const &, string const &, string const &);\
typedef Schema(*monitorableParameters_function_type2)();\
typedef Schema(*reconfigurableParameters_function_type1)(string const &, string const &, string const &);\
typedef Schema(*reconfigurableParameters_function_type2)();\
typedef void (*help_function_type)(std::string const &)

#define EXFEL_PYTHON_FACTORY_BINDING_BASE(baseClass) \
.def("create", create_function_type1(&baseClass::create), (bp::arg("config")))\
.def("create", create_function_type2(&baseClass::create), (bp::arg("classId"), bp::arg("parameters")=exfel::util::Hash() )).staticmethod("create")\
.def("createDefault", createDefault_function_type(&baseClass::createDefault), (bp::arg("classId"))).staticmethod("createDefault")\
.def("createChoice", createChoice_function_type(&baseClass::createChoice), (bp::arg("key"), bp::arg("input"))).staticmethod("createChoice")\
.def("createList", createList_function_type(&baseClass::createList), (bp::arg("key"), bp::arg("input"))).staticmethod("createList")\
.def("createSingle", createSingle_function_type(&baseClass::createSingle), (bp::arg("key"), bp::arg("classId"), bp::arg("input"))).staticmethod("createSingle")\
.def("getClassInfo", getClassInfo_function_type(&baseClass::getClassInfo))\
.def("classInfo", classInfo_function_type(&baseClass::classInfo)).staticmethod("classInfo")\
.def("expectedParameters", expectedParameters_function_type1(&baseClass::expectedParameters), (bp::arg("at")=exfel::util::INIT, bp::arg("currentState")=""))\
.def("expectedParameters", expectedParameters_function_type2(&baseClass::expectedParameters), (bp::arg("classId"), bp::arg("at")=exfel::util::INIT, bp::arg("currentState")="", bp::arg("displayedClassId")="")).staticmethod("expectedParameters")\
.def("initialParameters", initialParameters_function_type1(&baseClass::initialParameters), (bp::arg("classId"), bp::arg("currentState")="", bp::arg("displayedClassId")=""))\
.def("initialParameters", initialParameters_function_type2(&baseClass::initialParameters)).staticmethod("initialParameters")\
.def("monitorableParameters", monitorableParameters_function_type1(&baseClass::monitorableParameters), (bp::arg("classId"), bp::arg("currentState")="", bp::arg("displayedClassId")=""))\
.def("monitorableParameters", monitorableParameters_function_type2(&baseClass::monitorableParameters)).staticmethod("monitorableParameters")\
.def("reconfigurableParameters", reconfigurableParameters_function_type1(&baseClass::reconfigurableParameters), (bp::arg("classId"), bp::arg("currentState")="", bp::arg("displayedClassId")=""))\
.def("reconfigurableParameters", reconfigurableParameters_function_type2(&baseClass::reconfigurableParameters)).staticmethod("reconfigurableParameters")\
.def("help", help_function_type(&baseClass::help), ( bp::arg("classId")="" )).staticmethod("help")

#define EXFEL_PYTHON_FACTORY_DERIVED_BINDING(baseClass) \
.def("expectedParameters", &baseClass ## Py::expectedParameters).staticmethod("expectedParameters")\
.def("getClassInfo", getClassInfo_function_type(&baseClass ## Py::getClassInfo))

#endif	/* EXFEL_PYEXFEL_PYTHONLOADER_HH */

