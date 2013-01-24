/* 
 * $Id$
 * 
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_PYKARABO_DEFAULTVALUEVECTORWRAP_HH
#define	KARABO_PYKARABO_DEFAULTVALUEVECTORWRAP_HH

#include <boost/python.hpp>
#include <karabo/util/GenericElement.hh>

namespace bp = boost::python;

namespace karabo {
    namespace pyexfel {

        template<class T>
        class DefaultValueVectorWrap {
        public:

            typedef std::vector< T > VType;
            typedef karabo::util::VectorElement< T, std::vector > U;
            typedef karabo::util::DefaultValue< U, VType > DefValueVec;

            static void pyList2VectorDefaultValue(DefValueVec& self, const bp::object & obj) {
                if (PyList_Check(obj.ptr())) {

                    const bp::list& l = bp::extract<bp::list > (obj);
                    bp::ssize_t size = bp::len(l);

                    std::vector<T> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<T > (obj[i]);
                    }
                    self.defaultValue(v);

                } else {
                    throw KARABO_PYTHON_EXCEPTION("Python type of the defaultValue of VectorElement must be a list");
                }
            }
        };
        
        template<> 
        class DefaultValueVectorWrap<bool> {
        public:

            typedef std::deque< bool > VType;
            typedef karabo::util::VectorElement< bool, std::deque > U;
            typedef karabo::util::DefaultValue< U, VType > DefValueVec;

            static void pyList2VectorDefaultValue(DefValueVec& self, const bp::object & obj) {
                if (PyList_Check(obj.ptr())) {

                    const bp::list& l = bp::extract<bp::list > (obj);
                    bp::ssize_t size = bp::len(l);

                    std::deque<bool> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<bool > (obj[i]);
                    }
                    self.defaultValue(v);

                } else {
                    throw KARABO_PYTHON_EXCEPTION("Python type of the defaultValue of VectorElement must be a list");
                }
            }
        };

    }
}

#define KARABO_PYTHON_VECTOR_DEFAULT_VALUE(t, e) \
{\
typedef t EType;\
typedef std::vector< EType > VType;\
typedef VectorElement< EType, std::vector > U;\
typedef DefaultValue< U, VType > DefValueVec;\
bp::class_< DefValueVec, boost::noncopyable > ("DefaultValueVectorElement"#e, bp::no_init)\
.def("defaultValue"\
, &karabo::pyexfel::DefaultValueVectorWrap<EType>::pyList2VectorDefaultValue\
, (bp::arg("param"), bp::arg("defaultValuePyList")))\
.def("defaultValue"\
, (U & (DefaultValue<U, std::vector<EType, std::allocator<EType> > >::*)(VType const &))(&DefValueVec::defaultValue)\
, (bp::arg("defaultValue"))\
, bp::return_internal_reference<> ())\
.def("defaultValueFromString"\
, (U & (DefaultValue<U, std::vector<EType, std::allocator<EType> > >::*)(::std::string const &))(&DefValueVec::defaultValueFromString)\
, (bp::arg("defaultValue"))\
, bp::return_internal_reference<> ())\
.def("noDefaultValue"\
, (U & (DefaultValue<U, std::vector<EType, std::allocator<EType> > >::*)())(&DefValueVec::noDefaultValue)\
, bp::return_internal_reference<> ())\
;\
}

#endif	/* KARABO_PYKARABO_DEFAULTVALUEVECTORWRAP_HH */
