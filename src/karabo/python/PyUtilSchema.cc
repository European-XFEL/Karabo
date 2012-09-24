/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <exfel/util/Factory.hh>
#include "PythonMacros.hh"
#include "DefaultValueVectorWrap.hh"

namespace bp = boost::python;
using namespace exfel::util;
using namespace std;
// FROM BH: Why does this binding look completely different to other ones. Why is the coding convention not followed?
// This code must urgently be cleaned up !!!


void exportPyUtilSchema() {

    {//exposing ::exfel::util::Schema

        bp::class_< Schema, bp::bases< Hash > > s("Schema", bp::init< >());

        bp::enum_< Schema::AssignmentType > ("AssignmentType")
                .value("OPTIONAL", Schema::OPTIONAL_PARAM)
                .value("MANDATORY", Schema::MANDATORY_PARAM)
                .value("INTERNAL", Schema::INTERNAL_PARAM)
                .export_values()
                ;
        bp::enum_< Schema::ExpertLevelType > ("ExpertLevelType")
                .value("SIMPLE", Schema::SIMPLE)
                .value("MEDIUM", Schema::MEDIUM)
                .value("ADVANCED", Schema::ADVANCED)
                .export_values()
                ;
        bp::enum_< Schema::OccuranceType > ("OccuranceType")
                .value("EXACTLY_ONCE", Schema::EXACTLY_ONCE)
                .value("ONE_OR_MORE", Schema::ONE_OR_MORE)
                .value("ZERO_OR_ONE", Schema::ZERO_OR_ONE)
                .value("ZERO_OR_MORE", Schema::ZERO_OR_MORE)
                .value("EITHER_OR", Schema::EITHER_OR)
                .export_values()
                ;

        s.def(bp::self_ns::str(bp::self));

        s.def("help"
                , &Schema::help
                , (bp::arg("classId") = ""));

        s.def("validate"
                , &Schema::validate
                , (bp::arg("user"), bp::arg("injectDefaults") = true, bp::arg("allowUnrootedConfiguration") = false, bp::arg("allowAdditionalKeys") = false, bp::arg("allowMissingKeys") = false));

        s.def("mergeUserInput"
                , &Schema::mergeUserInput);

        s.def("initParameterDescription"
                , &Schema::initParameterDescription
                , (bp::arg("key"), bp::arg("accessMode") = INIT | WRITE, bp::arg("currentState") = "")
                , bp::return_internal_reference<> ());

        s.def("addExternalSchema"
                , &Schema::addExternalSchema
                , bp::arg("params")
                , bp::return_internal_reference<> ());

        s.def("hasKey", (bool(Schema::*)(const string&)) (&Schema::hasKey), bp::arg("key"));

        s.def("getDescriptionByKey"
                , &Schema::getDescriptionByKey
                , bp::arg("key")
                , bp::return_value_policy<bp::copy_const_reference > ());

    }// end Schema

    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    //DefaultValue<SimpleElement< EType >, EType >, where EType:
    //INT32, UINT32, INT64, UINT64, DOUBLE, STRING, BOOL
    EXFEL_PYTHON_ELEMENT_DEFAULT_VALUE(int, INT32)
    EXFEL_PYTHON_ELEMENT_DEFAULT_VALUE(unsigned int, UINT32)
    EXFEL_PYTHON_ELEMENT_DEFAULT_VALUE(long long, INT64)
    EXFEL_PYTHON_ELEMENT_DEFAULT_VALUE(unsigned long long, UINT64)
    EXFEL_PYTHON_ELEMENT_DEFAULT_VALUE(double, DOUBLE)
    EXFEL_PYTHON_ELEMENT_DEFAULT_VALUE(std::string, STRING)
    EXFEL_PYTHON_ELEMENT_DEFAULT_VALUE(bool, BOOL)

    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    //DefaultValue<VectorElement< EType, std::vector >, std::vector< EType > > where EType:
    //BOOL, INT32, UINT32, INT64, UINT64, STRING, DOUBLE

    bp::class_< DefaultValue< VectorElement< bool, std::deque >, std::deque< bool > >, boost::noncopyable > ("DefaultValueVectorElementBOOL", bp::no_init)
            .def("defaultValue"
            , &exfel::pyexfel::DefaultValueVectorWrap<bool>::pyList2VectorDefaultValue
            , (bp::arg("defValueVec"), bp::arg("defaultValuePyList")))
            .def(
            "defaultValue"
            , (VectorElement< bool, std::deque > & (DefaultValue<VectorElement<bool, std::deque>, std::deque<bool, std::allocator<bool> > >::*)(std::deque< bool > const &))(&DefaultValue< exfel::util::VectorElement< bool, std::deque >, std::deque< bool > >::defaultValue)
            , (bp::arg("defaultValue"))
            , bp::return_internal_reference<> ())
            .def(
            "defaultValueFromString"
            , (VectorElement< bool, std::deque > & (DefaultValue<VectorElement<bool, std::deque>, std::deque<bool, std::allocator<bool> > >::*)(string const &))(&DefaultValue< exfel::util::VectorElement< bool, std::deque >, std::deque< bool > >::defaultValueFromString)
            , (bp::arg("defaultValue"))
            , bp::return_internal_reference<> ())
            .def(
            "noDefaultValue"
            , (VectorElement< bool, std::deque > & (DefaultValue<VectorElement<bool, std::deque>, std::deque<bool, std::allocator<bool> > >::*)())(&DefaultValue< exfel::util::VectorElement< bool, std::deque >, std::deque< bool > >::noDefaultValue)
            , bp::return_internal_reference<> ());

    EXFEL_PYTHON_VECTOR_DEFAULT_VALUE(int, INT32)
    EXFEL_PYTHON_VECTOR_DEFAULT_VALUE(unsigned int, UINT32)
    EXFEL_PYTHON_VECTOR_DEFAULT_VALUE(long long, INT64)
    EXFEL_PYTHON_VECTOR_DEFAULT_VALUE(unsigned long long, UINT64)
    EXFEL_PYTHON_VECTOR_DEFAULT_VALUE(double, DOUBLE)
    EXFEL_PYTHON_VECTOR_DEFAULT_VALUE(std::string, STRING)
    
    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    //::exfel::util::GenericElement<exfel::util::SimpleElement< EType >, EType > , where 
    //EType: int, unsigned int, long long, unsigned long long, double, string, bool

    EXFEL_PYTHON_GENERIC_SIMPLE(int, INT32)
    EXFEL_PYTHON_GENERIC_SIMPLE(unsigned int, UINT32)
    EXFEL_PYTHON_GENERIC_SIMPLE(long long, INT64)
    EXFEL_PYTHON_GENERIC_SIMPLE(unsigned long long, UINT64)
    EXFEL_PYTHON_GENERIC_SIMPLE(double, DOUBLE)
    EXFEL_PYTHON_GENERIC_SIMPLE(std::string, STRING)
    EXFEL_PYTHON_GENERIC_SIMPLE(bool, BOOL)

    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    //exfel::util::GenericElement< exfel::util::VectorElement< EType, std::vector >, std::vector< EType > >
    //EType: int, unsigned int, long long, unsigned long long, double, string, bool

    EXFEL_PYTHON_GENERIC_VECTOR(int, INT32)
    EXFEL_PYTHON_GENERIC_VECTOR(unsigned int, UINT32)
    EXFEL_PYTHON_GENERIC_VECTOR(long long, INT64)
    EXFEL_PYTHON_GENERIC_VECTOR(unsigned long long, UINT64)
    EXFEL_PYTHON_GENERIC_VECTOR(double, DOUBLE)
    EXFEL_PYTHON_GENERIC_VECTOR(std::string, STRING)
   {//::exfel::util::GenericElement< exfel::util::VectorElement< bool, std::deque >, std::deque< bool > >
        typedef bool EType;
        typedef VectorElement< EType, std::deque > U;
        typedef std::deque< EType > VType;
        typedef GenericElement< U, VType > T;
        EXFEL_PYTHON_GENERIC_VECTOR_TYPES
        bp::class_< T, boost::noncopyable > ("GenericElementVectorBOOL", bp::init< Schema & >((bp::arg("expected"))))
                EXFEL_PYTHON_GENERIC_ELEMENT_DEFS
                ;
    }

    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    //SimpleElement< EType >, where EType:
    //int, long long, double, string, bool
    //In Python: INT32_ELEMENT, UINT32_ELEMENT, INT64_ELEMENT, UINT64_ELEMENT, DOUBLE_ELEMENT

    EXFEL_PYTHON_SIMPLE(int, INT32)
    EXFEL_PYTHON_SIMPLE(unsigned int, UINT32)
    EXFEL_PYTHON_SIMPLE(long long, INT64)
    EXFEL_PYTHON_SIMPLE(unsigned long long, UINT64)
    EXFEL_PYTHON_SIMPLE(double, DOUBLE)

 { //exfel::util::SimpleElement< std::string >  In Python: STRING_ELEMENT
        typedef SimpleElement< std::string > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::string > >, boost::noncopyable > STRING_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T > ();
        typedef T & (T::*options_function_type1)(::std::string const &, ::std::string const &);
        typedef T & (T::*options_function_type2)(::std::vector< std::string > const &);
        STRING_ELEMENT_exposer_t("STRING_ELEMENT", bp::init< Schema & >((bp::arg("expected"))))
                .def(
                "options"
                , options_function_type1(&T::options)
                , (bp::arg("opts"), bp::arg("sep"))
                , bp::return_internal_reference<> ())
                .def(
                "options"
                , options_function_type2(&T::options)
                , (bp::arg("opts"))
                , bp::return_internal_reference<> ());
    }
    { //::exfel::util::SimpleElement< bool >   In Python: BOOL_ELEMENT
        typedef SimpleElement< bool > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, bool > >, boost::noncopyable > BOOL_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T > ();
        typedef T & (T::*options_function_type1)(::std::string const &, ::std::string const &);
        typedef T & (T::*options_function_type2)(::std::vector< std::string > const &);
        BOOL_ELEMENT_exposer_t("BOOL_ELEMENT", bp::init< Schema & >((bp::arg("expected"))))
                .def(
                "options"
                , options_function_type1(&T::options)
                , (bp::arg("opts"), bp::arg("sep"))
                , bp::return_internal_reference<> ())
                .def(
                "options"
                , options_function_type2(&T::options)
                , (bp::arg("opts"))
                , bp::return_internal_reference<> ());
    }
   //NOTE: binding for exfel::util::SimpleElement<exfel::util::Types::Any>   In Python: INTERNAL_ANY_ELEMENT
   //can be found in PyUtilSimpleAnyElement.cc 
    
    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
    // Binding ::exfel::util::VectorElement< EType, std::vector >, where EType:
    //
    // In Python : VECTOR_INT32_ELEMENT, VECTOR_UINT32_ELEMENT, VECTOR_INT64_ELEMENT, VECTOR_UINT64_ELEMENT, VECTOR_DOUBLE

    EXFEL_PYTHON_VECTOR(int, INT32)
    EXFEL_PYTHON_VECTOR(unsigned int, UINT32)
    EXFEL_PYTHON_VECTOR(long long, INT64)
    EXFEL_PYTHON_VECTOR(unsigned long long, UINT64)
    EXFEL_PYTHON_VECTOR(double, DOUBLE)

 { //::exfel::util::VectorElement< std::string, std::vector >  In Python : VECTOR_STRING_ELEMENT
        typedef VectorElement< std::string, std::vector > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::vector< std::string > > >, boost::noncopyable > VECTOR_STRING_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T > ();
        typedef T & (T::*maxSize_function_type)(int const &);
        typedef T & (T::*minSize_function_type)(int const &);
        VECTOR_STRING_ELEMENT_exposer_t("VECTOR_STRING_ELEMENT", bp::init< exfel::util::Schema & >((bp::arg("expected"))))
                .def(
                "maxSize"
                , maxSize_function_type(&T::maxSize)
                , bp::return_internal_reference<> ())
                .def(
                "minSize"
                , minSize_function_type(&T::minSize)
                , bp::return_internal_reference<> ())
                ;
    }
    { //::exfel::util::VectorElement< bool, std::deque > In Python : VECTOR_BOOL_ELEMENT
        typedef VectorElement< bool, std::deque > T;
        typedef bp::class_< T, bp::bases< GenericElement< T, std::deque< bool > > >, boost::noncopyable > VECTOR_BOOL_ELEMENT_exposer_t;
        bp::implicitly_convertible< Schema &, T > ();
        typedef T & (T::*maxSize_function_type)(int const &);
        typedef T & (T::*minSize_function_type)(int const &);
        VECTOR_BOOL_ELEMENT_exposer_t("VECTOR_BOOL_ELEMENT", bp::init< exfel::util::Schema & >((bp::arg("expected"))))
                .def(
                "maxSize"
                , maxSize_function_type(&T::maxSize)
                , bp::return_internal_reference<> ())
                .def(
                "minSize"
                , minSize_function_type(&T::minSize)
                , bp::return_internal_reference<> ())
                ;
     }

}//end  exportPyUtilSchema

