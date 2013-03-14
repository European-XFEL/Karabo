/* $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 16, 2012, 12:43 PM
 */

#ifndef KARABO_PYKARABO_HASHWRAP_HH
#define	KARABO_PYKARABO_HASHWRAP_HH

#include <boost/python.hpp>

#ifdef KARATHON_BOOST_NUMPY
#include <boost/numpy.hpp>
#endif

#include <boost/filesystem.hpp>
#include <set>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>

namespace bp = boost::python;

#ifdef KARATHON_BOOST_NUMPY
namespace bn = boost::numpy;
#endif

namespace karabo {
    namespace pyexfel {

        class HashWrap {

            template<class ValueType>
            static bp::object stdVector2pyArray(const std::vector<ValueType>& v) {
#                ifdef KARATHON_BOOST_NUMPY
                Py_intptr_t shape[1] = {v.size()};
                bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<ValueType>());
                std::copy(v.begin(), v.end(), reinterpret_cast<ValueType*> (result.get_data()));
                return result;
#                else
                return stdVector2pyList(v);
#                endif
            }

            template<class ValueType>
            static bp::object stdVector2pyList(const std::vector<ValueType>& v) {
                bp::object it = bp::iterator<std::vector<ValueType> >();
                bp::object iter = it(v);
                bp::list l(iter);
                return l;
            }

            static bp::str stdVector2pyStr(const std::vector<char>& v) {
                return bp::str(&v[0], v.size());
            }

            static bp::str stdVector2pyStr(const std::vector<signed char>& v) {
                return bp::str(reinterpret_cast<const char*> (&v[0]), v.size());
            }

            static bp::str stdVector2pyStr(const std::vector<unsigned char>& v) {
                return bp::str(reinterpret_cast<const char*> (&v[0]), v.size());
            }

            static bp::object toObject(const karabo::util::Hash& self, const boost::any& operand) {
                try {
                    if (operand.type() == typeid (bool)) {
                        return bp::object(boost::any_cast<bool>(operand));
                    } else if (operand.type() == typeid (char)) {
                        return bp::object(boost::any_cast<char>(operand));
                    } else if (operand.type() == typeid (signed char)) {
                        return bp::object(boost::any_cast<signed char>(operand));
                    } else if (operand.type() == typeid (unsigned char)) {
                        return bp::object(boost::any_cast<unsigned char>(operand));
                    } else if (operand.type() == typeid (short)) {
                        return bp::object(boost::any_cast<short>(operand));
                    } else if (operand.type() == typeid (unsigned short)) {
                        return bp::object(boost::any_cast<unsigned short>(operand));
                    } else if (operand.type() == typeid (int)) {
                        return bp::object(boost::any_cast<int>(operand));
                    } else if (operand.type() == typeid (unsigned int)) {
                        return bp::object(boost::any_cast<unsigned int>(operand));
                    } else if (operand.type() == typeid (long long)) {
                        return bp::object(boost::any_cast<long long>(operand));
                    } else if (operand.type() == typeid (unsigned long long)) {
                        return bp::object(boost::any_cast<unsigned long long>(operand));
                    } else if (operand.type() == typeid (float)) {
                        return bp::object(boost::any_cast<float>(operand));
                    } else if (operand.type() == typeid (double)) {
                        return bp::object(boost::any_cast<double>(operand));
                    } else if (operand.type() == typeid (std::complex<float>)) {
                        return bp::object(boost::any_cast<std::complex<float> >(operand));
                    } else if (operand.type() == typeid (std::complex<double>)) {
                        return bp::object(boost::any_cast<std::complex<double> >(operand));
                    } else if (operand.type() == typeid (std::string)) {
                        return bp::object(boost::any_cast<std::string>(operand));
                    } else if (operand.type() == typeid (boost::filesystem::path)) {
                        return bp::object(boost::any_cast<boost::filesystem::path>(operand).string());
                    } else if (operand.type() == typeid (karabo::util::Hash)) {
                        return bp::object(boost::any_cast<karabo::util::Hash>(operand));
                    } else if (operand.type() == typeid (std::vector<bool>)) {
                        return stdVector2pyList(boost::any_cast < std::vector<bool> >(operand));
                    } else if (operand.type() == typeid (std::vector<char>)) {
                        return stdVector2pyList(boost::any_cast<std::vector<char> >(operand));
                    } else if (operand.type() == typeid (std::vector<signed char>)) {
                        return stdVector2pyList(boost::any_cast < std::vector<signed char> >(operand));
                    } else if (operand.type() == typeid (std::vector<unsigned char>)) {
                        return stdVector2pyList(boost::any_cast<std::vector<unsigned char> >(operand));
                    } else if (operand.type() == typeid (std::vector<short>)) {
                        return stdVector2pyArray(boost::any_cast<std::vector<short> >(operand));
                    } else if (operand.type() == typeid (std::vector<unsigned short>)) {
                        return stdVector2pyArray(boost::any_cast<std::vector<unsigned short> >(operand));
                    } else if (operand.type() == typeid (std::vector<int>)) {
                        return stdVector2pyArray(boost::any_cast<std::vector<int> >(operand));
                    } else if (operand.type() == typeid (std::vector<unsigned int>)) {
                        return stdVector2pyArray(boost::any_cast<std::vector<unsigned int> >(operand));
                    } else if (operand.type() == typeid (std::vector<long long>)) {
                        return stdVector2pyArray(boost::any_cast<std::vector<long long> >(operand));
                    } else if (operand.type() == typeid (std::vector<unsigned long long>)) {
                        return stdVector2pyArray(boost::any_cast<std::vector<unsigned long long> >(operand));
                    } else if (operand.type() == typeid (std::vector<float>)) {
                        return stdVector2pyArray(boost::any_cast<std::vector<float> >(operand));
                    } else if (operand.type() == typeid (std::vector<double>)) {
                        return stdVector2pyArray(boost::any_cast<std::vector<double> >(operand));
                    } else if (operand.type() == typeid (karabo::util::Schema)) {
                        return bp::object(boost::any_cast<karabo::util::Schema>(operand));
                    } else if (operand.type() == typeid (std::vector<std::string>)) {
                        return stdVector2pyList(boost::any_cast < std::vector<std::string> >(operand));
                    } else if (operand.type() == typeid (std::vector<karabo::util::Hash>)) {
                        return stdVector2pyList(boost::any_cast<std::vector<karabo::util::Hash> >(operand));
                    } else {
                        throw KARABO_PYTHON_EXCEPTION("Failed to convert inner Hash type of python object");
                    }
                } catch (const boost::bad_any_cast& e) {
                    KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(e.what()));
                }
            }

