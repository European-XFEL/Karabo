/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARATHON_MACROSFORPYTHON_HH
#define KARATHON_MACROSFORPYTHON_HH

#include <boost/python.hpp>
#include <boost/python/raw_function.hpp>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/NDArray.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/State.hh>
#include <karabo/util/TableElement.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/Validator.hh>
#include <karabo/util/VectorElement.hh>

#include "Wrapper.hh"


namespace bp = boost::python;

template <typename T>
struct AliasAttributeWrap {
    static T &aliasPy(T &self, const bp::object &obj) {
        if (PyLong_Check(obj.ptr())) {
            int param = bp::extract<int>(obj);
            return self.alias(param);
        } else if (PyUnicode_Check(obj.ptr())) {
            std::string param = bp::extract<std::string>(obj);
            return self.alias(param);
        } else if (PyFloat_Check(obj.ptr())) {
            double param = bp::extract<double>(obj);
            return self.alias(param);
        } else if (PyList_Check(obj.ptr())) {
            bp::ssize_t size = bp::len(obj);
            if (size == 0) {
                std::vector<std::string> params = std::vector<std::string>();
                return self.alias(params);
            }
            bp::object list0 = obj[0];
            if (list0.ptr() == Py_None) {
                std::vector<karabo::util::CppNone> v;
                for (bp::ssize_t i = 0; i < size; ++i) v.push_back(karabo::util::CppNone());
                return self.alias(v);
            }
            if (PyBool_Check(list0.ptr())) {
                std::vector<bool> v(size); // Special case here
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<bool>(obj[i]);
                }
                return self.alias(v);
            }
            if (PyLong_Check(list0.ptr())) {
                std::vector<int> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<int>(obj[i]);
                }
                return self.alias(v);
            }
            if (PyFloat_Check(list0.ptr())) {
                std::vector<double> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<double>(obj[i]);
                }
                return self.alias(v);
            }
            // XXX: Whoa! This check was already performed! -JW 14.7.2016
            if (PyLong_Check(list0.ptr())) {
                std::vector<long long> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<long>(obj[i]);
                }
                return self.alias(v);
            }
            if (PyUnicode_Check(list0.ptr())) {
                std::vector<std::string> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<std::string>(obj[i]);
                }
                return self.alias(v);
            }
            // XXX: Whoa! This check was ALSO already performed! -JW 14.7.2016
            if (PyUnicode_Check(list0.ptr())) {
                std::vector<std::string> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    bp::object str(bp::handle<>(PyUnicode_AsUTF8String(static_cast<bp::object>(obj[i]).ptr())));
                    Py_ssize_t size;
                    const char *data = PyUnicode_AsUTF8AndSize(str.ptr(), &size);
                    v[i] = std::string(data, size);
                }
                return self.alias(v);
            }
        }
        throw KARABO_PYTHON_EXCEPTION("Unknown data type of the 'alias' element");
    }
};

template <class T>
struct DefaultValueVectorWrap {
    typedef std::vector<T> VType;
    typedef karabo::util::VectorElement<T> U;
    typedef karabo::util::DefaultValue<U, VType> DefValueVec;

    static U &defaultValue(DefValueVec &self, const bp::object &obj) {
        if (PyList_Check(obj.ptr())) {
            VType v = karathon::Wrapper::fromPyListToStdVector<T>(obj);
            return self.defaultValue(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the defaultValue of VectorElement must be a list");
        }
    }
};

struct DefaultValueTableWrap {
    typedef std::vector<karabo::util::Hash> VType;
    typedef karabo::util::TableElement U;
    typedef karabo::util::TableDefaultValue<U> DefValueVec;

