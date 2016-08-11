/* 
 * File:   ImageData.hh

 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on April 20, 2015, 19:35 PM
 */

#ifndef KARABO_XMS_IMAGEDATA_HH
#define	KARABO_XMS_IMAGEDATA_HH

#include <karabo/util/Hash.hh>
#include <karabo/util/Dims.hh>
#include <karabo/util/Validator.hh>
#include <karabo/util/NDArray.hh>
#include <karabo/util/NDArrayElement.hh>
#include "Data.hh"
#include <karabo/util/DetectorGeometry.hh>
#include <karabo/util/ByteSwap.hh>

namespace karabo {
    namespace xms {

        namespace Dimension {

            enum DimensionType {
                UNDEFINED = 0,
                STACK = -1,
                DATA = 1,
            };
        }

        typedef Dimension::DimensionType DimensionType;

        namespace Encoding {

            enum EncodingType {


                UNDEFINED = -1,
                GRAY,
                RGB,
                RGBA,
                BGR,
                BGRA,
                CMYK,
                YUV,
                BAYER,
                JPEG,
                PNG,
                BMP,
                TIFF,
            };
        }

        typedef Encoding::EncodingType EncodingType;


        template<typename T>
        class ImageData : public Data {

        public:

            KARABO_CLASSINFO(ImageData<T>, "ImageData", "1.5")

            static void expectedParameters(karabo::util::Schema& s) {

                using namespace karabo::util;

                NDArrayElement<T>(s).key("data")
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

            ImageData();

            /**
             * Constructs from a hash that has to follow the correct format
             * @param imageHash
             */
            ImageData(const karabo::util::Hash& hash);

            ImageData(const karabo::util::Hash::Pointer& data);

            /**
             * Constructor from already existing memory
             * @param data
             * @param size
             * @param copy
             * @param dimensions
             * @param encoding
             * @param bitsPerPixel
             */
            ImageData(const T* const data,
                      const size_t nelems,
                      const bool copy = true,
                      const karabo::util::Dims& dims = karabo::util::Dims(),
                      const EncodingType encoding = Encoding::GRAY,
                      const int bitsPerPixel = 8);

            virtual ~ImageData();

            const karabo::util::NDArray<T>& getData() const;

            // Data MAYBE copied
            void setData(const T* data, const size_t nelems, const bool copy);

            // Data NOT copied
            void setData(const karabo::util::NDArray<T>& array);

            karabo::util::Dims getROIOffsets() const;

            void setROIOffsets(const karabo::util::Dims& offsets);

            int getBitsPerPixel() const;

            void setBitsPerPixel(const int bitsPerPixel);

            int getEncoding() const;

            void setEncoding(const int encoding);

            void toBigEndian();

            void toLittleEndian();

            void setIsBigEndian(const bool isBigEndian);

            bool isBigEndian() const;

            size_t getByteSize() const;

            karabo::util::Dims getDimensions() const;

            /**
             * Say x = fasted changing, y = medium fast and z = slowest changing index
             * then set the dimension like 
             * @code setDimensions(Dims(x,y,z))
             * Or in other words, if you think about width, height and depth use:
             * @code setDimensions(Dims(width, height, depth);
             * For 2D single images, leave away the depth
             * @param dims Dimensionality
             */
            void setDimensions(const karabo::util::Dims& dims);

            const std::vector<int> getDimensionTypes() const;

            void setDimensionTypes(const std::vector<int>& dimTypes);

            const std::string& getDimensionScales() const;

            void setDimensionScales(const std::string& scales);

            friend std::ostream& operator<<(std::ostream& os, const ImageData& image) {
                os << *image.hash();
                return os;
            }

            void setGeometry(const karabo::util::DetectorGeometry & geometry);

            karabo::util::DetectorGeometry getGeometry();

            const karabo::util::Hash& getHeader() const;

            void setHeader(const karabo::util::Hash & header);

        private:

            void swapEndianess();

            static void deallocateNonCopied(T*) {
                // Do nothing.
            }

        };


        /**********************************************************************
         * Declaration ImageDataElement
         **********************************************************************/

        template<typename T>
        class ImageDataElement : public DataElement<ImageDataElement<T>, ImageData<T> > {
        public:
            ImageDataElement(karabo::util::Schema& s) : DataElement<ImageDataElement<T>, ImageData<T> >(s) {
            }

            ImageDataElement<T>& setDimensionScales(const std::string& scales) {
                return DataElement<ImageDataElement<T>, ImageData<T> >::setDefaultValue("dimScales", scales);
            }

