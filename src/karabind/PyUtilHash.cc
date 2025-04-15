/*
 * File: PyUtilHash.cc
 * Author: CONTROLS DEV group
 *
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

#include <pybind11/complex.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <karabo/xms/ImageData.hh>
#include <sstream>
#include <string>
#include <typeinfo>

#include "PyTypes.hh"
#include "Wrapper.hh"
#include "karabo/data/types/FromLiteral.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"


namespace py = pybind11;

namespace karabo {
    namespace data {

        template <>
        Hash::Hash(const std::string& k, const py::object& o) {
            karabind::hashwrap::set(*this, k, o);
        }

        template <>
        Hash::Hash(const std::string& k1, const py::object& o1, const std::string& k2, const py::object& o2) {
            karabind::hashwrap::set(*this, k1, o1);
            karabind::hashwrap::set(*this, k2, o2);
        }

        template <>
        Hash::Hash(const std::string& k1, const py::object& o1, const std::string& k2, const py::object& o2,
                   const std::string& k3, const py::object& o3) {
            karabind::hashwrap::set(*this, k1, o1);
            karabind::hashwrap::set(*this, k2, o2);
            karabind::hashwrap::set(*this, k3, o3);
        }

        template <>
        Hash::Hash(const std::string& k1, const py::object& o1, const std::string& k2, const py::object& o2,
                   const std::string& k3, const py::object& o3, const std::string& k4, const py::object& o4) {
            karabind::hashwrap::set(*this, k1, o1);
            karabind::hashwrap::set(*this, k2, o2);
            karabind::hashwrap::set(*this, k3, o3);
            karabind::hashwrap::set(*this, k4, o4);
        }

        template <>
        Hash::Hash(const std::string& k1, const py::object& o1, const std::string& k2, const py::object& o2,
                   const std::string& k3, const py::object& o3, const std::string& k4, const py::object& o4,
                   const std::string& k5, const py::object& o5) {
            karabind::hashwrap::set(*this, k1, o1);
            karabind::hashwrap::set(*this, k2, o2);
            karabind::hashwrap::set(*this, k3, o3);
            karabind::hashwrap::set(*this, k4, o4);
            karabind::hashwrap::set(*this, k5, o5);
        }

        template <>
        Hash::Hash(const std::string& k1, const py::object& o1, const std::string& k2, const py::object& o2,
                   const std::string& k3, const py::object& o3, const std::string& k4, const py::object& o4,
                   const std::string& k5, const py::object& o5, const std::string& k6, const py::object& o6) {
            karabind::hashwrap::set(*this, k1, o1);
            karabind::hashwrap::set(*this, k2, o2);
            karabind::hashwrap::set(*this, k3, o3);
            karabind::hashwrap::set(*this, k4, o4);
            karabind::hashwrap::set(*this, k5, o5);
            karabind::hashwrap::set(*this, k6, o6);
        }
    } // namespace data
} // namespace karabo


namespace karabind {

    struct HashIteratorAccess {
        py::object operator()(karabo::data::Hash::iterator& it) const {
            return py::make_tuple(py::cast((*it).getKey()), wrapper::castAnyToPy((*it).getValueAsAny()));
        }
    };


    py::iterator make_iterator(karabo::data::Hash::iterator first, karabo::data::Hash::iterator last) {
        return py::detail::make_iterator_impl<HashIteratorAccess, py::return_value_policy::reference_internal,
                                              karabo::data::Hash::iterator, karabo::data::Hash::iterator, py::object>(
              first, last);
    }

    struct HashIteratorAccessAll {
        py::object operator()(karabo::data::Hash::iterator& it) const {
            using namespace karabo::data;
            py::object k = py::cast(it->getKey());
            py::object v = wrapper::castAnyToPy(it->getValueAsAny());
            py::object a = hashwrap::getRefAttributesByNode(*it);
            return py::make_tuple(k, v, a);
        }
    };


    py::iterator make_iterator_all(karabo::data::Hash::iterator first, karabo::data::Hash::iterator last) {
        return py::detail::make_iterator_impl<HashIteratorAccessAll, py::return_value_policy::reference_internal,
                                              karabo::data::Hash::iterator, karabo::data::Hash::iterator, py::object>(
              first, last);
    }

} // namespace karabind


using namespace karabo::data;
using namespace std;
using namespace karabind;


void exportPyUtilHash(py::module_& m) {
    const char cStringSep[] = {karabo::data::Hash::k_defaultSep, '\0'};

    py::enum_<Hash::MergePolicy>(m, "HashMergePolicy",
                                 "This enumeration defines possible options when merging 2 hashes.")
          .value("MERGE_ATTRIBUTES", Hash::MERGE_ATTRIBUTES)
          .value("REPLACE_ATTRIBUTES", Hash::REPLACE_ATTRIBUTES)
          .export_values();

    py::class_<Hash, std::shared_ptr<Hash>> h(m, "Hash", R"pbdoc(
            The Hash class can be regarded as a generic hash container, which
            associates a string key to a value of any type.
            Optionally attributes of any value-type can be associated to each
            hash-key.  The Hash preserves insertion order.  The Hash
            class is much like a XML-DOM container with the difference of
            allowing only unique keys on a given tree-level.
        )pbdoc");

    h.def(py::init<>());
    h.def(py::init<const std::string&>());
    h.def(py::init<const Hash&>());
    h.def(py::init<std::string const&, py::object const&>());
    h.def(py::init<std::string const&, py::object const&, std::string const&, py::object const&>());
    h.def(py::init<std::string const&, py::object const&, std::string const&, py::object const&, std::string const&,
                   py::object const&>());
    h.def(py::init<std::string const&, py::object const&, std::string const&, py::object const&, std::string const&,
                   py::object const&, std::string const&, py::object const&>());
    h.def(py::init<std::string const&, py::object const&, std::string const&, py::object const&, std::string const&,
                   py::object const&, std::string const&, py::object const&, std::string const&, py::object const&>());
    h.def(py::init<std::string const&, py::object const&, std::string const&, py::object const&, std::string const&,
                   py::object const&, std::string const&, py::object const&, std::string const&, py::object const&,
                   std::string const&, py::object const&>());

    h.def(
          "_getref_hash_", [](const Hash& self, Hash* other) { return py::cast(other); }, py::arg("ref"),
          py::return_value_policy::reference_internal, py::keep_alive<0, 1>());

    h.def(
          "_getref_vector_hash_", [](const Hash& self, std::vector<Hash>* other) { return py::cast(other); },
          py::arg("ref"), py::return_value_policy::reference_internal, py::keep_alive<0, 1>());

    h.def(
          "_get_ndarray_",
          [](Hash& self, const Hash& other) {
              return wrapper::castNDArrayToPy(reinterpret_cast<const NDArray&>(other));
          },
          py::arg("ndarray"), py::return_value_policy::reference_internal, py::keep_alive<0, 1>());

    h.def(
          "_getref_attrs_", [](const Hash& self, Hash::Attributes* attrs) { return py::cast(attrs); }, py::arg("ref"),
          py::return_value_policy::reference_internal, py::keep_alive<0, 1>());

    h.def("clear", &Hash::clear, "h.clear() makes empty the content of current Hash object 'h' (in place).\n");

    h.def("empty", &Hash::empty, "h.empty() -> True if 'h' is empty otherwise False.\n");

    struct ItemView {
        Hash& map;
    };

    py::class_<ItemView>(h, "ItemView")
          .def("__len__", [](ItemView& v) { v.map.size(); })
          .def(
                "__iter__", [](ItemView& v) { return make_iterator(v.map.begin(), v.map.end()); },
                py::keep_alive<0, 1>());

    h.def("items", [](Hash& self) { return std::unique_ptr<ItemView>(new ItemView{self}); }, py::keep_alive<0, 1>());

    struct ItemViewAll {
        Hash& map;
    };

    py::class_<ItemViewAll>(h, "ItemViewAll")
          .def("__len__", [](ItemView& v) { v.map.size(); })
          .def(
                "__iter__", [](ItemViewAll& v) { return make_iterator_all(v.map.begin(), v.map.end()); },
                py::keep_alive<0, 1>());

    h.def(
          "iterall", [](Hash& self) { return std::unique_ptr<ItemViewAll>(new ItemViewAll{self}); },
          py::keep_alive<0, 1>());

    h.def_static(
          "flat_iterall",
          [](Hash& self) {
              Hash flat;
              Hash::flatten(self, flat);
              return py::cast(flat).attr("iterall")();
          },
          py::keep_alive<0, 1>());

    h.def(
          "getKeys",
          [](const karabo::data::Hash& self, py::list& list) {
              std::vector<std::string> v{};
              self.getKeys(v);
              for (const auto& i : v) list.append(i);
          },
          py::arg("list"), R"pbdoc(
            Get keys in current Hash object and put them to python list argument.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                mykeys = []
                h.getKeys(mykeys)
                print(mykeys)

                ... returns: ['a', 'b', 'c']

          )pbdoc");

    // Overloaded getKeys
    h.def(
          "getKeys",
          [](const Hash& self) -> py::list {
              std::vector<std::string> v{};
              self.getKeys(v);
              return py::cast(std::move(v));
          },
          R"pbdoc(

            Returns list of all keys visible on the top level of the tree hierarchy.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                print(h.getKeys())

                ... returns:
                    ['a', 'b', 'c']
          )pbdoc");

    h.def(
          "keys", [](const Hash& self) { return py::cast(self).attr("getKeys")(); },
          R"pbdoc(

            Returns list of all keys visible on the top level of the tree hierarchy.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                print(h.keys())

                ... returns:
                    ['a', 'b', 'c']
          )pbdoc");

    h.def(
          "getValues",
          [](Hash& self) {
              std::vector<py::object> v{};
              for (Hash::const_iterator it = self.begin(); it != self.end(); ++it) {
                  v.push_back(wrapper::castAnyToPy(it->getValueAsAny()));
              }
              return py::cast(std::move(v));
          },
          R"pbdoc(
            Returns list of values associated with keys visible on the top level of the tree hierarchy.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                assert(h.getValues(), [Hash('b.c', 1), Hash('x', 2.22), Hash('y', 7.432), [1,2,3]])
          )pbdoc");

    h.def(
          "values", [](Hash& self) { return py::cast(self).attr("getValues")(); },
          R"pbdoc(
            Returns list of values associated with keys visible on the top level of the tree hierarchy.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                print(h.values())

                ... returns:

                ['b' +
                   'c' => 1 INT32
                 , 'x' => 2.22 DOUBLE
                   'y' => 7.432 DOUBLE
                 , [1, 2, 3]]
          )pbdoc");

    h.def(
          "getPaths",
          [](const Hash& self, py::list& o) {
              std::vector<std::string> v{};
              self.getPaths(v);
              for (const auto& i : v) o.append(i);
          },
          py::arg("target_container"), R"pbdoc(

            Put into the target container the full paths of current Hash object.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                mypaths = []
                h.getPaths(mypaths)
                print(mypaths)

                ... returns:
                        ['a.b.c', 'b.x', 'b.y', 'c']
          )pbdoc");

    h.def(
          "getPaths",
          [](const Hash& self) -> py::list {
              std::vector<std::string> v{};
              self.getPaths(v);
              return py::cast(std::move(v));
          },
          R"pbdoc(

            Returns list of all the paths being in current Hash object.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                print(h.getPaths())

                ... returns:
                    ['a.b.c', 'b.x', 'b.y', 'c']
          )pbdoc");

    h.def(
          "paths", [](const Hash& self) { return py::cast(self).attr("getPaths")(); },
          R"pbdoc(

            Returns list of all the paths being in current Hash object.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                print(h.paths())

                ... returns:
                    ['a.b.c', 'b.x', 'b.y', 'c']
          )pbdoc");

    h.def(
          "set",
          [](Hash& self, const std::string& key, const py::object& o, const std::string& sep) {
              karabind::hashwrap::set(self, key, o, sep);
          },
          py::arg("path"), py::arg("value"), py::arg("sep") = cStringSep,
          R"pbdoc(
            Set the new 'path'/'value' pair into the current Hash object. The third optional parameter is a separator,
            used to form a tree hierarchy out of 'path'.

            Example:
                h = Hash()
                h.set('a.b.c', 1)
                h.set('x/y/z', 2, \"/\")
                h.set('u/v/w', 3)
                print(h)
          )pbdoc");

    h.def(
          "__setitem__",
          [](Hash& self, const py::object& item, const py::object& value) {
              if (py::isinstance<py::tuple>(item)) {
                  py::tuple tup = item.cast<py::tuple>();
                  if (py::len(tup) != 2) throw py::cast_error("Invalid size");
                  auto key = tup[0].cast<std::string>();
                  Hash::Node& node = self.getNode(key, '.');
                  if (py::isinstance<py::ellipsis>(tup[1])) {
                      Hash::Attributes attrs;
                      wrapper::castPyToHashAttributes(value, attrs);
                      node.setAttributes(attrs);
                  } else if (py::isinstance<py::str>(tup[1])) {
                      auto attr = tup[1].cast<std::string>();
                      std::any anyval;
                      wrapper::castPyToAny(value, anyval);
                      self.setAttribute(key, attr, anyval);
                  } else {
                      throw py::cast_error("Invalid second index in tuple: only string or ellipsis allowed");
                  }
              } else if (py::isinstance<py::str>(item)) {
                  auto key = item.cast<std::string>();
                  karabind::hashwrap::set(self, key, value, ".");
              } else {
                  throw py::cast_error("Invalid index type: string or tuple expected");
              }
          },
          py::arg("path_or_tuple"), py::arg("value_or_dict"),
          R"pbdoc(
              h[path] = value <==> h.set(path, value)
              Use this setting of the new path/value item if the default separator fits.

              Example:
                  h = Hash()
                  h['a.b.c'] = 1
                  h['a.b.c', ...] = {'a':1, 'b':2}  # define attributes (REPLACE policy)
                  h['a.b.c', ...].update({'c':3})   # update attributes (MERGE policy)
                  h.set('x/y/z', 2, \"/\")
                  h['u/v/w'] = 3
                  print(h)
            )pbdoc");

    h.def(
          "setAs",
          [](Hash& self, const std::string& key, const py::object& value, const py::object& otype,
             const std::string& separator) {
              karabind::hashwrap::set(self, key, value, separator);
              Hash::Node& node = self.getNode(key, separator.at(0));
              auto cppType = wrapper::pyObjectToCppType(otype);
              node.setType(cppType);
          },
          py::arg("path"), py::arg("value"), py::arg("type"), py::arg("sep") = cStringSep,
          R"pbdoc(
            h.setAs(path, value, type)
            Use this method if the C++ value type cannot be deduced properly of python value

            Example:
                h = Hash()
                h.setAs('a.b.c', 1, Types.UINT64)
                print(h)
          )pbdoc");

    h.def(
          "get",
          [](Hash& self, const std::string& path, const std::string& separator, const py::object& default_return) {
              // This implements the standard Python dictionary behavior for get()
              if (!self.has(path, separator.at(0))) return default_return;
              return hashwrap::getRef(self, path, separator);
          },
          py::arg("path"), py::arg("sep") = cStringSep, py::arg("default") = py::none(),
          py::return_value_policy::reference_internal,
          R"pbdoc(
            Get the 'value' by 'path'. Optionally, the separator can be defined as second argument.
            If you want to emulate the Python dictionary get() method, a default return value can be
            passed using the 'default' keyword argument.

            Example:
                h = Hash('a.b.c', 1)
                print(h.get('a/b/c','/'))
                g = h.get('a.b')  # reference to Hash('c', 1)
                g.set('c', 2)
                assert(h.get('a.b.c') == 2)
                del(h)        # delete whole tree
                print(g)      # SEGFAULT: dangling reference
          )pbdoc");

    h.def(
          "__getitem__",
          [](Hash& self, py::object item) {
              if (py::isinstance<py::tuple>(item)) {
                  py::tuple tup = item.cast<py::tuple>();
                  if (py::len(tup) != 2) throw py::cast_error("Invalid size");
                  auto path = tup[0].cast<std::string>();
                  if (py::isinstance<py::ellipsis>(tup[1])) { // return ref to all attributes
                      return hashwrap::getRefAttributes(self, path, ".");
                  } else { // return specific attribute value
                      auto attr = tup[1].cast<std::string>();
                      return wrapper::castAnyToPy(self.getAttributeAsAny(path, attr));
                  }
              } else if (py::isinstance<py::str>(item)) {
                  // return ref to value
                  auto path = item.cast<std::string>();
                  return hashwrap::getRef(self, path, ".");
              } else {
                  throw py::cast_error("Invalid index type: a string or tuple expected");
              }
          },
          py::arg("key"), py::return_value_policy::reference_internal,
          R"pbdoc(
            Use this form of getting the 'value' using the 'path' if you need the default separator.

            Example:
                h = Hash('a.b.c', 1)
                # set attributes as dict for 'c' key
                h['a.b.c', ...] = {'attr1': 12, 'attr2': {'msg': 'Invalid arg'}}
                print(h['a.b.c'])
                # get one attribute...
                print(h['a.b.c', 'attr1'])
                g = h['a.b']         # reference (no counter)
                attrs = h['a.b.c', ...]  # get reference to attributes
                del(h)
                print(g)             # SEGFAULT, only del(g) is allowed
          )pbdoc");

    h.def(
          "has",
          [](Hash& self, const std::string& key, const std::string& separator) {
              return self.has(key, separator.at(0));
          },
          py::arg("path"), py::arg("sep") = cStringSep,
          "Returns true if given 'path' is found in current Hash object. Use separator as needed.");

    h.def(
          "__contains__",
          [](Hash& self, const std::string& key, const std::string& separator) {
              return self.has(key, separator.at(0));
          },
          py::arg("path"), py::arg("sep") = cStringSep,
          R"pbdoc(
            Check if 'path' is known in current Hash object. Use this form if you use the default separator.

            Example:
                h = Hash('a.b.c', 1)
                ...
                if 'a.b.c' in h:
                    h['a.b.c'] = 2
          )pbdoc");

    h.def(
          "erase",
          [](Hash& self, const std::string& path, const std::string& separator) -> py::bool_ {
              return self.erase(path, separator.at(0));
          },
          py::arg("path"), py::arg("sep") = cStringSep,
          R"pbdoc(
            h.erase(path) -> remove item identified by 'path' from 'h' if it exists (in place)
            Returns True if path is found, otherwise False

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                print(h)
                del(h['b.x'])
                print(h)
                h.erase('b.y')
                print(h)
                del(h['b'])
                print(h)
          )pbdoc");

    h.def(
          "__delitem__",
          [](Hash& self, const std::string& path, const std::string& separator) { self.erase(path, separator.at(0)); },
          py::arg("path"), py::arg("sep") = cStringSep,
          R"pbdoc(
            del h[path] <==> h.erase(path) <==> h.erasePath(path)

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                print(h)
                del(h['b.x'])
                print(h)
                h.erase('b.y')
                print(h)
                del h['b']
                print(h)
          )pbdoc");

    h.def(
          "erasePath",
          [](Hash& self, const std::string& path, const std::string& separator) {
              self.erasePath(path, separator.at(0));
          },
          py::arg("path"), py::arg("sep") = cStringSep,
          R"pbdoc(
            h.erase(path) -> remove item identified by 'path' from 'h' (in place)

            Example:
                h = Hash('a[0].b[0].c', 1, 'b[0].c.d', 2, 'c.d[0].e', 3, 'd.e', 4, 'e', 5, 'f.g.h.i.j.k', 6)
                print(h)
                h.erasePath['a[0].b[0].c']
                print(h)
                h.erasePath('b[0].c.d')
                print(h)
                h.erasePath['c.d[0].e']
                print(h)
          )pbdoc");

    h.def("__len__", &Hash::size,
          "h.__len__() -> number of (top level) items of Hash mapping <==> len(h) <==> len(h.keys())");

    h.def("__bool__", [](const Hash& self) -> bool { return !self.empty(); });

    h.def(
          "__iter__", [](const Hash& hr) { return py::make_iterator(hr.begin(), hr.end()); }, py::keep_alive<0, 1>(),
          R"pbdoc(
            h.__iter__() <==> iter(h) : iterator of (top level) items of 'h' mapping.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                i = iter(h)       # create iterator\n\t"
                n = i.next()      # position to the 1st node
                print(n.getKey())
                print(n.getValue())
                n = i.next()      # position to the 2nd node
                ...
            Example2:
                for n in h:
                    print(n.getKey())
                    print(n.getValue())
          )pbdoc");

    h.def(
          "getAs",
          [](const Hash& self, const std::string& path, const py::object& otype, const std::string& sep) {
              auto type = wrapper::pyObjectToCppType(otype);
              return hashwrap::getAs(self, path, type, sep);
          },
          py::arg("path"), py::arg("type"), py::arg("sep") = cStringSep,
          R"pbdoc(
            Get value by 'path' and convert it to 'type' type.  Optionally use separator.

            Example:
                h = Hash('a.b.c', True)
                print(h.getAs('a.b.c', Types.INT32))
                print(h.getAs('a.b.c', Types.STRING))
                print(h.getAs('a.b.c', Types.DOUBLE))
          )pbdoc");

    h.def(
          "getType",
          [](const Hash& self, const std::string& path, const std::string& separator) {
              auto type = static_cast<PyTypes::ReferenceType>(self.getType(path, separator.at(0)));
              return py::cast(type);
          },
          py::arg("path"), py::arg("sep") = cStringSep,
          R"pbdoc(
            Get type by 'path'.  Returns 'Types.<value>' object.

            Example:
                h = Hash('a.b.c', True)
                print(h.getType('a.b.c')
          )pbdoc");

    h.def(
          "merge",
          [](Hash& self, const Hash& other, const Hash::MergePolicy policy, const py::object& selectedPaths,
             const std::string& separator) {
              std::set<std::string> selectedPathsCpp;
              if (!selectedPaths.is_none()) {
                  // Cannot cast to std::set directly ... to std::vector
                  const auto& v = selectedPaths.cast<std::vector<std::string>>();
                  selectedPathsCpp.insert(v.begin(), v.end());
              }
              self.merge(other, policy, selectedPathsCpp, separator.at(0));
          },
          py::arg("hash"), py::arg("policy") = Hash::REPLACE_ATTRIBUTES, py::arg("selectedPaths") = py::none(),
          py::arg("sep") = cStringSep,
          R"pbdoc(
            h.merge(h2) <==> h += h2  :  merging 'h2' into 'h'.

            But merge allows to specify more details:
                policy:        REPLACE_ATTRIBUTES or MERGE_ATTRIBUTES
                selectedPaths: an iterable of paths in h2 to select
                                (None means to select all)
                sep:           single letter between keys in paths of nested Hash
          )pbdoc");

    h.def("__iadd__", &Hash::operator+=, py::arg("hash"), py::return_value_policy::reference_internal,
          R"pbdoc(
            This form of merging is preferable.

            Example:
                h = Hash('a.b1.c', 22)
                h2 = Hash('a.b2.c', 33)
                h += h2
          )pbdoc");

    h.def(
          "subtract",
          [](Hash& self, const Hash& other, const std::string& separator) { self.subtract(other, separator.at(0)); },
          py::arg("hash"), py::arg("sep") = cStringSep, "h.subtract(h2) <==> h -= h2  :  subtracting 'h2' from 'h'");

    h.def("__isub__", &Hash::operator-=, py::arg("hash"), py::return_value_policy::reference_internal,
          R"pbdoc(
            This form of subtracting is preferable.

            Example:
                h = Hash('a.b.c', 22, 'a.b.d', 33, 'a.c.d', 44)
                h2 = Hash('a.b', Hash())
                h -= h2
          )pbdoc");

    // Global free function to compare Hash, vector<Hash>, Hash::Node, Schema
    m.def(
          "similar",
          [](const py::object& left, const py::object& right) {
              if (py::isinstance<Hash>(left) && py::isinstance<Hash>(right)) {
                  const Hash& lhash = left.cast<Hash>();
                  const Hash& rhash = right.cast<Hash>();
                  return similar(lhash, rhash);
              }
              if (py::isinstance<Hash::Node>(left) && py::isinstance<Hash::Node>(right)) {
                  const Hash::Node& lnode = left.cast<Hash::Node>();
                  const Hash::Node& rnode = right.cast<Hash::Node>();
                  return similar(lnode, rnode);
              }
              if (py::isinstance<py::list>(left) && py::isinstance<py::list>(right)) {
                  const auto& vl = left.cast<std::vector<py::object>>();
                  const auto& vr = right.cast<std::vector<py::object>>();
                  py::object left0 = vl[0];
                  py::object right0 = vr[0];
                  if (vl.size() == vr.size()) {
                      if (py::isinstance<Hash>(left0) && py::isinstance<Hash>(right0)) {
                          auto vleft = left.cast<std::vector<Hash>>();
                          auto vright = right.cast<std::vector<Hash>>();
                          return similar(vleft, vright);
                      }
                  }
              }
              if (py::isinstance<Schema>(left) && py::isinstance<Schema>(right)) {
                  const auto& l = left.cast<Schema>();
                  const auto& r = right.cast<Schema>();
                  return similar(l, r);
              }
              return false;
          },
          py::arg("left"), py::arg("right"),
          R"pbdoc(
            Compares two hash related objects for similar structure: Hash, HashNode, list of Hash, Schema

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                flat = Hash()\n\th.flatten(flat) # 'flat' will contain 'flatten' hash
                tree = Hash()
                flat.unflatten(tree)
                result = similar(h, tree)
                # ... result will be 'True'
          )pbdoc");

    // Global free function to compare Hash, // TODO: could be extended for vector<Hash>, Hash::Node or Schema
    m.def(
          "fullyEqual",
          [](const Hash& left, const Hash& right, bool orderMatters) { return left.fullyEquals(right, orderMatters); },
          py::arg("left"), py::arg("right"), py::arg("orderMatters") = true,
          R"pbdoc(
            Compares two hashes for exact equality: keys, types, attributes, values
            If orderMatters is True (default) also the order of keys matters.
        )pbdoc");

    h.def(
          "isType",
          [](Hash& self, const std::string& path, const py::object& otype, const std::string& separator) {
              Types::ReferenceType targetType = wrapper::pyObjectToCppType(otype);
              Types::ReferenceType type = self.getType(path, separator.at(0));
              return (type == targetType);
          },
          py::arg("path"), py::arg("type"), py::arg("sep") = cStringSep,
          R"pbdoc(
            h.isType(path, type) -> True if reference type of value in Hash container for given 'path' is equal
            'type'.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                assert(h.isType('a.b.c', Types.INT32) == True)
                assert(h.isType('b.y', Types.DOUBLE) == True)
                assert(h.isType('c', Types.VECTOR_INT32) == True)
          )pbdoc");

    h.def(
          "flatten",
          [](const Hash& self, Hash& flat, const std::string& separator) { self.flatten(flat, separator.at(0)); },
          py::arg("flat"), py::arg("sep") = cStringSep,
          R"pbdoc(
            Make all key/value pairs flat and put them into 'target' container.  Optionally a separator can be
            used.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                flat = Hash()
                h.flatten(flat) # 'flat' will contain 'flatten' hash
                tree = Hash()
                flat.unflatten(tree)
                result = similar(h, tree)
                # ... result will be 'True'
          )pbdoc");

    h.def(
          "unflatten",
          [](const Hash& self, Hash& tree, const std::string& separator) { self.unflatten(tree, separator.at(0)); },
          py::arg("tree"), py::arg("sep") = cStringSep,
          R"pbdoc(
            Make all key/value pairs tree-like structured and put them into 'target' container.  Optionally use
            separator.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                flat = Hash()\n\th.flatten(flat) # 'flat' will contain 'flatten' hash
                tree = Hash()
                flat.unflatten(tree)
                result = similar(h, tree)

                ... result will be 'True'
          )pbdoc");

    h.def(
          "find",
          [](Hash& self, const std::string& path, const std::string& separator) {
              using namespace karabo::data;
              boost::optional<Hash::Node&> node = self.find(path, separator.at(0));
              if (!node) return std::shared_ptr<karabo::data::Hash::Node>();
              // Wrapping the pointer to the existing memory location with null deleter
              return std::shared_ptr<Hash::Node>(&node.get(), [](Hash::Node*) {});
          },
          py::arg("path"), py::arg("sep") = cStringSep,
          R"pbdoc(
          Find node in current Hash using \"path\".  Optionally the separator \"sep\" may be defined.
          Returns not a copy but reference to the existing Hash.Node object or \"None\".
          If you do any changes via returned object, these changes will be reflected in the current Hash object.

          Example:
            h = Hash('a.b.c', 1)
            node = h.find('a.b.c')
            if node is not None:
                node.setValue(2)
          )pbdoc");

    h.def(
          "setNode",
          [](karabo::data::Hash& self, const py::object& node) {
              if (py::isinstance<const karabo::data::Hash::Node&>(node)) {
                  return py::cast(self.setNode(node.cast<const karabo::data::Hash::Node&>()));
              }
              throw KARABO_PYTHON_EXCEPTION("Failed to extract C++ 'const Hash::Node&' from python object");
          },
          py::arg("node"),
          R"pbdoc(
            Set 'node' into current Hash object.  You cannot create node directly, you can extract the node from
            created Hash object.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                n = h.getNode('b')
                g = Hash()
                g.setNode(n)
                print(g)
          )pbdoc");

    h.def(
          "getNode",
          [](Hash& self, const std::string& path, const std::string& separator) {
              Hash::Node& nodeRef = self.getNode(path, separator.at(0));
              auto node = std::shared_ptr<Hash::Node>(&nodeRef, [](void*) {});
              return py::cast(node);
          },
          py::arg("path"), py::arg("sep") = cStringSep,
          R"pbdoc(

            Returns a reference of found node (not a copy!), so if you do any changes via returned object,
            these changes will be reflected in the current Hash object.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                n = h.getNode('b')
                g = Hash()
                g.setNode(n)
                print(g)
          )pbdoc");

    h.def(
          "hasAttribute",
          [](Hash& self, const std::string& path, const std::string& attribute, const std::string& separator) {
              return self.hasAttribute(path, attribute, separator.at(0));
          },
          py::arg("path"), py::arg("attribute"), py::arg("sep") = cStringSep,
          "Returns true if the questioned attribute exists, else returns false.");

    h.def(
          "getAttribute",
          [](karabo::data::Hash& self, const std::string& path, const std::string& attribute,
             const std::string& separator) {
              return wrapper::castAnyToPy(self.getAttributeAsAny(path, attribute, separator.at(0)));
          },
          py::arg("path"), py::arg("attribute"), py::arg("sep") = cStringSep,
          R"pbdoc(
            Get attribute value following given 'path' and 'attribute' name. Optionally use separator.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                h.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555]
                assert(h.getAttribute('a.b.c', 'attr1') == [1.234,2.987,5.555])
          )pbdoc");

    h.def(
          "getAttributeAs",
          [](Hash& self, const std::string& path, const std::string& attribute, const PyTypes::ReferenceType& pytype,
             const std::string& sep) {
              Types::ReferenceType type = PyTypes::to(pytype);
              const Hash::Node& node = self.getNode(path, sep.at(0));
              const Hash::Attributes::Node& anode = node.getAttributeNode(attribute);
              return wrapper::detail::castElementToPy(anode, type);
          },
          py::arg("path"), py::arg("attribute"), py::arg("type"), py::arg("sep") = cStringSep,
          R"pbdoc(
              h.getAttributeAs(path, attribute, type, sep = '.') -> value of 'type' type.

              Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                h.setAttribute('a.b.c', 'attr1', [1.234, 2.987, 5.555])

                Here you have to be sure that you have imported numpy as np:

                import numpy as np
                assert(h.getAttributeAs('a.b.c', 'attr1', Types.NDARRAY).all()
                        == np.array([1.234,2.987,5.555], dtype=np.double).all())
          )pbdoc");

    h.def(
          "getAttributeType",
          [](const Hash& self, const std::string& path, const std::string& attributeKey, const std::string& sep) {
              const auto& attrs = self.getAttributes(path, sep.at(0));
              const auto& node = attrs.getNode(attributeKey);
              PyTypes::ReferenceType type = static_cast<PyTypes::ReferenceType>(node.getType());
              return py::cast(type);
          },
          py::arg("path"), py::arg("key"), py::arg("sep") = cStringSep,
          "Returns ReferenceType for given \"path\" and attribute \"key\"");

    h.def(
          "getAttributes",
          [](const Hash& self, const std::string& path, const std::string& separator) -> const Hash::Attributes* {
              return &self.getAttributes(path, separator.at(0));
          },
          py::arg("path"), py::arg("sep") = cStringSep, py::return_value_policy::reference_internal, R"pbdoc(
              h.getAttributes(path, sep='.') -> iterable container of attributes which is an internal reference,
                                                not a copy.
              Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])\n\t"
                h.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555])
                h.setAttribute('a.b.c', 'attr2', 1)
                h.setAttribute('a.b.c', 'attr3', False)
                for a in h.getAttributes('a.b.c'):
                    print(a.getKey(), a.getValue())
                attrs = h.getAttributes('a.b.c')
                attrs['attr2'] = 2
                assert(h.getAttribute('a.b.c', 'attr2') == 2)
          )pbdoc");

    h.def(
          "copyAttributes",
          [](Hash& self, const std::string& path, const std::string& separator) {
              return py::cast(self.getAttributes(path, separator.at(0)));
          },
          py::arg("path"), py::arg("sep") = cStringSep,
          R"pbdoc(
              h.copyAttributes(path, sep='.') -> iterable container of attributes.

              Example:
                    h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                    h.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555])\n\th.setAttribute('a.b.c', 'attr2', 1)
                    h.setAttribute('a.b.c', 'attr3', False)
                    for a in h.copyAttributes('a.b.c'):
                        print(a.getKey(), a.getValue())
                    attrs = h.copyAttributes('a.b.c')
                    attrs['attr2'] = 2
                    assert(h.getAttribute('a.b.c', 'attr2') == 1)
          )pbdoc");

    h.def(
          "setAttribute",
          [](Hash& self, const std::string& path, const std::string& attribute, const py::object& value,
             const std::string& separator) {
              std::any any;
              wrapper::castPyToAny(value, any);
              self.setAttribute(path, attribute, any, separator.at(0));
          },
          py::arg("path"), py::arg("attribute"), py::arg("value"), py::arg("sep") = cStringSep,
          R"pbdoc(
            Set attribute associated with path.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1, 2, 3])
                h.setAttribute('a.b.c', 'attr1', [1.234, 2.987, 5.555])
                assert(h.getAttribute('a.b.c', 'attr1') == [1.234, 2.987, 5.555])
          )pbdoc");

    h.def(
          "setAttributes",
          [](Hash& self, const std::string& path, const Hash::Attributes& attributes, const std::string& separator) {
              self.setAttributes(path, attributes, separator.at(0));
          },
          py::arg("path"), py::arg("attributes"), py::arg("sep") = cStringSep,
          R"pbdoc(
            h.setAttributes(path, attributes, sep='.') allows to associate 'attributes' with 'path' in this Hash.

            Example:
                h = Hash('a.b.c', 1, 'b.x', 2.22, 'b.y', 7.432, 'c', [1,2,3])
                h.setAttribute('a.b.c', 'attr1', [1.234,2.987,5.555])
                h.setAttribute('a.b.c', 'attr2', 1)
                h.setAttribute('a.b.c', 'attr3', False)
                a = h.getAttributes('a.b.c')
                h.setAttributes('c', a)    # copy attributes under the different path
          )pbdoc");

    h.def("__copy__", [](const Hash& self) { return self; });
    using namespace pybind11::literals;
    h.def(
          "__deepcopy__",
          [](const Hash& self, py::dict) {
              Hash h;
              std::vector<std::string> paths;
              self.getPaths(paths); // Not a "deep" paths
              for (const auto& path : paths) {
                  const Hash::Node& node = self.getNode(path);
                  if (node.hasAttribute(KARABO_HASH_CLASS_ID)) {
                      const std::string& classId = node.getAttribute<string>(KARABO_HASH_CLASS_ID);
                      if (classId == "NDArray") {
                          const NDArray& nda = reinterpret_cast<const NDArray&>(node.getValue<Hash>());
                          NDArray arr = nda.copy(); // copied NDArray
                          h.set(path, arr);
                      } else if (classId == "ImageData") {
                          using karabo::xms::ImageData;
                          const ImageData& img = reinterpret_cast<const ImageData&>(node.getValue<Hash>());
                          ImageData newimg = img.copy();
                          h.set(path, newimg);
                      } else
                          throw KARABO_NOT_SUPPORTED_EXCEPTION("Currently no 'deepcopy' support for subtype: " +
                                                               classId);
                  } else {
                      h.set(path, node.getValueAsAny());
                  }
              }
              return py::cast(h);
          },
          "memo"_a);

    // These 2 lines provide binding of Hash operators: '==' and '!=' defined in C++ karabo::data::Hash class.
    // The binding looks the same as in boost python, see the link ...
    // https://www.boost.org/doc/libs/1_51_0/libs/python/doc/v2/operators.html#self_t-spec-ops

    h.def(py::self == py::self);
    h.def(py::self != py::self);

    h.def("__str__", [](const Hash& self) {
        std::ostringstream oss;
        oss << "<";
        try {
            karabind::wrapper::hashToStream(oss, self, 0);
        } catch (...) {
            KARABO_RETHROW;
        }
        oss << ">";
        return oss.str();
    });

    h.def("__repr__", [](const Hash& self) {
        std::ostringstream oss;
        oss << "<";
        try {
            karabind::wrapper::hashToStream(oss, self, 0);
        } catch (...) {
            KARABO_RETHROW;
        }
        oss << ">";
        return oss.str();
    });

    // The following statements require #include <pybind11/stl_bind.h> & PYBIND11_MAKE_OPAQUE
    auto vh = py::bind_vector<std::vector<Hash>>(m, "VectorHash");

    vh.def("__copy__", [](const std::vector<Hash>& self) {
        std::vector<py::object> vhc;
        for (const auto& h : self) {
            vhc.push_back(py::cast(h).attr("__copy__")());
        }
        return py::cast(std::move(vhc));
    });

    vh.def("__deepcopy__", [](const std::vector<Hash>& self, py::dict memo) {
        std::vector<py::object> vhc;
        for (const Hash& h : self) {
            auto g = py::cast(h).attr("__deepcopy__")(memo);
            vhc.push_back(g);
        }
        return py::cast(std::move(vhc));
    });

    // Pickling does not work ... first needs to implement pickling for Hash
    // vh.def(py::pickle(
    //       [](const std::vector<Hash>& v) { // __getstate__
    //           return py::tuple(py::cast(v));
    //       },
    //       [](py::tuple t) { // __setstate__
    //           return t.cast<std::vector<Hash>>();
    //       }));

    //     py::class_<CppArrayRefHandler, std::shared_ptr<CppArrayRefHandler>>(m, "_CppArrayRefHandler_")
    //         .def(py::init<CppArrayRefHandler>());

    auto vhp = py::bind_vector<std::vector<Hash::Pointer>>(m, "VectorHashPointer");

    // vhp.def(py::pickle([](const std::vector<Hash::Pointer>& v) { return py::tuple(py::cast(v)); },
    //                    [](py::tuple t) { return t.cast<std::vector<Hash::Pointer>>(); }));
}