            static void pyList2stdVector(karabo::util::Hash& self, const std::string& key, const bp::object& list, bp::ssize_t size, const char sep) {
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

                } else if (PyDict_Check(list0.ptr())) {
                    std::vector<karabo::util::Hash> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        bp::dict d = bp::extract<bp::dict > (list[i]);
                        pyDict2Hash(v[i], d, sep);
                    }
                    self.set(key, v, sep);

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

            static void pyStr2stdVector(karabo::util::Hash& self, const std::string& key, const bp::object& pystr, const char sep) {
                if (!PyString_Check(pystr.ptr()))
                    throw KARABO_PYTHON_EXCEPTION("Failed to convert  python string to vector of unsigned char ");
                const std::string& stdstr = bp::extract<std::string > (pystr);
                self.set(key, std::vector<unsigned char>(stdstr.begin(), stdstr.end()), sep);
            }

        public:

            static const karabo::util::Hash& pyDict2Hash(karabo::util::Hash& self, const bp::dict& dictionary, const char sep) {
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
                return stdVector2pyList(tmp);
            }

            static bp::object pythonGetPaths(const karabo::util::Hash & self) {
                std::vector<std::string> tmp;
                self.getPaths(tmp);
                return stdVector2pyList(tmp);
            }

            static bp::object pythonGetValues(const karabo::util::Hash& self) {
                bp::list t;
                for (karabo::util::Hash::const_iterator it = self.begin(); it != self.end(); it++)
                    t.append(toObject(self, it->getValueAsAny()));
                return t;
            }