            ImageDataElement<T>& setDimensions(const std::string& dimensions) {
                std::vector<unsigned long long> tmp = karabo::util::fromString<unsigned long long, std::vector>(dimensions);
                std::reverse(tmp.begin(), tmp.end());
                return DataElement<ImageDataElement<T>, ImageData<T> >::setDefaultValue("dims", tmp);
            }

            ImageDataElement<T>& setEncoding(const EncodingType& encoding) {
                return DataElement<ImageDataElement<T>, ImageData<T> >::setDefaultValue("encoding", (int) encoding);
            }

            ImageDataElement<T>& setGeometry(karabo::util::DetectorGeometry & geometry) {
                geometry.toSchema("data.geometry", this->m_schema);
                return DataElement<ImageDataElement<T>, ImageData<T> >::setDefaultValue("detectorGeometry", geometry.toHash());
            }

        };

        typedef ImageDataElement<bool> IMAGEDATA_BOOL_ELEMENT;
        typedef ImageDataElement<signed char> IMAGEDATA_INT8_ELEMENT;
        typedef ImageDataElement<signed short> IMAGEDATA_INT16_ELEMENT;
        typedef ImageDataElement<int> IMAGEDATA_INT32_ELEMENT;
        typedef ImageDataElement<long long> IMAGEDATA_INT64_ELEMENT;
        typedef ImageDataElement<unsigned char> IMAGEDATA_UINT8_ELEMENT;
        typedef ImageDataElement<unsigned short> IMAGEDATA_UINT16_ELEMENT;
        typedef ImageDataElement<unsigned int> IMAGEDATA_UINT32_ELEMENT;
        typedef ImageDataElement<unsigned long long> IMAGEDATA_UINT64_ELEMENT;
        typedef ImageDataElement<float> IMAGEDATA_FLOAT_ELEMENT;
        typedef ImageDataElement<double> IMAGEDATA_DOUBLE_ELEMENT;


        /**********************************************************************
         * Implementation ImageData
         **********************************************************************/

        template <typename T>
        ImageData<T>::ImageData() {
        }

        template <typename T>
        ImageData<T>::ImageData(const karabo::util::Hash& hash) : Data(hash) {
        }

        template <typename T>
        ImageData<T>::ImageData(const karabo::util::Hash::Pointer& data) : Data(data) {
        }

        template<typename T>
        ImageData<T>::ImageData(const T* const data,
                    const size_t nelems,
                    const bool copy,
                    const karabo::util::Dims& dims,
                    const EncodingType encoding,
                    const int bitsPerPixel) : Data() {

            setData(data, nelems, copy);
            setDimensions(dims);
            if (dims.size() == 0) {
                setROIOffsets(karabo::util::Dims(0));
            } else {
                std::vector<unsigned long long> offsets(dims.rank(), 0);
                setROIOffsets(karabo::util::Dims(offsets));
            }
            setEncoding(encoding);
            setBitsPerPixel(bitsPerPixel);
        }

        template <typename T>
        ImageData<T>::~ImageData() {
        }

        template<typename T>
        const karabo::util::NDArray<T>& ImageData<T>::getData() const {
            return m_hash->get<karabo::util::NDArray<T> >("data");
        }

        template<typename T>
        void ImageData<T>::setData(const T* data, const size_t nelems, const bool copy) {
            if (copy) {
                karabo::util::NDArray<T> array(data, nelems, karabo::util::Dims(nelems));
                setData(array);
            }
            else {
                boost::shared_ptr<karabo::util::ArrayData<T> > arrayPtr(new karabo::util::ArrayData<T>(data, nelems, &ImageData::deallocateNonCopied));
                karabo::util::NDArray<T> array(data, nelems, karabo::util::Dims(nelems));
                setData(array);
            }
        }

        template<typename T>
        void ImageData<T>::setData(const karabo::util::NDArray<T>& array) {
            m_hash->set<karabo::util::NDArray<T> >("data", array);
        }

        template <typename T>
        karabo::util::Dims ImageData<T>::getROIOffsets() const {
            return karabo::util::Dims(m_hash->get<std::vector<unsigned long long> >("roiOffsets"));
        }

        template <typename T>
        void ImageData<T>::setROIOffsets(const karabo::util::Dims& offsets) {
            m_hash->set<std::vector<unsigned long long> >("roiOffsets", offsets.toVector());
        }

        template <typename T>
        int ImageData<T>::getBitsPerPixel() const {
            return m_hash->get<int>("bitsPerPixel");
        }

        template <typename T>
        void ImageData<T>::setBitsPerPixel(const int bitsPerPixel) {
            m_hash->set<int>("bitsPerPixel", bitsPerPixel);
        }

