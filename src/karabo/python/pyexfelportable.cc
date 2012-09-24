/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <exfel/util/ConfigConstants.hh>
#include <boost/any.hpp>
#include <vector>
#include <deque>
#include "PythonMacros.hh"

namespace bp = boost::python;
using namespace exfel::util;

void exportPyUtilClassInfo();
void exportPyUtilHash3();
void exportPyUtilSchemaSimple();
void exportPyUtilTypes();
void exportPyVectorContainer();

void exportPyIoWriterSimple();
void exportPyIoReaderSimple();
void exportPyIoFormat();

bp::object anyExtract(boost::any const& self) {
    if (self.empty()) return bp::object();

    EXFEL_PYTHON_ANY_EXTRACT(std::string)
    EXFEL_PYTHON_ANY_EXTRACT(int)
    EXFEL_PYTHON_ANY_EXTRACT(unsigned int)
    EXFEL_PYTHON_ANY_EXTRACT(long long)
    EXFEL_PYTHON_ANY_EXTRACT(unsigned long long)
    EXFEL_PYTHON_ANY_EXTRACT(short)
    EXFEL_PYTHON_ANY_EXTRACT(unsigned short)
    EXFEL_PYTHON_ANY_EXTRACT(signed char)
    EXFEL_PYTHON_ANY_EXTRACT(unsigned char)
    EXFEL_PYTHON_ANY_EXTRACT(char)
    EXFEL_PYTHON_ANY_EXTRACT(double)
    EXFEL_PYTHON_ANY_EXTRACT(float)

    if (self.type() == typeid (bool)) {
        return bp::object(boost::any_cast<bool>(self));
    }
    if (self.type() == typeid (std::deque<bool>)) {
        typedef std::deque<bool> VContainer;
        VContainer container(boost::any_cast<VContainer > (self));
        std::string str;
        str.append("[");
        for (size_t i = 0; i < container.size(); i++) {
            std::stringstream tmp;
            tmp << container[i];
            str.append(tmp.str());
            if (i < container.size() - 1) str.append(",");
        }
        str.append("]");
        return bp::object(str);
    }

    throw std::runtime_error("Unknown value type boost::any");
}

BOOST_PYTHON_MODULE(libpyexfelportable) {

    bp::class_<boost::any > ("boost_any", bp::no_init)
            .def("empty", &boost::any::empty)
            .def("extract", anyExtract)
            ;

    bp::class_<std::vector<boost::any> >("stl_vector_boost_any", bp::no_init);

    bp::class_<std::pair<const std::string, boost::any> >("hashPair")
            .def_readonly("key", &std::pair<const std::string, boost::any>::first)
            .def_readwrite("value", &std::pair<const std::string, boost::any>::second)
            ;

    bp::enum_<exfel::util::AccessType > ("AccessType")
            .value("INIT", exfel::util::INIT)
            .value("READ", exfel::util::READ)
            .value("WRITE", exfel::util::WRITE)
            .export_values()
            ;

    exportPyVectorContainer();
    exportPyUtilHash3();
    exportPyUtilSchemaSimple();
    exportPyUtilTypes();
    exportPyUtilClassInfo();

    exportPyIoWriterSimple();
    exportPyIoReaderSimple();
    exportPyIoFormat();
}

