/* 
 * File:   ImageData.cc
 * Author: 
 * 
 * Created on April 20, 2015, 19:35 PM
 */

#include "ImageData.hh"
#include "karabo/io/HashXmlSerializer.hh"
#include "karabo/io/FileTools.hh"
#include <karabo/util/ByteSwap.hh>
#include <boost/filesystem/operations.hpp>

using namespace karabo::util;

namespace karabo {
    namespace xms {


        KARABO_REGISTER_FOR_CONFIGURATION(Data, NDArray, ImageData)

        void ImageData::expectedParameters(karabo::util::Schema& s) {

            VECTOR_UINT32_ELEMENT(s).key("roiOffsets")
                    .displayedName("ROI Offsets")
                    .description("Describes the offset of the Region-of-Interest; it will contain zeros if the image has no ROI defined")
                    .readOnly()
                    .commit();
            INT32_ELEMENT(s).key("encoding")
                    .displayedName("Encoding")
                    .description("Describes the color space of pixel encoding of the data (e.g. GRAY, RGB, JPG, PNG etc.")
                    .readOnly()
                    .commit();
            INT32_ELEMENT(s).key("channelSpace")
                    .displayedName("Channel space")
                    .description("Describes the channel encoding, i.e. signed/unsigned/floating point, bits per channel and bytes per pixel")
                    .readOnly()
                    .commit();
        }


        ImageData::ImageData() {
        }


        ImageData::ImageData(const karabo::util::Hash& hash) : NDArray(hash) {
        }


        ImageData::ImageData(const karabo::util::Hash::Pointer& data) : NDArray(data) {
        }


        ImageData::~ImageData() {
        }


        karabo::util::Dims ImageData::getROIOffsets() const {
            return karabo::util::Dims(m_hash->get<std::vector<unsigned long long> >("roiOffsets"));
        }


        void ImageData::setROIOffsets(const karabo::util::Dims& offsets) {
            m_hash->set<std::vector<unsigned long long> >("roiOffsets", offsets.toVector());
        }


        int ImageData::getEncoding() const {
            return m_hash->get<int>("encoding");
        }


        void ImageData::setEncoding(const int encoding) {
            m_hash->set<int>("encoding", encoding);
        }


        int ImageData::getChannelSpace() const {
            return m_hash->get<int>("channelSpace");
        }


        void ImageData::setChannelSpace(const int channelSpace) {
            m_hash->set<int>("channelSpace", channelSpace);
        }


        void ImageData::swapEndianess() {
            ensureDataOwnership();
            int cs = getChannelSpace();
            switch (cs) {
                case ChannelSpace::u_16_2:
                case ChannelSpace::s_16_2:
                {
                    unsigned short* data = reinterpret_cast<unsigned short*> (const_cast<char*> (getDataPointer()));
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = byteSwap16(data[i]);
                    }
                    break;
                }
                case ChannelSpace::u_32_4:
                case ChannelSpace::s_32_4:
                case ChannelSpace::f_32_4:
                {
                    unsigned int* data = reinterpret_cast<unsigned int*> (const_cast<char*> (getDataPointer()));
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = byteSwap32(data[i]);
                    }
                    break;
                }
                case ChannelSpace::u_64_8:
                case ChannelSpace::s_64_8:
                case ChannelSpace::f_64_8:
                {
                    unsigned long long* data = reinterpret_cast<unsigned long long*> (const_cast<char*> (getDataPointer()));
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = byteSwap64(data[i]);
                    }
                    break;
                }
                default:
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Endianess conversion not implemented for this channel type");
            }
        }


        void ImageData::toLittleEndian() {
            if (isBigEndian()) {
                swapEndianess();
                setIsBigEndian(false);
            }
        }


        void ImageData::toBigEndian() {
            if (!isBigEndian()) {
                swapEndianess();
                setIsBigEndian(true);
            }
        }
        
        
        void ImageData::setDimensions(const karabo::util::Dims& dims) {
            Dims tmp(dims);
            tmp.reverse();
            NDArray::setDimensions(tmp);                    
        }


        //        const ImageData& ImageData::write(const std::string& filename, const bool enableAppendMode) const {
        //            karabo::util::Hash h("RawImageFile.filename", filename, "RawImageFile.enableAppendMode", enableAppendMode);
        //            karabo::io::Output<ImageData >::Pointer out = karabo::io::Output<ImageData >::create(h);
        //            out->write(*this);
        //            return *this;
        //        }

    }
}
