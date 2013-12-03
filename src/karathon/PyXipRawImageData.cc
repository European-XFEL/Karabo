/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>, <serguei.essenov@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/xip/RawImageData.hh>
#include "RawImageDataWrap.hh"
#include "Wrapper.hh"

namespace bp = boost::python;
using namespace karathon;
using namespace karabo::xip;
using namespace karabo::util;
using namespace std;

void exportPyXipRawImageData() {

    bp::enum_< karabo::xip::Encoding::EncodingType>("EncodingType")
            .value("UNDEFINED", karabo::xip::Encoding::UNDEFINED)
            .value("GRAY", karabo::xip::Encoding::GRAY)
            .value("RGB", karabo::xip::Encoding::RGB)
            .value("RGBA", karabo::xip::Encoding::RGBA)
            .value("BGR", karabo::xip::Encoding::BGR)
            .value("BGRA", karabo::xip::Encoding::BGRA)
            .value("CMYK", karabo::xip::Encoding::CMYK)
            .value("YUV", karabo::xip::Encoding::YUV)
            .value("BAYER", karabo::xip::Encoding::BAYER)
            .value("JPEG", karabo::xip::Encoding::JPEG)
            .value("PNG", karabo::xip::Encoding::PNG)
            .export_values()
            ;

    bp::enum_< karabo::xip::ChannelSpace::ChannelSpaceType>("ChannelSpaceType")
            .value("UNDEFINED", karabo::xip::ChannelSpace::UNDEFINED)
            .value("u_8_1", karabo::xip::ChannelSpace::u_8_1)
            .value("s_8_1", karabo::xip::ChannelSpace::s_8_1)
            .value("u_10_2", karabo::xip::ChannelSpace::u_10_2)
            .value("s_10_2", karabo::xip::ChannelSpace::s_10_2)
            .value("u_12_2", karabo::xip::ChannelSpace::u_12_2)
            .value("s_12_2", karabo::xip::ChannelSpace::s_12_2)
            .value("u_12_1p5", karabo::xip::ChannelSpace::u_12_1p5)
            .value("s_12_1p5", karabo::xip::ChannelSpace::s_12_1p5)
            .value("u_16_2", karabo::xip::ChannelSpace::u_16_2)
            .value("s_16_2", karabo::xip::ChannelSpace::s_16_2)
            .value("f_16_2", karabo::xip::ChannelSpace::f_16_2)
            .value("u_32_4", karabo::xip::ChannelSpace::u_32_4)
            .value("s_32_4", karabo::xip::ChannelSpace::s_32_4)
            .value("f_32_4", karabo::xip::ChannelSpace::f_32_4)
            .value("u_64_8", karabo::xip::ChannelSpace::u_64_8)
            .value("s_64_8", karabo::xip::ChannelSpace::s_64_8)
            .value("f_64_8", karabo::xip::ChannelSpace::f_64_8)
            .export_values()
            ;

    bp::enum_< karabo::xip::Endianness::EndiannessType>("EndiannessType")
            .value("UNDEFINED", karabo::xip::Endianness::UNDEFINED)
            .value("LSB", karabo::xip::Endianness::LSB)
            .value("MSB", karabo::xip::Endianness::MSB)
            .export_values()
            ;


    bp::class_< RawImageDataWrap, boost::shared_ptr<RawImageDataWrap>, boost::noncopyable >("RawImageData", bp::init<>())
#ifdef WITH_BOOST_NUMPY
            .def(bp::init < bp::object const&,
                 karabo::xip::Encoding::EncodingType const,
                 karabo::util::Hash const &, bool const >((
                 bp::arg("ndarray"),
                 bp::arg("encoding"),
                 bp::arg("header") = bp::object(karabo::util::Hash()),
                 bp::arg("isBigEndian") = false)))
#endif
            .def(bp::init < bp::object const&,
                 Dims const &,
                 karabo::xip::Encoding::EncodingType const,
                 karabo::xip::ChannelSpaceType const,
                 karabo::util::Hash const &, bool const>((bp::arg("bytearray"),
                 bp::arg("dimensions"),
                 bp::arg("encoding"),
                 bp::arg("channelSpace"),
                 bp::arg("header") = karabo::util::Hash(),
                 bp::arg("isBigEndian") = false)))
            .def(bp::init < Hash &, bool >((bp::arg("imageHash"), bp::arg("sharesData") = false)))
//            .def(bp::init< RawImageData const & >((bp::arg("image"))))
            .def("setData", &RawImageDataWrap::setData, bp::arg("data"))
            .def("getData", &RawImageDataWrap::getData)
            .def("allocateData", &RawImageDataWrap::allocateData, bp::arg("byteSize"))
            .def("size", &RawImageDataWrap::size)
            .def("getByteSize", &RawImageDataWrap::getByteSize)
            .def("setByteSize", &RawImageDataWrap::setByteSize, bp::arg("byteSize"))
            .def("getDimensions", &RawImageDataWrap::getDimensions)
            .def("setDimensions", &RawImageDataWrap::setDimensions)
            .def("getEncoding", &RawImageDataWrap::getEncoding)
            .def("setEncoding", &RawImageDataWrap::setEncoding)
            .def("getChannelSpace", &RawImageDataWrap::getChannelSpace)
            .def("setChannelSpace", &RawImageDataWrap::setChannelSpace)
            .def("setIsBigEndian", &RawImageDataWrap::setIsBigEndian, bp::arg("bigFlag"))
            .def("isBigEndian", &RawImageDataWrap::isBigEndian)
            .def("getHeader", &RawImageDataWrap::getHeader)
            .def("setHeader", &RawImageDataWrap::setHeader, bp::arg("header"))
            .def("toHash", &RawImageDataWrap::toHash, bp::return_internal_reference<> ()/*bp::return_value_policy< bp::copy_const_reference >()*/)
//            .def("swap", &RawImageDataWrap::swap, bp::arg("image"))
            .def("toRGBAPremultiplied", &RawImageDataWrap::toRGBAPremultiplied)
            ;
}
