/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <karabo/util/Hash.hh>

namespace bp = boost::python;

template<class T>
struct vector_item {

    static bp::object vecToStr(bp::object const& self) {
        T container = bp::extract< T > (self);
        std::string str("[");
        for (size_t i = 0; i < container.size(); i++) {
            std::stringstream tmp;
            tmp << container[i];
            str.append(tmp.str());
            if (i < container.size() - 1) str.append(",");
        }
        str.append("]");
        return bp::object(str);
    }
};

/**
 * The following macro KARABO_PYTHON_VECTORTYPE
 * is used for python binding of vector types.
 * In Python: vecInt32, vecUInt32, vecInt64, vecUInt64, vecInt8, vecUInt8,
 * vecCHAR, vecInt16, vecUInt16, vecDouble, vecFloat
 *
 */
#define KARABO_PYTHON_VECTORTYPE(t,e)\
{\
typedef std::vector<t> vecT;\
bp::class_< vecT >("vec"#e)\
.def( bp::vector_indexing_suite< vecT, true >() )\
.def("__str__", &vector_item< vecT >().vecToStr )\
;\
}

void exportPyVectorContainer() {

    KARABO_PYTHON_VECTORTYPE(std::string, String)
    KARABO_PYTHON_VECTORTYPE(int, Int32)   
    KARABO_PYTHON_VECTORTYPE(unsigned int, UInt32)
    KARABO_PYTHON_VECTORTYPE(long long, Int64)
    KARABO_PYTHON_VECTORTYPE(unsigned long long, UInt64)
    KARABO_PYTHON_VECTORTYPE(signed char, Int8)
    KARABO_PYTHON_VECTORTYPE(unsigned char, UInt8)
    KARABO_PYTHON_VECTORTYPE(char, CHAR)
    KARABO_PYTHON_VECTORTYPE(signed short, Int16)
    KARABO_PYTHON_VECTORTYPE(unsigned short, UInt16)
    KARABO_PYTHON_VECTORTYPE(double, Double)
    KARABO_PYTHON_VECTORTYPE(float, Float)

    bp::class_< std::deque< bool > >("vecBool")
        .def(bp::vector_indexing_suite< std::deque< bool >, true > ())
        .def("__str__", &vector_item< std::deque< bool > >().vecToStr )
        ;

}
