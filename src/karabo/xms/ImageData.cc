/* 
 * File:   ImageData.cc
 * Author: 
 * 
 * Created on April 20, 2015, 19:35 PM
 */

#include <karabo/util/VectorElement.hh>

#include "ImageData.hh"
#include "karabo/util/ToSize.hh"
#include "karabo/util/Types.hh"

namespace karabo {
    namespace xms {

        using namespace karabo::util;


        void ImageData::expectedParameters(karabo::util::Schema& s) {

            NDARRAY_ELEMENT(s).key("pixels")
                    .displayedName("Pixel Data")
                    .description("The N-dimensional array containing the pixels")
                    .readOnly()
                    .commit();

            VECTOR_UINT64_ELEMENT(s).key("dims")
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
            VECTOR_UINT64_ELEMENT(s).key("roiOffsets")
                    .displayedName("ROI Offsets")
                    .description("Describes the offset of the Region-of-Interest; it will contain zeros if the image has no ROI defined")
                    .readOnly()
                    .commit();
            // TODO Convert into a serializable object later
            // Will then read: GEOMETRY_ELEMENT(s).key("geometry") [...]
            NODE_ELEMENT(s).key("geometry")
                    .displayedName("Geometry")
                    .commit();
            NODE_ELEMENT(s).key("header")
                    .displayedName("Header")
                    .description("Hash containing user-defined header data")
                    .commit();
        }


        ImageData::ImageData() : ImageData(karabo::util::NDArray(karabo::util::Dims())) {
        }


        ImageData::ImageData(const karabo::util::NDArray& data,
                             const karabo::util::Dims& dims,
                             const EncodingType encoding,
                             const int bitsPerPixel) {

            setData(data);
            setDimensions(dims);
            setEncoding(encoding);
            int bitsPerPixel_intern = bitsPerPixel;
            if (bitsPerPixel_intern <= 0) {
                const size_t numBytes = karabo::util::Types::to<karabo::util::ToSize>(data.getType());
                bitsPerPixel_intern = numBytes * 8;
            }
            setBitsPerPixel(bitsPerPixel_intern);

            int rank = dims.rank();
            if (dims.size() == 0) {
                rank = data.getShape().rank();
            }
            std::vector<unsigned long long> offsets(rank, 0);
            setROIOffsets(karabo::util::Dims(offsets));

            setDimensionScales(std::string());
        }


        karabo::util::Dims ImageData::getROIOffsets() const {
            return karabo::util::Dims(get<std::vector<unsigned long long> >("roiOffsets"));
        }


        void ImageData::setROIOffsets(const karabo::util::Dims& offsets) {
            set<std::vector<unsigned long long> >("roiOffsets", offsets.toVector());
        }


        int ImageData::getBitsPerPixel() const {
            return get<int>("bitsPerPixel");
        }


        void ImageData::setBitsPerPixel(const int bitsPerPixel) {
            const size_t numBytes = karabo::util::Types::to<karabo::util::ToSize>(getData().getType());
            set<int>("bitsPerPixel", std::min<int>(bitsPerPixel, numBytes * 8));
        }


        int ImageData::getEncoding() const {
            return get<int>("encoding");
        }


        void ImageData::setEncoding(const int encoding) {
            set<int>("encoding", encoding);
        }


        karabo::util::Dims ImageData::getDimensions() const {
            return karabo::util::Dims(get<std::vector<unsigned long long> >("dims"));
        }


        void ImageData::setDimensions(const karabo::util::Dims& dims) {
            size_t rank = dims.rank();
            if (dims.size() == 0) {
                // Will use the shape information of underlying NDArray as best guess
                std::vector<unsigned long long> shape = get<NDArray>("pixels").getShape().toVector();
                set("dims", shape);
                rank = shape.size();
            } else {
                // Make sure dimensions match the size of the data
                get<NDArray>("pixels").setShape(dims); // throws if size does not fit
                set<std::vector<unsigned long long> >("dims", dims.toVector());
            }
            // In case the dimensionTypes were not yet set, inject a default here
            if (!has("dimTypes")) {
                setDimensionTypes(std::vector<int>(rank, Dimension::UNDEFINED));
            }
        }


        const std::vector<int> ImageData::getDimensionTypes() const {
            return get<std::vector<int> >("dimTypes");
        }


        void ImageData::setDimensionTypes(const std::vector<int>& dimTypes) {
            set<std::vector<int> >("dimTypes", dimTypes);
        }


        const std::string& ImageData::getDimensionScales() const {
            return get<std::string>("dimScales");
        }


        void ImageData::setDimensionScales(const std::string& scales) {
            set("dimScales", scales);
        }


        karabo::util::DetectorGeometry ImageData::getGeometry() const {
            boost::optional<const karabo::util::Hash::Node&> node = find("detectorGeometry");
            return (node ? karabo::util::DetectorGeometry(node->getValue<karabo::util::Hash>())
                    : karabo::util::DetectorGeometry());
        }


        void ImageData::setGeometry(const karabo::util::DetectorGeometry & geometry) {
            set("detectorGeometry", geometry.toHash());
        }


        const karabo::util::Hash& ImageData::getHeader() const {
            boost::optional<const karabo::util::Hash::Node&> node = find("header");
            if (node) {
                return node->getValue<karabo::util::Hash>();
            } else {
                static const karabo::util::Hash h;
                return h;
            }
        }


        void ImageData::setHeader(const karabo::util::Hash & header) {
            set("header", header);
        }


        const karabo::util::NDArray& ImageData::getData() const {
            return get<karabo::util::NDArray >("pixels");
        }


        void ImageData::setData(const karabo::util::NDArray& array) {
            set<karabo::util::NDArray >("pixels", array);
        }

    }
}
