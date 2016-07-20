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
#include <boost/algorithm/string.hpp>

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
                std::string extension = m_filename.extension().string();
                boost::algorithm::to_lower(extension);

                const char* data = image.getDataPointer();
                karabo::util::Dims dims = image.getDimensions();
                size_t size = image.getByteSize();
                int encoding = image.getEncoding();
                int channelSpace = image.getChannelSpace();

                bool rawImageFile = false;
                karabo::util::Hash imageInfo;
                if (extension == ".raw" || extension == ".rgb") {
                    rawImageFile = true;
                    imageInfo += image.hash();
                    imageInfo.erase("data");
                }

#define _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding) { \
    FILE* stream = fmemopen((void*)data, size, "r"); /* Open memory as stream */ \
    \
    if (encoding==karabo::xip::Encoding::JPEG) { \
        cImg.load_jpeg(stream); \
    } else if (encoding==karabo::xip::Encoding::PNG) { \
        cImg.load_png(stream); \
    } else if (encoding==karabo::xip::Encoding::BMP) { \
        cImg.load_bmp(stream); \
    } else if (encoding==karabo::xip::Encoding::GRAY) {	\
        cImg.load_raw(stream, dims.x1(), dims.x2(), 1, 1); \
    } else if (encoding==karabo::xip::Encoding::RGB) { \
        cImg.load_rgb(stream, dims.x1(), dims.x2()); \
    } else if (encoding==karabo::xip::Encoding::RGBA) { \
        cImg.load_rgba(stream, dims.x1(), dims.x2()); \
    } else { \
        KARABO_NOT_SUPPORTED_EXCEPTION("RawImageFileWriter::write(const RawImageData&) is not supported yet" \
            "for encoding " + karabo::util::toString(encoding)); \
    } \
    \
    fclose(stream); \
}

#define _KARABO_XIP_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, filename) { \
    std::ofstream imageFile(filename.c_str(), std::ios::out); \
    \
    if (imageFile.is_open()) { \
        /* Write binary data to file */ \
        imageFile.write(data, size); \
        imageFile.close(); \
    } \
    if (rawImageFile) { \
        boost::filesystem::path infoFilename(filename); \
        std::ofstream infoFile(infoFilename.replace_extension(".info").c_str(), std::ios::out); \
         \
        if (infoFile.is_open()) { \
            /* Write image info to file */ \
            infoFile << imageInfo; \
            infoFile.close(); \
        } \
    } \
}

#define _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename) \
{ \
    if (rawImageFile) { \
        /* Output file will contain raw pixel values */ \
         \
        if (cImg.spectrum()==1) \
            /* Dump GRAYSCALE data */ \
            cImg.save_raw(filename.c_str()); \
        else if (cImg.spectrum()==3) \
            /* Dump RGB data */ \
            cImg.save_rgb(filename.c_str()); \
        else if (cImg.spectrum()==4) \
            /* Dump RGBA data */ \
            cImg.save_rgba(filename.c_str()); \
        else { \
	  KARABO_NOT_SUPPORTED_EXCEPTION("RawImageFileWriter::write(const RawImageData&) cannot write image, channel number  = " + karabo::util::toString(cImg.spectrum())); \
            return; \
        } \
        boost::filesystem::path infoFilename(filename); \
        std::ofstream infoFile(infoFilename.replace_extension(".info").c_str(), std::ios::out); \
         \
        if (infoFile.is_open()) { \
            /* Write image info to file */ \
            infoFile << imageInfo; \
            infoFile.close(); \
        } \
         \
    } else { \
        /* Let CImg write image file according to extension */ \
        cImg.save(filename.c_str()); \
    } \
 }

