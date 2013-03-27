/* 
 * File:   Wrapper.hh
 * Author: esenov
 *
 * Created on March 17, 2013, 10:52 PM
 */

#ifndef WRAPPER_HH
#define	WRAPPER_HH

#include <boost/python.hpp>
#include <boost/any.hpp>
#include <boost/numpy.hpp>
#include <karabo/util/Hash.hh>

namespace bn = boost::numpy;
namespace bp = boost::python;

namespace karabo {
    namespace pyexfel {


        struct Wrapper {
            static bool try_to_use_numpy;

            template<class ValueType>
            static bp::object fromStdVectorToPyArray(const std::vector<ValueType>& v) {
                if (try_to_use_numpy) {
                    Py_intptr_t shape[1] = {v.size()};
                    bn::ndarray result = bn::zeros(1, shape, bn::dtype::get_builtin<ValueType>());
                    std::copy(v.begin(), v.end(), reinterpret_cast<ValueType*> (result.get_data()));
                    return result;
                }
                return fromStdVectorToPyList(v);
            }

            template<class ValueType>
            static bp::object fromStdVectorToPyList(const std::vector<ValueType>& v) {
                bp::list pylist;
                for (size_t i = 0; i < v.size(); i++) pylist.append(bp::object(v[i]));
                return pylist;
            }

            static bp::object fromStdVectorToPyHashList(const std::vector<karabo::util::Hash>& v) {
                bp::object it = bp::iterator<std::vector<karabo::util::Hash> >();
                bp::object iter = it(v);
                bp::list l(iter);
                return l;
            }

            static bp::object fromStdVectorToPyByteArray(const std::vector<char>& v) {
                const char* data = &v[0];
                Py_ssize_t size = v.size();
                return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(data, size)));
            }

            static bp::object fromStdVectorToPyByteArray(const std::vector<signed char>& v) {
                const char* data = reinterpret_cast<const char*> (&v[0]);
                Py_ssize_t size = v.size();
                return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(data, size)));
            }

            static bp::object fromStdVectorToPyByteArray(const std::vector<unsigned char>& v) {
                const char* data = reinterpret_cast<const char*> (&v[0]);
                Py_ssize_t size = v.size();
                return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(data, size)));
            }

            static bp::str fromStdVectorToPyStr(const std::vector<char>& v) {
                return bp::str(&v[0], v.size());
            }

            static bp::str fromStdVectorToPyStr(const std::vector<signed char>& v) {
                return bp::str(reinterpret_cast<const char*> (&v[0]), v.size());
            }

            static bp::str fromStdVectorToPyStr(const std::vector<unsigned char>& v) {
                return bp::str(reinterpret_cast<const char*> (&v[0]), v.size());
            }

            static bp::object toObject(const boost::any& operand);
            static void toAny(const bp::object& operand, boost::any& any);
        };
    }
}

#endif	/* WRAPPER_HH */

