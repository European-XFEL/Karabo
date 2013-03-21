/* $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 16, 2012, 12:43 PM
 */

#ifndef KARABO_PYKARABO_HASHWRAP_HH
#define	KARABO_PYKARABO_HASHWRAP_HH

#include <boost/python.hpp>
#include <boost/filesystem.hpp>
#include <set>

#include <karabo/util/Hash.hh>
#include <karabo/util/Types.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/ToLiteral.hh>


#include "Wrapper.hh"

namespace bp = boost::python;


namespace karabo {
    namespace pyexfel {


        class HashWrap {

            static void setPyListAsStdVector(karabo::util::Hash& self, const std::string& key, const bp::object& list, bp::ssize_t size, const char sep) {
                // elements of our vectors require to be of the same type
                bp::object list0 = list[0];
                if (PyInt_Check(list0.ptr())) {
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
                } else if (PyString_Check(list0.ptr())) {
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

            static void setPyStrAsStdVector(karabo::util::Hash& self, const std::string& key, const bp::object& pystr, const char sep) {
                if (!PyString_Check(pystr.ptr()))
                    throw KARABO_PYTHON_EXCEPTION("Failed to convert  python string to vector of unsigned char ");
                const std::string& stdstr = bp::extract<std::string > (pystr);
                self.set(key, std::vector<unsigned char>(stdstr.begin(), stdstr.end()), sep);
            }

        public:

            static const karabo::util::Hash& setPyDictAsHash(karabo::util::Hash& self, const bp::dict& dictionary, const char sep) {
                std::string separator(1, sep);
                bp::list keys(dictionary.iterkeys());
                for (bp::ssize_t i = 0; i < bp::len(keys); i++) {
                    pythonSet(self, bp::extract<std::string > (keys[i]), dictionary[keys[i]], separator);
                }
                return self;
            }

            static bp::object pythonEmpty(const karabo::util::Hash & self) {
                return bp::object(self.empty() ? 1 : 0);
            }

            static bp::object pythonGetKeys(const karabo::util::Hash & self) {
                std::vector<std::string> tmp;
                self.getKeys(tmp);
                return Wrapper::fromStdVectorToPyList(tmp);
            }

            static bp::object pythonGetPaths(const karabo::util::Hash & self) {
                std::vector<std::string> tmp;
                self.getPaths(tmp);
                return Wrapper::fromStdVectorToPyList(tmp);
            }

            static bp::object pythonGetValues(const karabo::util::Hash& self) {
                bp::list t;
                for (karabo::util::Hash::const_iterator it = self.begin(); it != self.end(); it++)
                    t.append(Wrapper::toObject(it->getValueAsAny()));
                return t;
            }

            static bp::object pythonGet(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return Wrapper::toObject(self.getNode(path, separator.at(0)).getValueAsAny());
            }

            static bp::object pythonGetAs(const karabo::util::Hash& self, const std::string& path, const std::string& type, const std::string& separator = ".") {
                using namespace karabo::util;
                Types::ReferenceType reftype = Types::from<FromLiteral>(type);
                switch (reftype) {
                    case Types::BOOL:
                        return bp::object(self.getAs<bool>(path, separator.at(0)));
                    case Types::CHAR:
                        return bp::object(self.getAs<char>(path, separator.at(0)));
                    case Types::INT8:
                        return bp::object(self.getAs<signed char>(path, separator.at(0)));
                    case Types::UINT8:
                        return bp::object(self.getAs<unsigned char>(path, separator.at(0)));
                    case Types::INT16:
                        return bp::object(self.getAs<short>(path, separator.at(0)));
                    case Types::UINT16:
                        return bp::object(self.getAs<unsigned short>(path, separator.at(0)));
                    case Types::INT32:
                        return bp::object(self.getAs<int>(path, separator.at(0)));
                    case Types::UINT32:
                        return bp::object(self.getAs<unsigned int>(path, separator.at(0)));
                    case Types::INT64:
                        return bp::object(self.getAs<long long>(path, separator.at(0)));
                    case Types::UINT64:
                        return bp::object(self.getAs<unsigned long long>(path, separator.at(0)));
                    case Types::FLOAT:
                        return bp::object(self.getAs<float>(path, separator.at(0)));
                    case Types::DOUBLE:
                        return bp::object(self.getAs<double>(path, separator.at(0)));
                    case Types::STRING:
                        return bp::object(self.getAs<std::string>(path, separator.at(0)));
                    case Types::VECTOR_BOOL:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<bool, std::vector > (path, separator.at(0)));
                    case Types::VECTOR_CHAR:
                        return Wrapper::fromStdVectorToPyByteArray(self.getAs<char, std::vector>(path, separator.at(0)));
                    case Types::VECTOR_INT8:
                        return Wrapper::fromStdVectorToPyByteArray(self.getAs<signed char, std::vector > (path, separator.at(0)));
                    case Types::VECTOR_UINT8:
                        return Wrapper::fromStdVectorToPyByteArray(self.getAs<unsigned char, std::vector>(path, separator.at(0)));
                    case Types::VECTOR_INT16:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<short, std::vector>(path, separator.at(0)));
                    case Types::VECTOR_UINT16:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned short, std::vector>(path, separator.at(0)));
                    case Types::VECTOR_INT32:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<int, std::vector>(path, separator.at(0)));
                    case Types::VECTOR_UINT32:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned int, std::vector>(path, separator.at(0)));
                    case Types::VECTOR_INT64:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<long long, std::vector>(path, separator.at(0)));
                    case Types::VECTOR_UINT64:
                        return Wrapper::fromStdVectorToPyArray(self.getAs<unsigned long long, std::vector>(path, separator.at(0)));
                        //                    case Types::HASH:
                        //                        return bp::object(self.getAs<karabo::util::Hash>(path, separator.at(0)));
                        //                    case Types::VECTOR_HASH:
                        //                        return Wrapper::fromStdVectorToPyList(self.getAs<karabo::util::Hash, std::vector>(path, separator.at(0)));
                    default:
                        break;
                }
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
            }

            static bp::object pythonGetNode(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return bp::object(static_cast<karabo::util::Hash::Node>(self.getNode(path, separator.at(0))));
            }

            static bp::object pythonSetNode(karabo::util::Hash& self, const bp::object& node) {
                if (bp::extract<const karabo::util::Hash::Node&>(node).check()) {
                    return bp::object(self.setNode(bp::extract<const karabo::util::Hash::Node&>(node)));
                }
                throw KARABO_PYTHON_EXCEPTION("Failed to extract C++ 'const Hash::Node&' from python object");
            }

            static void pythonSet(karabo::util::Hash& self, const std::string& key, const bp::object & obj, const std::string& separator = ".") {
                boost::any any;
                Wrapper::toAny(obj, any);
                self.set<boost::any>(key, any, separator.at(0));
            }

            static void pythonErase(karabo::util::Hash& self, const bp::object & keyObj, const std::string& separator = ".") {
                const char sep = separator.at(0);
                std::string key;
                if (bp::extract<std::string > (keyObj).check()) {
                    key = bp::extract<std::string > (keyObj);
                } else {
                    throw KARABO_PYTHON_EXCEPTION("Currently values can only be retrieved by string keys");
                }
                self.erase(key, sep);
            }

            static bool pythonHas(karabo::util::Hash& self, const std::string& key, const std::string& separator = ".") {
                return self.has(key, separator.at(0));
            }

            static bool pythonIs(karabo::util::Hash& self, const std::string& path, const std::string& type, const std::string& separator = ".") {
                //std::string path;
                karabo::util::Types::ReferenceType refType = karabo::util::Types::from(type);
                return self.is(path, refType, separator.at(0));
            }

            static void pythonFlatten(const karabo::util::Hash& self, karabo::util::Hash& flat, const std::string& separator = ".") {
                self.flatten(flat, separator.at(0));
            }

            static void pythonUnFlatten(const karabo::util::Hash& self, karabo::util::Hash& tree, const std::string& separator = ".") {
                self.unflatten(tree, separator.at(0));
            }

            static bp::object pythonGetType(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                const char sep = separator.at(0);
                const std::string stdstr = karabo::util::Types::to<karabo::util::ToLiteral>(self.getType(path, sep));
                return bp::str(stdstr.c_str(), stdstr.size());
            }

            static bp::object pythonGetTypeAsId(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                const char sep = separator.at(0);
                const int type = self.getType(path, sep);
                return bp::object(type);
            }

            static bool pythonHasAttribute(karabo::util::Hash& self, const std::string& path, const std::string& attribute, const std::string& separator = ".") {
                return self.hasAttribute(path, attribute, separator.at(0));
            }

            static bp::object pythonGetAttribute(karabo::util::Hash& self, const std::string& path, const std::string& attribute, const std::string& separator = ".") {
                return Wrapper::toObject(self.getAttributeAsAny(path, attribute, separator.at(0)));
            }

            static bp::object pythonGetAttributeAs(karabo::util::Hash& self, const std::string& path, const std::string& attribute, const std::string& type, const std::string& separator = ".") {
                using namespace karabo::util;
                Types::ReferenceType reftype = Types::from<FromLiteral>(type);
                switch (reftype) {
                    case Types::BOOL:
                        return bp::object(self.getAttributeAs<bool>(path, attribute, separator.at(0)));
                    case Types::CHAR:
                        return bp::object(self.getAttributeAs<char>(path, attribute, separator.at(0)));
                    case Types::INT8:
                        return bp::object(self.getAttributeAs<signed char>(path, attribute, separator.at(0)));
                    case Types::UINT8:
                        return bp::object(self.getAttributeAs<unsigned char>(path, attribute, separator.at(0)));
                    case Types::INT16:
                        return bp::object(self.getAttributeAs<short>(path, attribute, separator.at(0)));
                    case Types::UINT16:
                        return bp::object(self.getAttributeAs<unsigned short>(path, attribute, separator.at(0)));
                    case Types::INT32:
                        return bp::object(self.getAttributeAs<int>(path, attribute, separator.at(0)));
                    case Types::UINT32:
                        return bp::object(self.getAttributeAs<unsigned int>(path, attribute, separator.at(0)));
                    case Types::INT64:
                        return bp::object(self.getAttributeAs<long long>(path, attribute, separator.at(0)));
                    case Types::UINT64:
                        return bp::object(self.getAttributeAs<unsigned long long>(path, attribute, separator.at(0)));
                    case Types::FLOAT:
                        return bp::object(self.getAttributeAs<float>(path, attribute, separator.at(0)));
                    case Types::DOUBLE:
                        return bp::object(self.getAttributeAs<double>(path, attribute, separator.at(0)));
                    case Types::STRING:
                        return bp::object(self.getAttributeAs<std::string>(path, attribute, separator.at(0)));
                        //                    case Types::VECTOR_CHAR:
                        //                        return Wrapper::fromStdVectorToPyByteArray(self.getAttributeAs<char, std::vector>(path, attribute, separator.at(0)));
                        //                    case Types::VECTOR_INT8:
                        //                        return Wrapper::fromStdVectorToPyByteArray(self.getAttributeAs<signed char, std::vector > (path, attribute, separator.at(0)));
                        //                    case Types::VECTOR_UINT8:
                        //                        return Wrapper::fromStdVectorToPyByteArray(self.getAttributeAs<unsigned char, std::vector>(path, attribute, separator.at(0)));
                        //                    case Types::VECTOR_INT16:
                        //                        return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<short, std::vector>(path, attribute, separator.at(0)));
                        //                    case Types::VECTOR_UINT16:
                        //                        return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<unsigned short, std::vector>(path, attribute, separator.at(0)));
                        //                    case Types::VECTOR_INT32:
                        //                        return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<int, std::vector>(path, attribute, separator.at(0)));
                        //                    case Types::VECTOR_UINT32:
                        //                        return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<unsigned int, std::vector>(path, attribute, separator.at(0)));
                        //                    case Types::VECTOR_INT64:
                        //                        return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<long long, std::vector>(path, attribute, separator.at(0)));
                        //                    case Types::VECTOR_UINT64:
                        //                        return Wrapper::fromStdVectorToPyArray(self.getAttributeAs<unsigned long long, std::vector>(path, attribute, separator.at(0)));
                    default:
                        break;
                }
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type is not yet supported");
            }

            static bp::object pythonGetAttributes(karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return bp::object(self.getAttributes(path, separator.at(0)));
            }

            static void pythonSetAttribute(karabo::util::Hash& self, const std::string& path, const std::string& attribute, const bp::object& value, const std::string& separator = ".") {
                boost::any any;
                Wrapper::toAny(value, any);
                self.setAttribute(path, attribute, any, separator.at(0));
            }

            static void pythonSetAttributes(karabo::util::Hash& self, const std::string& path, const bp::object& attributes, const std::string& separator = ".") {
                if (bp::extract<karabo::util::Hash::Attributes>(attributes).check()) {
                    self.setAttributes(path, bp::extract<karabo::util::Hash::Attributes>(attributes), separator.at(0));
                    return;
                }
                throw KARABO_PYTHON_EXCEPTION("Python object contains not a C++ 'Hash::Attributes' type");
            }


            struct null_deleter {

                void operator()(void const *) const {
                }
            };

            static boost::shared_ptr<karabo::util::Hash::Node> pythonFind(karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                boost::optional<karabo::util::Hash::Node&> node = self.find(path, separator.at(0));
                if (!node)
                    return boost::shared_ptr<karabo::util::Hash::Node>();
                return boost::shared_ptr<karabo::util::Hash::Node>(&node.get(), null_deleter());
                //return bp::object(node.get());
            }
        };
    }
}

// Define 'bp::object' specialization for templated constructors of Hash class

namespace karabo {
    namespace util {

        template<>
        karabo::util::Hash::Hash(const std::string& key, const bp::object& value);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4,
                                 const std::string& key5, const bp::object& value5);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1,
                                 const std::string& key2, const bp::object& value2,
                                 const std::string& key3, const bp::object& value3,
                                 const std::string& key4, const bp::object& value4,
                                 const std::string& key5, const bp::object& value5,
                                 const std::string& key6, const bp::object& value6);
    }
}

#endif