    static U &defaultValue(DefValueVec &self, const bp::object &obj) {
        if (PyList_Check(obj.ptr())) {
            VType v = karathon::Wrapper::fromPyListToStdVector<karabo::util::Hash>(obj);
            return self.defaultValue(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the defaultValue of TableElement must be a list");
        }
    }
};

template <class T>
struct ReadOnlySpecificVectorWrap {
    typedef std::vector<T> VType;
    typedef karabo::util::VectorElement<T> U;
    typedef karabo::util::ReadOnlySpecific<U, VType> ReadOnlySpecVec;
    typedef karabo::util::AlarmSpecific<U, VType, ReadOnlySpecVec> AlarmSpecVec;

    static ReadOnlySpecVec &initialValue(ReadOnlySpecVec &self, const bp::object &obj) {
        if (PyList_Check(obj.ptr())) {
            VType v = karathon::Wrapper::fromPyListToStdVector<T>(obj);
            return self.initialValue(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the initialValue of VectorElement must be a list");
        }
    }

    static AlarmSpecVec warnLowValue(ReadOnlySpecVec &self, const bp::object &obj) {
        if (PyList_Check(obj.ptr())) {
            VType v = karathon::Wrapper::fromPyListToStdVector<T>(obj);
            return self.warnLow(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the warnLow value of VectorElement must be a list");
        }
    }

    static AlarmSpecVec warnHighValue(ReadOnlySpecVec &self, const bp::object &obj) {
        if (PyList_Check(obj.ptr())) {
            VType v = karathon::Wrapper::fromPyListToStdVector<T>(obj);
            return self.warnHigh(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the warnHigh value of VectorElement must be a list");
        }
    }

    static AlarmSpecVec alarmLowValue(ReadOnlySpecVec &self, const bp::object &obj) {
        if (PyList_Check(obj.ptr())) {
            VType v = karathon::Wrapper::fromPyListToStdVector<T>(obj);
            return self.alarmLow(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the alarmLow value of VectorElement must be a list");
        }
    }

    static AlarmSpecVec alarmHighValue(ReadOnlySpecVec &self, const bp::object &obj) {
        if (PyList_Check(obj.ptr())) {
            VType v = karathon::Wrapper::fromPyListToStdVector<T>(obj);
            return self.alarmHigh(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the alarmHigh value of VectorElement must be a list");
        }
    }
};

struct NDArrayElementWrap {
    static karabo::util::NDArrayElement &dtype(karabo::util::NDArrayElement &self, const bp::object &obj) {
        using namespace karabo::util;
        Types::ReferenceType reftype = Types::UNKNOWN;

        if (bp::extract<std::string>(obj).check()) {
            std::string type = bp::extract<std::string>(obj);
            reftype = Types::from<FromLiteral>(type);
        } else if (bp::extract<karathon::PyTypes::ReferenceType>(obj).check()) {
            karathon::PyTypes::ReferenceType type = bp::extract<karathon::PyTypes::ReferenceType>(obj);
            reftype = karathon::PyTypes::to(type);
        } else {
            throw KARABO_PYTHON_EXCEPTION(
                  "Python type of dtype of NDArrayElement must be a string or Types enumerated value.");
        }
        return self.dtype(reftype);
    }

    static karabo::util::NDArrayElement &shape(karabo::util::NDArrayElement &self, const bp::object &obj) {
        if (PyUnicode_Check(obj.ptr())) {
            return self.shape(bp::extract<std::string>(obj));
        } else if (PyList_Check(obj.ptr())) {
            const std::vector<long long> v = karathon::Wrapper::fromPyListToStdVector<long long>(obj);
            const std::string shapeStr = karabo::util::toString<long long>(v);
            return self.shape(shapeStr);
        } else {
            throw KARABO_PYTHON_EXCEPTION(
                  "Python type of the shape value of NDArrayElement must be a list or a string");
        }
    }
};


///////////////////////////////////////////////////////////////////////////
// DefaultValue<SimpleElement< EType> > where EType:
// BOOL, INT32, UINT32, INT64, UINT64, STRING, DOUBLE
#define KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(U, EType, e)                                        \
    {                                                                                           \
        typedef DefaultValue<U, EType> DefValue;                                                \
        bp::class_<DefValue, boost::noncopyable>("DefaultValue" #e, bp::no_init)                \
              .def("defaultValue", (U & (DefValue::*)(EType const &))(&DefValue::defaultValue), \
                   (bp::arg("defaultValue")), bp::return_internal_reference<>())                \
              .def("defaultValueFromString",                                                    \
                   (U & (DefValue::*)(std::string const &))(&DefValue::defaultValueFromString), \
                   (bp::arg("defaultValue")), bp::return_internal_reference<>())                \
              .def("noDefaultValue", (U & (DefValue::*)())(&DefValue::noDefaultValue),          \
                   bp::return_internal_reference<>());                                          \
    }

///////////////////////////////////////////////////////////////////////////
// DefaultValue<VectorElement<T>, std::vector<T> >
#define KARABO_PYTHON_VECTOR_DEFAULT_VALUE(T, e)                                                                   \
    {                                                                                                              \
        typedef std::vector<T> VType;                                                                              \
        typedef karabo::util::VectorElement<T> U;                                                                  \
        typedef karabo::util::DefaultValue<U, VType> DefValueVec;                                                  \
        bp::class_<DefValueVec, boost::noncopyable>("DefaultValueVector" #e, bp::no_init)                          \
              .def("defaultValue", &DefaultValueVectorWrap<T>::defaultValue, (bp::arg("self"), bp::arg("pyList")), \
                   bp::return_internal_reference<>())                                                              \
              .def("defaultValueFromString",                                                                       \
                   (U & (DefValueVec::*)(std::string const &))(&DefValueVec::defaultValueFromString),              \
                   (bp::arg("defaultValueString")), bp::return_internal_reference<>())                             \
              .def("noDefaultValue", (U & (DefValueVec::*)())(&DefValueVec::noDefaultValue),                       \
                   bp::return_internal_reference<>());                                                             \
    }

/////////////////////////////////////////////////////////////
#define KARABO_PYTHON_ELEMENT_ALARMSPECIFIC(U, EType, Rtype, e)                                                     \
    {                                                                                                               \
        typedef Rtype<U, EType> ReturnSpec;                                                                         \
        typedef AlarmSpecific<U, EType, ReturnSpec> AlarmSpec;                                                      \
        bp::class_<AlarmSpec, boost::noncopyable>("AlarmSpecific" #e #Rtype, bp::no_init)                           \
              .def("needsAcknowledging", (ReturnSpec & (AlarmSpec::*)(const bool))(&AlarmSpec::needsAcknowledging), \
                   bp::return_internal_reference<>())                                                               \
              .def("info", (AlarmSpec & (AlarmSpec::*)(const std::string &))(&AlarmSpec::info),                     \
                   bp::return_internal_reference<>());                                                              \
    }


/////////////////////////////////////////////////////////////
#define KARABO_PYTHON_ELEMENT_ROLLINGSTATSPECIFIC(U, EType, e)                                                        \
    {                                                                                                                 \
        typedef ReadOnlySpecific<U, EType> ReadOnlySpec;                                                              \
        typedef RollingStatsSpecific<U, EType> RollingStatsSpec;                                                      \
        typedef AlarmSpecific<U, EType, RollingStatsSpec> AlarmSpec;                                                  \
        bp::class_<RollingStatsSpec, boost::noncopyable>("RollingStatsSpecific" #e, bp::no_init)                      \
              .def("warnVarianceLow",                                                                                 \
                   (AlarmSpec & (RollingStatsSpec::*)(const double))(&RollingStatsSpec::warnVarianceLow),             \
                   bp::return_internal_reference<>())                                                                 \
              .def("warnVarianceHigh",                                                                                \
                   (AlarmSpec & (RollingStatsSpec::*)(const double))(&RollingStatsSpec::warnVarianceHigh),            \
                   bp::return_internal_reference<>())                                                                 \
              .def("alarmVarianceLow",                                                                                \
                   (AlarmSpec & (RollingStatsSpec::*)(const double))(&RollingStatsSpec::alarmVarianceLow),            \
                   bp::return_internal_reference<>())                                                                 \
              .def("alarmVarianceHigh",                                                                               \
                   (AlarmSpec & (RollingStatsSpec::*)(const double))(&RollingStatsSpec::alarmVarianceHigh),           \
                   bp::return_internal_reference<>())                                                                 \
              .def("evaluationInterval",                                                                              \
                   (ReadOnlySpec & (RollingStatsSpec::*)(const unsigned int))(&RollingStatsSpec::evaluationInterval), \
                   bp::return_internal_reference<>());                                                                \
    }

/////////////////////////////////////////////////////////////
#define KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(U, EType, e)                                                        \
    {                                                                                                              \
        typedef ReadOnlySpecific<U, EType> ReadOnlySpec;                                                           \
        typedef AlarmSpecific<U, EType, ReadOnlySpec> AlarmSpec;                                                   \
        typedef RollingStatsSpecific<U, EType> RollingStatsSpec;                                                   \
        bp::class_<ReadOnlySpec, boost::noncopyable>("ReadOnlySpecific" #e, bp::no_init)                           \
              .def("alarmHigh", (AlarmSpec & (ReadOnlySpec::*)(EType const &))(&ReadOnlySpec::alarmHigh),          \
                   bp::return_internal_reference<>())                                                              \
              .def("alarmLow", (AlarmSpec & (ReadOnlySpec::*)(EType const &))(&ReadOnlySpec::alarmLow),            \
                   bp::return_internal_reference<>())                                                              \
              .def("warnHigh", (AlarmSpec & (ReadOnlySpec::*)(EType const &))(&ReadOnlySpec::warnHigh),            \
                   bp::return_internal_reference<>())                                                              \
              .def("warnLow", (AlarmSpec & (ReadOnlySpec::*)(EType const &))(&ReadOnlySpec::warnLow),              \
                   bp::return_internal_reference<>())                                                              \
              .def("enableRollingStats", &ReadOnlySpec::enableRollingStats, bp::return_internal_reference<>())     \
              .def("initialValueFromString",                                                                       \
                   (ReadOnlySpec & (ReadOnlySpec::*)(std::string const &))(&ReadOnlySpec::initialValueFromString), \
                   bp::return_internal_reference<>())                                                              \
              .def("initialValue", (ReadOnlySpec & (ReadOnlySpec::*)(EType const &))(&ReadOnlySpec::initialValue), \
                   bp::return_internal_reference<>())                                                              \
              .def("defaultValue", (ReadOnlySpec & (ReadOnlySpec::*)(EType const &))(&ReadOnlySpec::initialValue), \
                   bp::return_internal_reference<>())                                                              \
              .def("archivePolicy",                                                                                \
                   (ReadOnlySpec &                                                                                 \
                    (ReadOnlySpec::*)(karabo::util::Schema::ArchivePolicy const &))(&ReadOnlySpec::archivePolicy), \
                   bp::return_internal_reference<>())                                                              \
              .def("commit", (void(ReadOnlySpec::*)())(&ReadOnlySpec::commit));                                    \
    }

/////////////////////////////////////////////////////////////
#define KARABO_PYTHON_VECTOR_READONLYSPECIFIC(T, e)                                                                    \
    {                                                                                                                  \
        typedef std::vector<T> VType;                                                                                  \
        typedef karabo::util::VectorElement<T> U;                                                                      \
        typedef karabo::util::ReadOnlySpecific<U, VType> ReadOnlySpecVec;                                              \
        typedef karabo::util::AlarmSpecific<U, VType, ReadOnlySpecVec> AlarmSpecVec;                                   \
        typedef RollingStatsSpecific<U, VType> RollingStatsSpecArr;                                                    \
        bp::class_<ReadOnlySpecVec, boost::noncopyable>("ReadOnlySpecificVector" #e, bp::no_init)                      \
              .def("initialValue", &ReadOnlySpecificVectorWrap<T>::initialValue, (bp::arg("self"), bp::arg("pyList")), \
                   bp::return_internal_reference<>())                                                                  \
              .def("defaultValue", &ReadOnlySpecificVectorWrap<T>::initialValue, (bp::arg("self"), bp::arg("pyList")), \
                   bp::return_internal_reference<>())                                                                  \
              .def("alarmHigh", &ReadOnlySpecificVectorWrap<T>::alarmHighValue, (bp::arg("self"), bp::arg("pyList")))  \
              .def("alarmLow", &ReadOnlySpecificVectorWrap<T>::alarmLowValue, (bp::arg("self"), bp::arg("pyList")))    \
              .def("warnHigh", &ReadOnlySpecificVectorWrap<T>::warnHighValue, (bp::arg("self"), bp::arg("pyList")))    \
              .def("warnLow", &ReadOnlySpecificVectorWrap<T>::warnLowValue, (bp::arg("self"), bp::arg("pyList")))      \
              .def("enableRollingStats", (RollingStatsSpecArr(ReadOnlySpecVec::*)(std::string const &))(               \
                                               &ReadOnlySpecVec::enableRollingStats))                                  \
              .def("initialValueFromString",                                                                           \
                   (ReadOnlySpecVec &                                                                                  \
                    (ReadOnlySpecVec::*)(std::string const &))(&ReadOnlySpecVec::initialValueFromString),              \
                   bp::return_internal_reference<>())                                                                  \
              .def("archivePolicy",                                                                                    \
                   (ReadOnlySpecVec & (ReadOnlySpecVec::*)(karabo::util::Schema::ArchivePolicy const &))(              \
                         &ReadOnlySpecVec::archivePolicy),                                                             \
                   bp::return_internal_reference<>())                                                                  \
              .def("commit", (void(ReadOnlySpecVec::*)())(&ReadOnlySpecVec::commit));                                  \
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
#define KARABO_PYTHON_SIMPLE(t, e)                                                                  \
    {                                                                                               \
        typedef t EType;                                                                            \
        typedef SimpleElement<EType> T;                                                             \
        bp::implicitly_convertible<Schema &, T>();                                                  \
        bp::class_<T, boost::noncopyable>(#e "_ELEMENT", bp::init<Schema &>((bp::arg("expected")))) \
              KARABO_PYTHON_COMMON_ATTRIBUTES(T) KARABO_PYTHON_OPTIONS_NONVECTOR(T)                 \
                    KARABO_PYTHON_NUMERIC_ATTRIBUTES(T);                                            \
    }

template <typename T>
class CommonWrap {
   public:
    static bp::object allowedStatesPy(bp::tuple args, bp::dict kwargs) {
        T &self = bp::extract<T &>(args[0]);
        std::vector<karabo::util::State> states;
        for (unsigned int i = 1; i < bp::len(args); ++i) {
            const std::string state = bp::extract<std::string>(args[i].attr("name"));
            states.push_back(karabo::util::State::fromString(state));
        }
        self.allowedStates(states);
        return args[0];
    }
};

#define KARABO_PYTHON_COMMON_ATTRIBUTES(T)                                                                        \
    .def("observerAccess", &T::observerAccess, bp::return_internal_reference<>())                                 \
          .def("userAccess", &T::userAccess, bp::return_internal_reference<>())                                   \
          .def("operatorAccess", &T::operatorAccess, bp::return_internal_reference<>())                           \
          .def("expertAccess", &T::expertAccess, bp::return_internal_reference<>())                               \
          .def("adminAccess", &T::adminAccess, bp::return_internal_reference<>())                                 \
          .def("allowedStates", bp::raw_function(&CommonWrap<T>::allowedStatesPy, 2))                             \
          .def("assignmentInternal", &T::assignmentInternal, bp::return_internal_reference<>())                   \
          .def("assignmentMandatory", &T::assignmentMandatory, bp::return_internal_reference<>())                 \
          .def("assignmentOptional", &T::assignmentOptional, bp::return_internal_reference<>())                   \
          .def("alias", &AliasAttributeWrap<T>::aliasPy, bp::return_internal_reference<>())                       \
          .def("commit", &T::commit, bp::return_internal_reference<>())                                           \
          .def("commit", (T & (T::*)(karabo::util::Schema &))(&T::commit), bp::arg("expected"),                   \
               bp::return_internal_reference<>())                                                                 \
          .def("description", &T::description, bp::return_internal_reference<>())                                 \
          .def("displayedName", &T::displayedName, bp::return_internal_reference<>())                             \
          .def("unit", &T::unit, bp::return_internal_reference<>())                                               \
          .def("metricPrefix", &T::metricPrefix, bp::return_internal_reference<>())                               \
          .def("init", &T::init, bp::return_internal_reference<>())                                               \
          .def("key", &T::key, bp::return_internal_reference<>())                                                 \
          .def("setSpecialDisplayType", (T & (T::*)(std::string const &))(&T::setSpecialDisplayType),             \
               bp::arg("displayType"), bp::return_internal_reference<>())                                         \
          .def("readOnly", &T::readOnly, bp::return_internal_reference<>())                                       \
          .def("reconfigurable", &T::reconfigurable, bp::return_internal_reference<>())                           \
          .def("tags", (T & (T::*)(std::string const &, std::string const &))(&T::tags),                          \
               (bp::arg("tags"), bp::arg("sep") = " ,;"), bp::return_internal_reference<>())                      \
          .def("tags", (T & (T::*)(std::vector<std::string> const &))(&T::tags), (bp::arg("tags")),               \
               bp::return_internal_reference<>())                                                                 \
          .def("daqPolicy", (T & (T::*)(karabo::util::DAQPolicy const &))(&T::daqPolicy), (bp::arg("daqPolicy")), \
               bp::return_internal_reference<>())


#define KARABO_PYTHON_OPTIONS_NONVECTOR(T)                                                          \
    .def("options", (T & (T::*)(std::string const &, std::string const &))(&T::options),            \
         (bp::arg("opts"), bp::arg("sep") = " ,;"), bp::return_internal_reference<>())              \
          .def("options", (T & (T::*)(std::vector<EType> const &))(&T::options), (bp::arg("opts")), \
               bp::return_internal_reference<>())


#define KARABO_PYTHON_NUMERIC_ATTRIBUTES(T)                                             \
    .def("hex", &T::hex, bp::return_internal_reference<>())                             \
          .def("oct", &T::oct, bp::return_internal_reference<>())                       \
          .def("bin", (T & (T::*)()) & T::bin, bp::return_internal_reference<>())       \
          .def("bin", (T & (T::*)(const std::string &)) & T::bin, (bp::arg("meaning")), \
               bp::return_internal_reference<>())                                       \
          .def("maxExc", &T::maxExc, bp::return_internal_reference<>())                 \
          .def("maxInc", &T::maxInc, bp::return_internal_reference<>())                 \
          .def("minExc", &T::minExc, bp::return_internal_reference<>())                 \
          .def("minInc", &T::minInc, bp::return_internal_reference<>())


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
#define KARABO_PYTHON_VECTOR(t, e)                                                                              \
    {                                                                                                           \
        typedef t EType;                                                                                        \
        typedef VectorElement<EType, std::vector> T;                                                            \
        bp::implicitly_convertible<Schema &, T>();                                                              \
        bp::class_<T, boost::noncopyable>("VECTOR_" #e "_ELEMENT",                                              \
                                          bp::init<karabo::util::Schema &>((bp::arg("expected"))))              \
              KARABO_PYTHON_COMMON_ATTRIBUTES(T)                                                                \
                    .def("maxSize", (T & (T::*)(int const &))(&T::maxSize), bp::return_internal_reference<>())  \
                    .def("minSize", (T & (T::*)(int const &))(&T::minSize), bp::return_internal_reference<>()); \
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
#define KARABO_PYTHON_NODE_CHOICE_LIST(NameElem)                                                                     \
    .def("observerAccess", &NameElem::observerAccess, bp::return_internal_reference<>())                             \
          .def("userAccess", &NameElem::userAccess, bp::return_internal_reference<>())                               \
          .def("operatorAccess", &NameElem::operatorAccess, bp::return_internal_reference<>())                       \
          .def("expertAccess", &NameElem::expertAccess, bp::return_internal_reference<>())                           \
          .def("adminAccess", &NameElem::adminAccess, bp::return_internal_reference<>())                             \
          .def("key", &NameElem::key, bp::return_internal_reference<>())                                             \
          .def("description", &NameElem::description, bp::return_internal_reference<>())                             \
          .def("displayedName", &NameElem::displayedName, bp::return_internal_reference<>())                         \
          .def("alias", &AliasAttributeWrap<NameElem>::aliasPy, bp::return_internal_reference<>())                   \
          .def("tags", (NameElem & (NameElem::*)(std::string const &, std::string const &))(&NameElem::tags),        \
               (bp::arg("tags"), bp::arg("sep") = " ,;"), bp::return_internal_reference<>())                         \
          .def("tags", (NameElem & (NameElem::*)(std::vector<std::string> const &))(&NameElem::tags),                \
               (bp::arg("tags")), bp::return_internal_reference<>())                                                 \
          .def("commit", &NameElem::commit, bp::return_internal_reference<>())                                       \
          .def("commit", (NameElem & (NameElem::*)(karabo::util::Schema &))(&NameElem::commit), bp::arg("expected"), \
               bp::return_internal_reference<>())


// Macro KARABO_PYTHON_ANY_EXTRACT is used in pyexfel.cc and pyexfelportable.cc for binding.
// Extracting boost.python object with correct data type from boost::any
#define KARABO_PYTHON_ANY_EXTRACT(t)                             \
    if (self.type() == typeid(t)) {                              \
        return boost::python::object(boost::any_cast<t>(self));  \
    }                                                            \
    if (self.type() == typeid(std::vector<t>)) {                 \
        typedef std::vector<t> VContainer;                       \
        VContainer container(boost::any_cast<VContainer>(self)); \
        std::string str;                                         \
        str.append("[");                                         \
        for (size_t i = 0; i < container.size(); i++) {          \
            std::stringstream tmp;                               \
            tmp << container[i];                                 \
            str.append(tmp.str());                               \
            if (i < container.size() - 1) str.append(",");       \
        }                                                        \
        str.append("]");                                         \
        return boost::python::object(str);                       \
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

#define KARABO_PYTHON_IMAGE_ELEMENT(U)                                                                           \
    .def("description", (U & (U::*)(string const &))(&U::description), bp::arg("desc"),                          \
         bp::return_internal_reference<>())                                                                      \
          .def("displayedName", (U & (U::*)(string const &))(&U::displayedName), bp::arg("displayedName"),       \
               bp::return_internal_reference<>())                                                                \
          .def("key", (U & (U::*)(string const &))(&U::key), bp::arg("name"), bp::return_internal_reference<>()) \
          .def("alias", (U & (U::*)(int const &))(&U::alias), bp::return_internal_reference<>())                 \
          .def("observerAccess", &U::observerAccess, bp::return_internal_reference<>())                          \
          .def("userAccess", &U::userAccess, bp::return_internal_reference<>())                                  \
          .def("operatorAccess", &U::operatorAccess, bp::return_internal_reference<>())                          \
          .def("expertAccess", &U::expertAccess, bp::return_internal_reference<>())                              \
          .def("adminAccess", &U::adminAccess, bp::return_internal_reference<>())                                \
          .def("commit", (void(U::*)())(&U::commit))

#endif /* KARATHON_MACROSFORPYTHON_HH */
