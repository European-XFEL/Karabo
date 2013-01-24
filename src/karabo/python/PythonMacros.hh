/* 
 * $Id$
 * 
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_PYKARABO_MACROSFORPYTHON_HH
#define	KARABO_PYKARABO_MACROSFORPYTHON_HH

#include <boost/python.hpp>
#include <karabo/util/GenericElement.hh>
namespace bp = boost::python;
        
template <class U, class EType>
struct GenericElementWrap{            
    static U & aliasPy(karabo::util::GenericElement<U, EType>& self, const bp::object& obj) {
        
        if (PyInt_Check(obj.ptr())) {
            int param = bp::extract<int>(obj);
            return self.alias(param);
        } else if (PyString_Check(obj.ptr())) {
            std::string param = bp::extract<std::string>(obj);
            return self.alias(param);
        } else if (PyFloat_Check(obj.ptr())) {
            double param = bp::extract<double>(obj);
            return self.alias(param);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Unknown data type of the 'alias' element");
        }
    }
};

#define KARABO_PYTHON_DEFAULT_VALUE \
.def("defaultValue"\
, (U & ( DefValue::* )( EType const & ) )( &DefValue::defaultValue )\
, ( bp::arg("defaultValue") )\
, bp::return_internal_reference<> () )\
.def("defaultValueFromString"\
, (U & ( DefValue::* )( std::string const & ) )( &DefValue::defaultValueFromString )\
, ( bp::arg("defaultValue") )\
, bp::return_internal_reference<> () )\
.def("noDefaultValue"\
, (U & ( DefValue::* )(  ) )( &DefValue::noDefaultValue )\
, bp::return_internal_reference<> () )

#define KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(t , e)\
{\
typedef t EType;\
typedef SimpleElement< EType > U;\
typedef DefaultValue< U, EType > DefValue;\
bp::class_< DefValue, boost::noncopyable > ("DefaultValueSimpleElement"#e, bp::no_init)\
KARABO_PYTHON_DEFAULT_VALUE\
;\
}
////////////////////////////////////////////////////////////////////

/**
 * The following three macros:
 * KARABO_PYTHON_GENERIC_SIMPLE_TYPES
 * KARABO_PYTHON_GENERIC_ELEMENT_DEFS
 * KARABO_PYTHON_GENERIC_SIMPLE
 * are used for python binding of
 * @code
 * ::karabo::util::GenericElement<karabo::util::SimpleElement< EType >, EType >
 * @endcode
 * where EType: int, long long, double, string, bool
 * 
 */
#define KARABO_PYTHON_GENERIC_SIMPLE_TYPES( U, EType ) \
typedef karabo::util::GenericElement< U, EType > T;\
bp::implicitly_convertible< karabo::util::Schema &, T >();\
typedef U & ( T::*advanced_function_type )(  ) ;\
typedef U & ( T::*allowedStates_function_type )( std::string const & ) ;\
typedef karabo::util::DefaultValue< U, EType > & ( T::*assignmentInternal_function_type )(  ) ;\
typedef U & ( T::*assignmentMandatory_function_type )(  ) ;\
typedef karabo::util::DefaultValue< U, EType > & ( T::*assignmentOptional_function_type )(  ) ;\
typedef T & ( T::*commit_function_type )(  ) ;\
typedef U & ( T::*description_function_type )( std::string const & ) ;\
typedef U & ( T::*displayedName_function_type )( std::string const & ) ;\
typedef U & ( T::*unitName_function_type )( std::string const & ) ;\
typedef U & ( T::*unitSymbol_function_type )( std::string const & ) ;\
typedef U & ( T::*init_function_type )(  ) ;\
typedef U & ( T::*key_function_type )( ::std::string const & ) ;\
typedef U & ( T::*readOnly_function_type )(  ) ;\
typedef U & ( T::*reconfigurable_function_type )(  )

