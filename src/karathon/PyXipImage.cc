/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/xip/Image.hh>
#include <karabo/xip/Environment.hh>

namespace bp = boost::python;
using namespace karabo::xip;
using namespace karabo::util;
using namespace std;


void exportPyXipImageType() {
    bp::class_<AbstractImageType> ("AbstractImageType");

    bp::enum_<karabo::xip::ImageType>("ImageType")
            .value("CPU", karabo::xip::CPU)
            .value("GPU", karabo::xip::GPU)
            .export_values()
            ;
}


template <class T>
void exportPyXipImage() {
    //exposing karabo::xip::Image<T>, where T : float or double
    typedef karabo::xip::Image<T> ImageT;
    bp::class_< ImageT, boost::noncopyable> img(string("Image" + karabo::util::Types::convert<FromTypeInfo, ToLiteral > (typeid (T))).c_str(), bp::init<int>((bp::arg("imageType"))));
    bp::implicitly_convertible< int const, karabo::xip::Image<T> >();

    img.def(bp::init< int, string const & >((bp::arg("imageType"), bp::arg("filename"))));
    img.def(bp::init< int, unsigned int, bp::optional< unsigned int, unsigned int > >((bp::arg("imageType"), bp::arg("dx"), bp::arg("dy") = (unsigned int const) (1), bp::arg("dz") = (unsigned int const) (1))));
    img.def(bp::init< int, unsigned int, unsigned int, unsigned int, float const & >((bp::arg("imageType"), bp::arg("dx"), bp::arg("dy"), bp::arg("dz"), bp::arg("value"))));
    img.def(bp::init< int, unsigned int, unsigned int, unsigned int, std::string const &, bool >((bp::arg("imageType"), bp::arg("dx"), bp::arg("dy"), bp::arg("dz"), bp::arg("values"), bp::arg("repeatValues"))));
    img.def(bp::init< int, T const *, unsigned int, unsigned int, unsigned int >((bp::arg("imageType"), bp::arg("dataBuffer"), bp::arg("dx"), bp::arg("dy"), bp::arg("dz"))));
    img.def(bp::init< int, std::vector<T> const &, unsigned int, unsigned int, unsigned int >((bp::arg("imageType"), bp::arg("dataBuffer"), bp::arg("dx"), bp::arg("dy"), bp::arg("dz"))));
    img.def(bp::init< int, karabo::util::Hash const & >((bp::arg("imageType"), bp::arg("header"))));
    img.def(bp::init< int, karabo::util::Hash const &, T const & >((bp::arg("imageType"), bp::arg("header"), bp::arg("value"))));

    // Instance Characteristics
    img.def("dimensionality", &ImageT::dimensionality);

    img.def("isEmpty", &ImageT::isEmpty);

    img.def("dimX", &ImageT::dimX);
    img.def("dimY", &ImageT::dimY);
    img.def("dimZ", &ImageT::dimZ);

    img.def("getHeader"
            , (Hash const & (ImageT::*)() const) (&ImageT::getHeader)
            , bp::return_value_policy< bp::copy_const_reference >());

    img.def("setHeader", &ImageT::setHeader);

    img.def("size", &ImageT::size);

    img.def("byteSize", &ImageT::byteSize);

    img.def("pixelType", &ImageT::pixelType);

    img.def("getStatistics", &ImageT::getStatistics);

    img.def("print"
            , (ImageT & (ImageT::*)(string const &, bool const, int, int, int))(&ImageT::print)
            , (bp::arg("title") = "", bp::arg("displayPixels") = (bool const) (true), bp::arg("maxDimX") = (int) (28), bp::arg("maxDimY") = (int) (28), bp::arg("maxDimZ") = (int) (8))
            , bp::return_value_policy< bp::copy_non_const_reference >());

    // Operators

    img.def("__getitem__"
            , (T const & (ImageT::*)(size_t const) const) (&ImageT::operator[])
            , (bp::arg("offset"))
            , bp::return_value_policy< bp::copy_const_reference >());

    img.def("__getitem__"
            , (T & (ImageT::*)(size_t const))(&ImageT::operator[])
            , (bp::arg("offset"))
            , bp::return_value_policy< bp::copy_non_const_reference >());

    img.def("__call__"
            , (T const & (ImageT::*)(size_t const, size_t const, size_t const) const) (&ImageT::operator())
            , (bp::arg("x"), bp::arg("y") = (unsigned int const) (0), bp::arg("z") = (unsigned int const) (0))
            , bp::return_value_policy< bp::copy_const_reference >());

    img.def("__call__"
            , (T & (ImageT::*)(size_t const, size_t const, size_t const))(&ImageT::operator())
            , (bp::arg("x"), bp::arg("y") = (unsigned int const) (0), bp::arg("z") = (unsigned int const) (0))
            , bp::return_value_policy< bp::copy_non_const_reference >());

    // Convenience Functions

    img.def("getSum", &ImageT::getSum);
    img.def("getMean", &ImageT::getMean);
}
template void exportPyXipImage<double>();
template void exportPyXipImage<float>();




