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


        KARABO_REGISTER_FOR_CONFIGURATION(Data, ImageData)

        void ImageData::expectedParameters(::Schema& s) {

            NDARRAY_UINT8_ELEMENT(s).key("data")
                    .displayedName("Data")
                    .description("Pixel array")
                    .readOnly()
                    .commit();
            VECTOR_UINT32_ELEMENT(s).key("dims")
                    .displayedName("Dimensions")
                    .description("The length of the array reflects total dimensionality and each element the extension in this dimension")
                    .readOnly()
                    .commit();
            VECTOR_INT32_ELEMENT(s).key("dimTypes")
                    .displayedName("Dimension Types")
                    .description("Any dimension should have an enumerated type")
                    .readOnly()
                    .commit();
            STRING_ELEMENT(s).key("dimScales")
                    .displayedName("Dimension Scales")
                    .description("")
                    .readOnly()
                    .commit();
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
            INT32_ELEMENT(s).key("bitsPerPixel")
                    .displayedName("Bits per pixel")
                    .description("The number of bits needed for each pixel")
                    .readOnly()
                    .commit();
            BOOL_ELEMENT(s).key("isBigEndian")
                    .displayedName("Is big endian")
                    .description("Flags whether the raw data are in big or little endian")
                    .readOnly()
                    .commit();
            NODE_ELEMENT(s).key("geometry")
                    .displayedName("Geometry")
                    .commit();
            NODE_ELEMENT(s).key("header")
                    .displayedName("Hash containing user-defined header data")
                    .commit();
        }


        ImageData::ImageData() {
        }

        ImageData::ImageData(const unsigned char* const data,
                             const size_t size,
                             const bool copy,
                             const Dims& dims,
                             const EncodingType encoding,
                             const int bitsPerPixel) : Data() {

            setData(data, size, copy);
            setDimensions(dims);
            if (dims.size() == 0) {
                setROIOffsets(Dims(0));
            } else {
                std::vector<unsigned long long> offsets(dims.rank(), 0);
                setROIOffsets(Dims(offsets));
            }
            setEncoding(encoding);
            setBitsPerPixel(bitsPerPixel);
        }


        ImageData::ImageData(const Hash& hash) : Data(hash) {
        }


        ImageData::ImageData(const Hash::Pointer& data) : Data(data) {
        }


        ImageData::~ImageData() {
        }


        const NDArray<unsigned char>& ImageData::getData() const {
            return m_hash->get<NDArray<unsigned char> >("data");
        }


        void ImageData::setData(const unsigned char* data, const size_t size, const bool copy) {
            if (copy) {
                NDArray<unsigned char> array(data, size, Dims(size));
                setData(array);
            }
            else {
                boost::shared_ptr<ArrayData<unsigned char> > arrayPtr(new ArrayData<unsigned char>(data, size, &ImageData::deallocateNonCopied));
                NDArray<unsigned char> array(data, size, Dims(size));
                setData(array);
            }
        }


        void ImageData::setData(const karabo::util::NDArray<unsigned char>& array) {
            m_hash->set<NDArray<unsigned char> >("data", array);
        }


        Dims ImageData::getROIOffsets() const {
            return Dims(m_hash->get<std::vector<unsigned long long> >("roiOffsets"));
        }


        void ImageData::setROIOffsets(const Dims& offsets) {
            m_hash->set<std::vector<unsigned long long> >("roiOffsets", offsets.toVector());
        }


        int ImageData::getBitsPerPixel() const {
            return m_hash->get<int>("bitsPerPixel");
        }


        void ImageData::setBitsPerPixel(const int bitsPerPixel) {
            m_hash->set<int>("bitsPerPixel", bitsPerPixel);
        }


        int ImageData::getEncoding() const {
            return m_hash->get<int>("encoding");
        }


        void ImageData::setEncoding(const int encoding) {
            m_hash->set<int>("encoding", encoding);
        }


        void ImageData::swapEndianess() {
            int bpp = getBitsPerPixel();
            NDArray<unsigned char>& dataArray = m_hash->get<NDArray<unsigned char> >("data");

            switch (bpp) {
                case 8:
                    // No swap needed.
                    break;
                case 16:
                {
                    unsigned short* data = reinterpret_cast<unsigned short*> (&(*dataArray.getData())[0]);
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = byteSwap16(data[i]);
                    }
                    break;
                }
                case 32:
                {
                    unsigned int* data = reinterpret_cast<unsigned int*> (&(*dataArray.getData())[0]);
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = byteSwap32(data[i]);
                    }
                    break;
                }
                case 64:
                {
                    unsigned long long* data = reinterpret_cast<unsigned long long*> (&(*dataArray.getData())[0]);
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = byteSwap64(data[i]);
                    }
                    break;
                }
                default:
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Endianess conversion not implemented for this channel type");
            }
        }

        void ImageData::deallocateNonCopied(unsigned char *) {
            // Do nothing.
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


        bool ImageData::isBigEndian() const {
            return m_hash->get<bool>("isBigEndian");
        }


        void ImageData::setIsBigEndian(const bool isBigEndian) {
            m_hash->set<bool>("isBigEndian", isBigEndian);
        }


        size_t ImageData::getByteSize() const {
            const NDArray<unsigned char>& data = m_hash->get<NDArray<unsigned char> >("data");
            const Dims& shape = data.getShape();
            size_t size = 1;
            for (int idx = 0; idx < shape.rank(); ++idx) {
                size *= static_cast<size_t>(shape.extentIn(idx));
            }
            return size;
        }


        Dims ImageData::getDimensions() const {
            return Dims(m_hash->get<std::vector<unsigned long long> >("dims"));
        }


        void ImageData::setDimensions(const Dims& dims) {
            // XXX: Make sure dimensions match the size of the data!
            m_hash->set<std::vector<unsigned long long> >("dims", dims.toVector());
            // In case the dimensionTypes were not yet set, inject a default here
            if (!m_hash->has("dimTypes")) {
                setDimensionTypes(vector<int>(dims.rank(), Dimension::UNDEFINED));
            }
        }


        const std::vector<int> ImageData::getDimensionTypes() const {
            return m_hash->get<std::vector<int> >("dimTypes");
        }


        void ImageData::setDimensionTypes(const std::vector<int>& dimTypes) {
            m_hash->set<std::vector<int> >("dimTypes", dimTypes);
        }


        const std::string& ImageData::getDimensionScales() const {
            return m_hash->get<string>("dimScales");
        }


        void ImageData::setDimensionScales(const std::string& scales) {
            m_hash->set("dimScales", scales);
        }


        const ImageData& ImageData::write(const std::string& filename, const bool enableAppendMode) const {
            Hash h("ImageDataFileWriter.filename", filename, "ImageDataFileWriter.enableAppendMode", enableAppendMode);
            karabo::io::Output<ImageData >::Pointer out = karabo::io::Output<ImageData >::create(h);
            out->write(*this);
            return *this;
        }


        DetectorGeometry ImageData::getGeometry() {
            return DetectorGeometry(m_hash->get<Hash>("detectorGeometry"));
        }


        void ImageData::setGeometry(const DetectorGeometry & geometry) {
            m_hash->set("detectorGeometry", geometry.toHash());
        }


        const Hash& ImageData::getHeader() const {
            return m_hash->get<Hash>("header");
        }


        void ImageData::setHeader(const Hash & header) {
            m_hash->set("header", header);
        }

    }
}
