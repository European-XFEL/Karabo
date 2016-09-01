/* 
 * File:   ImageData.cc
 * Author: 
 * 
 * Created on April 20, 2015, 19:35 PM
 */

#include "ImageData.hh"


namespace karabo {
    namespace xms {

        using namespace karabo::util;


        void ImageData::expectedParameters(karabo::util::Schema& s) {

            // TODO -- Add NDARRAY_ELEMENT, once it's done

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
            VECTOR_UINT32_ELEMENT(s).key("roiOffsets")
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
                    .displayedName("Hash containing user-defined header data")
                    .commit();
        }


        ImageData::ImageData() {
        }


        ImageData::ImageData(const karabo::util::NDArray& data,
                             const karabo::util::Dims& dims,
                             const EncodingType encoding,
                             const int bitsPerPixel) {

            setData(data);
            setDimensions(dims);
            setEncoding(encoding);
            setBitsPerPixel(bitsPerPixel);

            int rank = dims.rank();
            if (dims.size() == 0) {
                rank = data.getShape().rank();
            }
            std::vector<unsigned long long> offsets(rank, 0);
            setROIOffsets(karabo::util::Dims(offsets));
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
            set<int>("bitsPerPixel", bitsPerPixel);
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
            if (dims.size() == 0) {
                // Will use the shape information of underlying NDArray as best guess
                std::vector<unsigned long long> shape = get<NDArray>("pixels").getShape().toVector();
                set("dims", shape);
            } else {
                // XXX: Make sure dimensions match the size of the data!
                set<std::vector<unsigned long long> >("dims", dims.toVector());
            }
            // In case the dimensionTypes were not yet set, inject a default here
            if (!has("dimTypes")) {
                setDimensionTypes(std::vector<int>(dims.rank(), Dimension::UNDEFINED));
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


        karabo::util::DetectorGeometry ImageData::getGeometry() {
            return karabo::util::DetectorGeometry(get<karabo::util::Hash>("detectorGeometry"));
        }


        void ImageData::setGeometry(const karabo::util::DetectorGeometry & geometry) {
            set("detectorGeometry", geometry.toHash());
        }


        const karabo::util::Hash& ImageData::getHeader() const {
            return get<karabo::util::Hash>("header");
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
