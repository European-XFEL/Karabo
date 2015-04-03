/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 10, 2012, 6:24 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_CPUIMAGEBINARYSERIALIZER_HH
#define	KARABO_XIP_CPUIMAGEBINARYSERIALIZER_HH

#include <karabo/io/BinarySerializer.hh>

#include "CpuImage.hh"
/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xip {

        /**
         * The CpuImageBinarySerializer class.
         */
        template <class TPix>
        class CpuImageBinarySerializer : public karabo::io::BinarySerializer<CpuImage<TPix> > {

            typedef karabo::io::BinarySerializer<RawImageData> RawImageDataSerializer;

        public:

            KARABO_CLASSINFO(CpuImageBinarySerializer<TPix>, "Bin", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
            }

            CpuImageBinarySerializer(const karabo::util::Hash& input) {
                m_rawImageDataSerializer = RawImageDataSerializer::create("Bin", karabo::util::Hash(), false); // No validation for speed
            }

            void save(const CpuImage<TPix>& image, std::vector<char>& archive) {

                RawImageData raw(image.pixelPointer(), image.size(), false, image.dims());
                raw.setHeader(image.getHeader());
                m_rawImageDataSerializer->save(raw, archive);

//                const karabo::util::Hash& header = image.getHeader();
//
//                std::vector<char> hashArchive;
//                m_hashSerializer->save(header, hashArchive);
//
//                unsigned int headerByteSize = hashArchive.size();
//                //std::cout << "HeaderByteSize: " << headerByteSize << std::endl;
//
//                unsigned int imageByteSize = image.byteSize();
//                //std::cout << "ImageByteSize: " << imageByteSize << std::endl;
//
//                size_t sizeTagByteSize = sizeof (unsigned int);
//                //std::cout << "SizeTagSize: " << sizeTagByteSize << std::endl;
//
//                archive.resize(sizeTagByteSize + headerByteSize + imageByteSize);
//                std::memcpy(&archive[0], reinterpret_cast<char*> (&headerByteSize), sizeTagByteSize);
//                std::memcpy(&archive[sizeTagByteSize], &hashArchive[0], headerByteSize);
//                std::memcpy(&archive[sizeTagByteSize + headerByteSize], image.pixelPointer(), imageByteSize);
            }

            void load(CpuImage<TPix>& image, const char* archive, const size_t nBytes) {

                // TODO Check correct TPix
                RawImageData raw;
                m_rawImageDataSerializer->load(raw, archive, nBytes);
                const karabo::util::Dims& dims = raw.getDimensions();
                CpuImage<TPix> tmp(dims.x1(), dims.x2(), dims.x3());
                std::memcpy(tmp.pixelPointer(), raw.getDataPointer(), raw.getByteSize());
                tmp.setHeader(raw.getHeader());
                tmp.swap(image);
//
//                size_t sizeTagByteSize = sizeof (unsigned int);
//                //std::cout << "SizeTagSize: " << sizeTagByteSize << std::endl;
//
//                unsigned int headerByteSize;
//                std::memcpy(&headerByteSize, const_cast<char*> (archive), sizeTagByteSize);
//                //std::cout << "HeaderByteSize: " << headerByteSize << std::endl;
//
//                unsigned int imageByteSize = nBytes - sizeTagByteSize - headerByteSize;
//                //std::cout << "ImageByteSize: " << imageByteSize << std::endl;
//
//                karabo::util::Hash header;
//                m_rawImageDataSerializer->load(header, archive + sizeTagByteSize, headerByteSize);
//                CpuImage<TPix> tmp(header);
//                std::memcpy(tmp.pixelPointer(), const_cast<char*> (archive + sizeTagByteSize + headerByteSize), imageByteSize);
//                tmp.swap(image);
            }

            /**
             * Destructor.
             */
            virtual ~CpuImageBinarySerializer() {
            };


        private: // members

            RawImageDataSerializer::Pointer m_rawImageDataSerializer;


        private: // functions

        };

    }
}

KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::xip::CpuImage<float> >)
KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::xip::CpuImage<double> >)
KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::xip::CpuImage<unsigned int> >)
KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::xip::CpuImage<unsigned short> >)
KARABO_REGISTER_CONFIGURATION_BASE_CLASS(karabo::io::BinarySerializer<karabo::xip::CpuImage<unsigned char> >)

#endif
