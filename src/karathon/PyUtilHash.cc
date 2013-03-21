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
#include <karabo/util/Hash.hh>
#include "HashWrap.hh"
#include "NodeWrap.hh"
#include "AttributesWrap.hh"
#include "AttributesNodeWrap.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;
using namespace karabo::pyexfel;

void exportPyUtilHash() {

#    ifdef KARATHON_BOOST_NUMPY
    bn::initialize();
#    endif


    bp::class_<Hash::Attributes::map_iterator>("Hash.Attributes.MapIterator", bp::no_init);

    bp::class_<Hash::Attributes::const_map_iterator>("Hash.Attributes.ConstMapIterator", bp::no_init);

    bp::class_<Hash::Attributes::Node, boost::shared_ptr<Hash::Attributes::Node> > an("Hash.Attributes.Node", bp::no_init);
    an.def("getKey", &AttributesNodeWrap().pythonGetKey);
    an.def("setValue", &AttributesNodeWrap().pythonSetValue, (bp::arg("value")));
    an.def("getValue", &AttributesNodeWrap().pythonGetValue);
    an.def("getValueAs", &AttributesNodeWrap().pythonGetValueAs, (bp::arg("type")));
    an.def("getType", &AttributesNodeWrap().pythonGetType);
    an.def("setType", &AttributesNodeWrap().pythonSetType, (bp::arg("type")));
    
    
    
    bp::class_<Hash::Attributes> a("Hash.Attributes", bp::no_init);
    a.def("has", &AttributesWrap().pythonHas, (bp::arg("key")));
    a.def("isType", &AttributesWrap().pythonHas, (bp::arg("key"), bp::arg("type")));
    a.def("erase", &AttributesWrap().pythonErase, (bp::arg("key")));
    a.def("size", &AttributesWrap().pythonSize);
    a.def("empty", &AttributesWrap().pythonEmpty);
    a.def("clear", &AttributesWrap().pythonClear);
    a.def("getNode", &AttributesWrap().pythonGetNode, (bp::arg("key")));
    a.def("get", &AttributesWrap().pythonGet, (bp::arg("key")));
    a.def("__getitem__", &AttributesWrap().pythonGet, (bp::arg("key")));
    a.def("getAs", &AttributesWrap().pythonGetAs, (bp::arg("key"), bp::arg("type")));
    a.def("set", &AttributesWrap().pythonSet, (bp::arg("key"), bp::arg("value")));
    a.def("__setitem__", &AttributesWrap().pythonSet, (bp::arg("key"), bp::arg("value")));
    a.def("find", &AttributesWrap().pythonFind, (bp::arg("key")));
    a.def("getIt", &AttributesWrap().pythonGetIt, (bp::arg("it")));

    bp::class_<Hash::Node, boost::shared_ptr<Hash::Node> > n("Hash.Node", bp::no_init);
    n.def("getKey", &NodeWrap().pythonGetKey);
    n.def("setValue", &NodeWrap().pythonSetValue, (bp::arg("value")));
    n.def("getValue", &NodeWrap().pythonGetValue);
    n.def("getValueAs", &NodeWrap().pythonGetValueAs, (bp::arg("type")));
    n.def("setAttribute", &NodeWrap().pythonSetAttribute, (bp::arg("key"), bp::arg("value")));
    n.def("getAttribute", &NodeWrap().pythonGetAttribute, (bp::arg("key")));
    n.def("getAttributeAs", &NodeWrap().pythonGetAttributeAs, (bp::arg("key"), bp::arg("type")));
    n.def("hasAttribute", &NodeWrap().pythonHasAttribute, (bp::arg("key")));
    n.def("setAttributes", &NodeWrap().pythonSetAttributes, (bp::arg("attributes")));
    n.def("getAttributes", &NodeWrap().pythonGetAttributes);
    n.def("getType", &NodeWrap().pythonGetType);
    n.def("setType", &NodeWrap().pythonSetType, (bp::arg("type")));


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
    h.def("getAs", &HashWrap().pythonGetAs, (bp::arg("path"), bp::arg("type"), bp::arg("sep") = "."));
    h.def("getType", &HashWrap().pythonGetType, (bp::arg("key"), bp::arg("sep") = "."));
    h.def("getTypeAsId", &HashWrap().pythonGetTypeAsId, (bp::arg("key"), bp::arg("sep") = "."));
    h.def("merge", &Hash::merge, (bp::arg("hash")));
    //    h.def("__add__", &Hash::operator+, (bp::arg("hash1"), bp::arg("hash2")), bp::return_value_policy<copy_non_const_reference>());
    h.def("__iadd__", &Hash::operator+=, (bp::arg("hash")), bp::return_internal_reference<>());
    h.def("isType", &HashWrap().pythonIs, (bp::arg("path"), bp::arg("type"), bp::arg("sep") = "."));
    h.def(bp::self_ns::str(bp::self));
    //    h.def("copy", &HashWrap().pyDict2Hash, (bp::arg("dict"), bp::arg("sep") = "."), bp::return_value_policy<bp::copy_const_reference > ());
    //    h.def("update", &Hash::update, (bp::arg("hash")));
    h.def("flatten", &HashWrap().pythonFlatten, (bp::arg("flat"), bp::arg("sep") = "."));
    h.def("unflatten", &HashWrap().pythonUnFlatten, (bp::arg("tree"), bp::arg("sep") = "."));
    h.def("find", &HashWrap().pythonFind, (bp::arg("path"), bp::arg("sep") = "."));
    h.def("setNode", &HashWrap().pythonSetNode, (bp::arg("node")));
    h.def("getNode", &HashWrap().pythonGetNode, (bp::arg("path"), bp::arg("sep") = "."));
    h.def("getAttribute", &HashWrap().pythonGetAttribute, (bp::arg("path"), bp::arg("attribute"), bp::arg("sep") = "."));
    h.def("getAttributeAs", &HashWrap().pythonGetAttributeAs, (bp::arg("path"), bp::arg("attribute"), bp::arg("type"), bp::arg("sep") = "."));
    h.def("getAttributes", &HashWrap().pythonGetAttributes, (bp::arg("path"), bp::arg("sep") = "."));
    h.def("setAttribute", &HashWrap().pythonSetAttribute, (bp::arg("path"), bp::arg("attribute"), bp::arg("value"), bp::arg("sep") = "."));
    h.def("setAttributes", &HashWrap().pythonSetAttributes, (bp::arg("path"), bp::arg("attributes"), bp::arg("sep") = "."));

    bp::class_<std::vector<Hash> > v("VectorHash");
    v.def("__iter__", bp::iterator < std::vector<Hash> > ());
    v.def("__len__", &std::vector<std::vector<Hash > >::size);
    v.def("clear", &std::vector<std::vector<Hash > >::clear);
}

