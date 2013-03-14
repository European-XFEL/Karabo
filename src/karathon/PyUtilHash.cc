/* $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on February 29, 2012, 9:02 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <string>
#include "HashWrap.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;
using namespace karabo::pyexfel;

void exportPyUtilHash() {

#    ifdef KARATHON_BOOST_NUMPY
    bn::initialize();
#    endif

    bp::class_<Hash > h("Hash");

    h.def(bp::init< std::string const & >());

    h.def(bp::init< Hash const & >());

    h.def(bp::init< std::string const &, bp::object const & >());

    h.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const & >());

    h.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const & >());

    h.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const & >());

    h.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const & >());

    h.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const & >());

    h.def("clear", &Hash::clear);

    h.def("empty", &HashWrap().pythonEmpty);

    h.def("getKeys", &HashWrap().pythonGetKeys);

    h.def("keys", &HashWrap().pythonGetKeys);

    h.def("getValues", &HashWrap().pythonGetValues);

    h.def("values", &HashWrap().pythonGetValues);

    h.def("getPaths", &HashWrap().pythonGetPaths);

    h.def("set", &HashWrap().pythonSet, (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

    h.def("__setitem__", &HashWrap().pythonSet, (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

    h.def("get", &HashWrap().pythonGet, (bp::arg("path"), bp::arg("sep") = "."));

    h.def("__getitem__", &HashWrap().pythonGet, (bp::arg("path"), bp::arg("sep") = "."));

    h.def("has", &HashWrap().pythonHas, (bp::arg("path"), bp::arg("sep") = "."));

    h.def("__contains__", &HashWrap().pythonHas, (bp::arg("path"), bp::arg("sep") = "."));

    h.def("erase", &HashWrap().pythonErase, (bp::arg("path"), bp::arg("sep") = "."));

    h.def("__delitem__", &HashWrap().pythonErase, (bp::arg("path"), bp::arg("sep") = "."));

    h.def("__len__", &Hash::size);

    h.def("__iter__", bp::iterator<Hash > ());

    h.def("getAsStr", &HashWrap().pythonGetAsStr, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getAsStrArr", &HashWrap().pythonGetAsStrArr, (bp::arg("path"), bp::arg("sep") = "."));
    
//    h.def("getAsBool", (bool (Hash::*)(const std::string&, const char))&Hash::get<bool>, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getAsInt", &HashWrap().pythonGetAsInt, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getAsIntArr", &HashWrap().pythonGetAsIntArr, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getAsLong", &HashWrap().pythonGetAsLong, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getAsLongArr", &HashWrap().pythonGetAsLongArr, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getAsFloat", &HashWrap().pythonGetAsFloat, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getAsFloatArr", &HashWrap().pythonGetAsFloatArr, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getAsDouble", &HashWrap().pythonGetAsDouble, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getAsDoubleArr", &HashWrap().pythonGetAsDoubleArr, (bp::arg("path"), bp::arg("sep") = "."));
    
    h.def("getType", &HashWrap().pythonGetType, (bp::arg("key"), bp::arg("sep") = "."));

    h.def("getTypeAsId", &HashWrap().pythonGetTypeAsId, (bp::arg("key"), bp::arg("sep") = "."));

    h.def("merge", &Hash::merge, (bp::arg("hash")));
    
//    h.def("__add__", &Hash::operator+, (bp::arg("hash1"), bp::arg("hash2")), bp::return_value_policy<copy_non_const_reference>());
    h.def("__iadd__", &Hash::operator+=, (bp::arg("hash")), bp::return_internal_reference<>());

    h.def("is_type", &HashWrap().pythonIs, (bp::arg("path"), bp::arg("type"), bp::arg("sep") = "."));

    h.def(bp::self_ns::str(bp::self));

    h.def("copy", &HashWrap().pyDict2Hash, (bp::arg("dict"), bp::arg("sep") = "."), bp::return_value_policy<bp::copy_const_reference > ());

//    h.def("update", &Hash::update, (bp::arg("hash")));

    h.def("flatten", &HashWrap().pythonFlatten, (bp::arg("flat"), bp::arg("sep") = "."));

    h.def("unflatten", &HashWrap().pythonUnFlatten, (bp::arg("tree"), bp::arg("sep") = "."));


    bp::class_<std::vector<Hash> > v("VectorHash");

    v.def("__iter__", bp::iterator < std::vector<Hash> > ());
    v.def("__len__", &std::vector<std::vector<Hash > >::size);
    v.def("clear", &std::vector<std::vector<Hash > >::clear);
}

