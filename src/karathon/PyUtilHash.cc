/* $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on February 29, 2012, 9:02 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <string>
#include <karabo/util/Hash.hh>
#include <karabo/util/Exception.hh>
#include "HashWrap.hh"
#include "NodeWrap.hh"
#include "AttributesWrap.hh"
#include "AttributesNodeWrap.hh"

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;
using namespace karathon;

typedef karabo::util::Element<std::string, karabo::util::OrderedMap<std::string, karabo::util::Element<std::string, bool> > > HashNode;

// Translate C++ karabo::util::Exception into python RuntimeError exception
void translator(const karabo::util::Exception& e) {
    PyErr_SetString(PyExc_RuntimeError, (e.userFriendlyMsg() + " -- " + e.detailedMsg()).c_str());
}

void exportPyUtilHash() {

//    #ifdef WITH_BOOST_NUMPY
//    bn::initialize();
//    #endif
 
    bp::docstring_options docs(true, true, false);
    
    // register a translator
    bp::register_exception_translator<karabo::util::Exception>(translator);

    // Types
    bp::enum_<PyTypes::ReferenceType>("Types", "This enumeration describes reference types supported in configuration system.")
            .value("BOOL", PyTypes::BOOL)
            .value("VECTOR_BOOL", PyTypes::VECTOR_BOOL)
            .value("CHAR", PyTypes::CHAR)
            .value("VECTOR_CHAR", PyTypes::VECTOR_CHAR)
            .value("INT8", PyTypes::INT8)
            .value("VECTOR_INT8", PyTypes::VECTOR_INT8)
            .value("UINT8", PyTypes::UINT8)
            .value("VECTOR_UINT8", PyTypes::VECTOR_UINT8)
            .value("INT16", PyTypes::INT16)
            .value("VECTOR_INT16", PyTypes::VECTOR_INT16)
            .value("UINT16", PyTypes::UINT16)
            .value("VECTOR_UINT16", PyTypes::VECTOR_UINT16)
            .value("INT32", PyTypes::INT32)
            .value("VECTOR_INT32", PyTypes::VECTOR_INT32)
            .value("UINT32", PyTypes::UINT32)
            .value("VECTOR_UINT32", PyTypes::VECTOR_UINT32)
            .value("INT64", PyTypes::INT64)
            .value("VECTOR_INT64", PyTypes::VECTOR_INT64)
            .value("UINT64", PyTypes::UINT64)
            .value("VECTOR_UINT64", PyTypes::VECTOR_UINT64)
            .value("FLOAT", PyTypes::FLOAT)
            .value("VECTOR_FLOAT", PyTypes::VECTOR_FLOAT)
            .value("DOUBLE", PyTypes::DOUBLE)
            .value("VECTOR_DOUBLE", PyTypes::VECTOR_DOUBLE)
            .value("COMPLEX_FLOAT", PyTypes::COMPLEX_FLOAT)
            .value("VECTOR_COMPLEX_FLOAT", PyTypes::VECTOR_COMPLEX_FLOAT)
            .value("COMPLEX_DOUBLE", PyTypes::COMPLEX_DOUBLE)
            .value("VECTOR_COMPLEX_DOUBLE", PyTypes::VECTOR_COMPLEX_DOUBLE)
            .value("STRING", PyTypes::STRING)
            .value("VECTOR_STRING", PyTypes::VECTOR_STRING)
            .value("HASH", PyTypes::HASH)
            .value("VECTOR_HASH", PyTypes::VECTOR_HASH)
            .value("SCHEMA", PyTypes::SCHEMA)
            .value("ANY", PyTypes::ANY)
            .value("NONE", PyTypes::NONE)
            .value("VECTOR_NONE", PyTypes::VECTOR_NONE)
            .value("UNKNOWN", PyTypes::UNKNOWN)
            .value("SIMPLE", PyTypes::SIMPLE)
            .value("SEQUENCE", PyTypes::SEQUENCE)
            .value("POINTER", PyTypes::POINTER)
            .value("RAW_ARRAY", PyTypes::RAW_ARRAY)
            .value("ARRAY_BOOL", PyTypes::ARRAY_BOOL)
            .value("ARRAY_CHAR", PyTypes::ARRAY_CHAR)
            .value("ARRAY_INT8", PyTypes::ARRAY_INT8)
            .value("ARRAY_UINT8", PyTypes::ARRAY_UINT8)
            .value("ARRAY_INT16", PyTypes::ARRAY_INT16)
            .value("ARRAY_UINT16", PyTypes::ARRAY_UINT16)
            .value("ARRAY_INT32", PyTypes::ARRAY_INT32)
            .value("ARRAY_UINT32", PyTypes::ARRAY_UINT32)
            .value("ARRAY_INT64", PyTypes::ARRAY_INT64)
            .value("ARRAY_UINT64", PyTypes::ARRAY_UINT64)
            .value("ARRAY_FLOAT", PyTypes::ARRAY_FLOAT)
            .value("ARRAY_DOUBLE", PyTypes::ARRAY_DOUBLE)
            .value("HASH_POINTER", PyTypes::HASH_POINTER)
            .value("VECTOR_HASH_POINTER", PyTypes::VECTOR_HASH_POINTER)
            .value("PYTHON", PyTypes::PYTHON_DEFAULT)
            .value("NUMPY", PyTypes::NUMPY_DEFAULT)
            .value("NDARRAY_BOOL", PyTypes::NDARRAY_BOOL)
            .value("NDARRAY_INT16", PyTypes::NDARRAY_INT16)
            .value("NDARRAY_UINT16", PyTypes::NDARRAY_UINT16)
            .value("NDARRAY_INT32", PyTypes::NDARRAY_INT32)
            .value("NDARRAY_UINT32", PyTypes::NDARRAY_UINT32)
            .value("NDARRAY_INT64", PyTypes::NDARRAY_INT64)
            .value("NDARRAY_UINT64", PyTypes::NDARRAY_UINT64)
            .value("NDARRAY_FLOAT", PyTypes::NDARRAY_FLOAT)
            .value("NDARRAY_DOUBLE", PyTypes::NDARRAY_DOUBLE)
            .value("NDARRAY_COMPLEX_FLOAT", PyTypes::NDARRAY_COMPLEX_FLOAT)
            .value("NDARRAY_COMPLEX_DOUBLE", PyTypes::NDARRAY_COMPLEX_DOUBLE)
            ;
    
   bp::class_<PyTypes>("TypesClass", bp::no_init)
            .def("to", (const karabo::util::Types::ReferenceType (*)(const PyTypes::ReferenceType&))&PyTypes::to, (bp::arg("Python_types"))).staticmethod("to")
            .def("from", (const PyTypes::ReferenceType (*)(const karabo::util::Types::ReferenceType&))&PyTypes::from, (bp::arg("C++_types"))).staticmethod("from")
            .def("category", (const PyTypes::ReferenceType (*)(int))&PyTypes::category, (bp::arg("C++_types"))).staticmethod("category")
            ;


    bp::def("setStdVectorDefaultConversion", &HashWrap().setDefault, (bp::arg("PYTHON_or_NUMPY_types")));
    bp::def("isStdVectorDefaultConversion", &HashWrap().isDefault, (bp::arg("PYTHON_or_NUMPY_types")));


    //    using boost::python::iterator;
    //    bp::def("range", &karathon::range);

    //    bp::class_<Hash::Attributes::map_iterator>("HashAttributesMapIterator", bp::no_init);
    //
    //    bp::class_<Hash::Attributes::const_map_iterator>("HashAttributesConstMapIterator", bp::no_init);

    bp::class_<Hash::Attributes::Node, boost::shared_ptr<Hash::Attributes::Node> > an("HashAttributesNode", bp::no_init);
    an.def("getKey", &AttributesNodeWrap().getKey, "Get key of current node in attribute's container");
    an.def("__str__", &AttributesNodeWrap().getKey);
    an.def("setValue", &AttributesNodeWrap().setValue, (bp::arg("value")), "Set value for current node in attribute's container");
    an.def("getValue", &AttributesNodeWrap().getValue, "Get value for current node in attribute's container");
    an.def("getValueAs", &AttributesNodeWrap().getValueAs, (bp::arg("type")), "Get value as a type given as an argument for current node");
    an.def("getType", &AttributesNodeWrap().getType, "Get type of the value kept in current node");
    an.def("setType", &AttributesNodeWrap().setType, (bp::arg("type")), "Set type for value kept in current node");



    bp::class_<Hash::Attributes> a("HashAttributes",
                                   "The HashAttributes class is a heterogeneous container with string key and \"any object\" value\n"
                                   "that preserves insertion order, i.e. it is behaving like an ordered map");
    a.def(bp::init<>());
    a.def(bp::init< std::string const &, bp::object const & >());
    a.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const & >());
    a.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const & >());
    a.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const & >());
    a.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const & >());
    a.def(bp::init< std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const &,
          std::string const &, bp::object const & >());
    a.def("has", &AttributesWrap().has, (bp::arg("key")), "Returns True if HashAttributes container contains given \"key\"");
    a.def("__contains__", &AttributesWrap().has, (bp::arg("key")), "Returns True if HashAttributes container contains given \"key\"");
    a.def("isType", &AttributesWrap().has, (bp::arg("key"), bp::arg("type")), "Returns True if HashAttributes container has given \"key\" of reference \"type\"..");
    a.def("erase", &AttributesWrap().erase, (bp::arg("key")), "Erase \"key\" attribute");
    a.def("__delitem__", &AttributesWrap().erase, (bp::arg("key")), "Erase \"key\" attribute");
    a.def("size", &AttributesWrap().size, "Returns number of entries in HashAttributes container");
    a.def("__len__", &AttributesWrap().size, "Returns number of entries in HashAttributes container");
    a.def("empty", &AttributesWrap().empty, "Returns True if HashAttributes container is empty.");
    a.def("bool", &AttributesWrap().size, "This function automatically called when HashAttributes object checked in \"if\" expression. \"False\" means that container is empty.");
    a.def("clear", &AttributesWrap().clear, "Make HashAttributes container empty.");
    a.def("getNode", &AttributesWrap().getNode, (bp::arg("key")), "Returns HashAttributesNode object associated with \"key\" attribute.");
    a.def("get", &AttributesWrap().get, (bp::arg("key")), "Returns value for \"key\" attribute.");
    a.def("__getitem__", &AttributesWrap().__getitem__, (bp::arg("key")), "Pythonic style for getting value of attribute: x = attrs['abc']");
    a.def("getAs", &AttributesWrap().getAs, (bp::arg("key"), bp::arg("type")), "Get the value of the \"key\" attribute and convert it to type \"type\".");
    a.def("set", &AttributesWrap().set, (bp::arg("key"), bp::arg("value")), "Set the \"value\" for \"key\" attribute.");
    a.def("__setitem__", &AttributesWrap().set, (bp::arg("key"), bp::arg("value")), "Pythonic style for setting value of attribute: attrs['abc'] = 123");
    //    a.def("find", &AttributesWrap().find, (bp::arg("key")), "");
    //    a.def("getIt", &AttributesWrap().getIt, (bp::arg("it")));
    a.def("__iter__", bp::iterator<Hash::Attributes, bp::return_internal_reference<> >());

    bp::class_<HashNode, boost::shared_ptr<HashNode> > n("HashNode", bp::init<>());
    n.def("__repr__", &NodeWrap().getKey);
    n.def("__str__", &NodeWrap().getKey);
    n.def("getKey", &NodeWrap().getKey, "Returns the key of current node.");
    n.def("setValue", &NodeWrap().setValue, (bp::arg("value")), "Sets the new value of current node.");
    n.def("getValue", &NodeWrap().getValue, "Gets the value of current node.");
    n.def("getValueAs", &NodeWrap().getValueAs, (bp::arg("type")), "Gets the value of current node converted to given reference type");
    n.def("setAttribute", &NodeWrap().setAttribute, (bp::arg("key"), bp::arg("value")), "Sets the \"key\" attribute to some \"value\" in current node.");
    n.def("getAttribute", &NodeWrap().getAttribute, (bp::arg("key")), "Gets the value of \"key\" attribute  in current node.");
    n.def("getAttributeAs", &NodeWrap().getAttributeAs, (bp::arg("key"), bp::arg("type")), "Gets the value of \"key\" attribute converted to type \"type\".");
    n.def("hasAttribute", &NodeWrap().hasAttribute, (bp::arg("key")), "Check that current node has the \"key\" attribute.");
    n.def("setAttributes", &NodeWrap().setAttributes, (bp::arg("attributes")), "Sets new set of attributes in current node.");
    n.def("getAttributes", &NodeWrap().getAttributes, bp::return_internal_reference<1> (), "Gets all attributes in current node as HashAttributes object. This object is internal reference not a copy.");
    n.def("copyAttributes", &NodeWrap().copyAttributes, "Gets a copy of all attributes in current node as HashAttributes object.");
    n.def("getType", &NodeWrap().getType, "Gets the value type as a reference type");
    n.def("setType", &NodeWrap().setType, (bp::arg("type")), "Sets the value type as a reference \"type\".");


    bp::enum_<Hash::MergePolicy>("HashMergePolicy", "This enumeration defines possible options when merging 2 hashes.")
            .value("MERGE_ATTRIBUTES", Hash::MERGE_ATTRIBUTES)
            .value("REPLACE_ATTRIBUTES", Hash::REPLACE_ATTRIBUTES)
            ;

    bp::class_<Hash, boost::shared_ptr<Hash> > h("Hash", "The Hash class can be regarded as a generic hash container, which associates a string key to a value of any type.\n"
                                                 "Optionally attributes of any value-type can be associated to each hash-key.  The Hash preserves insertion order.  The Hash\n"
                                                 "class is much like a XML-DOM container with the difference of allowing only unique keys on a given tree-level.");
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
    h.def("clear", &Hash::clear,
          "h.clear() makes empty the content of current Hash object 'h' (in place).\n");
    h.def("empty", &HashWrap().empty,
          "h.empty() -> True if 'h' is empty otherwise False.\n");
    h.def("getKeys", (void (*)(const karabo::util::Hash&, const bp::object&))&HashWrap().getKeys, (bp::arg("target_container")),
          "This function follows the API of C++ counterpart. Put into the target container all the keys visible\n"
          "on the top level of the tree hierarchy.\n"
          "\nExample:\n\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n"
          "\tmykeys = []\n\th.getKeys(mykeys)\nprint mykeys\n\n... returns:\n\t['a', 'b', 'c']\n");
    h.def("getKeys", &HashWrap().keys,
          "Returns list of all keys visible on the top level of the tree hierarchy.\n"
          "\nExample:\n\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n"
          "\tprint h.getKeys()\n\n... returns:\n\t['a', 'b', 'c']\n");
    h.def("keys", &HashWrap().keys,
          "Returns list of all keys visible on the top level of the tree hierarchy.\n"
          "\nExample:\n\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n"
          "\tprint h.keys()\n\n... returns:\n\t['a', 'b', 'c']\n");
    h.def("getValues", &HashWrap().getValues,
          "Returns list of values associated with keys visible on the top level of the tree hierarchy.\n"
          "\nExample:\n\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n"
          "\tprint h.getValues()\n\n... returns:\n\t[<libkarathon.Hash at 0x3188b18>,\n\t <libkarathon.Hash at 0x3188f68>,\n"
          "\t [True, False, True, True]]\n");

    h.def("values", &HashWrap().getValues,
          "Returns list of values associated with keys visible on the top level of the tree hierarchy.\n"
          "\nExample:\n\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n"
          "\tprint h.values()\n\n... returns:\n\t[<libkarathon.Hash at 0x3188b18>,\n\t <libkarathon.Hash at 0x3188f68>,\n"
          "\t [True, False, True, True]]\n");
    h.def("getPaths", &HashWrap().getPaths, (bp::arg("target_container")),
          "Put into the target container the full paths of current Hash object.\n"
          "\nExample:\n\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n"
          "\tmypaths = []\n\th.getPaths(mypaths)\nprint mypaths\n\n... returns:\n\t['a.b.c', 'b.x', 'b.y', 'c']");
    h.def("getPaths", &HashWrap().paths,
          "Returns list of all the paths being in current Hash object.\n"
          "\nExample:\n\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n"
          "\tprint h.getPaths()\n\n... returns:\n\t['a.b.c', 'b.x', 'b.y', 'c']");
    h.def("paths", &HashWrap().paths,
          "Returns list of all the paths being in current Hash object.\n"
          "\nExample:\n\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n"
          "\tprint h.paths()\n\n... returns:\n\t['a.b.c', 'b.x', 'b.y', 'c']");
    h.def("set", &HashWrap().set, (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."),
          "Set the new 'path'/'value' pair into the current Hash object. The third optional parameter is a separator,\n"
          "used to form a tree hierarchy out of 'path'.\nExample:\n\th = Hash()\n\th.set('a.b.c', 1)\n\th.set('x/y/z', 2, \"/\")\n"
          "\th.set('u/v/w', 3)\n\tprint h");
    h.def("__setitem__", &HashWrap().set, (bp::arg("path"), bp::arg("value"), bp::arg("sep") = "."),
          "h[path] = value <==> h.set(path, value)\nUse this setting of the new path/value item if the default separator fits."
          "\nExample:\n\th = Hash()\n\th['a.b.c'] = 1\n\th.set('x/y/z', 2, \"/\")\n\th['u/v/w'] = 3\n\tprint h");
    h.def("setAs", &HashWrap().setAs, (bp::arg("path"), bp::arg("value"), bp::arg("type"), bp::arg("sep") = "."),
          "h.setAs(path, value, type)\nUse this method if the C++ value type cannot be deduced properly of python value"
          "\nExample:\n\th = Hash()\n\th.setAs('a.b.c', 1L, Types.UINT64)\n\tprint h");            
    h.def("get", &HashWrap().getRef, (bp::arg("path"), bp::arg("sep") = "."),
          "Get the 'value' by 'path'. Optionally, the separator can be defined as second argument.\n"
          "Example:\n\th = Hash('a.b.c', 1)\n\tprint h.get('a/b/c','/')");
    h.def("__getitem__", &HashWrap().getRef, (bp::arg("iterator"), bp::arg("sep") = "."),
          "Use this form of getting the 'value' using the 'path' if you need the default separator.\n"
          "Example:\n\th = Hash('a.b.c', 1)\n\tprint h['a.b.c']");
    h.def("has", &HashWrap().has, (bp::arg("path"), bp::arg("sep") = "."),
          "Returns true if given 'path' is found in current Hash object. Use separator as needed.");
    h.def("__contains__", &HashWrap().has, (bp::arg("path"), bp::arg("sep") = "."),
          "Check if 'path' is known in current Hash object. Use this form if you use the default separator.\n"
          "Example:\n\th = Hash('a.b.c', 1)\n\t...\n\tif 'a.b.c' in h:\n\t\th['a.b.c'] = 2");
    h.def("erase", &HashWrap().erase, (bp::arg("path"), bp::arg("sep") = "."),
          "h.erase(path) -> remove item identified by 'path' from 'h' (in place)\nExample:\n"
          "\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\tprint h\n\t"
          "del h['b.x']\n\tprint h\n\th.erase('b.y')\n\tprint h\n\tdel h['b']");
    h.def("__delitem__", &HashWrap().erase, (bp::arg("path"), bp::arg("sep") = "."),
          "del h[path] <==> h.erase(path)\nExample:\n"
          "\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\tprint h\n\t"
          "del h['b.x']\n\tprint h\n\th.erase('b.y')\n\tprint h\n\tdel h['b']");
    h.def("eraseFound", &HashWrap().eraseFound, (bp::arg("path"), bp::arg("sep") = "."),
          "h.eraseFound(path) -> remove item identified by 'path' from 'h' (in place) if it was found.\n"
          "Returns True if path is found, otherwise False"
          );
    h.def("erasePath", &HashWrap().erasePath, (bp::arg("path"), bp::arg("sep") = "."),
          "h.erase(path) -> remove item identified by 'path' from 'h' (in place)\nExample:\n"
          "\th = Hash('a[0].b[0].c', 1, 'b[0].c.d', 2, 'c.d[0].e', 3, 'd.e', 4, 'e', 5, 'f.g.h.i.j.k', 6)\n\tprint h\n\t"
          "h.erasePath['a[0].b[0].c']\n\tprint h\n\th.erasePath('b[0].c.d')\n\tprint h\n\th.erasePath['c.d[0].e']");
    h.def("__len__", &Hash::size,
          "h.__len__() -> number of (top level) items of Hash mapping <==> len(h) <==> len(h.keys())");
    h.def("bool", &Hash::size);
    h.def("__iter__", bp::iterator<Hash, bp::return_internal_reference<> > (),
          "h.__iter__() <==> iter(h) : iterator of (top level) items of 'h' mapping.\nExample:\n\t"
          "h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\ti = iter(h)       # create iterator\n\t"
          "n = i.next()      # position to the 1st node\n\tprint n.getKey()\n\tprint n.getValue()\n\t"
          "n = i.next()      # position to the 2nd node\n\t...\n\nExample2:\n\t"
          "for n in h:\n\t\tprint n.getKey()\n\t\tprint.getValue()");
    h.def("getAs", &HashWrap().getAs, (bp::arg("path"), bp::arg("type"), bp::arg("sep") = "."),
          "Get value by 'path' and convert it to 'type' type.  Optionally use separator.\n"
          "Example:\n\th = Hash('a.b.c', True)\n\tprint h.getAs('a.b.c', Types.INT32)\n\t"
          "print h.getAs('a.b.c', Types.STRING)\n\tprint h.getAs('a.b.c', Types.DOUBLE)");
    h.def("getType", &HashWrap().getType, (bp::arg("path"), bp::arg("sep") = "."),
          "Get type by 'path'.  Returns 'Types.<value>' object.\n"
          "Example:\n\th = Hash('a.b.c', True)\n\tprint h.getType('a.b.c')");
    h.def("merge", &Hash::merge, (bp::arg("hash")),
          "h.merge(h2) <==> h += h2  :  merging 'h2' into 'h'");
    //    h.def("__add__", &Hash::operator+, (bp::arg("hash1"), bp::arg("hash2")), bp::return_value_policy<copy_non_const_reference>());
    h.def("__iadd__", &Hash::operator+=, (bp::arg("hash")), bp::return_internal_reference<>(),
          "This form of merging is preferable.\nExample:\n"
          "\th = Hash('a.b1.c', 22)\n\th2 = Hash('a.b2.c', 33)\n\th += h2");
    h.def("subtract", &Hash::subtract, (bp::arg("hash")),
          "h.subtract(h2) <==> h -= h2  :  subtracting 'h2' from 'h'");
    //    h.def("__add__", &Hash::operator+, (bp::arg("hash1"), bp::arg("hash2")), bp::return_value_policy<copy_non_const_reference>());
    h.def("__isub__", &Hash::operator-=, (bp::arg("hash")), bp::return_internal_reference<>(),
          "This form of subtracting is preferable.\nExample:\n"
          "\th = Hash('a.b.c', 22, 'a.b.d', 33, 'a.c.d', 44)\n\th2 = Hash('a.b', Hash())\n\th -= h2");

    // Global free function to compare Hash, vector<Hash>, Hash::Node
    def("similar", &similarWrap, (bp::arg("left"), bp::arg("right")),
        "Compares two hashes for equality.\nExample:\n"
        "\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
        "flat = Hash()\n\th.flatten(flat) # 'flat' will contain 'flatten' hash\n\t"
        "tree = Hash()\n\tflat.unflatten(tree)\n\tresult = similar(h, tree)\n"
        "... result will be 'True'");

    h.def("isType", &HashWrap().is, (bp::arg("path"), bp::arg("type"), bp::arg("sep") = "."),
          "h.isType(path, type) -> True if reference type of value in Hash container for given 'path' is equal 'type'.\nExample:\n\t"
          "h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
          "assert h.isType('a.b.c', Types.INT32) == True\n\t"
          "assert h.isType('b.y', Types.DOUBLE) == True\n\t"
          "assert h.isType('c', Types.VECTOR_INT32) == True");
    h.def(bp::self_ns::str(bp::self));
    h.def(bp::self_ns::repr(bp::self));
    //h.def("copy", &HashWrap().pyDict2Hash, (bp::arg("dict"), bp::arg("sep") = "."), bp::return_value_policy<bp::copy_const_reference > ());
    //    h.def("update", &Hash::update, (bp::arg("hash")));
    h.def("flatten", &HashWrap().flatten, (bp::arg("flat"), bp::arg("sep") = "."),
          "Make all key/value pairs flat and put them into 'target' container.  Optionally a separator can be used.\nExample:\n"
          "\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
          "flat = Hash()\n\th.flatten(flat) # 'flat' will contain 'flatten' hash\n\t"
          "tree = Hash()\n\tflat.unflatten(tree)\n\tresult = similar(h, tree)\n"
          "... result will be 'True'");
    h.def("unflatten", &HashWrap().unflatten, (bp::arg("tree"), bp::arg("sep") = "."),
          "Make all key/value pairs tree-like structured and put them into 'target' container.  Optionally use separator.\nExample:\n"
          "\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
          "flat = Hash()\n\th.flatten(flat) # 'flat' will contain 'flatten' hash\n\t"
          "tree = Hash()\n\tflat.unflatten(tree)\n\tresult = similar(h, tree)\n"
          "... result will be 'True'");
    h.def("find", &HashWrap().find, (bp::arg("path"), bp::arg("sep") = "."),
          "Find node in current Hash using \"path\".  Optionally the separator \"sep\" may be defined.\n"
          "Returns not a copy but reference to the existing Hash.Node object or \"None\".\n"
          "If you do any changes via returned object, these changes will be reflected in the current Hash object.\n"
          "Example:\n\th = Hash('a.b.c', 1)\n\tnode = h.find('a.b.c')\n\tif node is not None: node.setValue(2)");
    h.def("setNode", &HashWrap().setNode, (bp::arg("node")),
          "Set \"node\" into current Hash object.  You cannot create node directly, you can extract the node from created Hash object.\nExample:\n"
          "\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
          "n = h.getNode('b')\n\tg = Hash()\n\tg.setNode(n)\n\tprint g");
    h.def("getNode", &HashWrap().getNode, (bp::arg("path"), bp::arg("sep") = "."), //bp::return_internal_reference<1> (),
          "Returns a reference of found node (not a copy!), so if you do any changes via returned object,\n"
          "these changes will be reflected in the current Hash object.\nExample:\n"
          "\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
          "n = h.getNode('b')\n\tg = Hash()\n\tg.setNode(n)\n\tprint g");
    h.def("hasAttribute", &HashWrap().hasAttribute, (bp::arg("path"), bp::arg("attribute"), bp::arg("sep") = "."),
          "Returns true if the questioned attribute exists, else returns false.");
    h.def("getAttribute", &HashWrap().getAttribute, (bp::arg("path"), bp::arg("attribute"), bp::arg("sep") = "."),
          "Get attribute value following given 'path' and 'attribute' name. Optionally use separator.\nExample:\n\t"
          "h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\th.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555]\n\t"
          "assert h.getAttribute('a.b.c', 'attr1') == [1.234,2.987,5.555]");
    h.def("getAttributeAs", &HashWrap().getAttributeAs, (bp::arg("path"), bp::arg("attribute"), bp::arg("type"), bp::arg("sep") = "."),
          "h.getAttributeAs(path, attribute, type, sep = '.') -> value of 'type' type.\nExample:\n\t"
          "h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\th.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555])\n"
          "\nHere you have to be sure that you have imported numpy as np:\n\n\timport numpy as np\n\t"
          "assert h.getAttributeAs('a.b.c', 'attr1', Types.NDARRAY_DOUBLE).all() == np.array([1.234,2.987,5.555], dtype=np.double).all()");
    h.def("getAttributes", &HashWrap().getAttributes, (bp::arg("path"), bp::arg("sep") = "."), bp::return_internal_reference<1> (),
          "h.getAttributes(path, sep='.') -> iterable container of attributes which is an internal reference, not a copy.\nExample:\n\t"
          "h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
          "h.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555])\n\th.setAttribute('a.b.c', 'attr2', 1)\n\t"
          "h.setAttribute('a.b.c', 'attr3', False)\n\t"
          "for a in h.getAttributes('a.b.c'):\n\t\tprint a.getKey(), a.getValue()\n\t"
          "attrs = h.getAttributes('a.b.c')\n\tattrs['attr2'] = 2\n\t"
          "assert h.getAttribute('a.b.c', 'attr2') == 2");
    h.def("copyAttributes", &HashWrap().copyAttributes, (bp::arg("path"), bp::arg("sep") = "."),
          "h.copyAttributes(path, sep='.') -> iterable container of attributes.\nExample:\n\t"
          "h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
          "h.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555])\n\th.setAttribute('a.b.c', 'attr2', 1)\n\t"
          "h.setAttribute('a.b.c', 'attr3', False)\n\t"
          "for a in h.copyAttributes('a.b.c'):\n\t\tprint a.getKey(), a.getValue()"
          "attrs = h.copyAttributes('a.b.c')\n\tattrs['attr2'] = 2\n\t"
          "assert h.getAttribute('a.b.c', 'attr2') == 1");
    h.def("setAttribute", &HashWrap().setAttribute, (bp::arg("path"), bp::arg("attribute"), bp::arg("value"), bp::arg("sep") = "."),
          "Set attribute associated with path.\nExample:\n\th = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
          "h.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555])\n\tassert h.getAttribute('a.b.c', 'attr1') == [1.234,2.987,5.555]");
    h.def("setAttributes", &HashWrap().setAttributes, (bp::arg("path"), bp::arg("attributes"), bp::arg("sep") = "."),
          "h.setAttributes(path, attributes, sep='.') allows to associate 'attributes' with 'path' in this Hash.\nExample:\n\t"
          "h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
          "h.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555])\n\th.setAttribute('a.b.c', 'attr2', 1)\n\t"
          "h.setAttribute('a.b.c', 'attr3', False)\n\ta = h.getAttributes('a.b.c')\n\t"
          "h.setAttributes('c', a)    # copy attributes under the different path");
    h.def("__copy__", &HashWrap().copy);
    h.def("__deepcopy__", &HashWrap().copy);
    h.def(bp::self == bp::self);
    h.def(bp::self != bp::self);

    bp::class_<std::vector<Hash>, boost::shared_ptr<std::vector<Hash> > >("VectorHash")
        .def(bp::vector_indexing_suite<std::vector<Hash> >());
}

