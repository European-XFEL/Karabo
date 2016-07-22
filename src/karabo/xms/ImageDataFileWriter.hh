/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XMS_IMAGEDATAFILEWRITER_HH
#define	KARABO_XMS_IMAGEDATAFILEWRITER_HH

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <karabo/io/Output.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/xip/FromChannelSpace.hh>

#include "ImageData.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package xms
     */
    namespace xms {

        class ImageDataFileWriter : public karabo::io::Output< ImageData > {


            karabo::util::Hash m_input;
            boost::filesystem::path m_filename;
            int m_number;

        public:

            KARABO_CLASSINFO(ImageDataFileWriter, "ImageDataFileWriter", "1.0")

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

            ImageDataFileWriter(const karabo::util::Hash& config) : karabo::io::Output< ImageData >(config) {
                m_input = config;
                m_filename = config.get<std::string>("filename");
                m_number = -1;
                if (this->m_appendModeEnabled) {
                    m_number = 0;
                }
            }

            virtual ~ImageDataFileWriter() {
            };

            void write(const ImageData& image) {

#define _KARABO_XMS_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, filename) { \
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

                std::string extension = m_filename.extension().string();
                boost::algorithm::to_lower(extension);

                const karabo::util::NDArray<unsigned char>& dataArray = image.getData();
                const char* data = reinterpret_cast<char*>(&(*dataArray.getData())[0]);
                karabo::util::Dims dims = image.getDimensions();
                size_t size = image.getByteSize();
                int encoding = image.getEncoding();

                bool rawImageFile = false;
                karabo::util::Hash imageInfo;
                if (extension == ".raw" || extension == ".rgb" || extension == ".rgba") {
                    rawImageFile = true;
                    imageInfo += *(image.hash());
                    imageInfo.erase("data");
                }

                switch (encoding) {
                    case Encoding::GRAY:
                    case Encoding::RGB:
                    case Encoding::RGBA:
                        if (!(extension == ".raw" || extension == ".rgba" || extension == ".rgb")) {
                            KARABO_NOT_SUPPORTED_EXCEPTION("ImageDataFileWriter::write(const ImageData&) : File extension \"" + extension +
                                                           "\" must match encoding \"" + karabo::util::toString(encoding) + "\"");
                        }
                        _KARABO_XMS_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        break;

                    // case Encoding::BGR: // TODO
                    // case Encoding::BGRA: // TODO

                    case Encoding::JPEG:
                        if (extension != ".jpg" && extension != ".jpeg") {
                            KARABO_NOT_SUPPORTED_EXCEPTION("ImageDataFileWriter::write(const ImageData&) : File extension \"" + extension +
                                                           "\" must match encoding \"" + karabo::util::toString(encoding) + "\"");
                        }
                        _KARABO_XMS_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        break;

                    case Encoding::PNG:
                        if (extension != ".png") {
                            KARABO_NOT_SUPPORTED_EXCEPTION("ImageDataFileWriter::write(const ImageData&) : File extension \"" + extension +
                                                           "\" must match encoding \"" + karabo::util::toString(encoding) + "\"");
                        }
                        _KARABO_XMS_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        break;

                    case Encoding::BMP:
                        if (extension != ".bmp") {
                            KARABO_NOT_SUPPORTED_EXCEPTION("ImageDataFileWriter::write(const ImageData&) : File extension \"" + extension +
                                                           "\" must match encoding \"" + karabo::util::toString(encoding) + "\"");
                        }
                        _KARABO_XMS_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        break;

                    case Encoding::TIFF:
                        if (extension != ".tif" && extension != ".tiff") {
                            KARABO_NOT_SUPPORTED_EXCEPTION("ImageDataFileWriter::write(const ImageData&) : File extension \"" + extension +
                                                           "\" must match encoding \"" + karabo::util::toString(encoding) + "\"");
                        }
                        _KARABO_XMS_WRITE_DATA_TO_FILE(data, size, imageInfo, rawImageFile, m_filename);
                        break;

                    default:
                        KARABO_NOT_SUPPORTED_EXCEPTION("ImageDataFileWriter::write(const ImageData&) is not"
                                                        " supported yet for encoding " + karabo::util::toString(encoding));
                        break;
                }

            }

        };
    } /* namespace xms */
} /* namespace karabo */

#endif /* KARABO_XMS_IMAGEDATAFILEWRITER_HH */