            static bp::object pythonGet(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getNode(path, separator.at(0)).getValueAsAny());
            }

            static bp::object pythonGetAsStr(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<std::string>(path, separator.at(0)));
            }

            static bp::object pythonGetAsStrArr(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<std::string, std::vector>(path, separator.at(0)));
            }

            static bp::object pythonGetAsInt(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<int>(path, separator.at(0)));
            }

            static bp::object pythonGetAsIntArr(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<int, std::vector>(path, separator.at(0)));
            }

            static bp::object pythonGetAsBool(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<bool>(path, separator.at(0)));
            }

            static bp::object pythonGetAsBoolArr(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<bool, std::vector > (path, separator.at(0)));
            }

            static bp::object pythonGetAsLong(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<long long>(path, separator.at(0)));
            }

            static bp::object pythonGetAsLongArr(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<long long, std::vector>(path, separator.at(0)));
            }

            static bp::object pythonGetAsFloat(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<float>(path, separator.at(0)));
            }

            static bp::object pythonGetAsFloatArr(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<float, std::vector>(path, separator.at(0)));
            }

            static bp::object pythonGetAsDouble(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<double>(path, separator.at(0)));
            }

            static bp::object pythonGetAsDoubleArr(const karabo::util::Hash& self, const std::string& path, const std::string& separator = ".") {
                return toObject(self, self.getAs<double, std::vector>(path, separator.at(0)));
            }

            static void pythonSet(karabo::util::Hash& self, const std::string& key, const bp::object & obj, const std::string& separator = ".") {
                const char sep = separator.at(0);
                if (PyBool_Check(obj.ptr())) {
                    self.set<bool>(key, bp::extract<bool>(obj), sep);
                } else if (PyInt_Check(obj.ptr())) {
                    self.set<int>(key, bp::extract<int>(obj), sep);
                } else if (PyFloat_Check(obj.ptr())) {
                    self.set<double>(key, bp::extract<double>(obj), sep);
                } else if (PyString_Check(obj.ptr())) {
                    self.set<std::string > (key, bp::extract<std::string >(obj), sep);
                } else if (PyLong_Check(obj.ptr())) {
                    self.set<long long>(key, bp::extract<long>(obj), sep);
                } else if (PyList_Check(obj.ptr())) {
                    const bp::list& lst = bp::extract<bp::list>(obj);
                    bp::ssize_t size = bp::len(lst);
                    if (size == 0)
                        self.set(key, std::vector<karabo::util::Hash>(), sep);
                    else
                        pyList2stdVector(self, key, lst, size, sep);
                } else if (PyDict_Check(obj.ptr())) {
                    karabo::util::Hash hash;
                    pyDict2Hash(hash, bp::extract<bp::dict>(obj), sep);
                    self.set(key, hash, sep);
                } else if (bp::extract<bn::ndarray>(obj).check()) {
                    const bn::ndarray& a = bp::extract<bn::ndarray>(obj);
                    int nd = a.get_nd();
                    Py_intptr_t const * shapes = a.get_shape();
                    int nelems = 1;
                    for (int i = 0; i < nd; i++) nelems *= shapes[i];
                    //std::cout << "HashWrap::pythonSet: nelems = " << nelems << std::endl;
                    if (a.get_dtype() == bn::dtype::get_builtin<double>()) {
                        double* data = reinterpret_cast<double*> (a.get_data());
                        std::vector<double> v(data, data + nelems);
                        self.set<std::vector<double> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<float>()) {
                        float* data = reinterpret_cast<float*> (a.get_data());
                        std::vector<float> v(data, data + nelems);
                        self.set<std::vector<float> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<short>()) {
                        short* data = reinterpret_cast<short*> (a.get_data());
                        std::vector<short> v(data, data + nelems);
                        self.set<std::vector<short> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned short>()) {
                        unsigned short* data = reinterpret_cast<unsigned short*> (a.get_data());
                        std::vector<unsigned short> v(data, data + nelems);
                        self.set<std::vector<unsigned short> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<int>()) {
                        int* data = reinterpret_cast<int*> (a.get_data());
                        std::vector<int> v(data, data + nelems);
                        self.set<std::vector<int> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned int>()) {
                        unsigned int* data = reinterpret_cast<unsigned int*> (a.get_data());
                        std::vector<unsigned int> v(data, data + nelems);
                        self.set<std::vector<unsigned int> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<long long>()) {
                        long long* data = reinterpret_cast<long long*> (a.get_data());
                        std::vector<long long> v(data, data + nelems);
                        self.set<std::vector<long long> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned long long>()) {
                        unsigned long long* data = reinterpret_cast<unsigned long long*> (a.get_data());
                        std::vector<unsigned long long> v(data, data + nelems);
                        self.set<std::vector<unsigned long long> >(key, v);
                    }
                } else if (bp::extract<karabo::util::Schema > (obj).check()) {
                    self.set<karabo::util::Schema > (key, bp::extract<karabo::util::Schema > (obj));
                } else if (bp::extract<karabo::util::Hash > (obj).check()) {
                    self.set<karabo::util::Hash > (key, bp::extract<karabo::util::Hash > (obj));
                } else {
                    throw KARABO_PYTHON_EXCEPTION("Python type can not be mapped into Hash");
                }
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

