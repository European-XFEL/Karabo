/* $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 16, 2012, 12:43 PM
 */

#ifndef KARABO_PYKARABO_HASHWRAP_HH
#define	KARABO_PYKARABO_HASHWRAP_HH

#include <boost/python.hpp>
#include <boost/numpy.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>

namespace bp = boost::python;
namespace bn = boost::numpy;

namespace karabo {
    namespace pyexfel {

        class HashWrap {
        public:

            static bp::object pythonEmpty(const karabo::util::Hash & self) {
                return bp::object(self.empty() ? 1 : 0);
            }

            static bp::object pythonGetKeys(const karabo::util::Hash & self) {
                return stdVector2pyList(self.getKeysAsVector());
            }

            static bp::object pythonGetLeaves(const karabo::util::Hash& self, const std::string& sep = ".") {
                return stdVector2pyList(self.getLeavesAsVector(sep));
            }

            static bp::object pythonGetValues(const karabo::util::Hash& self) {
                bp::list t;
                for (karabo::util::Hash::const_iterator it = self.begin(); it != self.end(); it++)
                    t.append(pythonGetArgIt(self, it));
                return t;
            }

            static bp::object pythonGet(const karabo::util::Hash& self, const bp::object & keyObj) {
                karabo::util::Hash::const_iterator it;

                if (bp::extract<std::string > (keyObj).check()) {
                    std::string key = bp::extract<std::string > (keyObj);
                    it = self.find(key);
                    if (it == self.end()) throw PARAMETER_EXCEPTION("Key \"" + key + "\" does not exist");
                } else {
                    throw PYTHON_EXCEPTION("Currently values can only be retrieved by string keys");
                }
                return pythonGetArgIt(self, it);
            }

            static bp::object pythonGetArgString(const karabo::util::Hash& self, const std::string & key) {
                karabo::util::Hash::const_iterator it = self.find(key);
                if (it == self.end()) throw PARAMETER_EXCEPTION("Key \"" + key + "\" does not exist");
                return pythonGetArgIt(self, it);
            }

