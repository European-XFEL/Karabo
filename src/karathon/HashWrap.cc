/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <set>
#include <complex>
#include <sstream>
#include <iostream>             // std::cout, std::endl
#include <algorithm>            // std::copy

#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/Types.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/ToLiteral.hh>
#include "Wrapper.hh"

#include "HashWrap.hh"

namespace bp = boost::python;

namespace karabo {
    namespace util {

        template<>
        karabo::util::Hash::Hash(const std::string& key, const bp::object& value) {
            karathon::HashWrap::set(*this, key, value);
        }

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2) {
            karathon::HashWrap::set(*this, key1, value1);
            karathon::HashWrap::set(*this, key2, value2);
        }

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3) {
            karathon::HashWrap::set(*this, key1, value1);
            karathon::HashWrap::set(*this, key2, value2);
            karathon::HashWrap::set(*this, key3, value3);
        }

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4) {
            karathon::HashWrap::set(*this, key1, value1);
            karathon::HashWrap::set(*this, key2, value2);
            karathon::HashWrap::set(*this, key3, value3);
            karathon::HashWrap::set(*this, key4, value4);
        }

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4,
                                 const std::string& key5, const bp::object& value5) {
            karathon::HashWrap::set(*this, key1, value1);
            karathon::HashWrap::set(*this, key2, value2);
            karathon::HashWrap::set(*this, key3, value3);
            karathon::HashWrap::set(*this, key4, value4);
            karathon::HashWrap::set(*this, key5, value5);
        }

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4,
                                 const std::string& key5, const bp::object& value5,
                                 const std::string& key6, const bp::object& value6) {
            karathon::HashWrap::set(*this, key1, value1);
            karathon::HashWrap::set(*this, key2, value2);
            karathon::HashWrap::set(*this, key3, value3);
            karathon::HashWrap::set(*this, key4, value4);
            karathon::HashWrap::set(*this, key5, value5);
            karathon::HashWrap::set(*this, key6, value6);
        }
    }
}

namespace karathon {

