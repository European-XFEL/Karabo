/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_RAWIMAGEWRITER_HH
#define	KARABO_XIP_RAWIMAGEWRITER_HH

#include "CImg.h"

#include <boost/filesystem.hpp>

#include <karabo/io/Output.hh>
#include <karabo/util/PathElement.hh>
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
                const char* data = image.getDataPointer();
                int encoding = image.getEncoding();
                size_t size = image.getByteSize();
                std::string extension = m_filename.extension().string();
                
                if (encoding == karabo::xip::Encoding::JPEG || encoding == karabo::xip::Encoding::PNG ||
                        encoding == karabo::xip::Encoding::BMP || encoding == karabo::xip::Encoding::TIFF) {
                    // RawImageData contains image as JPEG, PNG, BMP or TIFF

                    bool convertThenWrite = true; // Assume by default that image needs format conversion
                    cimg_library::CImg<unsigned char> cImg;
                    
                    if (encoding == karabo::xip::Encoding::JPEG) {
                        // RawImageData is JPEG image

                        if (extension==".jpg" || extension==".JPG" || extension==".jpeg" || extension==".JPEG") {
                            // Target file is also JPEG -> write data to file _directly_
                            convertThenWrite = false;
                        } else {
                            FILE* stream = fmemopen((void*)data, size, "r");
                            cImg.load_jpeg(stream);
                        }

                    } else if (encoding == karabo::xip::Encoding::PNG) {
                        // RawImageData is PNG image

                        if (extension==".png" || extension==".PNG") {
                            // Target file is also PNG -> write data to file _directly_
                            convertThenWrite = false;
                        } else {
                            FILE* stream = fmemopen((void*)data, size, "r");
                            cImg.load_png(stream);
                        }
                    } else if (encoding == karabo::xip::Encoding::BMP) {
                        // RawImageData is BMP image

                        if (extension==".bmp" || extension==".BMP") {
                            // Target file is also BMP -> write data to file _directly_
                            convertThenWrite = false;
                        } else {
                            FILE* stream = fmemopen((void*)data, size, "r");
                            cImg.load_bmp(stream);
                        }
                    } else {
                        // RawImageData is TIFF image
                        
                        if (extension==".tif" || extension==".TIF" || extension==".tiff" || extension==".TIFF") {
                            // Target file is also TIFF -> write data to file _directly_
                            convertThenWrite = false;
                        } else {
                            KARABO_NOT_SUPPORTED_EXCEPTION("RawImageFileWriter::write(const RawImageData&) not support yet TIFF conversion");
                        }
                        
                    }
                    
                    if (convertThenWrite) {
                        cImg.save(m_filename.c_str());
                    } else {
                        std::ofstream imageFile(m_filename.c_str(), std::ios::out);

                        if (imageFile.is_open()) {
                            imageFile.write(data, size);
                            imageFile.close();
                        }
                        
                    }
                    
                } else if (encoding == karabo::xip::Encoding::GRAY ||
                        encoding == karabo::xip::Encoding::RGB || encoding == karabo::xip::Encoding::RGBA ||
                        encoding == karabo::xip::Encoding::BGR || encoding == karabo::xip::Encoding::BGRA) {
                    
                    karabo::util::Dims dims = image.getDimensions();
                    karabo::util::Types::ReferenceType imType = karabo::xip::FromChannelSpace::from(image.getChannelSpace());

                    #define _KARABO_CREATE_AND_WRITE_CIMG(inType, castType) \
                        case karabo::util::Types::inType: \
                        { \
                            cimg_library::CImg<castType> cImg; \
                            cImg.assign(dims.toVector()[0], dims.toVector()[1], 1, dims.toVector()[2], (const castType)(data[0])); \
                            cImg.save(m_filename.string().c_str()); \
                            break; \
                        }
                    
                    switch(imType) {
                        _KARABO_CREATE_AND_WRITE_CIMG(CHAR, char);
                        _KARABO_CREATE_AND_WRITE_CIMG(INT8, signed char);
                        _KARABO_CREATE_AND_WRITE_CIMG(UINT8, unsigned char);
                        _KARABO_CREATE_AND_WRITE_CIMG(INT16, short);
                        _KARABO_CREATE_AND_WRITE_CIMG(UINT16, unsigned short);
                        _KARABO_CREATE_AND_WRITE_CIMG(INT32, int);
                        _KARABO_CREATE_AND_WRITE_CIMG(UINT32, unsigned int);
                        _KARABO_CREATE_AND_WRITE_CIMG(INT64, long long);
                        _KARABO_CREATE_AND_WRITE_CIMG(UINT64, unsigned long long);
                        _KARABO_CREATE_AND_WRITE_CIMG(FLOAT, float);
                        _KARABO_CREATE_AND_WRITE_CIMG(DOUBLE, double);
                        default:
                            break;
                    }
                
                } else {
                    KARABO_NOT_SUPPORTED_EXCEPTION("RawImageFileWriter::write(const RawImageData&) is not supported yet"
                            "for this encoding.");
                }
                
                if (extension==".raw" || extension==".RAW")
                    // In case of output as "raw" data, image info will be written to additional file
                    writeImageInfo = true;
                else
                    writeImageInfo = false;
                
                if (writeImageInfo) {
                    boost::filesystem::path infoFilename(m_filename);
                    std::ofstream infoFile(infoFilename.replace_extension(".info").c_str(), std::ios::out);
                    karabo::util::Hash imageInfo(image.hash());
                    imageInfo.erase("data"); // image itself will not be written to info file
                    
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