#define KARABO_PYTHON_GENERIC_ELEMENT_DEFS \
.def("advanced"\
, advanced_function_type( &T::advanced )\
, bp::return_internal_reference<> () )\
.def("allowedStates"\
, allowedStates_function_type( &T::allowedStates )\
, bp::return_internal_reference<> () )\
.def("assignmentInternal"\
, assignmentInternal_function_type( &T::assignmentInternal )\
, bp::return_internal_reference<> () )\
.def("assignmentMandatory"\
, assignmentMandatory_function_type( &T::assignmentMandatory )\
, bp::return_internal_reference<> () )\
.def("assignmentOptional"\
, assignmentOptional_function_type( &T::assignmentOptional )\
, bp::return_internal_reference<> () )\
.def("alias"\
, &GenericElementWrap<U, EType>::aliasPy\
, bp::return_internal_reference<> ())\
.def("commit"\
, commit_function_type( &T::commit )\
, bp::return_internal_reference<> () )\
.def("commit"\
, (T & (T::*)(karabo::util::Schema &))(&T::commit )\
, bp::arg("expected")\
, bp::return_internal_reference<> () )\
.def("description"\
, description_function_type( &T::description )\
, ( bp::arg("desc") )\
, bp::return_internal_reference<> () )\
.def("displayedName"\
, displayedName_function_type( &T::displayedName )\
, ( bp::arg("name") )\
, bp::return_internal_reference<> () )\
.def("displayType"\
, (U & (T::*)( std::string const & ))(&T::displayType)\
, bp::arg("type")\
, bp::return_internal_reference<> () )\
.def("unitName"\
, unitName_function_type( &T::unitName )\
, ( bp::arg("unitName") )\
, bp::return_internal_reference<> () )\
.def("unitSymbol"\
, unitSymbol_function_type( &T::unitSymbol )\
, ( bp::arg("unitSymbol") )\
, bp::return_internal_reference<> () )\
.def("init"\
, init_function_type( &T::init )\
, bp::return_internal_reference<> () )\
.def("key"\
, key_function_type( &T::key )\
, ( bp::arg("name") )\
, bp::return_internal_reference<> () )\
.def("readOnly"\
, readOnly_function_type( &T::readOnly )\
, bp::return_internal_reference<> () )\
.def("reconfigurable"\
, reconfigurable_function_type( &T::reconfigurable )\
, bp::return_internal_reference<> () )

