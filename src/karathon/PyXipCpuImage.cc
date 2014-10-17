/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>
#include "boost/python/suite/indexing/vector_indexing_suite.hpp"
#include <karabo/xip/CpuImage.hh>
#include "FromNumpy.hh"
#include "ToNumpy.hh"
#include "Wrapper.hh"

namespace bp = boost::python;
using namespace karabo::xip;
using namespace karabo::util;
using namespace std;

namespace karathon {

    template <class T>
    struct CpuImageWrap {
        
        static void assignNdarrayPy(CpuImage<T>& self, const bp::object& obj) {

            if (!PyArray_Check(obj.ptr())) throw KARABO_PYTHON_EXCEPTION("The 1st argument must be of type 'numpy.ndarray'");

            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());
            T* data = reinterpret_cast<T*> (PyArray_DATA(arr));
            //size_t size = PyArray_NBYTES(arr);

            int rank = PyArray_NDIM(arr);
            npy_intp* shapes = PyArray_DIMS(arr);
            std::vector<unsigned long long> tmp(rank);
            for (int i = 0; i < rank; ++i) tmp[rank - i - 1] = shapes[i];
            karabo::util::Dims dimensions;
            dimensions.fromVector(tmp);
            
            self.assign(data, dimensions.x(), dimensions.y(), dimensions.z());
        }

        static bp::object getNdarrayPy(const CpuImage<T>& self) {
            if (self.isEmpty()) throw KARABO_PYTHON_EXCEPTION("Empty CpuImage");
            
            std::vector<unsigned long long> dims = self.dims();
            std::reverse(dims.begin(), dims.end());
            
            npy_intp shape[dims.size()];
            for (size_t i = 0; i < dims.size(); ++i) shape[i] = dims[i];
            
            int npyType = karabo::util::Types::convert<FromTypeInfo, ToNumpy > (typeid (T));
            PyObject* pyobj = PyArray_SimpleNew(dims.size(), shape, npyType);
            
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), self.pixelPointer(), PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj));
        }
    };
}

