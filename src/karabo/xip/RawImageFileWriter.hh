/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_RAWIMAGEWRITER_HH
#define	KARABO_XIP_RAWIMAGEWRITER_HH

#include <boost/filesystem.hpp>

#include <karabo/io/Output.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/xip/CpuImage.hh>
#include <karabo/xip/FromChannelSpace.hh>

#include "RawImageData.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package xip
     */
    namespace xip {

        class RawImageFileWriter : public karabo::io::Output< RawImageData > {

            karabo::util::Hash m_input;
            boost::filesystem::path m_filename;
            int m_number;
            
        public:

            KARABO_CLASSINFO(RawImageFileWriter, "RawImageFile", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Master)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
                using namespace karabo::util;

                PATH_ELEMENT(expected)
                        .key("filename")
                        .description("Name of the file to be written")
                        .displayedName("Filename")
                        .isOutputFile()
                        .assignmentMandatory()
                        .commit();
            }

            RawImageFileWriter(const karabo::util::Hash& config) : karabo::io::Output< RawImageData >(config) {
                m_input = config;
                m_filename = config.get<std::string>("filename");
                m_number = -1;
                if (this->m_appendModeEnabled) {
                    m_number = 0;
                }
            }

            virtual ~RawImageFileWriter() {
            };

            void write(const RawImageData& image) {
                
                bool writeImageInfo;
                karabo::util::Dims dims = image.getDimensions();
                karabo::util::Types::ReferenceType imType = karabo::xip::FromChannelSpace::from(image.getChannelSpace());
                size_t size = image.getByteSize();
                const char* data = image.getDataPointer();
                
                if (m_filename.extension().string()==".raw" || m_filename.extension().string()==".RAW")
                    // In case of output as "raw" data, image info will be written to additional file
                    writeImageInfo = true;
                else
                    writeImageInfo = false;
                
                #define _KARABO_CREATE_AND_WRITE_CPUIMAGE(inType, castType) \
                    case karabo::util::Types::inType: \
                    { \
                        karabo::xip::CpuImage<castType> cpuImage; \
                        cpuImage.assign(dims.toVector()[0], dims.toVector()[1], dims.toVector()[2], (const castType)(data[0])); \
                        cpuImage.write(m_filename.string()); \
                        break; \
                    }
                
                switch(imType) {
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(CHAR, char);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(INT8, signed char);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(UINT8, unsigned char);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(INT16, short);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(UINT16, unsigned short);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(INT32, int);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(UINT32, unsigned int);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(INT64, long long);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(UINT64, unsigned long long);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(FLOAT, float);
                    _KARABO_CREATE_AND_WRITE_CPUIMAGE(DOUBLE, double);
                    default:
                        break;
                }
                
                if (writeImageInfo) {
                    std::ofstream infoFile(m_filename.replace_extension(".info").c_str(), std::ios::out);
                    karabo::util::Hash imageInfo(image.hash());
                    imageInfo.erase("data");
                    
                    if (infoFile.is_open()) {
                        infoFile << imageInfo;
                        infoFile.close();
                    }
                }
                
            }
        };
    } /* namespace xip */
} /* namespace karabo */

#endif /* KARABO_XIP_RAWIMAGEWRITER_HH */
