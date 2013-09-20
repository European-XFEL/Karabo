/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/xip/CpuImage.hh>

namespace bp = boost::python;
using namespace karabo::xip;
using namespace karabo::util;
using namespace std;

template <class T>
void exportPyXipCpuImage() {
    //exposing karabo::xip::CpuImage<T>, where T : int or double
    typedef karabo::xip::CpuImage<T> CpuImageT;
    bp::class_< CpuImageT, boost::noncopyable> cpuimg(string("CpuImage" + karabo::util::Types::convert<FromTypeInfo, ToLiteral > (typeid (T))).c_str(), bp::init<>());
    bp::implicitly_convertible< string const, CpuImageT >();
    bp::implicitly_convertible< Hash const &, CpuImageT >();
    bp::implicitly_convertible< cimg_library::CImg< int > const &, CpuImageT >();

    cpuimg.def( bp::init< string const & >(( bp::arg("filename") )) );
    cpuimg.def( bp::init< unsigned int, bp::optional< unsigned int, unsigned int > >(( bp::arg("dx"), bp::arg("dy")=(unsigned int const)(1), bp::arg("dz")=(unsigned int const)(1) )) );
    cpuimg.def( bp::init< unsigned int, unsigned int, unsigned int, int const & >(( bp::arg("dx"), bp::arg("dy"), bp::arg("dz"), bp::arg("value") )) );
    cpuimg.def( bp::init< unsigned int, unsigned int, unsigned int, string const &, bool >(( bp::arg("dx"), bp::arg("dy"), bp::arg("dz"), bp::arg("values"), bp::arg("repeatValues") )) );
    cpuimg.def( bp::init< T const *, unsigned int, unsigned int, unsigned int >(( bp::arg("dataBuffer"), bp::arg("dx"), bp::arg("dy"), bp::arg("dz") )) );
    cpuimg.def( bp::init< std::vector< T > const &, unsigned int, unsigned int, unsigned int >(( bp::arg("dataBuffer"), bp::arg("dx"), bp::arg("dy"), bp::arg("dz") )) );
    cpuimg.def( bp::init< Hash const & >(( bp::arg("header") )) );   
    cpuimg.def( bp::init< Hash const &, int const & >(( bp::arg("header"), bp::arg("value") )) );
    cpuimg.def( bp::init< cimg_library::CImg< int > const & >(( bp::arg("cimg") )) );
    
    // Instance Characteristics
    cpuimg.def("dimensionality", &CpuImageT::dimensionality);

    cpuimg.def("isEmpty", &CpuImageT::isEmpty);

    cpuimg.def("dimX", &CpuImageT::dimX);
    cpuimg.def("dimY", &CpuImageT::dimY);
    cpuimg.def("dimZ", &CpuImageT::dimZ);

    cpuimg.def("getHeader"
            , (Hash const & (CpuImageT::*)() const) (&CpuImageT::getHeader)
            , bp::return_value_policy< bp::copy_const_reference >());

    cpuimg.def("setHeader", &CpuImageT::setHeader);

    cpuimg.def("size", &CpuImageT::size);

    cpuimg.def("byteSize", &CpuImageT::byteSize);

    cpuimg.def("pixelType", &CpuImageT::pixelType);

    cpuimg.def("getStatistics", &CpuImageT::getStatistics);

    cpuimg.def("print"
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
    
    // In-Place Construction
    
    //CpuImage& assign(const TPix * const dataBuffer, const int dx, const int dy, const int dz)     
    cpuimg.def("assign"
                , (CpuImageT & (CpuImageT::*)(T const * const,int const,int const,int const))(&CpuImageT::assign)
                , ( bp::arg("dataBuffer"), bp::arg("dx"), bp::arg("dy"), bp::arg("dz") )
                , bp::return_internal_reference<> () );
    
    //karabo::xip::CpuImage< T >::read
     cpuimg.def("read"
                , (CpuImageT & (CpuImageT::*)(string const &))(&CpuImageT::read)
                , ( bp::arg("filename") )
                , bp::return_internal_reference<> () );
    
}
template void exportPyXipCpuImage<int>();
template void exportPyXipCpuImage<double>();
template void exportPyXipCpuImage<char>();
template void exportPyXipCpuImage<float>();