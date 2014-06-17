/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>, <serguei.essenov@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/xip/RawImageData.hh>
#include <boost/python/make_constructor.hpp>
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


    bp::class_< RawImageData, boost::shared_ptr<RawImageData> >("RawImageData", bp::init<>())
            //            .def(bp::init < bp::object&,
            //                 const bool,
            //                 const karabo::xip::Encoding::EncodingType,
            //                 const bool >((
            //                 bp::arg("ndarray"),
            //                 bp::arg("copy") = true,
            //                 bp::arg("encoding") = karabo::xip::Encoding::GRAY,
            //                 bp::arg("isBigEndian") = karabo::util::isBigEndian()
            //                 )))
            //            .def(bp::init < bp::object&,
            //                 const Dims&,
            //                 const bool,
            //                 const karabo::xip::Encoding::EncodingType,
            //                 const karabo::xip::ChannelSpaceType,
            //                 const bool>((
            //                 bp::arg("bytearray"),
            //                 bp::arg("dimensions"),
            //                 bp::arg("copy") = true,
            //                 bp::arg("encoding") = karabo::xip::Encoding::GRAY,
            //                 bp::arg("channelSpace") = karabo::xip::ChannelSpace::UNDEFINED,
            //                 bp::arg("isBigEndian") = karabo::util::isBigEndian())))

            .def("__init__", bp::make_constructor(&RawImageDataWrap::make, bp::default_call_policies(),
                                                  (bp::arg("ndarray"),
                                                  bp::arg("copy") = true,
                                                  bp::arg("encoding") = karabo::xip::Encoding::GRAY,
                                                  bp::arg("isBigEndian") = karabo::util::isBigEndian())))
                                                  

            .def(bp::init < Hash &, bool >((bp::arg("imageHash"), bp::arg("copiesHash") = true)))
            //.def("setData", &RawImageDataWrap::setData, bp::arg("data"))
            .def("getData", &RawImageDataWrap::getDataPy)
            .def("getSize", &RawImageDataWrap::getSize)
            .def("getByteSize", &RawImageDataWrap::getByteSize)
            .def("getDimensions", &RawImageDataWrap::getDimensionsPy)
            .def("getEncoding", &RawImageDataWrap::getEncoding)
            .def("setEncoding", &RawImageDataWrap::setEncoding)
            .def("getChannelSpace", &RawImageDataWrap::getChannelSpace)
            .def("setIsBigEndian", &RawImageDataWrap::setIsBigEndian, bp::arg("bigFlag"))
            .def("isBigEndian", &RawImageDataWrap::isBigEndian)
            .def("getHeader", &RawImageDataWrap::getHeader)
            .def("setHeader", &RawImageDataWrap::setHeaderPy, bp::arg("header"))
            .def("hash", (const Hash & (RawImageData::*)() const) (&RawImageData::hash), bp::return_internal_reference<>())
            .def("toRGBAPremultiplied", &RawImageDataWrap::toRGBAPremultiplied)
            ;
}
