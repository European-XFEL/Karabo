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
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>
namespace bp = boost::python;


template <class T>
struct AliasAttributeWrap {

    static T & aliasPy(T& self, const bp::object& obj) {
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

///////////////////////////////////////////////////////////////////////////
//DefaultValue<SimpleElement< EType> > where EType:
//BOOL, INT32, UINT32, INT64, UINT64, STRING, DOUBLE
#define KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(U , EType, e)\
{\
typedef DefaultValue< U, EType > DefValue;\
bp::class_< DefValue, boost::noncopyable > ("DefaultValue"#e, bp::no_init)\
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
, bp::return_internal_reference<> () )\
;\
}

///////////////////////////////////////////////////////////////////////////
//DefaultValue<VectorElement< EType, std::vector >, std::vector< EType > > where EType:
//BOOL, INT32, UINT32, INT64, UINT64, STRING, DOUBLE
#define KARABO_PYTHON_VECTOR_DEFAULT_VALUE(t, e)\
{\
typedef t EType;\
typedef std::vector< EType > VType;\
typedef karabo::util::VectorElement< EType, std::vector> U;\
typedef karabo::util::DefaultValue< U, VType > DefValueVec;\
bp::class_< DefValueVec, boost::noncopyable > ("DefaultValueVector"#e, bp::no_init)\
.def("defaultValue"\
, &karabo::karathon::DefaultValueVectorWrap<EType>::pyList2VectorDefaultValue\
, (bp::arg("defValueVec"), bp::arg("defaultValuePyList")))\
.def("defaultValueFromString"\
, (U & (DefValueVec::*)(std::string const &))(&DefValueVec::defaultValueFromString)\
, (bp::arg("defaultValue"))\
, bp::return_internal_reference<> ())\
.def("noDefaultValue"\
, (U & (DefValueVec::*)())(&DefValueVec::noDefaultValue)\
, bp::return_internal_reference<> ())\
;\
}

/////////////////////////////////////////////////////////////
#define KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(U, EType, e)\
{\
typedef ReadOnlySpecific< U, EType > ReadOnlySpec;\
bp::class_< ReadOnlySpec, boost::noncopyable >( "ReadOnlySpecific"#e, bp::no_init)\
.def("alarmHigh"\
, (ReadOnlySpec & (ReadOnlySpec::*)(EType const &))( &ReadOnlySpec::alarmHigh)\
, bp::return_internal_reference<> () )\
.def("alarmLow"\
, (ReadOnlySpec & (ReadOnlySpec::*)(EType const &))( &ReadOnlySpec::alarmLow)\
, bp::return_internal_reference<> () )\
.def("warnHigh"\
, (ReadOnlySpec & (ReadOnlySpec::*)(EType const &))( &ReadOnlySpec::warnHigh)\
, bp::return_internal_reference<> () )\
.def("warnLow"\
, (ReadOnlySpec & (ReadOnlySpec::*)(EType const &))( &ReadOnlySpec::warnLow)\
, bp::return_internal_reference<> () )\
.def("initialValueFromString"\
, (ReadOnlySpec & (ReadOnlySpec::*)(std::string const &))( &ReadOnlySpec::initialValueFromString)\
, bp::return_internal_reference<> () )\
.def("initialValue"\
, (ReadOnlySpec & (ReadOnlySpec::*)(EType const &))( &ReadOnlySpec::initialValue)\
, bp::return_internal_reference<> () )\
.def("commit", (void (ReadOnlySpec::*)())(&ReadOnlySpec::commit))\
;\
}

/////////////////////////////////////////////////////////////
#define KARABO_PYTHON_VECTOR_READONLYSPECIFIC(t , e)\
{\
typedef t EType;\
typedef std::vector< EType > VType;\
typedef karabo::util::VectorElement< EType, std::vector> U;\
typedef karabo::util::ReadOnlySpecific< U, VType > ReadOnlySpecVec;\
bp::class_< ReadOnlySpecVec, boost::noncopyable > ("ReadOnlySpecificVector"#e, bp::no_init)\
.def("initialValueFromString"\
, (ReadOnlySpecVec & (ReadOnlySpecVec::*)(std::string const &))( &ReadOnlySpecVec::initialValueFromString)\
, bp::return_internal_reference<> () )\
.def("commit", (void (ReadOnlySpecVec::*)())(&ReadOnlySpecVec::commit))\
;\
}


/**
 * The following macro KARABO_PYTHON_SIMPLE
 * is used for python binding of
 * @code
 * karabo::util::SimpleElement< EType >
 * @endcode
 * where EType: int, long long, double.
 * In Python: INT32_ELEMENT, ..UINT32.., INT64_ELEMENT, ..UINT64..,
 * DOUBLE_ELEMENT, STRING_ELEMENT, BOOL_ELEMENT
 *
 */
#define KARABO_PYTHON_SIMPLE(t, e)\
{\
typedef t EType;\
typedef SimpleElement< EType > T;\
bp::implicitly_convertible< Schema &, T >();\
bp::class_< T, boost::noncopyable >( #e"_ELEMENT", bp::init< Schema & >((bp::arg("expected"))) )\
KARABO_PYTHON_COMMON_ATTRIBUTES \
KARABO_PYTHON_OPTIONS_NONVECTOR \
KARABO_PYTHON_NUMERIC_ATTRIBUTES \
;\
}

#define KARABO_PYTHON_COMMON_ATTRIBUTES \
.def("advanced", &T::advanced\
, bp::return_internal_reference<> () )\
.def("allowedStates", &T::allowedStates\
, ( bp::arg("states"), bp::arg("sep")=" ,;" )\
, bp::return_internal_reference<> ())\
.def("assignmentInternal", &T::assignmentInternal\
, bp::return_internal_reference<> () )\
.def("assignmentMandatory", &T::assignmentMandatory\
, bp::return_internal_reference<> () )\
.def("assignmentOptional", &T::assignmentOptional\
, bp::return_internal_reference<> () )\
.def("alias"\
, &AliasAttributeWrap<T>::aliasPy\
, bp::return_internal_reference<> ())\
.def("commit", &T::commit\
, bp::return_internal_reference<> () )\
.def("commit"\
, (T & (T::*)(karabo::util::Schema &))(&T::commit )\
, bp::arg("expected")\
, bp::return_internal_reference<> () )\
.def("description", &T::description\
, bp::return_internal_reference<> () )\
.def("displayedName", &T::displayedName\
, bp::return_internal_reference<> () )\
.def("unit", &T::unit\
, bp::return_internal_reference<> () )\
.def("metricPrefix", &T::metricPrefix\
, bp::return_internal_reference<> () )\
.def("init", &T::init\
, bp::return_internal_reference<> () )\
.def("key", &T::key\
, bp::return_internal_reference<> () )\
.def("readOnly", &T::readOnly\
, bp::return_internal_reference<> () )\
.def("reconfigurable", &T::reconfigurable\
, bp::return_internal_reference<> () )\
.def("tags"\
, (T & (T::*)(std::string const &, std::string const &))(&T::tags)\
, (bp::arg("tags"), bp::arg("sep")=" ,;")\
, bp::return_internal_reference<> ())\
.def("tags"\
, (T & (T::*)(std::vector<std::string> const &))(&T::tags)\
, (bp::arg("tags"))\
, bp::return_internal_reference<> ())\


#define KARABO_PYTHON_OPTIONS_NONVECTOR \
.def("options"\
, (T & (T::*)(std::string const &, std::string const &))(&T::options)\
, (bp::arg("opts"), bp::arg("sep")=" ,;")\
, bp::return_internal_reference<> ())\
.def("options"\
, (T & (T::*)(std::vector<std::string> const &))(&T::options)\
, (bp::arg("opts"))\
, bp::return_internal_reference<> ())


#define KARABO_PYTHON_NUMERIC_ATTRIBUTES \
.def("maxExc", &T::maxExc\
, bp::return_internal_reference<> ())\
.def("maxInc", &T::maxInc\
, bp::return_internal_reference<> ())\
.def("minExc", &T::minExc\
, bp::return_internal_reference<> ())\
.def("minInc", &T::minInc\
, bp::return_internal_reference<> ())


/**
 * The following macro KARABO_PYTHON_VECTOR is used for python binding of
 * @code
 * karabo::util::VectorElement< EType, std::vector >
 * @endcode
 * where EType: int, long long, double. 
 * In Python: VECTOR_INT32_ELEMENT, ..UINT32.., VECTOR_INT64_ELEMENT, ..UINT64..,
 *  VECTOR_DOUBLE_ELEMENT, VECTOR_STRING_ELEMENT, VECTOR_BOOL_ELEMENT.
 *
 */
#define KARABO_PYTHON_VECTOR(t, e)\
{\
typedef t EType;\
typedef VectorElement< EType, std::vector > T;\
bp::implicitly_convertible< Schema &, T >();\
bp::class_< T, boost::noncopyable >( "VECTOR_"#e"_ELEMENT", bp::init< karabo::util::Schema & >(( bp::arg("expected") )) )\
KARABO_PYTHON_COMMON_ATTRIBUTES \
.def("maxSize"\
, (T & ( T::*)(int const & )) (&T::maxSize)\
, bp::return_internal_reference<> () )\
.def("minSize"\
, (T & ( T::*)(int const & ))(&T::minSize)\
, bp::return_internal_reference<> () )\
;\
}

/**
 * The following macro KARABO_PYTHON_NODE_CHOICE_LIST is used for python binding of
 * @code
 * karabo::util::NodeElement, karabo::util::ListElement, karabo::util::ChoiceElement
 * @endcode
 *
 * In Python: NODE_ELEMENT, CHOICE_ELEMENT, LIST_ELEMENT
 *
 */
#define KARABO_PYTHON_NODE_CHOICE_LIST(NameElem)\
.def("advanced", &NameElem::advanced\
, bp::return_internal_reference<> ())\
.def("key", &NameElem::key\
, bp::return_internal_reference<> ())\
.def("description", &NameElem::description\
, bp::return_internal_reference<> ())\
.def("displayedName", &NameElem::displayedName\
, bp::return_internal_reference<> ())\
.def("alias"\
, &AliasAttributeWrap<NameElem>::aliasPy\
, bp::return_internal_reference<> ())\
.def("tags"\
, (NameElem & (NameElem::*)(std::string const &, std::string const &))(&NameElem::tags)\
, (bp::arg("tags"), bp::arg("sep") = " ,;")\
, bp::return_internal_reference<> ())\
.def("tags"\
, (NameElem & (NameElem::*)(std::vector<std::string> const &))(&NameElem::tags)\
, (bp::arg("tags"))\
, bp::return_internal_reference<> ())\
.def("commit", &NameElem::commit\
, bp::return_internal_reference<> ())\
.def("commit"\
, (NameElem & (NameElem::*)(karabo::util::Schema &))(&NameElem::commit)\
, bp::arg("expected")\
, bp::return_internal_reference<> ())

#define KARABO_PYTHON_PATH_ELEMENT(PathElem)\
{\
typedef PathElem T;\
bp::implicitly_convertible< Schema &, T >();\
bp::class_<T> ("PATH_ELEMENT", bp::init<Schema & >((bp::arg("expected"))))\
KARABO_PYTHON_COMMON_ATTRIBUTES \
KARABO_PYTHON_OPTIONS_NONVECTOR \
.def("isInputFile"\
, &PathElement::isInputFile\
, bp::return_internal_reference<> ())\
.def("isOutputFile"\
, &PathElement::isOutputFile\
, bp::return_internal_reference<> ())\
.def("isDirectory"\
, &PathElement::isDirectory\
, bp::return_internal_reference<> ())\
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

