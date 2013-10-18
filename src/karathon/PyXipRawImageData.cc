/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
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
      
    
    bp::class_< RawImageData, boost::shared_ptr<RawImageData> > r("RawImageData");
   
        r.def(bp::init< size_t const, 
                        Dims const &, 
                        karabo::xip::Encoding::EncodingType const, 
                        karabo::xip::ChannelSpace::ChannelSpaceType const, 
                        bp::optional< Hash const &, bool const> >(( bp::arg("byteSize"), 
                        bp::arg("dimensions"), 
                        bp::arg("encoding"),
                        bp::arg("channelSpace"), 
                        bp::arg("header")=karabo::util::Hash(),
                        bp::arg("isBigEndian")=(bool const)(karabo::util::isBigEndian()) )) );  
        
        r.def(bp::init< Hash &, bp::optional< bool > >(( bp::arg("imageHash"), bp::arg("sharesData")=(bool)(false) )) );    
        
        r.def(bp::init< RawImageData const & >(( bp::arg("image") )) );   
        
        r.def("setData"
            , (void (*)(RawImageData&, const bp::object &))(&RawImageDataWrap::setData)
            , bp::arg("data"));
        
        r.def("getData", &RawImageDataWrap::getData);
            
        r.def("allocateData"
            , (void (RawImageData::*)(size_t const) )(&RawImageData::allocateData)
            , bp::arg("byteSize") ); 
        
        r.def("size"
            , (size_t (RawImageData::*)() const)(&RawImageData::size));
        
        r.def("getByteSize"
            , (size_t (RawImageData::*)() const)(&RawImageData::getByteSize));       
        
        r.def("setByteSize"
            , (void (RawImageData::*)(size_t const &))(&RawImageData::setByteSize)
            , bp::arg("byteSize") );

        r.def("getDimensions", &RawImageDataWrap::getDimensions);
        
        r.def("setDimensions", &RawImageDataWrap::setDimensions);
        
        r.def("getEncoding"
            , (int (RawImageData::*)() const)(&RawImageData::getEncoding));
        
        r.def("setEncoding", &RawImageDataWrap::setEncoding);
        
        r.def("getChannelSpace"
            , (int (RawImageData::*)() const)(&RawImageData::getChannelSpace));
        
        r.def("setChannelSpace", &RawImageDataWrap::setChannelSpace);
        
        r.def("setIsBigEndian", &RawImageDataWrap::setIsBigEndian);
        
        r.def("isBigEndian"
            , (bool (RawImageData::*)() const)(&RawImageData::isBigEndian));
        
        r.def("getHeader"
            , (Hash (RawImageData::*)() const)(&RawImageData::getHeader));
        
        r.def("setHeader",&RawImageDataWrap::setHeader);  
        
        r.def("toHash"
            , (Hash const & (RawImageData::*)() const)(&RawImageData::toHash)
            , bp::return_value_policy< bp::copy_const_reference >() );
        
        r.def("swap"
            , (void (RawImageData::*)(RawImageData &) )( &RawImageData::swap)
            , bp::arg("image") );
        
}