#define _KARABO_XIP_CREATE_CIMG_WRITE_TO_FILE(data, dims, size, encoding, channelSpace, imageInfo, rawImageFile, filename) \
{ \
    switch(channelSpace) { \
        case karabo::xip::ChannelSpace::u_8_1: \
        { \
            cimg_library::CImg<unsigned char> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        case karabo::xip::ChannelSpace::s_8_1: \
        { \
            cimg_library::CImg<signed char> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        case karabo::xip::ChannelSpace::u_10_2: \
        case karabo::xip::ChannelSpace::u_12_2: \
        case karabo::xip::ChannelSpace::u_16_2: \
        { \
            cimg_library::CImg<unsigned short> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        case karabo::xip::ChannelSpace::s_10_2: \
        case karabo::xip::ChannelSpace::s_12_2: \
        case karabo::xip::ChannelSpace::s_16_2: \
        { \
            cimg_library::CImg<short> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        case karabo::xip::ChannelSpace::u_32_4: \
        { \
            cimg_library::CImg<unsigned int> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        case karabo::xip::ChannelSpace::s_32_4: \
        { \
            cimg_library::CImg<int> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        case karabo::xip::ChannelSpace::u_64_8: \
        { \
            cimg_library::CImg<unsigned long long> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        case karabo::xip::ChannelSpace::s_64_8: \
        { \
            cimg_library::CImg<long long> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        case karabo::xip::ChannelSpace::f_32_4: \
        { \
            cimg_library::CImg<float> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        case karabo::xip::ChannelSpace::f_64_8: \
        { \
            cimg_library::CImg<double> cImg; \
            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding); \
            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, filename); \
            break; \
        } \
        default: \
            break; \
    } \
} \

                switch (encoding) {
                    case karabo::xip::Encoding::GRAY:
                        if (extension == ".raw") {
                            _KARABO_XIP_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        } else {
                            _KARABO_XIP_CREATE_CIMG_WRITE_TO_FILE(data, dims, size, encoding, channelSpace, imageInfo, rawImageFile, m_filename);
                        }
                        break;

                    case karabo::xip::Encoding::RGB:
                        if (extension == ".raw" || extension == ".rgb") {
                            _KARABO_XIP_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        } else {
                            _KARABO_XIP_CREATE_CIMG_WRITE_TO_FILE(data, dims, size, encoding, channelSpace, imageInfo, rawImageFile, m_filename);
                        }
                        break;

                    case karabo::xip::Encoding::RGBA:
                        if (extension == ".raw" || extension == ".rgba") {
                            _KARABO_XIP_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        } else {
                            _KARABO_XIP_CREATE_CIMG_WRITE_TO_FILE(data, dims, size, encoding, channelSpace, imageInfo, rawImageFile, m_filename);
                        }
                        break;

                        //                    case karabo::xip::Encoding::BGR: // TODO

                        //                    case karabo::xip::Encoding::BGRA: // TODO

                    case karabo::xip::Encoding::JPEG:
                        if (extension == ".jpg" || extension == ".jpeg") {
                            _KARABO_XIP_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        } else {
                            cimg_library::CImg<unsigned char> cImg;
                            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding);
                            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, m_filename);
                        }
                        break;

                    case karabo::xip::Encoding::PNG:
                        if (extension == ".png") {
                            _KARABO_XIP_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        } else {
                            cimg_library::CImg<unsigned char> cImg;
                            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding);
                            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, m_filename);
                        }
                        break;

                    case karabo::xip::Encoding::BMP:
                        if (extension == ".bmp") {
                            _KARABO_XIP_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        } else {
                            cimg_library::CImg<unsigned char> cImg;
                            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding);
                            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, m_filename);
                        }
                        break;

                    case karabo::xip::Encoding::TIFF:
                        if (extension == ".tif" || extension == ".tiff") {
                            _KARABO_XIP_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        } else {
                            cimg_library::CImg<unsigned char> cImg;
                            _KARABO_XIP_LOAD_DATA_TO_CIMG(cImg, data, dims, size, encoding);
                            _KARABO_XIP_WRITE_CIMG_TO_FILE(cImg, imageInfo, rawImageFile, m_filename);
                        }
                        break;

                    default:
                        KARABO_NOT_SUPPORTED_EXCEPTION("RawImageFileWriter::write(const RawImageData&) is not"
                                                       " supported yet for encoding " + karabo::util::toString(encoding));
                        break;
                }

            }

        };
    } /* namespace xip */
} /* namespace karabo */

#endif /* KARABO_XIP_RAWIMAGEWRITER_HH */