        template <typename T>
        int ImageData<T>::getEncoding() const {
            return m_hash->get<int>("encoding");
        }

        template <typename T>
        void ImageData<T>::setEncoding(const int encoding) {
            m_hash->set<int>("encoding", encoding);
        }

        template <typename T>
        void ImageData<T>::toLittleEndian() {
            if (isBigEndian()) {
               swapEndianess();
               setIsBigEndian(false);
            }
        }

        template <typename T>
        void ImageData<T>::toBigEndian() {
            if (!isBigEndian()) {
               swapEndianess();
               setIsBigEndian(true);
            }
        }

        template <typename T>
        void ImageData<T>::setIsBigEndian(const bool isBigEndian) {
            m_hash->set<bool>("isBigEndian", isBigEndian);
        }

        template <typename T>
        bool ImageData<T>::isBigEndian() const {
            return m_hash->get<bool>("isBigEndian");
        }

        template<typename T>
        size_t ImageData<T>::getByteSize() const {
            const karabo::util::NDArray<T>& data = m_hash->get<karabo::util::NDArray<T> >("data");
            const karabo::util::Dims& shape = data.getShape();
            size_t size = 1;
            for (unsigned int idx = 0; idx < shape.rank(); ++idx) {
                size *= static_cast<size_t>(shape.extentIn(idx));
            }
            return size * sizeof(T);
        }

        template <typename T>
        karabo::util::Dims ImageData<T>::getDimensions() const {
            return karabo::util::Dims(m_hash->get<std::vector<unsigned long long> >("dims"));
        }

        template <typename T>
        void ImageData<T>::setDimensions(const karabo::util::Dims& dims) {
            // XXX: Make sure dimensions match the size of the data!
            m_hash->set<std::vector<unsigned long long> >("dims", dims.toVector());
            // In case the dimensionTypes were not yet set, inject a default here
            if (!m_hash->has("dimTypes")) {
                setDimensionTypes(std::vector<int>(dims.rank(), Dimension::UNDEFINED));
            }
        }

        template <typename T>
        const std::vector<int> ImageData<T>::getDimensionTypes() const {
            return m_hash->get<std::vector<int> >("dimTypes");
        }

        template <typename T>
        void ImageData<T>::setDimensionTypes(const std::vector<int>& dimTypes) {
            m_hash->set<std::vector<int> >("dimTypes", dimTypes);
        }

        template <typename T>
        const std::string& ImageData<T>::getDimensionScales() const {
            return m_hash->get<std::string>("dimScales");
        }

        template <typename T>
        void ImageData<T>::setDimensionScales(const std::string& scales) {
            m_hash->set("dimScales", scales);
        }

        template <typename T>
        karabo::util::DetectorGeometry ImageData<T>::getGeometry() {
            return karabo::util::DetectorGeometry(m_hash->get<karabo::util::Hash>("detectorGeometry"));
        }

        template <typename T>
        void ImageData<T>::setGeometry(const karabo::util::DetectorGeometry & geometry) {
            m_hash->set("detectorGeometry", geometry.toHash());
        }

        template <typename T>
        const karabo::util::Hash& ImageData<T>::getHeader() const {
            return m_hash->get<karabo::util::Hash>("header");
        }

        template <typename T>
        void ImageData<T>::setHeader(const karabo::util::Hash & header) {
            m_hash->set("header", header);
        }

        template<typename T>
        void ImageData<T>::swapEndianess() {
            int bpp = getBitsPerPixel();
            karabo::util::NDArray<T>& dataArray = m_hash->get<karabo::util::NDArray<T> >("data");

            switch (bpp) {
                case 8:
                    // No swap needed.
                    break;
                case 16:
                {
                    unsigned short* data = reinterpret_cast<unsigned short*> (&(*dataArray.getData())[0]);
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = karabo::util::byteSwap16(data[i]);
                    }
                    break;
                }
                case 32:
                {
                    unsigned int* data = reinterpret_cast<unsigned int*> (&(*dataArray.getData())[0]);
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = karabo::util::byteSwap32(data[i]);
                    }
                    break;
                }
                case 64:
                {
                    unsigned long long* data = reinterpret_cast<unsigned long long*> (&(*dataArray.getData())[0]);
                    for (size_t i = 0; i < getDimensions().size(); ++i) {
                        data[i] = karabo::util::byteSwap64(data[i]);
                    }
                    break;
                }
                default:
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Endianess conversion not implemented for this channel type");
            }
        }
    }
}



#endif	/* KARABO_XMS_IMAGEDATA_HH */

