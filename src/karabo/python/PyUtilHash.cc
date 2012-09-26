/* $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on February 29, 2012, 9:02 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "HashWrap.hh"

using namespace karabo::util;
using namespace std;
using namespace karabo::pyexfel;

void exportPyUtilHash() {

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

    h.def("set",
            &HashWrap().pythonSet,
            (bp::arg("key"), bp::arg("value")));

    h.def("setAsBool"
            , (void ( Hash::*)(string const &, bool const &))(&Hash::set));

    h.def("get",
            &HashWrap().pythonGet,
            (bp::arg("key")));

    h.def("getTypeAsString"
            , (string(Hash::*)(string const &) const) (&Hash::getTypeAsString)
            , (bp::arg("key")));

    h.def("getTypeAsId"
            , (Types::Type(Hash::*)(string const &) const) (&Hash::getTypeAsId)
            , (bp::arg("key")));

    h.def("setFromPath",
            &HashWrap().pythonSetFromPath,
            (bp::arg("key"), bp::arg("value"), bp::arg("sep") = "."));

    h.def("setFromPath"
            , (void ( Hash::*)(string const &))(&Hash::setFromPath)
            , (bp::arg("path")));

    h.def("setFromPathAsBool"
            , (void ( Hash::*)(string const &, bool const &, string))(&Hash::setFromPath)
            , (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."));

    h.def("getFromPath",
            &HashWrap().pythonGetFromPath,
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

    h.def("hasFromPath"
            , (bool(Hash::*)(string const &, string) const) (&Hash::hasFromPath)
            , (bp::arg("path"), bp::arg("sep")=".") );
    
    h.def("isFromPath"
            , (bool(Hash::*)(string const &, Types::Type, string const &) ) (&Hash::isFromPath)
            , (bp::arg("path"), bp::arg("type"), bp::arg("sep")=".") );
    
    h.def("eraseFromPath"
            , (size_t(Hash::*)(string const &, string const &) ) (&Hash::eraseFromPath)
            , (bp::arg("path"), bp::arg("sep")=".") );
    
    h.def("clear", &Hash::clear);

    h.def("empty", &HashWrap().pythonEmpty);
    
    h.def(bp::self_ns::str(bp::self));

    h.def("getKeys", &HashWrap().pythonGetKeys);
    h.def("keys", &HashWrap().pythonGetKeys);

    h.def("values", &HashWrap().pythonGetValues);

    h.def("getLeaves", &HashWrap().pythonGetLeaves
            , (bp::arg("sep") = "."));
    h.def("leaves", &HashWrap().pythonGetLeaves
            , (bp::arg("sep") = "."));

    h.def("copy", &HashWrap().pyDict2Hash
            , (bp::arg("dict"))
            , bp::return_value_policy<bp::copy_const_reference > ());

    h.def("copyFromPath", &HashWrap().pyDict2HashFromPath
            , (bp::arg("dict"), bp::arg("sep") = ".")
            , bp::return_value_policy<bp::copy_const_reference > ());

    h.def("update", &Hash::update, (bp::arg("hash")));

    h.def("flatten", &HashWrap().pythonFlatten
            , (bp::arg("sep") = "."));

    h.def("unflatten", &HashWrap().pythonUnFlatten
            , (bp::arg("sep") = "."));

    h.def("__getitem__", &HashWrap().pythonGet);

    h.def("__setitem__", &HashWrap().pythonSet);

    h.def("__delitem__", &HashWrap().pythonErase);

    h.def("__len__", &Hash::size);

    h.def("__contains__", &Hash::has);

    h.def("__iter__", bp::iterator<Hash > ());

    bp::class_<std::vector<Hash> > v("VectorHash");

    v.def("__iter__", bp::iterator < std::vector<Hash> > ());
    v.def("__len__", &std::vector<std::vector<Hash > >::size);
    v.def("clear", &std::vector<std::vector<Hash > >::clear);
}