#define KARABO_PYTHON_GENERIC_SIMPLE(t, e)\
{\
typedef t EType;\
typedef SimpleElement< EType > U;\
KARABO_PYTHON_GENERIC_SIMPLE_TYPES(U, EType);\
bp::class_< GenericElement< U, EType >, boost::noncopyable > ("GenericElementSimpleElement"#e, bp::init< Schema & >((bp::arg("expected"))))\
KARABO_PYTHON_GENERIC_ELEMENT_DEFS \
;\
}

///////////////////////////////////////////////////////////

/**
 * The following two macros:
 * KARABO_PYTHON_GENERIC_VECTOR_TYPES
 * KARABO_PYTHON_GENERIC_VECTOR
 * are used for python binding of
 * @code
 * karabo::util::GenericElement< karabo::util::VectorElement< EType, std::vector >, std::vector<EType> >
 * @endcode
 * where EType: int, long long, double, string
 * and special case of 'bool' type: 
 * @code
 * karabo::util::GenericElement< karabo::util::VectorElement< bool, std::deque >, std::deque< bool > >
 * @endcode
 */
#define KARABO_PYTHON_GENERIC_VECTOR_TYPES \
bp::implicitly_convertible< karabo::util::Schema &, T >();\
typedef U & (T::*advanced_function_type)();\
typedef U & (T::*allowedStates_function_type )( std::string const & ) ;\
typedef ::karabo::util::DefaultValue< U, VType > & (T::*assignmentInternal_function_type)();\
typedef U & (T::*assignmentMandatory_function_type)();\
typedef ::karabo::util::DefaultValue< U, VType > & (T::*assignmentOptional_function_type)();\
typedef ::karabo::util::GenericElement< U, VType > & (T::*commit_function_type)();\
typedef U & (T::*description_function_type)(::std::string const &);\
typedef U & (T::*unitName_function_type)(::std::string const &);\
typedef U & (T::*unitSymbol_function_type)(::std::string const &);\
typedef U & (T::*displayedName_function_type)(::std::string const &);\
typedef U & (T::*init_function_type)();\
typedef U & (T::*key_function_type)(::std::string const &);\
typedef U & (T::*readOnly_function_type)();\
typedef U & (T::*reconfigurable_function_type)();

#define KARABO_PYTHON_GENERIC_VECTOR(t, e)\
{\
typedef t EType;\
typedef karabo::util::VectorElement< EType, std::vector > U;\
typedef std::vector< EType > VType;\
typedef karabo::util::GenericElement< U, VType > T;\
KARABO_PYTHON_GENERIC_VECTOR_TYPES \
bp::class_< karabo::util::GenericElement< U, std::vector< EType > >, boost::noncopyable >( "GenericElementVector"#e, bp::init< Schema & >(( bp::arg("expected") )) )\
KARABO_PYTHON_GENERIC_ELEMENT_DEFS \
;\
}

/**
 * The following macro KARABO_PYTHON_SIMPLE
 * is used for python binding of
 * @code
 * karabo::util::SimpleElement< EType >
 * @endcode
 * where EType: int, long long, double.
 * In Python: INT32_ELEMENT, INT64_ELEMENT, DOUBLE_ELEMENT
 * (Bindings STRING_ELEMENT, BOOL_ELEMENT defined in PyUtilSchema.cc)
 *
 */
#define KARABO_PYTHON_SIMPLE(t, e)\
{\
typedef t EType;\
typedef SimpleElement< EType > T;\
bp::implicitly_convertible< Schema &, T >();\
typedef T & (T::*maxExc_function_type)(EType const &);\
typedef T & (T::*maxInc_function_type)(EType const &);\
typedef T & (T::*minExc_function_type)(EType const &);\
typedef T & (T::*minInc_function_type)(EType const &);\
typedef T & (T::*options_function_type1)(::std::string const &, ::std::string const &);\
typedef T & (T::*options_function_type2)(::std::vector< std::string > const &);\
bp::class_< T, bp::bases< GenericElement< T, EType > >, boost::noncopyable >( #e"_ELEMENT", bp::init< Schema & >(( bp::arg("expected") )) )\
.def("maxExc"\
, maxExc_function_type(&T::maxExc)\
, (bp::arg("val"))\
, bp::return_internal_reference<> ())\
.def("maxInc"\
, maxInc_function_type(&T::maxInc)\
, (bp::arg("val"))\
, bp::return_internal_reference<> ())\
.def("minExc"\
, minExc_function_type(&T::minExc)\
, (bp::arg("val"))\
, bp::return_internal_reference<> ())\
.def("minInc"\
, minInc_function_type(&T::minInc)\
, (bp::arg("val"))\
, bp::return_internal_reference<> ())\
.def("options"\
, options_function_type1(&T::options)\
, (bp::arg("opts"), bp::arg("sep"))\
, bp::return_internal_reference<> ())\
.def("options"\
, options_function_type2(&T::options)\
, (bp::arg("opts"))\
, bp::return_internal_reference<> ())\
;\
}

/**
 * The following macro KARABO_PYTHON_VECTOR is used for python binding of
 * @code
 * karabo::util::VectorElement< EType, std::vector >
 * @endcode
 * where EType: int, long long, double. 
 * In Python: VECTOR_INT32_ELEMENT, VECTOR_INT64_ELEMENT, VECTOR_DOUBLE_ELEMENT.
 * (Bindings VECTOR_STRING_ELEMENT, VECTOR_BOOL_ELEMENT defined in PyUtilSchema.cc)
 *
 */
#define KARABO_PYTHON_VECTOR(t, e)\
{\
typedef t EType;\
typedef std::vector< EType > VType;\
typedef VectorElement< EType, std::vector > T;\
bp::implicitly_convertible< Schema &, T >();\
typedef T & ( T::*maxExc_function_type )( EType const & ) ;\
typedef T & ( T::*maxInc_function_type )( EType const & ) ;\
typedef T & ( T::*maxSize_function_type )( int const & ) ;\
typedef T & ( T::*minExc_function_type )( EType const & ) ;\
typedef T & ( T::*minInc_function_type )( EType const & ) ;\
typedef T & ( T::*minSize_function_type )( int const & ) ;\
bp::class_< T, bp::bases< GenericElement< T, VType > >, boost::noncopyable >( "VECTOR_"#e"_ELEMENT", bp::init< karabo::util::Schema & >(( bp::arg("expected") )) )\
.def("maxExc"\
, maxExc_function_type( &T::maxExc )\
, bp::return_internal_reference<> () )\
.def("maxInc"\
, maxInc_function_type( &T::maxInc )\
, bp::return_internal_reference<> () )\
.def("maxSize"\
, maxSize_function_type( &T::maxSize )\
, bp::return_internal_reference<> () )\
.def("minExc"\
, minExc_function_type( &T::minExc )\
, bp::return_internal_reference<> () )\
.def("minInc"\
, minInc_function_type( &T::minInc )\
, bp::return_internal_reference<> () )\
.def("minSize"\
, minSize_function_type( &T::minSize )\
, bp::return_internal_reference<> () )\
;\
}

//Macro KARABO_PYTHON_ANY_EXTRACT is used in pyexfel.cc and pyexfelportable.cc for binding.
//Extracting boost.python object with correct data type from boost::any
#define KARABO_PYTHON_ANY_EXTRACT(t)\
if (self.type() == typeid(t)) {\
return boost::python::object(boost::any_cast<t>(self));\
}\
if (self.type() == typeid(std::vector<t>)) {\
typedef std::vector<t> VContainer;\
VContainer container(boost::any_cast<VContainer > (self));\
std::string str;\
str.append("[");\
for (size_t i=0; i<container.size(); i++){\
std::stringstream tmp;\
tmp << container[i];\
str.append(tmp.str());\
if (i < container.size() - 1) str.append(",");\
}\
str.append("]");\
return boost::python::object(str);\
}

/**
 * The following macro KARABO_PYTHON_IMAGE_ELEMENT is used for python binding of
 * @code
 * karabo::util::ImageElement
 * @endcode
 * 
 * In Python: IMAGE_ELEMENT
 *
 */

#define KARABO_PYTHON_IMAGE_ELEMENT \
{\
typedef ImageElement<int> U;\
bp::implicitly_convertible< Schema &, U>();\
bp::class_<U>("IMAGE_ELEMENT", bp::init<Schema &>(bp::arg("expected")))\
.def("description"\
, (U & (U::*)(string const &))(&U::description)\
, bp::arg("desc")\
, bp::return_internal_reference<> () )\
.def("displayType"\
, (U & (U::*)(string const &))(&U::displayType)\
, bp::arg("type")\
, bp::return_internal_reference<> () )\
.def("displayedName"\
, (U & (U::*)(string const &))(&U::displayedName)\
, bp::arg("displayedName")\
, bp::return_internal_reference<> () )\
.def("key"\
, (U & (U::*)(string const &))(&U::key)\
, bp::arg("name")\
, bp::return_internal_reference<> () )\
.def("alias"\
, (U & (U::*)(int const &))(&U::alias)\
, bp::return_internal_reference<> ())\
.def("commit", (void (U::*)())(&U::commit))\
;\
}

/**
 * The following macro KARABO_PYTHON_TARGETACTUAL_ELEMENT is used for python binding of
 * @code
 * karabo::util::TargetActualElement< EType >
 * @endcode
 * where EType: int, double. 
 * In Python: INT32_TARGETACTUAL_ELEMENT, DOUBLE_TARGETACTUAL_ELEMENT.
 *
 */
#define KARABO_PYTHON_TARGETACTUAL_ELEMENT(t, e)\
{\
typedef t EType;\
typedef TargetActualElement<EType> U;\
bp::implicitly_convertible< Schema &, U>();\
bp::class_<U>(#e"_TARGETACTUAL_ELEMENT", bp::init<Schema &>(bp::arg("expected")))\
.def("commit", (void (U::*)())(&U::commit))\
.def("description"\
, (U & (U::*)(string const &))(&U::description)\
, bp::arg("desc")\
, bp::return_internal_reference<> () )\
.def("displayedName"\
, (U & (U::*)(string const &))(&U::displayedName)\
, bp::arg("displayedName")\
, bp::return_internal_reference<> () )\
.def("key"\
, (U & (U::*)(string const &))(&U::key)\
, bp::arg("name")\
, bp::return_internal_reference<> () )\
.def("unitName"\
, (U & (U::*)(string const &))(&U::unitName)\
, bp::arg("unitName")\
, bp::return_internal_reference<> () )\
.def("unitSymbol"\
, (U & (U::*)(string const &))(&U::unitSymbol)\
, bp::arg("unitSymbol")\
, bp::return_internal_reference<> () )\
.def("targetAssignmentMandatory"\
, (U & (U::*)())(&U::targetAssignmentMandatory)\
, bp::return_internal_reference<> () )\
.def("targetAssignmentOptional"\
, (U & (U::*)())(&U::targetAssignmentOptional)\
, bp::return_internal_reference<> () )\
.def("targetIsInitOnly"\
, (U & (U::*)())(&U::targetIsInitOnly)\
, bp::return_internal_reference<> () )\
.def("targetIsReconfigurable"\
, (U & (U::*)())(&U::targetIsReconfigurable)\
, bp::return_internal_reference<> () )\
.def("targetAllowedStates"\
, (U & (U::*)(string const &, string const &))(&U::targetAllowedStates)\
, (bp::arg("states"), bp::arg("sep") = ",")\
, bp::return_internal_reference<> () )\
.def("targetDefaultValue"\
, (U & (U::*)(EType const &))(&U::targetDefaultValue)\
, bp::arg("defaultValue")\
, bp::return_internal_reference<> () )\
.def("targetHardOptions"\
, (U & (U::*)(string const &, string const &))(&U::targetHardOptions)\
, (bp::arg("options"), bp::arg("sep") = ",;")\
, bp::return_internal_reference<> () )\
.def("targetHardMax"\
, (U & (U::*)(EType const &))(&U::targetHardMax)\
, bp::arg("value")\
, bp::return_internal_reference<> () )\
.def("targetHardMin"\
, (U & (U::*)(EType const &))(&U::targetHardMin)\
, bp::arg("value")\
, bp::return_internal_reference<> () )\
.def("targetConfigurableMax"\
, (U & (U::*)(EType const &))(&U::targetConfigurableMax)\
, bp::arg("value")\
, bp::return_internal_reference<> () )\
.def("targetConfigurableMin"\
, (U & (U::*)(EType const &))(&U::targetConfigurableMin)\
, bp::arg("value")\
, bp::return_internal_reference<> () )\
.def("actualWarnLow"\
, (U & (U::*)(EType const &))(&U::actualWarnLow)\
, bp::arg("warnLow")\
, bp::return_internal_reference<> () )\
.def("actualWarnHigh"\
, (U & (U::*)(EType const &))(&U::actualWarnHigh)\
, bp::arg("warnHigh")\
, bp::return_internal_reference<> () )\
.def("actualAlarmLow"\
, (U & (U::*)(EType const &))(&U::actualAlarmLow)\
, bp::arg("alarmLow")\
, bp::return_internal_reference<> () )\
.def("actualAlarmHigh"\
, (U & (U::*)(EType const &))(&U::actualAlarmHigh)\
, bp::arg("alarmHigh")\
, bp::return_internal_reference<> () )\
;\
}

#endif	/* KARABO_PYKARABO_MACROSFORPYTHON_HH */