            static bp::object pythonGetArgIt(const karabo::util::Hash& self, const karabo::util::Hash::const_iterator & it) {
                if (self.is<bool>(it)) {
                    return bp::object(self.get<bool>(it));
                } else if (self.is<char>(it)) {
                    return bp::object(self.get<char>(it));
                } else if (self.is<signed char>(it)) {
                    return bp::object(self.get<signed char>(it));
                } else if (self.is<unsigned char>(it)) {
                    return bp::object(self.get<unsigned char>(it));
                } else if (self.is<short>(it)) {
                    return bp::object(self.get<short>(it));
                } else if (self.is<unsigned short>(it)) {
                    return bp::object(self.get<unsigned short>(it));
                } else if (self.is<int>(it)) {
                    return bp::object(self.get<int>(it));
                } else if (self.is<unsigned int>(it)) {
                    return bp::object(self.get<unsigned int>(it));
                } else if (self.is<long long>(it)) {
                    return bp::object(self.get<long long>(it));
                } else if (self.is<unsigned long long>(it)) {
                    return bp::object(self.get<unsigned long long>(it));
                } else if (self.is<float>(it)) {
                    return bp::object(self.get<float>(it));
                } else if (self.is<double>(it)) {
                    return bp::object(self.get<double>(it));
                } else if (self.is<std::complex<float> >(it)) {
                    return bp::object(self.get<std::complex<float> >(it));
                } else if (self.is < std::complex<double> >(it)) {
                    return bp::object(self.get<std::complex<double> >(it));
                } else if (self.is<std::string > (it)) {
                    return bp::object(self.get<std::string > (it));
                } else if (self.is<boost::filesystem::path > (it)) {
                    return bp::object(self.get<boost::filesystem::path > (it).string());
                } else if (self.is<karabo::util::Hash > (it)) {
                    return bp::object(self.get<karabo::util::Hash > (it));
                } else if (self.is < std::deque<bool> >(it)) {
                    return bp::object(stdDeque2pyList(self.get < std::deque<bool> >(it)));
                } else if (self.is<std::vector<char> >(it)) {
                    return bp::object(stdVector2pyStr(self.get<std::vector<char> >(it)));
                } else if (self.is < std::vector<signed char> >(it)) {
                    return bp::object(stdVector2pyStr(self.get < std::vector<signed char> >(it)));
                } else if (self.is<std::vector<unsigned char> >(it)) {
                    return bp::object(stdVector2pyStr(self.get<std::vector<unsigned char> >(it)));
                } else if (self.is<std::vector<short> >(it)) {
                    const std::vector<short>& v = boost::any_cast<std::vector<short> >(it->second);
                    Py_intptr_t shape[1] = { v.size() };
                    bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<short>());
                    std::copy(v.begin(), v.end(), reinterpret_cast<short*>(result.get_data()));
                    return result;
                    //return bn::from_data(&v[0], bn::dtype::get_builtin<short>(),
                    //        bp::make_tuple(v.size()), bp::make_tuple(sizeof(v[0])), bp::object());
                    //return bp::object(stdVector2pyList(self.get<std::vector<short> >(it)));
                } else if (self.is<std::vector<unsigned short> >(it)) {
                    const std::vector<unsigned short>& v = boost::any_cast<std::vector<unsigned short> >(it->second);
                    Py_intptr_t shape[1] = { v.size() };
                    bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<unsigned short>());
                    std::copy(v.begin(), v.end(), reinterpret_cast<unsigned short*>(result.get_data()));
                    return result;
                    //return bn::from_data(&v[0], bn::dtype::get_builtin<unsigned short>(),
                    //        bp::make_tuple(v.size()), bp::make_tuple(sizeof(v[0])), bp::object());
                    //return bp::object(stdVector2pyList(self.get<std::vector<unsigned short> >(it)));
                } else if (self.is<std::vector<int> >(it)) {
                    const std::vector<int>& v = boost::any_cast<std::vector<int> >(it->second);
                    Py_intptr_t shape[1] = { v.size() };
                    bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<int>());
                    std::copy(v.begin(), v.end(), reinterpret_cast<int*>(result.get_data()));
                    return result;
                    //return bn::from_data(&v[0], bn::dtype::get_builtin<int>(), bp::make_tuple(v.size()), bp::make_tuple(sizeof(v[0])), bp::object());
                    //return bp::object(stdVector2pyList(self.get<std::vector<int> >(it)));
                } else if (self.is<std::vector<unsigned int> >(it)) {
                    const std::vector<unsigned int>& v = boost::any_cast<std::vector<unsigned int> >(it->second);
                    Py_intptr_t shape[1] = { v.size() };
                    bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<unsigned int>());
                    std::copy(v.begin(), v.end(), reinterpret_cast<unsigned int*>(result.get_data()));
                    return result;
                    //return bn::from_data(&v[0], bn::dtype::get_builtin<unsigned int>(),
                    //        bp::make_tuple(v.size()), bp::make_tuple(sizeof(v[0])), bp::object());
                    //return bp::object(stdVector2pyList(self.get<std::vector<unsigned int> >(it)));
                } else if (self.is<std::vector<long long> >(it)) {
                    const std::vector<long long>& v = boost::any_cast<std::vector<long long> >(it->second);
                    Py_intptr_t shape[1] = { v.size() };
                    bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<long long>());
                    std::copy(v.begin(), v.end(), reinterpret_cast<long long*>(result.get_data()));
                    return result;
                    //return bn::from_data(&v[0], bn::dtype::get_builtin<long long>(),
                    //        bp::make_tuple(v.size()), bp::make_tuple(sizeof(v[0])), bp::object());
                    //return bp::object(stdVector2pyList(self.get<std::vector<long long> >(it)));
                } else if (self.is<std::vector<unsigned long long> >(it)) {
                    const std::vector<unsigned long long>& v = boost::any_cast<std::vector<unsigned long long> >(it->second);
                    Py_intptr_t shape[1] = { v.size() };
                    bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<unsigned long long>());
                    std::copy(v.begin(), v.end(), reinterpret_cast<unsigned long long*>(result.get_data()));
                    return result;
                    //return bn::from_data(&v[0], bn::dtype::get_builtin<unsigned long long>(),
                    //        bp::make_tuple(v.size()), bp::make_tuple(sizeof(v[0])), bp::object());
                    //return bp::object(stdVector2pyList(self.get<std::vector<unsigned long long> >(it)));
                } else if (self.is<std::vector<float> >(it)) {
                    const std::vector<float>& v = boost::any_cast<std::vector<float> >(it->second);
                    Py_intptr_t shape[1] = { v.size() };
                    bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<float>());
                    std::copy(v.begin(), v.end(), reinterpret_cast<float*>(result.get_data()));
                    return result;
                    //return bn::from_data(&v[0], bn::dtype::get_builtin<float>(),
                    //        bp::make_tuple(v.size()), bp::make_tuple(sizeof(v[0])), bp::object());
                    //return bp::object(stdVector2pyList(self.get<std::vector<float> >(it)));
                } else if (self.is<std::vector<double> >(it)) {
                    const std::vector<double>& v = boost::any_cast<std::vector<double> >(it->second);
                    Py_intptr_t shape[1] = { v.size() };
                    bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<double>());
                    std::copy(v.begin(), v.end(), reinterpret_cast<double*>(result.get_data()));
                    return result;
                    //return bn::from_data(&v[0], bn::dtype::get_builtin<double>(),
                    //        bp::make_tuple(v.size()), bp::make_tuple(sizeof(v[0])), bp::object());
                    //return bp::object(stdVector2pyList(self.get<std::vector<double> >(it)));
                    //} else if (self.is<std::vector<std::complex<float> > >(it)) {

                    //} else if (self.is<std::vector<std::complex<double> > >(it)) {
                } else if (self.is<karabo::util::Schema > (it)) {
                    return bp::object(self.get<karabo::util::Schema > (it));
                } else if (self.is<std::vector<std::string> >(it)) {
                    return bp::object(stdVector2pyList(self.get<std::vector<std::string> >(it)));
                    //} else if (self.is<std::vector<boost::filesystem::path> >(it)) {
                    //return bp::object(stdVector2pyList(self.get<std::vector<> >(it)));
                } else if (self.is<std::vector<karabo::util::Hash> >(it)) {
                    return bp::object(stdVector2pyList(self.get<std::vector<karabo::util::Hash> >(it)));
                } else {
                    throw PYTHON_EXCEPTION("Failed to convert inner Hash type of python object");
                }
            }

            static bp::object pythonGetFromPath(const karabo::util::Hash& self, const std::string& path, std::string sep = ".") {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) {
                    throw LOGIC_EXCEPTION("No path (nested key value given)");
                } else if (nElements == 1) {
                    std::string key(v[0]);
                    boost::tuple<bool, std::string, int> arrayType = self.checkKeyForArrayType(key);
                    if (arrayType.get < 0 > () == true) {
                        throw PYTHON_EXCEPTION("Recursive array retrieval is not implemented yet, ask BH");
                    } else {
                        return pythonGetArgString(self, key);
                    }
                } else {
                    std::string shorterPath = path.substr(0, path.find_last_of(sep));
                    std::string last = *(v.rbegin());
                    return pythonGetFromPath(self.r_get<karabo::util::Hash > (shorterPath, sep), last);


                }
            }

            static void pythonSet(karabo::util::Hash& self, const std::string& key, const bp::object & obj) {
                if (PyInt_Check(obj.ptr())) {
                    self.set<int>(key, bp::extract<int>(obj));
                } else if (PyFloat_Check(obj.ptr())) {
                    self.set<double>(key, bp::extract<double>(obj));
                } else if (PyString_Check(obj.ptr())) {
                    self.set<std::string > (key, bp::extract<std::string > (obj));
                } else if (PyLong_Check(obj.ptr())) {
                    self.set<long long>(key, bp::extract<long>(obj));
                } else if (PyBool_Check(obj.ptr())) {
                    self.set<bool>(key, bp::extract<bool>(obj));
                } else if (PyList_Check(obj.ptr())) {
                    const bp::list& l = bp::extract<bp::list > (obj);
                    bp::ssize_t size = bp::len(l);
                    if (size == 0) self.set(key, std::vector<karabo::util::Hash > ());
                    else pyList2stdVector(self, key, l, size);
                } else if (PyDict_Check(obj.ptr())) {
                    karabo::util::Hash hash;
                    pyDict2Hash(hash, bp::extract<bp::dict > (obj));
                    self.set<karabo::util::Hash > (key, hash);
                } else if (bp::extract<bn::ndarray>(obj).check()) {
                    const bn::ndarray& a = bp::extract<bn::ndarray>(obj);
                    int nd = a.get_nd();
                    Py_intptr_t const * shapes = a.get_shape();
                    int nelems = 1;
                    for(int i = 0; i<nd; i++) nelems *= shapes[i];
                    //std::cout << "HashWrap::pythonSet: nelems = " << nelems << std::endl;
                    if (a.get_dtype() == bn::dtype::get_builtin<double>()) {
                        double* data = reinterpret_cast<double*>(a.get_data());
                        std::vector<double> v(data, data + nelems);
                        self.set<std::vector<double> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<float>()) {
                        float* data = reinterpret_cast<float*>(a.get_data());
                        std::vector<float> v(data, data + nelems);
                        self.set<std::vector<float> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<short>()) {
                        short* data = reinterpret_cast<short*>(a.get_data());
                        std::vector<short> v(data, data + nelems);
                        self.set<std::vector<short> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned short>()) {
                        unsigned short* data = reinterpret_cast<unsigned short*>(a.get_data());
                        std::vector<unsigned short> v(data, data + nelems);
                        self.set<std::vector<unsigned short> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<int>()) {
                        int* data = reinterpret_cast<int*>(a.get_data());
                        std::vector<int> v(data, data + nelems);
                        self.set<std::vector<int> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned int>()) {
                        unsigned int* data = reinterpret_cast<unsigned int*>(a.get_data());
                        std::vector<unsigned int> v(data, data + nelems);
                        self.set<std::vector<unsigned int> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<long long>()) {
                        long long* data = reinterpret_cast<long long*>(a.get_data());
                        std::vector<long long> v(data, data + nelems);
                        self.set<std::vector<long long> >(key, v);
                    } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned long long>()) {
                        unsigned long long* data = reinterpret_cast<unsigned long long*>(a.get_data());
                        std::vector<unsigned long long> v(data, data + nelems);
                        self.set<std::vector<unsigned long long> >(key, v);
                    }
                } else if (bp::extract<karabo::util::Schema > (obj).check()) {
                    self.set<karabo::util::Schema > (key, bp::extract<karabo::util::Schema > (obj));
                } else if (bp::extract<karabo::util::Hash > (obj).check()) {
                    self.set<karabo::util::Hash > (key, bp::extract<karabo::util::Hash > (obj));
                } else {
                    throw PYTHON_EXCEPTION("Python type can not be mapped into Hash");
                }
            }

            static void pythonSetFromPath(karabo::util::Hash& self, const std::string& key, const bp::object& obj, const std::string& sep = ".") {
                if (PyInt_Check(obj.ptr())) {
                    self.setFromPath<int>(key, bp::extract<int>(obj), sep);
                } else if (PyFloat_Check(obj.ptr())) {
                    self.setFromPath<double>(key, bp::extract<double>(obj), sep);
                } else if (PyString_Check(obj.ptr())) {
                    self.setFromPath<std::string > (key, bp::extract<std::string > (obj), sep);
                } else if (PyLong_Check(obj.ptr())) {
                    self.setFromPath<long long>(key, bp::extract<long>(obj), sep);
                } else if (PyBool_Check(obj.ptr())) {
                    self.setFromPath<bool>(key, bp::extract<bool>(obj), sep);
                } else if (bp::extract<karabo::util::Schema > (obj).check()) {
                    self.setFromPath<karabo::util::Schema > (key, bp::extract<karabo::util::Schema > (obj), sep);
                } else if (bp::extract<karabo::util::Hash > (obj).check()) {
                    self.setFromPath<karabo::util::Hash > (key, bp::extract<karabo::util::Hash > (obj), sep);
                } else if (PyList_Check(obj.ptr())) {
                    // TODO Check whether a const & will also do
                    const bp::list& l = bp::extract<bp::list > (obj);
                    bp::ssize_t size = bp::len(l);
                    if (size == 0) self.setFromPath(key, std::vector<karabo::util::Hash > (), sep);
                    else pyList2stdVectorFromPath(self, key, l, size, sep);
                } else if (PyDict_Check(obj.ptr())) {
                    karabo::util::Hash hash;
                    pyDict2Hash(hash, bp::extract<bp::dict > (obj));
                    self.setFromPath<karabo::util::Hash > (key, hash);
                } else {
                    throw PYTHON_EXCEPTION("Python type can not be mapped into Hash");
                }
            }

            static void pyStr2stdVector(karabo::util::Hash& self, const std::string& key, const bp::object& pystr) {
                // elements of our vectors require to be of the same type
                if (!PyString_Check(pystr.ptr())) {
                    throw PYTHON_EXCEPTION("Failed to convert  python string to vector of unsigned char ");
                }
                const std::string& stdstr = bp::extract<std::string > (pystr);
                self.set(key, std::vector<unsigned char>(stdstr.begin(), stdstr.end()));
            }

            static void pyList2stdVector(karabo::util::Hash& self, const std::string& key, const bp::object& list, bp::ssize_t size) {
                // elements of our vectors require to be of the same type
                bp::object list0 = list[0];
                if (PyInt_Check(list0.ptr())) {
                    std::vector<int> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<int>(list[i]);
                    }
                    self.set(key, v);
                } else if (PyFloat_Check(list0.ptr())) {
                    std::vector<double> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<double>(list[i]);
                    }
                    self.set(key, v);
                } else if (PyString_Check(list0.ptr())) {
                    std::vector<std::string> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<std::string > (list[i]);
                    }
                    self.set(key, v);

                } else if (PyLong_Check(list0.ptr())) {
                    std::vector<long long> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<long>(list[i]);
                    }
                    self.set(key, v);

                } else if (PyBool_Check(list0.ptr())) {
                    std::deque<bool> v(size); // Special case here
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<bool>(list[i]);
                    }
                    self.set(key, v);

                } else if (PyDict_Check(list0.ptr())) {
                    std::vector<karabo::util::Hash> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        bp::dict d = bp::extract<bp::dict > (list[i]);
                        pyDict2Hash(v[i], d);
                    }
                    self.set(key, v);

                } else if (bp::extract<karabo::util::Hash > (list[0]).check()) {
                    std::vector<karabo::util::Hash> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<karabo::util::Hash > (list[i]);
                    }
                    self.set(key, v);

                } else {
                    throw PYTHON_EXCEPTION("Failed to convert inner type of python list");
                }
            }

            static void pyList2stdVectorFromPath(karabo::util::Hash& self, const std::string& key, const bp::object& list, bp::ssize_t size, const std::string & sep) {
                // elements of our vectors require to be of the same type
                bp::object list0 = list[0];
                if (PyInt_Check(list0.ptr())) {
                    std::vector<int> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<int>(list[i]);
                    }
                    self.setFromPath(key, v, sep);
                } else if (PyFloat_Check(list0.ptr())) {
                    std::vector<double> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<double>(list[i]);
                    }
                    self.setFromPath(key, v, sep);
                } else if (PyString_Check(list0.ptr())) {
                    std::vector<std::string> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<std::string > (list[i]);
                    }
                    self.setFromPath(key, v, sep);

                } else if (PyLong_Check(list0.ptr())) {
                    std::vector<long long> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<long>(list[i]);
                    }
                    self.setFromPath(key, v, sep);

                } else if (PyBool_Check(list0.ptr())) {
                    std::deque<bool> v(size); // Special case here
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<bool>(list[i]);
                    }
                    self.setFromPath(key, v, sep);

                } else if (PyDict_Check(list0.ptr())) {
                    std::vector<karabo::util::Hash> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        bp::dict d = bp::extract<bp::dict > (list[i]);
                        pyDict2Hash(v[i], d);
                    }
                    self.setFromPath(key, v, sep);

                } else if (bp::extract<karabo::util::Hash > (list[0]).check()) {
                    std::vector<karabo::util::Hash> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<karabo::util::Hash > (list[i]);
                    }
                    self.setFromPath(key, v, sep);

                } else {
                    throw PYTHON_EXCEPTION("Failed to convert inner type of python list");
                }
            }

            static bp::str stdVector2pyStr(const std::vector<char>& v) {
                return bp::str(&v[0], v.size());
            }

            static bp::str stdVector2pyStr(const std::vector<signed char>& v) {
                return bp::str(reinterpret_cast<const char*>(&v[0]), v.size());
            }

            static bp::str stdVector2pyStr(const std::vector<unsigned char>& v) {
                return bp::str(reinterpret_cast<const char*> (&v[0]), v.size());
            }

            template<class T >
            static bp::list stdVector2pyList(const std::vector<T>& v) {
                bp::object it = bp::iterator<std::vector<T> >();
                bp::object iter = it(v);
                bp::list l(iter);
                return l;
            }

            template<class T >
            static bp::list stdDeque2pyList(const std::deque<T>& v) {
                bp::object it = bp::iterator<std::deque<T> >();
                bp::object iter = it(v);
                bp::list l(iter);
                return l;
            }

            static const karabo::util::Hash& pyDict2Hash(karabo::util::Hash& self, const bp::dict & dictionary) {
                bp::list keys(dictionary.iterkeys());
                for (bp::ssize_t i = 0; i < bp::len(keys); i++) {
                    pythonSet(self, bp::extract<std::string > (keys[i]), dictionary[keys[i]]);
                }
                return self;
            }

            static const karabo::util::Hash& pyDict2HashFromPath(karabo::util::Hash& self, const bp::dict& dictionary, const std::string & sep) {
                bp::list keys(dictionary.iterkeys());
                for (bp::ssize_t i = 0; i < bp::len(keys); i++) {
                    pythonSetFromPath(self, bp::extract<std::string > (keys[i]), dictionary[keys[i]], sep);
                }
                return self;
            }

            static void pythonErase(karabo::util::Hash& self, const bp::object & keyObj) {
                karabo::util::Hash::iterator it;

                if (bp::extract<std::string > (keyObj).check()) {
                    std::string key = bp::extract<std::string > (keyObj);
                    it = self.find(key);
                    if (it == self.end()) throw PARAMETER_EXCEPTION("Key \"" + key + "\" does not exist");
                } else {
                    throw PYTHON_EXCEPTION("Currently values can only be retrieved by string keys");
                }
                self.erase(it);
            }

            static bp::object pythonFlatten(const karabo::util::Hash& self, const std::string& sep = ".") {
                return bp::object(self.flatten(sep));
            }

            static bp::object pythonUnFlatten(const karabo::util::Hash& self, const std::string& sep = ".") {
                return bp::object(self.unflatten(sep));
            }

        };
    }
}

namespace karabo {
    namespace util {

        template<>
        karabo::util::Hash::Hash(const std::string& key, const bp::object& value);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2, const bp::object& value2);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2, const bp::object& value2,
                const std::string& key3, const bp::object& value3);

        template<>
        karabo::util::Hash::Hash(const std::string& key1, const bp::object& value1, const std::string& key2, const bp::object& value2,
                const std::string& key3, const bp::object& value3, const std::string& key4, const bp::object& value4);
    }
}

#endif

