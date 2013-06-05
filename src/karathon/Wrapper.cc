/* 
 * File:   Wrapper.cc
 * Author: esenov
 *
 * Created on March 17, 2013, 11:06 PM
 */

#include "Wrapper.hh"
#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace pyexfel {

        bp::object Wrapper::toObject(const boost::any& operand, bool numpyFlag) {
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
                    return fromStdVectorToPyArray(boost::any_cast < std::vector<bool> >(operand), numpyFlag);
                } else if (operand.type() == typeid (std::vector<char>)) {
                    return fromStdVectorToPyByteArray(boost::any_cast<std::vector<char> >(operand));
                } else if (operand.type() == typeid (std::vector<signed char>)) {
                    return fromStdVectorToPyByteArray(boost::any_cast < std::vector<signed char> >(operand));
                } else if (operand.type() == typeid (std::vector<unsigned char>)) {
                    return fromStdVectorToPyByteArray(boost::any_cast<std::vector<unsigned char> >(operand));
                } else if (operand.type() == typeid (std::vector<short>)) {
                    return fromStdVectorToPyArray(boost::any_cast<std::vector<short> >(operand), numpyFlag);
                } else if (operand.type() == typeid (std::vector<unsigned short>)) {
                    return fromStdVectorToPyArray(boost::any_cast<std::vector<unsigned short> >(operand), numpyFlag);
                } else if (operand.type() == typeid (std::vector<int>)) {
                    return fromStdVectorToPyArray(boost::any_cast<std::vector<int> >(operand), numpyFlag);
                } else if (operand.type() == typeid (std::vector<unsigned int>)) {
                    return fromStdVectorToPyArray(boost::any_cast<std::vector<unsigned int> >(operand), numpyFlag);
                } else if (operand.type() == typeid (std::vector<long long>)) {
                    return fromStdVectorToPyArray(boost::any_cast<std::vector<long long> >(operand), numpyFlag);
                } else if (operand.type() == typeid (std::vector<unsigned long long>)) {
                    return fromStdVectorToPyArray(boost::any_cast<std::vector<unsigned long long> >(operand), numpyFlag);
                } else if (operand.type() == typeid (std::vector<float>)) {
                    return fromStdVectorToPyArray(boost::any_cast<std::vector<float> >(operand), numpyFlag);
                } else if (operand.type() == typeid (std::vector<double>)) {
                    return fromStdVectorToPyArray(boost::any_cast<std::vector<double> >(operand), numpyFlag);
                } else if (operand.type() == typeid (karabo::util::Hash)) {
                    return bp::object(boost::any_cast<karabo::util::Hash>(operand));
                } else if (operand.type() == typeid (karabo::util::Schema)) {
                    return bp::object(boost::any_cast<karabo::util::Schema>(operand));
                } else if (operand.type() == typeid (std::vector<std::string>)) {
                    return fromStdVectorToPyList(boost::any_cast < std::vector<std::string> >(operand));
                } else if (operand.type() == typeid (std::vector<karabo::util::Hash>)) {
                    return fromStdVectorToPyHashList(boost::any_cast<std::vector<karabo::util::Hash> >(operand));
                } else {
                    throw KARABO_PYTHON_EXCEPTION("Failed to convert inner Hash type of python object");
                }
            } catch (const boost::bad_any_cast& e) {
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(e.what()));
            }
        }

        void Wrapper::toAny(const bp::object& obj, boost::any& any) {
            if (PyBool_Check(obj.ptr())) {
                bool b = bp::extract<bool>(obj);
                any = b;
                return;
            }
            if (PyInt_Check(obj.ptr())) {
                int b = bp::extract<int>(obj);
                any = b;
                return;
            }
            if (PyFloat_Check(obj.ptr())) {
                double b = bp::extract<double>(obj);
                any = b;
                return;
            }
            if (PyString_Check(obj.ptr())) {
                std::string b = bp::extract<std::string >(obj);
                any = b;
                return;
            }
            if (PyLong_Check(obj.ptr())) {
                //return PyLong_AsLongLong(obj.ptr());
                long long b = bp::extract<long>(obj);
                any = b;
                return;
            }
            if (PyByteArray_Check(obj.ptr())) {
                size_t size = PyByteArray_Size(obj.ptr());
                char* data = PyByteArray_AsString(obj.ptr());
                std::vector<char> b(data, data + size);
                any = b;
                return;
            }
            if (bp::extract<char* const>(obj).check()) {
                char* const b = bp::extract<char* const>(obj);
                any = b;
                return;
            }
            if (bp::extract<wchar_t* const>(obj).check()) {
                wchar_t* const b = bp::extract<wchar_t* const>(obj);
                any = b;
                return;
            }
            if (bp::extract<karabo::util::Hash>(obj).check()) {
                Hash h = bp::extract<Hash>(obj);
                any = h;
                return;
            }
            if (bp::extract<karabo::util::Schema>(obj).check()) {
                Schema s = bp::extract<Schema>(obj);
                any = s;
                return;
            }
            if (bp::extract<bn::ndarray>(obj).check()) {
                const bn::ndarray& a = bp::extract<bn::ndarray>(obj);
                int nd = a.get_nd();
                Py_intptr_t const * shapes = a.get_shape();
                int nelems = 1;
                for (int i = 0; i < nd; i++) nelems *= shapes[i];
                if (a.get_dtype() == bn::dtype::get_builtin<bool>()) {
                    bool* data = reinterpret_cast<bool*> (a.get_data());
                    std::vector<bool> v(data, data + nelems);
                    any = v;
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<double>()) {
                    double* data = reinterpret_cast<double*> (a.get_data());
                    std::vector<double> v(data, data + nelems);
                    any = v;
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<float>()) {
                    float* data = reinterpret_cast<float*> (a.get_data());
                    std::vector<float> v(data, data + nelems);
                    any = v;
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<short>()) {
                    short* data = reinterpret_cast<short*> (a.get_data());
                    std::vector<short> v(data, data + nelems);
                    any = v;
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned short>()) {
                    unsigned short* data = reinterpret_cast<unsigned short*> (a.get_data());
                    std::vector<unsigned short> v(data, data + nelems);
                    any = v;
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<int>()) {
                    int* data = reinterpret_cast<int*> (a.get_data());
                    std::vector<int> v(data, data + nelems);
                    any = v;
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned int>()) {
                    unsigned int* data = reinterpret_cast<unsigned int*> (a.get_data());
                    std::vector<unsigned int> v(data, data + nelems);
                    any = v;
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<long long>()) {
                    long long* data = reinterpret_cast<long long*> (a.get_data());
                    std::vector<long long> v(data, data + nelems);
                    any = v;
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned long long>()) {
                    unsigned long long* data = reinterpret_cast<unsigned long long*> (a.get_data());
                    std::vector<unsigned long long> v(data, data + nelems);
                    any = v;
                    return;
                }
            }

            if (PyList_Check(obj.ptr())) {
                bp::object list0 = obj[0];
                bp::ssize_t size = bp::len(obj);
                if (PyBool_Check(list0.ptr())) {
                    std::vector<bool> v(size); // Special case here
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<bool>(obj[i]);
                    }
                    any = v;
                    return;
                }
                if (PyInt_Check(list0.ptr())) {
                    std::vector<int> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<int>(obj[i]);
                    }
                    any = v;
                    return;
                }
                if (PyFloat_Check(list0.ptr())) {
                    std::vector<double> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<double>(obj[i]);
                    }
                    any = v;
                    return;
                }
                if (PyLong_Check(list0.ptr())) {
                    std::vector<long long> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<long>(obj[i]);
                    }
                    any = v;
                    return;
                }
                if (PyString_Check(list0.ptr())) {
                    std::vector<std::string> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<std::string > (obj[i]);
                    }
                    any = v;
                    return;
                }
                if (bp::extract<Hash>(list0).check()) {
                    std::vector<Hash> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<Hash>(obj[i]);
                    }
                    any = v;
                    return;
                }
                if (bp::extract<Schema>(list0).check()) {
                    std::vector<Schema> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<Schema>(obj[i]);
                    }
                    any = v;
                    return;
                }
            }
            throw KARABO_PYTHON_EXCEPTION("Python type can not be mapped into Hash");
        }
    }
}