    void
    HashWrap::setPyListAsStdVector(karabo::util::Hash& self,
                                   const std::string& key,
                                   const bp::object& list,
                                   bp::ssize_t size,
                                   const char sep) {
        // elements of our vectors require to be of the same type
        bp::object list0 = list[0];
        if (PyLong_Check(list0.ptr())) {
            std::vector<int> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<int>(list[i]);
            }
            self.set(key, v, sep);
        } else if (PyFloat_Check(list0.ptr())) {
            std::vector<double> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<double>(list[i]);
            }
            self.set(key, v, sep);
        } else if (PyUnicode_Check(list0.ptr())) {
            std::vector<std::string> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<std::string > (list[i]);
            }
            self.set(key, v, sep);

        } else if (PyLong_Check(list0.ptr())) {
            std::vector<long long> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<long>(list[i]);
            }
            self.set(key, v, sep);

        } else if (PyBool_Check(list0.ptr())) {
            std::deque<bool> v(size); // Special case here
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<bool>(list[i]);
            }
            self.set(key, v, sep);

            //                } else if (PyDict_Check(list0.ptr())) {
            //                    std::vector<karabo::util::Hash> v(size);
            //                    for (bp::ssize_t i = 0; i < size; ++i) {
            //                        bp::dict d = bp::extract<bp::dict > (list[i]);
            //                        fromPyDictToHash(v[i], d, sep);
            //                    }
            //                    self.set(key, v, sep);

        } else if (bp::extract<karabo::util::Hash > (list[0]).check()) {
            std::vector<karabo::util::Hash> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<karabo::util::Hash > (list[i]);
            }
            self.set(key, v, sep);

        } else {
            throw KARABO_PYTHON_EXCEPTION("Failed to convert inner type of python list");
        }
    }

    void
    HashWrap::setPyStrAsStdVector(karabo::util::Hash& self,
                                  const std::string& key,
                                  const bp::object& pystr,
                                  const char sep) {
        if (!PyUnicode_Check(pystr.ptr()))
            throw KARABO_PYTHON_EXCEPTION("Failed to convert  python string to vector of unsigned char ");
        const std::string& stdstr = bp::extract<std::string > (pystr);
        self.set(key, std::vector<unsigned char>(stdstr.begin(), stdstr.end()), sep);
    }

    const karabo::util::Hash&
    HashWrap::setPyDictAsHash(karabo::util::Hash& self,
                              const bp::dict& dictionary,
                              const char sep) {
        std::string separator(1, sep);
        bp::list keys(dictionary.keys());
        for (bp::ssize_t i = 0; i < bp::len(keys); i++) {
            const bp::object& obj = dictionary[keys[i]];
            if (PyDict_Check(obj.ptr())) {
                const bp::dict& dictobj = bp::extract<bp::dict>(obj);
                karabo::util::Hash h;
                self.set(bp::extract<std::string > (keys[i]), setPyDictAsHash(h, dictobj, sep), sep);
            } else {
                set(self, bp::extract<std::string > (keys[i]), obj, separator);
            }
        }
        return self;
    }

    bp::object
    HashWrap::empty(const karabo::util::Hash & self) {
        return bp::object(self.empty() ? 1 : 0);
    }

    void
    HashWrap::getKeys(const karabo::util::Hash & self,
                      const bp::object& obj) {
        if (PyList_Check(obj.ptr())) {
            std::vector<std::string> v;
            self.getKeys(v);
            for (size_t i = 0; i < v.size(); i++) obj.attr("append")(bp::object(v[i]));
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type should be 'list'");
    }

    bp::list
    HashWrap::keys(const karabo::util::Hash & self) {
        bp::list l;
        getKeys(self, l);
        return l;
    }

    void
    HashWrap::getPaths(const karabo::util::Hash & self,
                       const bp::object& obj) {
        if (PyList_Check(obj.ptr())) {
            std::vector<std::string> v;
            self.getPaths(v);
            for (size_t i = 0; i < v.size(); i++) obj.attr("append")(bp::object(v[i]));
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type should be 'list'");
    }

    bp::list
    HashWrap::paths(const karabo::util::Hash & self) {
        bp::list l;
        getPaths(self, l);
        return l;
    }

    bp::object
    HashWrap::getValues(const karabo::util::Hash& self) {
        bp::list t;
        for (karabo::util::Hash::const_iterator it = self.begin(); it != self.end(); it++)
            t.append(Wrapper::toObject(it->getValueAsAny(), HashWrap::try_to_use_numpy));
        return t;
    }

    bp::object
    HashWrap::get(const karabo::util::Hash& self,
                  const std::string& path,
                  const std::string& separator) {
        return Wrapper::toObject(self.getNode(path, separator.at(0)).getValueAsAny(), HashWrap::try_to_use_numpy);
    }

    bp::object
    HashWrap::getAs(const karabo::util::Hash& self,
                    const std::string& path,
                    const PyTypes::ReferenceType& type,
                    const std::string& separator) {
        using namespace karabo::util;
        Types::ReferenceType srcType = self.getType(path, separator.at(0));
        switch (type) {
            case PyTypes::BOOL:
                return bp::object(self.getAs<bool>(path, separator.at(0)));
            case PyTypes::CHAR:
                return bp::object(self.getAs<char>(path, separator.at(0)));
            case PyTypes::INT8:
                return bp::object(self.getAs<signed char>(path, separator.at(0)));
            case PyTypes::UINT8:
                return bp::object(self.getAs<unsigned char>(path, separator.at(0)));
            case PyTypes::INT16:
                return bp::object(self.getAs<short>(path, separator.at(0)));
            case PyTypes::UINT16:
                return bp::object(self.getAs<unsigned short>(path, separator.at(0)));
            case PyTypes::INT32:
                return bp::object(self.getAs<int>(path, separator.at(0)));
            case PyTypes::UINT32:
                return bp::object(self.getAs<unsigned int>(path, separator.at(0)));
            case PyTypes::INT64:
                return bp::object(self.getAs<long long>(path, separator.at(0)));
            case PyTypes::UINT64:
                return bp::object(self.getAs<unsigned long long>(path, separator.at(0)));
            case PyTypes::FLOAT:
                return bp::object(self.getAs<float>(path, separator.at(0)));
            case PyTypes::DOUBLE:
                return bp::object(self.getAs<double>(path, separator.at(0)));
            case PyTypes::STRING:
                return bp::object(self.getAs<std::string>(path, separator.at(0)));
            case PyTypes::VECTOR_BOOL:
            {
                switch (srcType) {
                    case Types::STRING:
                    {
                        string s = self.get<string>(path, separator.at(0));
                        vector<bool> vv;
                        for (size_t i = 0; i < s.size(); i++) {
                            for (size_t j = 0; j < 8; j++) vv.push_back(s[i] & (1 << (7-j))? true : false);
                        }
                        return Wrapper::fromStdVectorToPyArray(vv);
                    }
                    default:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<bool, std::vector > (path, separator.at(0)));
                }
            }
            case PyTypes::VECTOR_CHAR:
            {
                switch (srcType) {
                    case Types::STRING:
                    {
                        const string& s = self.get<string>(path, separator.at(0));
                        vector<char> vec(s.begin(), s.end());
                        return Wrapper::fromStdVectorToPyByteArray(vec);
                    }
                    default:
                        return Wrapper::fromStdVectorToPyByteArray(self.getAs<char, std::vector>(path, separator.at(0)));
                }
            }
            case PyTypes::VECTOR_INT8:
            {
                switch (srcType) {
                    case Types::STRING:
                    {
                        const string& s = self.get<string>(path, separator.at(0));
                        vector<signed char> vec(s.begin(), s.end());
                        return Wrapper::fromStdVectorToPyByteArray(vec);
                    }
                    default:
                        return Wrapper::fromStdVectorToPyByteArray(self.getAs<signed char, std::vector > (path, separator.at(0)));
                }
            }
            case PyTypes::VECTOR_UINT8:
            {
                switch (srcType) {
                    case Types::STRING:
                    {
                        const string& s = self.get<string>(path, separator.at(0));
                        vector<unsigned char> vec(s.begin(), s.end());
                        return Wrapper::fromStdVectorToPyByteArray(vec);
                    }
                    default:
                        return Wrapper::fromStdVectorToPyByteArray(self.getAs<unsigned char, std::vector>(path, separator.at(0)));
                }
            }
            case PyTypes::VECTOR_INT16:
                return Wrapper::fromStdVectorToPyArray(self.getAs<short, std::vector>(path, separator.at(0)));
            case PyTypes::VECTOR_UINT16:
                return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned short, std::vector>(path, separator.at(0)));
            case PyTypes::VECTOR_INT32:
                return Wrapper::fromStdVectorToPyArray(self.getAs<int, std::vector>(path, separator.at(0)));
            case PyTypes::VECTOR_UINT32:
                return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned int, std::vector>(path, separator.at(0)));
            case PyTypes::VECTOR_INT64:
                return Wrapper::fromStdVectorToPyArray(self.getAs<long long, std::vector>(path, separator.at(0)));
            case PyTypes::VECTOR_UINT64:
                return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned long long, std::vector>(path, separator.at(0)));
            case PyTypes::VECTOR_FLOAT:
                return Wrapper::fromStdVectorToPyArray(self.getAs<float, std::vector>(path, separator.at(0)));
            case PyTypes::VECTOR_DOUBLE:
                return Wrapper::fromStdVectorToPyArray(self.getAs<double, std::vector>(path, separator.at(0)));
            case PyTypes::VECTOR_COMPLEX_FLOAT:
                return Wrapper::fromStdVectorToPyArray(self.getAs<std::complex<float>, std::vector>(path, separator.at(0)));
            case PyTypes::VECTOR_COMPLEX_DOUBLE:
                return Wrapper::fromStdVectorToPyArray(self.getAs<std::complex<double>, std::vector>(path, separator.at(0)));
                //                    case PyTypes::HASH:
                //                        return bp::object(self.getAs<karabo::util::Hash>(path, separator.at(0)));
                //                    case PyTypes::VECTOR_HASH:
                //                        return Wrapper::fromStdVectorToPyList(self.getAs<karabo::util::Hash, std::vector>(path, separator.at(0)));
            case PyTypes::NDARRAY_INT16:
                return Wrapper::fromStdVectorToPyArray(self.getAs<short, std::vector>(path, separator.at(0)), true);
            case PyTypes::NDARRAY_UINT16:
                return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned short, std::vector>(path, separator.at(0)), true);
            case PyTypes::NDARRAY_INT32:
                return Wrapper::fromStdVectorToPyArray(self.getAs<int, std::vector>(path, separator.at(0)), true);
            case PyTypes::NDARRAY_UINT32:
                return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned int, std::vector>(path, separator.at(0)), true);
            case PyTypes::NDARRAY_INT64:
                return Wrapper::fromStdVectorToPyArray(self.getAs<long long, std::vector>(path, separator.at(0)), true);
            case PyTypes::NDARRAY_UINT64:
                return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned long long, std::vector>(path, separator.at(0)), true);
            case PyTypes::NDARRAY_FLOAT:
                return Wrapper::fromStdVectorToPyArray(self.getAs<float, std::vector>(path, separator.at(0)), true);
            case PyTypes::NDARRAY_DOUBLE:
                return Wrapper::fromStdVectorToPyArray(self.getAs<double, std::vector>(path, separator.at(0)), true);
            case PyTypes::NDARRAY_COMPLEX_FLOAT:
                return Wrapper::fromStdVectorToPyArray(self.getAs<std::complex<float>, std::vector>(path, separator.at(0)), true);
            case PyTypes::NDARRAY_COMPLEX_DOUBLE:
                return Wrapper::fromStdVectorToPyArray(self.getAs<std::complex<double>, std::vector>(path, separator.at(0)), true);
            default:
                break;
        }
        throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
    }

    bp::object
    HashWrap::getNode(karabo::util::Hash& self,
                      const std::string& path,
                      const std::string & separator) {
        using namespace karabo::util;
        Hash::Node& nodeRef = self.getNode(path, separator.at(0));
        boost::optional<Hash::Node&> node(nodeRef);
        return bp::object(boost::shared_ptr<Hash::Node>(&(*node), null_deleter()));
    }

    bp::object
    HashWrap::setNode(karabo::util::Hash& self,
                      const bp::object & node) {
        if (bp::extract<const karabo::util::Hash::Node&>(node).check()) {
            return bp::object(self.setNode(bp::extract<const karabo::util::Hash::Node&>(node)));
        }
        throw KARABO_PYTHON_EXCEPTION("Failed to extract C++ 'const Hash::Node&' from python object");
    }

    void
    HashWrap::set(karabo::util::Hash& self,
                  const std::string& key,
                  const bp::object & obj,
                  const std::string & separator) {
        using namespace karabo::util;
        if (bp::extract<Hash>(obj).check()) {
            const Hash& h = bp::extract<Hash>(obj);
            self.set(key, h, separator.at(0));
            return;
        }
        if (PyDict_Check(obj.ptr())) {
            const bp::dict& d = bp::extract<bp::dict>(obj);
            Hash h;
            self.set(key, setPyDictAsHash(h, d, separator.at(0)), separator.at(0));
            return;
        }
        boost::any any;
        Wrapper::toAny(obj, any);
        self.set<boost::any>(key, any, separator.at(0));
    }

    void
    HashWrap::setAs(karabo::util::Hash& self,
                    const std::string& key,
                    const bp::object & value,
                    const bp::object& o_type,
                    const std::string & separator) {
        using namespace karabo::util;
        set(self, key, value, separator);
        Hash::Node& node = self.getNode(key, separator.at(0));
        Types::ReferenceType cppType;
        if (bp::extract<std::string>(o_type).check()) {
            std::string s_type = bp::extract<std::string>(o_type);
            cppType = Types::from<FromLiteral>(s_type);
        } else if (bp::extract<PyTypes::ReferenceType>(o_type).check()) {
            PyTypes::ReferenceType type = bp::extract<PyTypes::ReferenceType>(o_type);
            cppType = PyTypes::to(type);
        }
        node.setType(cppType);
    }

    void
    HashWrap::erase(karabo::util::Hash& self,
                    const bp::object & keyObj,
                    const std::string & separator) {
        const char sep = separator.at(0);
        std::string key;
        if (bp::extract<std::string > (keyObj).check()) {
            key = bp::extract<std::string > (keyObj);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Currently values can only be retrieved by string keys");
        }
        self.erase(key, sep);
    }

    bool
    HashWrap::eraseFound(karabo::util::Hash& self,
                         const bp::object & keyObj,
                         const std::string & separator) {
        const char sep = separator.at(0);
        std::string key;
        if (bp::extract<std::string > (keyObj).check()) {
            key = bp::extract<std::string > (keyObj);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Currently values can only be retrieved by string keys");
        }
        return self.eraseFound(key, sep);
    }

    void
    HashWrap::erasePath(karabo::util::Hash& self,
                        const bp::object & keyObj,
                        const std::string & separator) {
        const char sep = separator.at(0);
        std::string key;
        if (bp::extract<std::string > (keyObj).check()) {
            key = bp::extract<std::string > (keyObj);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Currently values can only be retrieved by string keys");
        }
        self.erasePath(key, sep);
    }

    bool
    HashWrap::has(karabo::util::Hash& self,
                  const std::string& key,
                  const std::string & separator) {
        return self.has(key, separator.at(0));
    }

    bool
    HashWrap::is(karabo::util::Hash& self,
                 const std::string& path,
                 const PyTypes::ReferenceType& type,
                 const std::string & separator) {
        if (type < PyTypes::LAST_CPP_TYPE)
            return type == PyTypes::from(self.getType(path, separator.at(0)));
        return false;
    }

    void
    HashWrap::flatten(const karabo::util::Hash& self,
                      karabo::util::Hash& flat,
                      const std::string & separator) {
        self.flatten(flat, separator.at(0));
    }

    void
    HashWrap::unflatten(const karabo::util::Hash& self,
                        karabo::util::Hash& tree,
                        const std::string & separator) {
        self.unflatten(tree, separator.at(0));
    }

    bp::object
    HashWrap::getType(const karabo::util::Hash& self,
                      const std::string& path,
                      const std::string & separator) {
        const char sep = separator.at(0);
        PyTypes::ReferenceType type = static_cast<PyTypes::ReferenceType> (self.getType(path, sep));
        return bp::object(type);
    }

    bool
    HashWrap::hasAttribute(karabo::util::Hash& self,
                           const std::string& path,
                           const std::string& attribute,
                           const std::string & separator) {
        return self.hasAttribute(path, attribute, separator.at(0));
    }

    bp::object
    HashWrap::getAttribute(karabo::util::Hash& self,
                           const std::string& path,
                           const std::string& attribute,
                           const std::string & separator) {
        return Wrapper::toObject(self.getAttributeAsAny(path, attribute, separator.at(0)), HashWrap::try_to_use_numpy);
    }

    bp::object
    HashWrap::getAttributeAs(karabo::util::Hash& self,
                             const std::string& path,
                             const std::string& attribute,
                             const PyTypes::ReferenceType& type,
                             const std::string & separator) {
        using namespace karabo::util;
        switch (type) {

            case PyTypes::BOOL:
                return bp::object(self.getAttributeAs<bool>(path, attribute, separator.at(0)));
            case PyTypes::CHAR:
                return bp::object(self.getAttributeAs<char>(path, attribute, separator.at(0)));
            case PyTypes::INT8:
                return bp::object(self.getAttributeAs<signed char>(path, attribute, separator.at(0)));
            case PyTypes::UINT8:
                return bp::object(self.getAttributeAs<unsigned char>(path, attribute, separator.at(0)));
            case PyTypes::INT16:
                return bp::object(self.getAttributeAs<short>(path, attribute, separator.at(0)));
            case PyTypes::UINT16:
                return bp::object(self.getAttributeAs<unsigned short>(path, attribute, separator.at(0)));
            case PyTypes::INT32:
                return bp::object(self.getAttributeAs<int>(path, attribute, separator.at(0)));
            case PyTypes::UINT32:
                return bp::object(self.getAttributeAs<unsigned int>(path, attribute, separator.at(0)));
            case PyTypes::INT64:
                return bp::object(self.getAttributeAs<long long>(path, attribute, separator.at(0)));
            case PyTypes::UINT64:
                return bp::object(self.getAttributeAs<unsigned long long>(path, attribute, separator.at(0)));
            case PyTypes::FLOAT:
                return bp::object(self.getAttributeAs<float>(path, attribute, separator.at(0)));
            case PyTypes::DOUBLE:
                return bp::object(self.getAttributeAs<double>(path, attribute, separator.at(0)));
            case PyTypes::STRING:
                return bp::object(self.getAttributeAs<std::string>(path, attribute, separator.at(0)));
            case PyTypes::VECTOR_BOOL:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<bool, std::vector > (path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_CHAR:
                return Wrapper::fromStdVectorToPyByteArray(self.getAttributeAs<char, std::vector>(path, attribute, separator.at(0)));
            case PyTypes::VECTOR_INT8:
                return Wrapper::fromStdVectorToPyByteArray(self.getAttributeAs<signed char, std::vector > (path, attribute, separator.at(0)));
            case PyTypes::VECTOR_UINT8:
                return Wrapper::fromStdVectorToPyByteArray(self.getAttributeAs<unsigned char, std::vector>(path, attribute, separator.at(0)));
            case PyTypes::VECTOR_INT16:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<short, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_UINT16:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<unsigned short, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_INT32:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<int, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_UINT32:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<unsigned int, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_INT64:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<long long, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_UINT64:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<unsigned long long, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_FLOAT:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<float, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_DOUBLE:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<double, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_COMPLEX_FLOAT:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<std::complex<float>, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::VECTOR_COMPLEX_DOUBLE:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<std::complex<double>, std::vector>(path, attribute, separator.at(0)), false);
            case PyTypes::NDARRAY_BOOL:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<bool, std::vector > (path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_INT16:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<short, std::vector>(path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_UINT16:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<unsigned short, std::vector>(path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_INT32:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<int, std::vector>(path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_UINT32:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<unsigned int, std::vector>(path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_INT64:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<long long, std::vector>(path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_UINT64:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<unsigned long long, std::vector>(path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_FLOAT:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<float, std::vector>(path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_DOUBLE:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<double, std::vector>(path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_COMPLEX_FLOAT:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<std::complex<float>, std::vector>(path, attribute, separator.at(0)), true);
            case PyTypes::NDARRAY_COMPLEX_DOUBLE:
                return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<std::complex<double>, std::vector>(path, attribute, separator.at(0)), true);
            default:
                break;
        }
        throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
    }

    const karabo::util::Hash::Attributes &
    HashWrap::getAttributes(karabo::util::Hash& self,
                            const std::string& path,
                            const std::string & separator) {

        return static_cast<const karabo::util::Hash::Attributes&> (self.getAttributes(path, separator.at(0)));
    }

    bp::object
    HashWrap::copyAttributes(karabo::util::Hash& self,
                             const std::string& path,
                             const std::string & separator) {

        return bp::object(self.getAttributes(path, separator.at(0)));
    }

    void
    HashWrap::setAttribute(karabo::util::Hash& self,
                           const std::string& path,
                           const std::string& attribute,
                           const bp::object& value,
                           const std::string & separator) {

        boost::any any;
        Wrapper::toAny(value, any);
        self.setAttribute(path, attribute, any, separator.at(0));
    }

    void
    HashWrap::setAttributes(karabo::util::Hash& self,
                            const std::string& path,
                            const bp::object& attributes,
                            const std::string & separator) {
        if (bp::extract<karabo::util::Hash::Attributes>(attributes).check()) {

            self.setAttributes(path, bp::extract<karabo::util::Hash::Attributes>(attributes), separator.at(0));
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Python object contains not a C++ 'Hash::Attributes' type");
    }

    boost::shared_ptr<karabo::util::Hash::Node>
    HashWrap::find(karabo::util::Hash& self,
                   const std::string& path,
                   const std::string & separator) {
        boost::optional<karabo::util::Hash::Node&> node = self.find(path, separator.at(0));

        if (!node)
            return boost::shared_ptr<karabo::util::Hash::Node>();
        // Wrapping the pointer to the existing memory location with null deleter
        return boost::shared_ptr<karabo::util::Hash::Node>(&node.get(), null_deleter());
    }

    bp::object
    HashWrap::getRef(karabo::util::Hash& self,
                     const bp::object& obj, const std::string & sep) {
        using namespace karabo::util;
        if (bp::extract<Hash::Node&>(obj).check()) {
            Hash::Node& node = bp::extract<Hash::Node&>(obj);
            if (node.getType() == Types::HASH) {
                boost::shared_ptr<Hash> hash = boost::shared_ptr<Hash>(&node.getValue<Hash>(), null_deleter());
                return bp::object(hash);
            }
            return Wrapper::toObject(node.getValueAsAny(), HashWrap::try_to_use_numpy);
        } else if (bp::extract<std::string>(obj).check()) {
            std::string path = bp::extract<std::string>(obj);
            if (self.getType(path, sep.at(0)) == karabo::util::Types::HASH) {
                Hash* hp = &self.get<Hash>(path, sep.at(0));
                boost::shared_ptr<Hash> hash = boost::shared_ptr<Hash>(hp, null_deleter());
                return bp::object(hash);
            }
            if (self.getType(path, sep.at(0)) == Types::VECTOR_HASH) {
                std::vector<Hash>* vhp = &self.get<std::vector<Hash> >(path, sep.at(0));
                boost::shared_ptr<std::vector<Hash> > vhash = boost::shared_ptr<std::vector<Hash> >(vhp, null_deleter());
                return bp::object(vhash);
            }
            Hash::Node& node = self.getNode(path, sep.at(0));
            return Wrapper::toObject(node.getValueAsAny(), HashWrap::try_to_use_numpy);
        }
        throw KARABO_PYTHON_EXCEPTION("Invalid type for Hash index. The type should be 'Node' or 'str'!");
    }

    bool
    similarWrap(const bp::object& left, const bp::object & right) {
        if (bp::extract<karabo::util::Hash>(left).check() && bp::extract<karabo::util::Hash>(right).check()) {
            const karabo::util::Hash& lhash = bp::extract<karabo::util::Hash>(left);
            const karabo::util::Hash& rhash = bp::extract<karabo::util::Hash>(right);
            return karabo::util::similar(lhash, rhash);
        }

        if (bp::extract<karabo::util::Hash::Node>(left).check() && bp::extract<karabo::util::Hash::Node>(right).check()) {
            const karabo::util::Hash::Node& lnode = bp::extract<karabo::util::Hash::Node>(left);
            const karabo::util::Hash::Node& rnode = bp::extract<karabo::util::Hash::Node>(right);
            return karabo::util::similar(lnode, rnode);
        }

        if (PyList_Check(left.ptr()) && PyList_Check(right.ptr())) {
            bp::object left0 = left[0];
            bp::object right0 = right[0];
            bp::ssize_t lsize = bp::len(left);
            bp::ssize_t rsize = bp::len(right);
            if (lsize == rsize) {
                bp::ssize_t size = lsize; // rsize == lsize
                if (bp::extract<karabo::util::Hash>(left0).check() && bp::extract<karabo::util::Hash>(right0).check()) {
                    std::vector<karabo::util::Hash> vleft(size);
                    std::vector<karabo::util::Hash> vright(size);
                    for (bp::ssize_t i = 0; i < size; i++) {
                        vleft[i] = bp::extract<karabo::util::Hash>(left[i]);
                        vright[i] = bp::extract<karabo::util::Hash>(right[i]);
                    }
                    return karabo::util::similar(vleft, vright);
                }
            }
        }

        if (bp::extract<karabo::util::Schema>(left).check() && bp::extract<karabo::util::Schema>(right).check()) {
            const karabo::util::Schema& l = bp::extract<karabo::util::Schema>(left);
            const karabo::util::Schema& r = bp::extract<karabo::util::Schema>(right);
            return karabo::util::similar(l, r);
        }
        
        return false;
    }

    void
    HashWrap::setDefault(const PyTypes::ReferenceType & type) {
        if (type == PyTypes::PYTHON_DEFAULT)
            try_to_use_numpy = false;
        else if (type == PyTypes::NUMPY_DEFAULT)
            try_to_use_numpy = true;
        else
            throw KARABO_PYTHON_EXCEPTION("Unsupported default type: use either Types.NUMPY or Types.PYTHON");
    }

    bool
    HashWrap::isDefault(const PyTypes::ReferenceType & type) {
        if (type == PyTypes::PYTHON_DEFAULT && !try_to_use_numpy)
            return true;
        else if (type == PyTypes::NUMPY_DEFAULT && try_to_use_numpy)
            return true;
        return false;
    }
}

bool karathon::HashWrap::try_to_use_numpy = false;
