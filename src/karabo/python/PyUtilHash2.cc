/* $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 29, 2012, 9:02 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "PythonLoader.hh"

using namespace exfel::util;
using namespace std;
using namespace exfel::pyexfel;
namespace bp = boost::python;

void exportPyUtilHash2() {

    bp::class_<Hash > h("Hash");

    h.def(bp::init< bp::object const & >());
    
    h.def(bp::init< std::string const & >());

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

    h.def("set",
            &Hash::pythonSet,
            (bp::arg("key"), bp::arg("value")));

    h.def("get",
            &Hash::pythonGet,
            (bp::arg("key")));

    h.def("setFromPath",
            &Hash::pythonSetFromPath,
            (bp::arg("key"), bp::arg("value"), bp::arg("sep") = "."));

    h.def("getFromPath",
            &Hash::pythonGetFromPath,
            (bp::arg("key"), bp::arg("sep") = "."));

    h.def("append"
            , (Hash const & (Hash::*)(Hash const &))(&Hash::append)
            , (bp::arg("hash"))
            , bp::return_value_policy< bp::copy_const_reference > ());

    h.def("getAsString"
            , (string(Hash::*)(string const &) const) (&Hash::getAsString)
            , (bp::arg("key")));

    h.def("has"
            , (bool(Hash::*)(string const &) const) (&Hash::has)
            , (bp::arg("key")));

    h.def("clear", &Hash::clear);

    h.def(bp::self_ns::str(bp::self));

    h.def("getKeys", &Hash::pythonGetKeys);
    h.def("keys", &Hash::pythonGetKeys);

    h.def("values", &Hash::pythonGetValues);

    h.def("getLeaves", &Hash::pythonGetLeaves
            , (bp::arg("sep") = "."));
    h.def("leaves", &Hash::pythonGetLeaves
            , (bp::arg("sep") = "."));

    h.def("update"
            , (&Hash::update)
            , (bp::arg("")));

    h.def("flatten", &Hash::pythonFlatten
            , (bp::arg("sep") = "."));

    h.def("unflatten", &Hash::pythonUnFlatten
            , (bp::arg("sep") = "."));

    h.def("__getitem__", &Hash::pythonGet);

    h.def("__setitem__", &Hash::pythonSet);

    h.def("__delitem__", &Hash::pythonErase);

    h.def("__len__", &Hash::size);

    h.def("__contains__", &Hash::has);

    h.def("__iter__", bp::iterator<Hash > ());

    bp::class_<std::vector<Hash> > v("VectorHash");

    v.def("__iter__", bp::iterator < std::vector<Hash> > ());
    v.def("__len__", &std::vector<std::vector<Hash > >::size);
    v.def("clear", &std::vector<std::vector<Hash > >::clear);
}

