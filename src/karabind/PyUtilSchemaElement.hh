/*
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

#ifndef KARABIND_PYUTILSCHEMAELEMENT_HH
#define KARABIND_PYUTILSCHEMAELEMENT_HH

#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "PyTypes.hh"
#include "Wrapper.hh"
#include "karabo/data/schema/ByteArrayElement.hh"
#include "karabo/data/schema/ChoiceElement.hh"
#include "karabo/data/schema/GenericElement.hh"
#include "karabo/data/schema/LeafElement.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/OverwriteElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/TableElement.hh"
#include "karabo/data/schema/Validator.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/data/types/AlarmConditions.hh"
#include "karabo/data/types/FromLiteral.hh"
#include "karabo/data/types/HashFilter.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/data/types/State.hh"
#include "karabo/data/types/ToLiteral.hh"


namespace py = pybind11;

template <typename T>
struct AliasAttributeWrap {
    static T& aliasPy(T& self, const py::object& obj) {
        if (py::isinstance<py::int_>(obj)) {
            int param = obj.cast<int>();
            return self.alias(param);
        } else if (py::isinstance<py::str>(obj)) {
            std::string param = obj.cast<std::string>();
            return self.alias(param);
        } else if (py::isinstance<py::float_>(obj)) {
            double param = obj.cast<double>();
            return self.alias(param);
        } else if (py::isinstance<py::list>(obj)) {
            py::ssize_t size = py::len(obj);
            if (size == 0) {
                std::vector<std::string> params = std::vector<std::string>();
                return self.alias(params);
            }
            const std::vector<py::object> vobj = obj.cast<std::vector<py::object>>();
            py::object list0 = vobj[0];
            if (list0.is_none()) {
                std::vector<karabo::data::CppNone> v;
                for (py::ssize_t i = 0; i < size; ++i) v.push_back(karabo::data::CppNone());
                return self.alias(v);
            }
            if (py::isinstance<py::bool_>(list0)) {
                std::vector<bool> v(size); // Special case here
                for (py::ssize_t i = 0; i < size; ++i) v[i] = vobj[i].cast<bool>();
                return self.alias(v);
            }
            if (py::isinstance<py::int_>(list0)) {
                std::vector<long long> v(size);
                for (py::ssize_t i = 0; i < size; ++i) v[i] = vobj[i].cast<int>();
                return self.alias(v);
            }
            if (py::isinstance<py::float_>(list0)) {
                std::vector<double> v(size);
                for (py::ssize_t i = 0; i < size; ++i) v[i] = vobj[i].cast<double>();
                return self.alias(v);
            }
            if (py::isinstance<py::str>(list0)) {
                std::vector<std::string> v(size);
                for (py::ssize_t i = 0; i < size; ++i) v[i] = vobj[i].cast<std::string>();
                return self.alias(v);
            }
        }
        throw KARABO_PYTHON_EXCEPTION("Unknown data type of the 'alias' element");
    }
};

template <class T>
struct DefaultValueVectorWrap {
    typedef std::vector<T> VType;
    typedef karabo::data::VectorElement<T> U;
    typedef karabo::data::DefaultValue<U, VType> DefValueVec;

    static U& defaultValue(DefValueVec& self, const py::object& obj) {
        if (py::isinstance<py::sequence>(obj)) {
            const auto& seq = obj.cast<py::sequence>();
            return self.defaultValue(karabind::wrapper::castPySequenceToStdVector<T>(seq));
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the defaultValue of VectorElement must be a list");
        }
    }
};

template <class T>
struct ReadOnlySpecificVectorWrap {
    typedef std::vector<T> VType;
    typedef karabo::data::VectorElement<T> U;
    typedef karabo::data::ReadOnlySpecific<U, VType> ReadOnlySpecVec;

    static ReadOnlySpecVec& initialValue(ReadOnlySpecVec& self, const py::object& obj) {
        if (py::isinstance<py::list>(obj)) {
            py::list lst = obj.cast<py::list>();
            VType v = lst.cast<std::vector<T>>();
            return self.initialValue(v);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Python type of the initialValue of VectorElement must be a list");
        }
    }
};


///////////////////////////////////////////////////////////////////////////
// DefaultValue<SimpleElement< EType> > where EType:
// BOOL, INT32, UINT32, INT64, UINT64, STRING, DOUBLE
///////////////////////////////////////////////////////////////////////////

#define KARABO_PYTHON_ELEMENT_DEFAULT_VALUE(U, EType, e)                                                      \
    {                                                                                                         \
        typedef DefaultValue<U, EType> DefValue;                                                              \
        py::class_<DefValue>(m, "DefaultValue" #e)                                                            \
              .def("defaultValue", &DefValue::defaultValue, py::arg("defaultValue"),                          \
                   py::return_value_policy::reference_internal)                                               \
              .def("defaultValueFromString", &DefValue::defaultValueFromString, py::arg("defaultValue"),      \
                   py::return_value_policy::reference_internal)                                               \
              .def("noDefaultValue", &DefValue::noDefaultValue, py::return_value_policy::reference_internal); \
    }

///////////////////////////////////////////////////////////////////////////
// DefaultValue<VectorElement<T>, std::vector<T> >
///////////////////////////////////////////////////////////////////////////

#define KARABO_PYTHON_VECTOR_DEFAULT_VALUE(T, e)                                                                  \
    {                                                                                                             \
        typedef std::vector<T> VType;                                                                             \
        typedef karabo::data::VectorElement<T> U;                                                                 \
        typedef karabo::data::DefaultValue<U, VType> DefValueVec;                                                 \
        py::class_<DefValueVec>(m, "DefaultValueVector" #e)                                                       \
              .def("defaultValue", &DefaultValueVectorWrap<T>::defaultValue, py::arg("pyList"),                   \
                   py::return_value_policy::reference_internal)                                                   \
              .def("defaultValueFromString", &DefValueVec::defaultValueFromString, py::arg("defaultValueString"), \
                   py::return_value_policy::reference_internal)                                                   \
              .def("noDefaultValue", (U & (DefValueVec::*)())(&DefValueVec::noDefaultValue),                      \
                   py::return_value_policy::reference_internal);                                                  \
    }

/////////////////////////////////////////////////////////////


#define KARABO_PYTHON_ELEMENT_READONLYSPECIFIC(U, EType, e)                                                    \
    {                                                                                                          \
        typedef ReadOnlySpecific<U, EType> ReadOnlySpec;                                                       \
        py::class_<ReadOnlySpec>(m, "ReadOnlySpecific" #e)                                                     \
              .def("initialValueFromString", &ReadOnlySpec::initialValueFromString,                            \
                   py::return_value_policy::reference_internal)                                                \
              .def("initialValue", &ReadOnlySpec::initialValue, py::return_value_policy::reference_internal)   \
              .def("defaultValue", &ReadOnlySpec::initialValue, py::return_value_policy::reference_internal)   \
              .def("archivePolicy", &ReadOnlySpec::archivePolicy, py::return_value_policy::reference_internal) \
              .def("commit", &ReadOnlySpec::commit);                                                           \
    }

/////////////////////////////////////////////////////////////

#define KARABO_PYTHON_VECTOR_READONLYSPECIFIC(T, e)                                                               \
    {                                                                                                             \
        typedef std::vector<T> VType;                                                                             \
        typedef karabo::data::VectorElement<T> U;                                                                 \
        typedef karabo::data::ReadOnlySpecific<U, VType> ReadOnlySpecVec;                                         \
        py::class_<ReadOnlySpecVec>(m, "ReadOnlySpecificVector" #e)                                               \
              .def("initialValue", &ReadOnlySpecificVectorWrap<T>::initialValue, py::arg("pyList"),               \
                   py::return_value_policy::reference_internal)                                                   \
              .def("defaultValue", &ReadOnlySpecificVectorWrap<T>::initialValue, py::arg("pyList"),               \
                   py::return_value_policy::reference_internal)                                                   \
              .def("initialValueFromString", &ReadOnlySpecVec::initialValueFromString,                            \
                   py::return_value_policy::reference_internal)                                                   \
              .def("archivePolicy", &ReadOnlySpecVec::archivePolicy, py::return_value_policy::reference_internal) \
              .def("commit", &ReadOnlySpecVec::commit);                                                           \
    }


template <typename T>
class CommonWrap {
   public:
    static py::object allowedStatesPy(py::args args, const py::kwargs kwargs) {
        T& self = args[0].cast<T&>();
        std::vector<karabo::data::State> states;
        for (unsigned int i = 1; i < py::len(args); ++i) {
            const std::string state = args[i].attr("name").cast<std::string>();
            states.push_back(karabo::data::State::fromString(state));
        }
        self.allowedStates(states);
        return args[0];
    }
};

///
/// The following macro KARABO_PYTHON_SIMPLE
/// is used for python binding of
/// @code
/// karabo::data::SimpleElement< EType >
/// @endcode
/// where EType: int, long long, double.
/// In Python: INT32_ELEMENT, ..UINT32.., INT64_ELEMENT, ..UINT64..,
/// DOUBLE_ELEMENT, STRING_ELEMENT, BOOL_ELEMENT
///
// Temporary comment and remove from macro
// py::implicitly_convertible<Schema &, T>();

#define KARABO_PYTHON_SIMPLE(t, e)                                                                            \
    {                                                                                                         \
        typedef t EType;                                                                                      \
        typedef SimpleElement<EType> T;                                                                       \
        py::class_<T>(m, #e "_ELEMENT")                                                                       \
              .def(py::init<Schema&>()) KARABO_PYTHON_COMMON_ATTRIBUTES(T) KARABO_PYTHON_OPTIONS_NONVECTOR(T) \
                    KARABO_PYTHON_NUMERIC_ATTRIBUTES(T);                                                      \
    }

#define KARABO_PYTHON_COMMON_ATTRIBUTES(T)                                                                       \
    .def("observerAccess", &T::observerAccess, py::return_value_policy::reference_internal)                      \
          .def("userAccess", &T::userAccess, py::return_value_policy::reference_internal)                        \
          .def("operatorAccess", &T::operatorAccess, py::return_value_policy::reference_internal)                \
          .def("expertAccess", &T::expertAccess, py::return_value_policy::reference_internal)                    \
          .def("adminAccess", &T::adminAccess, py::return_value_policy::reference_internal)                      \
          .def("allowedStates", &CommonWrap<T>::allowedStatesPy, py::return_value_policy::reference_internal)    \
          .def("assignmentInternal", &T::assignmentInternal, py::return_value_policy::reference_internal)        \
          .def("assignmentMandatory", &T::assignmentMandatory, py::return_value_policy::reference_internal)      \
          .def("assignmentOptional", &T::assignmentOptional, py::return_value_policy::reference_internal)        \
          .def("alias", &AliasAttributeWrap<T>::aliasPy, py::return_value_policy::reference_internal)            \
          .def("commit", &T::commit, py::return_value_policy::reference_internal)                                \
          .def("commit", (T & (T::*)(karabo::data::Schema&))(&T::commit), py::arg("expected"),                   \
               py::return_value_policy::reference_internal)                                                      \
          .def("description", &T::description, py::return_value_policy::reference_internal)                      \
          .def("displayedName", &T::displayedName, py::return_value_policy::reference_internal)                  \
          .def("unit", &T::unit, py::return_value_policy::reference_internal)                                    \
          .def("metricPrefix", &T::metricPrefix, py::return_value_policy::reference_internal)                    \
          .def("init", &T::init, py::return_value_policy::reference_internal)                                    \
          .def("key", &T::key, py::return_value_policy::reference_internal)                                      \
          .def("setSpecialDisplayType", (T & (T::*)(std::string const&))(&T::setSpecialDisplayType),             \
               py::arg("displayType"), py::return_value_policy::reference_internal)                              \
          .def("readOnly", &T::readOnly, py::return_value_policy::reference_internal)                            \
          .def("reconfigurable", &T::reconfigurable, py::return_value_policy::reference_internal)                \
          .def("tags", (T & (T::*)(std::string const&, std::string const&))(&T::tags), py::arg("tags"),          \
               py::arg("sep") = " ,;", py::return_value_policy::reference_internal)                              \
          .def("tags", (T & (T::*)(std::vector<std::string> const&))(&T::tags), py::arg("tags"),                 \
               py::return_value_policy::reference_internal)                                                      \
          .def("daqPolicy", (T & (T::*)(karabo::data::DAQPolicy const&))(&T::daqPolicy), (py::arg("daqPolicy")), \
               py::return_value_policy::reference_internal)

#define KARABO_PYTHON_OPTIONS_NONVECTOR(T)                                                              \
    .def("options", (T & (T::*)(std::string const&, std::string const&))(&T::options), py::arg("opts"), \
         py::arg("sep") = " ,;", py::return_value_policy::reference_internal)                           \
          .def("options", (T & (T::*)(std::vector<EType> const&))(&T::options), py::arg("opts"),        \
               py::return_value_policy::reference_internal)


#define KARABO_PYTHON_NUMERIC_ATTRIBUTES(T)                                                 \
    .def("hex", &T::hex, py::return_value_policy::reference_internal)                       \
          .def("oct", &T::oct, py::return_value_policy::reference_internal)                 \
          .def("bin", (T & (T::*)()) & T::bin, py::return_value_policy::reference_internal) \
          .def("bin", (T & (T::*)(const std::string&)) & T::bin, py::arg("meaning"),        \
               py::return_value_policy::reference_internal)                                 \
          .def("maxExc", &T::maxExc, py::return_value_policy::reference_internal)           \
          .def("maxInc", &T::maxInc, py::return_value_policy::reference_internal)           \
          .def("minExc", &T::minExc, py::return_value_policy::reference_internal)           \
          .def("minInc", &T::minInc, py::return_value_policy::reference_internal)

///
/// The following macro KARABO_PYTHON_VECTOR is used for python binding of
/// @code
/// karabo::data::VectorElement< EType, std::vector >
/// @endcode
/// where EType: int, long long, double.
/// In Python: VECTOR_INT32_ELEMENT, ..UINT32.., VECTOR_INT64_ELEMENT, ..UINT64..,
/// VECTOR_DOUBLE_ELEMENT, VECTOR_STRING_ELEMENT, VECTOR_BOOL_ELEMENT.
///

// Commented since complaints at run time (import error)
// py::implicitly_convertible<Schema &, T>();


#define KARABO_PYTHON_VECTOR(t, e)                                                                                 \
    {                                                                                                              \
        typedef t EType;                                                                                           \
        typedef VectorElement<EType, std::vector> T;                                                               \
        py::class_<T>(m, "VECTOR_" #e "_ELEMENT")                                                                  \
              .def(py::init<karabo::data::Schema&>()) KARABO_PYTHON_COMMON_ATTRIBUTES(T)                           \
              .def("maxSize", (T & (T::*)(int const&))(&T::maxSize), py::return_value_policy::reference_internal)  \
              .def("minSize", (T & (T::*)(int const&))(&T::minSize), py::return_value_policy::reference_internal); \
    }

///
/// The following macro KARABO_PYTHON_NODE_CHOICE_LIST is used for python binding of
/// @code
/// karabo::data::NodeElement, karabo::data::ChoiceElement
/// @endcode
///
/// In Python: NODE_ELEMENT, CHOICE_ELEMENT
///
#define KARABO_PYTHON_NODE_CHOICE(NameElem)                                                                           \
    .def("observerAccess", &NameElem::observerAccess, py::return_value_policy::reference_internal)                    \
          .def("userAccess", &NameElem::userAccess, py::return_value_policy::reference_internal)                      \
          .def("operatorAccess", &NameElem::operatorAccess, py::return_value_policy::reference_internal)              \
          .def("expertAccess", &NameElem::expertAccess, py::return_value_policy::reference_internal)                  \
          .def("adminAccess", &NameElem::adminAccess, py::return_value_policy::reference_internal)                    \
          .def("key", &NameElem::key, py::return_value_policy::reference_internal)                                    \
          .def("description", &NameElem::description, py::return_value_policy::reference_internal)                    \
          .def("displayedName", &NameElem::displayedName, py::return_value_policy::reference_internal)                \
          .def("alias", &AliasAttributeWrap<NameElem>::aliasPy, py::return_value_policy::reference_internal)          \
          .def("tags", (NameElem & (NameElem::*)(std::string const&, std::string const&))(&NameElem::tags),           \
               py::arg("tags"), py::arg("sep") = " ,;", py::return_value_policy::reference_internal)                  \
          .def("tags", (NameElem & (NameElem::*)(std::vector<std::string> const&))(&NameElem::tags), py::arg("tags"), \
               py::return_value_policy::reference_internal)                                                           \
          .def("commit", &NameElem::commit, py::return_value_policy::reference_internal)

#endif /* KARABIND_PYUTILSCHEMAELEMENT_HH */
