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

#include <climits>

namespace karabo {
    namespace xms {

        using namespace karabo::util;


        bool Encoding::isIndexable(int encoding) {
            switch (encoding) {
                case Encoding::UNDEFINED:
                    return false;
                case Encoding::GRAY:
                case Encoding::RGB:
                case Encoding::RGBA:
                case Encoding::BGR:
                case Encoding::BGRA:
                case Encoding::CMYK:
                case Encoding::YUV:
                case Encoding::BAYER:
                    return true;
                case Encoding::JPEG:
                case Encoding::PNG:
                case Encoding::BMP:
                case Encoding::TIFF:
                    return false;
                default:
                    throw KARABO_LOGIC_EXCEPTION("Encoding " + karabo::util::toString(encoding) + " invalid.");
                    return false; // pleasing the compiler
            }
        }


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
                    .description("Describes the color space of pixel encoding of"
                    " the data (e.g. GRAY, RGB, JPG, PNG etc.).")
                    .readOnly()
                    .commit();
            INT32_ELEMENT(s).key("bitsPerPixel")
                    .displayedName("Bits per pixel")
                    .description("The number of bits needed for each pixel")
                    .readOnly()
                    .commit();
            VECTOR_UINT64_ELEMENT(s).key("roiOffsets")
                    .displayedName("ROI Offsets")
                    .description("The offset of the Region-of-Interest (ROI); "
                    "it will contain zeros if the image has no ROI defined.")
                    .readOnly()
                    .commit();
            VECTOR_UINT64_ELEMENT(s).key("binning")
                    .displayedName("Binning")
                    .description("The number of binned adjacent pixels. They "
                    "are reported out of the camera as a single pixel.")
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


        ImageData::ImageData(const karabo::util::NDArray& data, const EncodingType encoding, const int bitsPerPixel)
            : ImageData(data, karabo::util::Dims(), encoding, bitsPerPixel) {
        }


        ImageData::ImageData(const karabo::util::NDArray& data,
                             const karabo::util::Dims& dims,
                             const EncodingType encoding,
                             const int bitsPerPixel) {

            setData(data);

            // Encoding might be deduced from data if not defined
            Dims dataDims(data.getShape());
            const int rank = dataDims.rank();
            EncodingType finalEncoding = encoding;
            if (encoding == Encoding::UNDEFINED) {
                // No encoding info -> try to guess it from ndarray shape
                if (rank == 2 || (rank == 3 && dataDims.x3() == 1)) {
                    finalEncoding = Encoding::GRAY;
                } else if (rank == 3 && dataDims.x3() == 3) {
                    finalEncoding = Encoding::RGB;
                } else if (rank == 3 && dataDims.x3() == 4) {
                    finalEncoding = Encoding::RGBA;
                } else if (rank == 3) {
                    // Assume it is a stack of GRAY images
                    finalEncoding = Encoding::GRAY;
                }
            }
            setEncoding(finalEncoding);

            // If Dims are not defined, they can be deduced from data as well in many cases
            if (dims.size() == 0) {
                if (!Encoding::isIndexable(finalEncoding)) {
                    throw KARABO_LOGIC_EXCEPTION("Dimensions must be supplied for encoded images");
                }
            } else {
                dataDims = dims;
            }

            // After setEncoding one can set dimensions
            setDimensions(dataDims);

            // bits per pixel - may calculate default, depending on type
            setBitsPerPixel((bitsPerPixel > 0 ? bitsPerPixel : defaultBitsPerPixel(finalEncoding, data)));

            const std::vector<unsigned long long> offsets(dataDims.rank(), 0ull);
            setROIOffsets(karabo::util::Dims(offsets));

            const std::vector<unsigned long long> binning(dataDims.rank(), 1ull);
            setBinning(karabo::util::Dims(binning));

            setDimensionScales(std::string());
        }


        karabo::util::Dims ImageData::getROIOffsets() const {
            return karabo::util::Dims(get<std::vector<unsigned long long> >("roiOffsets"));
        }


        void ImageData::setROIOffsets(const karabo::util::Dims& offsets) {
            set<std::vector<unsigned long long> >("roiOffsets", offsets.toVector());
        }


        karabo::util::Dims ImageData::getBinning() const {
            return karabo::util::Dims(get<std::vector<unsigned long long> >("binning"));
        }


        void ImageData::setBinning(const karabo::util::Dims& binning) {
            set<std::vector<unsigned long long> >("binning", binning.toVector());
        }


        int ImageData::getBitsPerPixel() const {
            return get<int>("bitsPerPixel");
        }


        void ImageData::setBitsPerPixel(const int bitsPerPixel) {
            // Maximum depends on type in data and on encoding.
            // But if encoding cannot specify a maximum, just believe the input.
            const int maxBitsPerPixel = defaultBitsPerPixel(getEncoding(), getData());
            const int finalBitsPerPixel = (maxBitsPerPixel == 0 ? bitsPerPixel
                                           : std::min<int>(bitsPerPixel, maxBitsPerPixel));
            set<int>("bitsPerPixel", finalBitsPerPixel);
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
                if (Encoding::isIndexable(getEncoding())) {
                    // Make sure dimensions match the size of the data for indexable encodings
                    get<NDArray>("pixels").setShape(dims); // throws if size does not fit
                }
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


        int ImageData::defaultBitsPerPixel(int encoding, const karabo::util::NDArray& data) {
            const size_t numBytes = karabo::util::Types::to<karabo::util::ToSize>(data.getType());

            int factor = -1;
            switch (encoding) {
                case Encoding::GRAY:
                    factor = 1;
                    break;
                case Encoding::BAYER:
                    return numBytes * 8; // Plain 8 in http://www.ni.com/white-paper/3903/en/, independent of CHAR_BIT!
                case Encoding::RGB:
                case Encoding::BGR:
                case Encoding::YUV:
                    // NDArray's Dims.x3 should be 3
                    factor = 3;
                    break;
                case Encoding::RGBA:
                case Encoding::BGRA:
                case Encoding::CMYK:
                    // NDArray's Dims.x3 should be 4
                    factor = 4;
                    break;
                default:
                    // JPEG, PNG, BMP, TIFF, UNDEFINED: return 0 to indicate that it is not defined
                    factor = 0;
            }
            return factor * numBytes * CHAR_BIT; // CHAR_BIT from <climits> - usually 8
        }
    }
}
