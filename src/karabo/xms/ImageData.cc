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

        ImageData::ImageData(const karabo::util::NDArray& data,
                             const karabo::util::Dims& dims,
                             const EncodingType encoding,
                             const int bitsPerPixel) {

            setData(data);
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
                std::vector<unsigned long long> shape = get<NDArray>("data").getShape().toVector();
                std::reverse(shape.begin(), shape.end());                
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
            return get<karabo::util::NDArray >("data");
        }
       
        void ImageData::setData(const karabo::util::NDArray& array) {
            set<karabo::util::NDArray >("data", array);
        }            

    }
}