template <class T>
void exportPyXipCpuImage() {

    //exposing karabo::xip::CpuImage<T>
    typedef karabo::xip::CpuImage<T> CpuImageT;
    bp::class_< CpuImageT, boost::noncopyable> cpuimg(string("CpuImage" + karabo::util::Types::convert<FromTypeInfo, ToLiteral > (typeid (T))).c_str(), bp::init<>());
    bp::implicitly_convertible< string const, CpuImageT >();
    bp::implicitly_convertible< Hash const &, CpuImageT >();
    bp::implicitly_convertible< cimg_library::CImg< T > const &, CpuImageT >();

    cpuimg.def( bp::init< string const & >(( bp::arg("filename") )) );
    cpuimg.def( bp::init< unsigned int, bp::optional< unsigned int, unsigned int > >(( bp::arg("dx"), bp::arg("dy")=(unsigned int const)(1), bp::arg("dz")=(unsigned int const)(1) )) );
    cpuimg.def( bp::init< unsigned int, unsigned int, unsigned int, T const & >(( bp::arg("dx"), bp::arg("dy"), bp::arg("dz"), bp::arg("value") )) );
    cpuimg.def( bp::init< unsigned int, unsigned int, unsigned int, string const &, bool >(( bp::arg("dx"), bp::arg("dy"), bp::arg("dz"), bp::arg("values"), bp::arg("repeatValues") )) );
    cpuimg.def( bp::init< T const *, unsigned int, unsigned int, unsigned int >(( bp::arg("dataBuffer"), bp::arg("dx"), bp::arg("dy"), bp::arg("dz") )) );
    cpuimg.def( bp::init< std::vector< T > const &, unsigned int, unsigned int, unsigned int >(( bp::arg("dataBuffer"), bp::arg("dx"), bp::arg("dy"), bp::arg("dz") )) );
    cpuimg.def( bp::init< Hash const & >(( bp::arg("header") )) );   
    cpuimg.def( bp::init< Hash const &, T const & >(( bp::arg("header"), bp::arg("value") )) );
    cpuimg.def( bp::init< cimg_library::CImg< T > const & >(( bp::arg("cimg") )) );

    /***************************************
     *      Instance Characteristics       *
     ***************************************/
    cpuimg.def("dimensionality", &CpuImageT::dimensionality);

    cpuimg.def("isEmpty", &CpuImageT::isEmpty);

    cpuimg.def("dimX", &CpuImageT::dimX);
    cpuimg.def("dimY", &CpuImageT::dimY);
    cpuimg.def("dimZ", &CpuImageT::dimZ);

    cpuimg.def("getHeader"
            , (Hash const & (CpuImageT::*)() const) (&CpuImageT::getHeader)
            , bp::return_value_policy< bp::copy_const_reference >());

    cpuimg.def("setHeader", &CpuImageT::setHeader);

    cpuimg.def("setHeaderParam"
            , (void (CpuImageT::*)(string const &, char const * const &))(&CpuImageT::setHeaderParam)
            , (bp::arg("key"), bp::arg("value")));

    cpuimg.def("setHeaderParam"
            , (void (CpuImageT::*)(string const &, string const &))(&CpuImageT::setHeaderParam)
            , (bp::arg("key"), bp::arg("value")));

    cpuimg.def("setHeaderParam"
            , (void (CpuImageT::*)(string const &, bool const))(&CpuImageT::setHeaderParam)
            , (bp::arg("key"), bp::arg("value")));

    cpuimg.def("setHeaderParam"
            , (void (CpuImageT::*)(string const &, int const))(&CpuImageT::setHeaderParam)
            , (bp::arg("key"), bp::arg("value")));

    cpuimg.def("setHeaderParam"
            , (void (CpuImageT::*)(string const &, double const))(&CpuImageT::setHeaderParam)
            , (bp::arg("key"), bp::arg("value")));

    cpuimg.def("size", &CpuImageT::size);

    cpuimg.def("byteSize", &CpuImageT::byteSize);

    cpuimg.def("pixelType", &CpuImageT::pixelType);

    cpuimg.def("getStatistics"
                , (Statistics (CpuImageT::*)())(&CpuImageT::getStatistics));

    //cpuimg.def(bp::self_ns::str(bp::self));

    cpuimg.def("imagePrint"
            , (CpuImageT const & (CpuImageT::*)(string const &, bool const, int, int, int) const)(&CpuImageT::print)
            , (bp::arg("title") = "", bp::arg("displayPixels") = (bool const) (true), bp::arg("maxDimX") = (int) (28), bp::arg("maxDimY") = (int) (28), bp::arg("maxDimZ") = (int) (8))
            , bp::return_value_policy< bp::copy_const_reference >());

    // Operators

    cpuimg.def("__getitem__"
            , (T const & (CpuImageT::*)(size_t const) const) (&CpuImageT::operator[])
            , (bp::arg("offset"))
            , bp::return_value_policy< bp::copy_const_reference >());

    cpuimg.def("__getitem__"
            , (T & (CpuImageT::*)(size_t const))(&CpuImageT::operator[])
            , (bp::arg("offset"))
            , bp::return_value_policy< bp::copy_non_const_reference >());

    cpuimg.def("__call__"
            , (T const & (CpuImageT::*)(size_t const, size_t const, size_t const) const) (&CpuImageT::operator())
            , (bp::arg("x"), bp::arg("y") = (unsigned int const) (0), bp::arg("z") = (unsigned int const) (0))
            , bp::return_value_policy< bp::copy_const_reference >());

    cpuimg.def("__call__"
            , (T & (CpuImageT::*)(size_t const, size_t const, size_t const))(&CpuImageT::operator())
            , (bp::arg("x"), bp::arg("y") = (unsigned int const) (0), bp::arg("z") = (unsigned int const) (0))
            , bp::return_value_policy< bp::copy_non_const_reference >());

    // Convenience Functions
    cpuimg.def("getSum", &CpuImageT::getSum);
    cpuimg.def("getMean", &CpuImageT::getMean);

    //Assign ndarray or existing image
    cpuimg.def("assign", &karathon::CpuImageWrap<T>::assignNdarrayPy, (bp::arg("input")));   
    cpuimg.def("assign"
            , (CpuImageT & (CpuImageT::*)(CpuImageT const &, bool))(&CpuImageT::assign)
            , (bp::arg("image"), bp::arg("isShared") = (bool)(false))
            , bp::return_internal_reference<> ());
    
    //Get ndarray
    cpuimg.def("getData", &karathon::CpuImageWrap<T>::getNdarrayPy);

    /***************************************
     *         Special functions           *
     ***************************************/

    cpuimg.def("swap"
            , (void (CpuImageT::*)(CpuImageT &))(&CpuImageT::swap)
            , (bp::arg("image")));

    cpuimg.def("swap"
            , (void (CpuImageT::*)(CpuImageT const &))(&CpuImageT::swap)
            , (bp::arg("image")));

    cpuimg.def("moveTo"
            , (CpuImageT & (CpuImageT::*)(CpuImageT &))(&CpuImageT::moveTo)
            , (bp::arg("image"))
            , bp::return_internal_reference<> ());

    cpuimg.def("clear"
            , (CpuImageT & (CpuImageT::*)())(&CpuImageT::clear)
            , bp::return_internal_reference<> ());


    cpuimg.def("read"
            , (CpuImageT & (CpuImageT::*)(string const &))(&CpuImageT::read)
            , (bp::arg("filename"))
            , bp::return_internal_reference<> ());

    cpuimg.def("write"
            , (CpuImageT const & (CpuImageT::*)(string const &, bool const) const) (&CpuImageT::write)
            , (bp::arg("filename"), bp::arg("enableAppendMode") = (bool const) (false))
            , bp::return_value_policy< bp::copy_const_reference >());

    cpuimg.def("offset"
            , (size_t(CpuImageT::*)(size_t const, size_t const, size_t const))(&CpuImageT::offset)
            , (bp::arg("x"), bp::arg("y") = (unsigned int const) (0), bp::arg("z") = (unsigned int const) (0)));
}
template void exportPyXipCpuImage<int>();
template void exportPyXipCpuImage<unsigned int>();
template void exportPyXipCpuImage<double>();
template void exportPyXipCpuImage<float>();
template void exportPyXipCpuImage<char>();
template void exportPyXipCpuImage<unsigned char>();
template void exportPyXipCpuImage<long long>();
template void exportPyXipCpuImage<unsigned long long>();
template void exportPyXipCpuImage<short>();
template void exportPyXipCpuImage<unsigned short>();