/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 25, 2011, 8:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_CPUIMAGE_HH
#define KARABO_XIP_CPUIMAGE_HH

#include <karabo/util/Configurator.hh>
#include <karabo/io/Input.hh>
#include <karabo/io/Output.hh>
#include <karabo/util/Types.hh>
#include <karabo/util/FromTypeInfo.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/xip/RawImageData.hh>

#include "AbstractImage.hh"
#include "CImg.h"
#include "Statistics.hh"
#include "SingleProcessor.hh"

namespace karabo {

    namespace xip {

        namespace ci = cimg_library;

        template <class TPix> class GpuImage;

        template <class TPix> class ImageFileReader;
        template <class TPix> class ImageFileWriter;

        struct CpuImageType {
            
            template <class T>
            static std::string classId() {
                using namespace karabo::util;
                return "Image-" +  Types::convert<FromTypeInfo, ToLiteral > (typeid (T));
            }
        };

        /**
         * Image class (computing done on CPU)
         */
        template<class TPix>
        class CpuImage : public AbstractImage<TPix> {

            // Grant friendship in order to copy-construct from foreign pixelTypes
            template <class UPix> friend class CpuImage;

            template <class UPix> friend class ImageFileReader;
            template <class UPix> friend class ImageFileWriter;

            typedef boost::shared_ptr<ci::CImgDisplay> CImgDisplayPointer;
            
            // std::vector<karabo::util::Hash> m_headerArchive;  // TODO Implement in Karabo-1.1
            karabo::util::Hash m_header;
            ci::CImg<TPix> m_cimg;
        
            static std::vector<boost::shared_ptr<ci::CImgDisplay> > m_displays;

        public:

            KARABO_CLASSINFO(CpuImage, CpuImageType::classId<TPix>(), "1.0")

            /***************************************
             *            Constructors             *
             ***************************************/

            CpuImage() : m_cimg(ci::CImg<TPix>()) {
            }

            explicit CpuImage(const std::string& filename) : m_cimg() {
                karabo::util::Hash h("ImageFile.filename", filename);
                typename karabo::io::Input<CpuImage<TPix> >::Pointer in = karabo::io::Input<CpuImage<TPix> >::create(h);
                in->read(*this);
            }

            /**
             * Standard Constructor
             * @param dx image width
             * @param dy image height
             * @param dz image depth
             */
            explicit CpuImage(const size_t dx, const size_t dy = 1, const size_t dz = 1) : m_cimg(ci::CImg<TPix>(dx, dy, dz)) {
            }

            /** 
             * Constructor
             * @param dx image width
             * @param dy image height
             * @param dz image depth
             * @param value default value to fill the image
             */
            CpuImage(const size_t dx, const size_t dy, const size_t dz, const TPix& value) : m_cimg(ci::CImg<TPix>(dx, dy, dz, 1, value)) {
            }

            CpuImage(const size_t dx, const size_t dy, const size_t dz, const std::string& values, const bool repeatValues) : m_cimg(ci::CImg<TPix>(dx, dy, dz, 1, values.c_str(), repeatValues)) {
            }

            CpuImage(const TPix * const dataBuffer, const size_t dx, const size_t dy, const size_t dz) : m_cimg(ci::CImg<TPix>(dataBuffer, dx, dy, dz)) {
            }

            CpuImage(const std::vector<TPix>& dataBuffer, const size_t dx, const size_t dy, const size_t dz) : m_cimg(ci::CImg<TPix>(&dataBuffer[0], dx, dy, dz)) {
            }

            CpuImage(const karabo::util::Hash& header) : m_cimg(ci::CImg<TPix>(header.get<int>("__dimX"), header.get<int>("__dimY"), header.get<int>("__dimZ"))) {
                m_header = header;
            }

            CpuImage(const karabo::util::Hash& header, const TPix& value) : m_cimg(ci::CImg<TPix>(header.get<int>("__dimX"), header.get<int>("__dimY"), header.get<int>("__dimZ"), value)) {
                m_header = header;
            }

            /***************************************
             *          Copy-Constructors          *
             ***************************************/

            // TODO Check whether really needed
            
//            /**
//             * Copying from foreign pixelType
//             * @param image
//             * @param isShared
//             */
//            template <class U>
//            CpuImage(const CpuImage<U>& image, bool isShared = false) {
//                m_header = image.m_header;
//                m_cimg.assign(image.getCImg(), isShared);
//            }
//
//            CpuImage(const CpuImage& image, bool isShared = false) {
//                m_header = image.m_header;
//                m_cimg.assign(image.getCImg(), isShared);
//            }

            template <class TImage>
            TImage to() {
                return TImage(*this);
            }

            /***************************************
             *        In-Place Construction        *
             ***************************************/

            inline CpuImage& assign() {
                m_header.clear();
                m_cimg.assign();
                return *this;
            }

            inline CpuImage& assign(const size_t dx, const size_t dy = 1, const size_t dz = 1) {
                m_header.clear();
                m_cimg.assign(dx, dy, dz);
                return *this;
            }

            inline CpuImage& assign(const int dx, const int dy, const int dz, const TPix& value) {
                m_header.clear();
                m_cimg.assign(dx, dy, dz, 1, value);
                return *this;
            }

            inline CpuImage& assign(const int dx, const int dy, const int dz, const std::string& values, const bool repeatValues) {
                m_header.clear();
                m_cimg.assign(dx, dy, dz, 1, values.c_str(), repeatValues);
                return *this;
            }

            inline CpuImage& assign(const TPix * const dataBuffer, const int dx, const int dy, const int dz) {
                m_header.clear();
                m_cimg.assign(dataBuffer, dx, dy, dz);
                return *this;
            }

            inline CpuImage& assign(const std::vector<TPix>& dataBuffer, const int dx, const int dy, const int dz) {
                return assign(&dataBuffer[0], dx, dy, dz);
            }

            /***************************************
             *     In-Place Copy-Construction      *
             ***************************************/

            /**
             * Copying from foreign pixelType
             * @param image
             * @param isShared
             * @return 
             */
            template <class UPix>
            inline CpuImage& assign(const CpuImage<UPix>& image, bool isShared = false) {
                m_header = image.m_header;
                m_cimg.assign(image.getCImg(), isShared);
                return *this;
            }

            inline CpuImage& assign(const CpuImage& image, bool isShared = false) {
                m_header = image.m_header;
                m_cimg.assign(image.getCImg(), isShared);
                return *this;
            }

            //            inline CpuImage& assign(const CpuImage<TPix, _gpu>& image) {
            //                CpuImage cpuImg(image);
            //                this->swap(cpuImg);
            //                return *this;
            //            }

            /***************************************
             *             Destructor              *
             ***************************************/

            virtual ~CpuImage() {
            }

            /***************************************
             *         Special functions           *
             ***************************************/

            void swap(CpuImage& image) {
                std::swap(m_header, image.m_header);
                m_cimg.swap(image.getCImg());
            }

            void swap(const CpuImage& image) {
                CpuImage& tmp = const_cast<CpuImage&> (image);
                this->swap(tmp);
            }

            /**
             * Moves the content of the instance image into another one in a way that memory copies are avoided if possible.
             * 
             * CAVEAT: The instance image is always empty after a call to this function.
             * 
             * @param image The image this instance should be moved into
             * @return The new image
             */
            CpuImage& moveTo(CpuImage& image) {
                swap(image);
                assign();
                return image;
            }

            inline CpuImage& clear() {
                return assign();
            }

            CpuImage& read(const std::string& filename) {
                CpuImage tmp(filename);
                this->swap(tmp);
                return *this;
            }

            const CpuImage& write(const std::string& filename, const bool enableAppendMode = false) const {
                karabo::util::Hash h("ImageFile.filename", filename, "ImageFile.enableAppendMode", enableAppendMode);
                typename karabo::io::Output<CpuImage<TPix> >::Pointer out = karabo::util::Configurator<karabo::io::Output<CpuImage<TPix> > >::create(h);
                out->write(*this);
                return *this;
            }

            inline size_t offset(const size_t x, const size_t y = 0, const size_t z = 0) {
                return m_cimg.offset(x, y, z);
            }            

            /***************************************
             *      Instance Characteristics       *
             ***************************************/
            
            inline const int dimensionality() const {
                // zero image
                if ((dimX() <= 1) && (dimY() <= 1) && (dimZ() <= 1)) return 0;
                // three dimensional image
                if ((dimX() > 1) && (dimY() > 1) && (dimZ() > 1)) return 3;
                // two dimensional image
                if (((dimX() > 2) && (dimY() > 1)) || ((dimX() > 1) && (dimZ() > 1)) || ((dimY() > 1) && (dimZ() > 1))) return 2;
                return 1;
            }
            
            inline std::vector<unsigned long long> dims() const {
                int nDims = dimensionality();
                if (nDims == 1) {
                    std::vector<unsigned long long> v(1);
                    v[0] = dimX();
                    return v;
                } 
                if (nDims == 2) {
                    std::vector<unsigned long long> v(2);
                    v[0] = dimX();
                    v[1] = dimY();
                    return v;
                }
                if (nDims == 3) {
                    std::vector<unsigned long long> v(3);
                    v[0] = dimX();
                    v[1] = dimY();
                    v[2] = dimZ();
                    return v;
                }
                return std::vector<unsigned long long>();
            }

            bool isEmpty() const {
                return m_cimg.is_empty();
            }

            inline size_t dimX() const {
                return m_cimg.width();
            }

            inline size_t dimY() const {
                return m_cimg.height();
            }

            inline size_t dimZ() const {
                return m_cimg.depth();
            }

            const karabo::util::Hash& getHeader() const {
                updateHeader(*this);
                return m_header;
            }

            void setHeader(const karabo::util::Hash& header) {
                m_header = header;
            }
            
            void setHeaderParam(const std::string& key, const char* const& value) {
                m_header.set(key, value);
            }
            
            void setHeaderParam(const std::string& key, const std::string& value) {
                m_header.set(key, value);
            }
            
            void setHeaderParam(const std::string& key, const bool value) {
                m_header.set(key, value);
            }
            
            void setHeaderParam(const std::string& key, const int value) {
                m_header.set(key, value);
            }
            
            void setHeaderParam(const std::string& key, const double value) {
                m_header.set(key, value);
            }
            
            inline size_t size() const {
                return m_cimg.size();
            }

            inline size_t byteSize() const {
                return size() * sizeof (TPix);
            }

            inline std::string pixelType() const {
                using namespace karabo::util;
                return Types::convert<FromTypeInfo, ToLiteral > (typeid (TPix));
            }

            Statistics getStatistics() const {
                ci::CImg<double> st = m_cimg.get_stats();
                return Statistics(st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[8], st[9], st[10]);
            }

            /***************************************
             *              Operators              *
             ***************************************/

            inline const TPix& operator[](const size_t offset) const {
                return m_cimg[offset];
            }

            inline TPix& operator[](const size_t offset) {
                return m_cimg[offset];
            }

            inline const TPix& operator()(const size_t x, const size_t y = 0, const size_t z = 0) const {
                return m_cimg(x, y, z);
            }

            inline TPix& operator()(const size_t x, const size_t y = 0, const size_t z = 0) {
                return m_cimg(x, y, z);
            }

            /**
             * Operator() Pixel-Buffer access [const]
             * @return Address of pixel buffer
             */
            inline operator const TPix*() const {
                return m_cimg.data();
            }

            /**
             * Operator() 
             * @return Address of pixel buffer
             */
            inline operator TPix*() {
                return m_cimg.data();
            }

            /**
             * Operator=()
             * Assignment operator. Fill all pixels of the instance image with the same value.
             * The image size is not modified.
             * @param val
             * @return Image
             */
            CpuImage& operator=(const TPix& val) {
                m_cimg.operator=(val);
                return *this;
            }

            /**
             * Operator=().
             * Assignment operator.
             * If instance image is non-shared, replace the instance image by a copy of the argument image.
             * If instance image is shared, replace the image content by the content of the argument image.
             *
             * @param image Another image
             * @return Image 
             */
            template <typename U>
            CpuImage& operator=(const CpuImage<U>& image) {
                return assign(image);
            }

            /**
             * Operator=().
             */
            CpuImage& operator=(const CpuImage<TPix>& image) {
                return assign(image);
            }

            /**
             * Operator+=();
             */
            template <typename U>
            CpuImage<TPix>& operator+=(const U& val) {
                m_cimg.operator+=(val);
                return *this;
            }

            /**
             * Operator+=().
             * @param image Another image
             * @return Image
             */
            template <typename U>
            CpuImage<TPix>& operator+=(const CpuImage<U>& image) {
                m_cimg.operator+=(image.getCImg());
                return *this;
            }
            
            /**
             * Operator-=();
             */
            template <typename U>
            CpuImage<TPix>& operator-=(const U& val) {
                m_cimg.operator-=(val);
                return *this;
            }

            /**
             * Operator-=().
             * @param image Another image
             * @return Image
             */
            template <typename U>
            CpuImage<TPix>& operator-=(const CpuImage<U>& image) {
                m_cimg.operator-=(image.getCImg());
                return *this;
            }

            CpuImage& operator++() {
                m_cimg.operator++();
                return *this;
            }
            
            CpuImage& operator--() {
                m_cimg.operator--();
                return *this;
            }

            template <typename U>
            CpuImage< typename ci::cimg::superset<TPix, U>::type > operator+(const U& val) const {
                return CpuImage< typename ci::cimg::superset<TPix, U>::type > (*this, false) += val;
            }
            
            template <typename U>
            CpuImage< typename ci::cimg::superset<TPix, U>::type > operator-(const U& val) const {
                return CpuImage< typename ci::cimg::superset<TPix, U>::type > (*this, false) -= val;
            }

            template <class UPix>
            CpuImage& operator*=(const UPix value) {
                m_cimg.operator*=(value);
                return *this;
            }

            template <class UPix>
            CpuImage& operator*=(const CpuImage<UPix>& image) {
                return ((*this) * image).moveTo(*this);
            }

            template<class UPix>
            CpuImage<typename ci::cimg::superset<TPix, UPix>::type> operator*(const UPix value) {
                return CpuImage<typename ci::cimg::superset<TPix, UPix>::type > (*this) *= value;
            }          

            template <class UPix>
            CpuImage<typename ci::cimg::superset<TPix, UPix>::type> operator*(const CpuImage<UPix>& image) const {
                return CpuImage<typename ci::cimg::superset<TPix, UPix>::type > (m_cimg.operator*(image.getCImg()));
            }

            template <class UPix>
            CpuImage& operator/=(const UPix value) {
                m_cimg.operator/=(value);
                return *this;
            }           

            template <class UPix>
            CpuImage& operator/=(const CpuImage<UPix>& image) {
                return ((*this) / image).moveTo(*this);
            }

            template<class UPix>
            CpuImage<typename ci::cimg::superset<TPix, UPix>::type> operator/(const UPix value) {
                return CpuImage<typename ci::cimg::superset<TPix, UPix>::type > (*this) /= value;
            }
          
            template <class UPix>
            CpuImage<typename ci::cimg::superset<TPix, UPix>::type> operator/(const CpuImage<UPix>& image) const {
                return CpuImage<typename ci::cimg::superset<TPix, UPix>::type > (m_cimg.operator/(image.getCImg()));
            }

            /***************************************
             *             Pixel Access            *  
             ***************************************/

            inline const TPix* pixelPointer() const {
                return m_cimg.data();
            }

            inline TPix* pixelPointer() {
                return m_cimg.data();
            }

            /**
             * Read a pixel value with Dirichlet boundary conditions.
             * @param value
             * @return 
             */
            TPix& at(const int offset, const TPix beyondBorderValue) {
                return m_cimg.at(offset, beyondBorderValue);
            }

            TPix at(const int offset, const TPix beyondBorderValue) const {
                return m_cimg.at(offset, beyondBorderValue);
            }

            //! Read a pixel value with Neumann boundary conditions.

            TPix& at(const int offset) {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._at(offset);
            }

            TPix at(const int offset) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._at(offset);
            }

            //! Read a pixel value with Dirichlet boundary conditions for the first coordinates (\c x).

            TPix& atX(const int x, const int y, const int z, const TPix beyondBoundaryValue) {
                return m_cimg.atX(x, y, z, 0, beyondBoundaryValue);
            }

            TPix atX(const int x, const int y, const int z, const TPix beyondBoundaryValue) const {
                return m_cimg.atX(x, y, z, 0, beyondBoundaryValue);
            }

            //! Read a pixel value with Neumann boundary conditions for the first coordinates (\c x).

            TPix& atX(const int x, const int y = 0, const int z = 0) {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._atX(x, y, z, 0);
            }

            TPix atX(const int x, const int y = 0, const int z = 0) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._atX(x, y, z, 0);
            }

            TPix& atXY(const int x, const int y, const int z, const TPix beyondBoundaryValue) {
                return m_cimg.atXY(x, y, z, 0, beyondBoundaryValue);
            }

            TPix atXY(const int x, const int y, const int z, const TPix beyondBoundaryValue) const {
                return m_cimg.atXY(x, y, z, 0, beyondBoundaryValue);
            }

            TPix& atXY(const int x, const int y = 0, const int z = 0) {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._atXY(x, y, z, 0);
            }

            TPix atXY(const int x, const int y = 0, const int z = 0) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._atXY(x, y, z, 0);
            }

            TPix& atXYZ(const int x, const int y, const int z, const TPix beyondBoundaryValue) {
                return m_cimg.atXYZ(x, y, z, 0, beyondBoundaryValue);
            }

            TPix atXYZ(const int x, const int y, const int z, const TPix beyondBoundaryValue) const {
                return m_cimg.atXYZ(x, y, z, 0, beyondBoundaryValue);
            }

            //! Read a pixel value with Neumann boundary conditions for the first coordinates (\c x).

            TPix& atXYZ(const int x, const int y = 0, const int z = 0) {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._atXYZ(x, y, z, 0);
            }

            TPix atXYZ(const int x, const int y = 0, const int z = 0) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._atXYZ(x, y, z, 0);
            }

            double linearAtX(const float fx, const int y, const int z, const double beyondBoundaryValue) const {
                return m_cimg.linear_atX(fx, y, z, 0, beyondBoundaryValue);
            }

            double linearAtX(const float fx, const int y = 0, const int z = 0) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._linear_atX(fx, y, z, 0);
            }

            double linearAtXY(const float fx, const float y, const int z, const double beyondBoundaryValue) const {
                return m_cimg.linear_atXY(fx, y, z, 0, beyondBoundaryValue);
            }

            double linearAtXY(const float fx, const float y = 0, const int z = 0) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._linear_atXY(fx, y, z, 0);
            }

            double linearAtXYZ(const float fx, const float y, const float z, const double beyondBoundaryValue) const {
                return m_cimg.linear_atXYZ(fx, y, z, 0, beyondBoundaryValue);
            }

            double linearAtXYZ(const float fx, const float y = 0, const float z = 0) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._linear_atXYZ(fx, y, z, 0);
            }

            double cubicAtX(const float fx, const int y, const int z, const double beyondBoundaryValue) const {
                return m_cimg.cubic_atX(fx, y, z, 0, beyondBoundaryValue);
            }

            double cubicAtX(const float fx, const int y = 0, const int z = 0) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._cubic_atX(fx, y, z, 0);
            }

            double cubicAtXY(const float fx, const float y, const int z, const double beyondBoundaryValue) const {
                return m_cimg.cubic_atXY(fx, y, z, 0, beyondBoundaryValue);
            }

            double cubicAtXY(const float fx, const float y = 0, const int z = 0) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._cubic_atXY(fx, y, z, 0);
            }

            double cubicAtXYZ(const float fx, const float y, const float z, const double beyondBoundaryValue) const {
                return m_cimg.cubic_atXYZ(fx, y, z, 0, beyondBoundaryValue);
            }

            double cubicAtXYZ(const float fx, const float y = 0, const float z = 0) const {
                if (this->isEmpty()) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Empty image");
                return m_cimg._cubic_atXYZ(fx, y, z, 0);
            }

            /***************************************
             *         Matrices & Vectors          *
             ***************************************/

            //! Return a vector with specified coefficients.

            static CpuImage vector(const TPix& a0) {
                return CpuImage(ci::CImg<TPix>::vector(a0));

            }

            //! Return a vector with specified coefficients.

            static CpuImage vector(const TPix& a0, const TPix& a1) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1));

            }

            //! Return a vector with specified coefficients.

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6, a7));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7, const TPix& a8) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6, a7, a8));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7, const TPix& a8, const TPix& a9) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7, const TPix& a8, const TPix& a9, const TPix& a10) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7, const TPix& a8, const TPix& a9, const TPix& a10,
                                   const TPix& a11) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7, const TPix& a8, const TPix& a9, const TPix& a10,
                                   const TPix& a11, const TPix& a12) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12));

            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7, const TPix& a8, const TPix& a9, const TPix& a10,
                                   const TPix& a11, const TPix& a12, const TPix& a13) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13));
            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7, const TPix& a8, const TPix& a9, const TPix& a10,
                                   const TPix& a11, const TPix& a12, const TPix& a13, const TPix& a14) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14));
            }

            static CpuImage vector(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7, const TPix& a8, const TPix& a9, const TPix& a10,
                                   const TPix& a11, const TPix& a12, const TPix& a13, const TPix& a14, const TPix& a15) {
                return CpuImage(ci::CImg<TPix>::vector(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15));
            }

            static CpuImage matrix(const TPix& a0) {
                return vector(a0);
            }

            static CpuImage matrix(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3) {
                return CpuImage(ci::CImg<TPix>::matrix(a0, a1, a2, a3));
            }

            static CpuImage matrix(const TPix& a0, const TPix& a1, const TPix& a2,
                                   const TPix& a3, const TPix& a4, const TPix& a5,
                                   const TPix& a6, const TPix& a7, const TPix& a8) {
                return CpuImage(ci::CImg<TPix>::matrix(a0, a1, a2, a3, a4, a5, a6, a7, a8));
            }

            static CpuImage matrix(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3,
                                   const TPix& a4, const TPix& a5, const TPix& a6, const TPix& a7,
                                   const TPix& a8, const TPix& a9, const TPix& a10, const TPix& a11,
                                   const TPix& a12, const TPix& a13, const TPix& a14, const TPix& a15) {
                return CpuImage(ci::CImg<TPix>::matrix(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15));
            }

            static CpuImage matrix(const TPix& a0, const TPix& a1, const TPix& a2, const TPix& a3, const TPix& a4,
                                   const TPix& a5, const TPix& a6, const TPix& a7, const TPix& a8, const TPix& a9,
                                   const TPix& a10, const TPix& a11, const TPix& a12, const TPix& a13, const TPix& a14,
                                   const TPix& a15, const TPix& a16, const TPix& a17, const TPix& a18, const TPix& a19,
                                   const TPix& a20, const TPix& a21, const TPix& a22, const TPix& a23, const TPix& a24) {
                return CpuImage(ci::CImg<TPix>::matrix(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
                                                       a16, a17, a18, a19, a20, a21, a22, a23, a24));
            }

            static CpuImage rotationMatrix3x3(const float x, const float y, const float z, const float w, const bool isQuaternionData = false) {
                return CpuImage(ci::CImg<TPix>::rotation_matrix(x, y, z, w, isQuaternionData));
            }

            /***************************************
             *        Convenience Functions        *
             ***************************************/

            /**
             * Computes the sum of all pixels
             * @return total sum of all pixels
             */
            double getSum() const {
                return m_cimg.sum();
            }

            double getMean() const {
                return m_cimg.mean();
            }



            /***************************************
             *          Value Manipulation         *
             ***************************************/

            /**
             * *this function fills an image with values
             * @param value
             * @return 
             */
            CpuImage& fill(const TPix& value) {
                m_cimg.fill(value);
                return *this;
            }

            CpuImage process(const std::string& processorName, const karabo::util::Hash& configuration = karabo::util::Hash()) {
                karabo::util::Hash h(processorName, configuration);
                typename SingleProcessor< CpuImage >::Pointer p = SingleProcessor< CpuImage>::create(h);
                return p->process(*this);
            }

            CpuImage& processInPlace(const std::string& processorName, const karabo::util::Hash& configuration = karabo::util::Hash()) {
                karabo::util::Hash h(processorName, configuration);
                typename SingleProcessor< CpuImage >::Pointer p = SingleProcessor< CpuImage>::create(h);
                p->processInPlace(*this);
                return *this;
            }

            CpuImage& randomize(const TPix& valueMin, const TPix& valueMax) {
                m_cimg.rand(valueMin, valueMax);
                return *this;
            }

            CpuImage getRandomize(const TPix& valueMin, const TPix& valueMax) {
                return CpuImage(m_cimg.get_rand(valueMin, valueMax));
            }

            CpuImage& permuteAxis(const std::string& order) {
                std::string fullOrder = order + "c";
                m_cimg.permute_axes(fullOrder.c_str());
                return *this;
            }

            CpuImage& getPermuteAxis(const std::string& order) {
                CpuImage ret(*this);
                return ret.permuteAxis(order);
            }

            /**
             * Prints image information to console
             * @param title Any custom title for the current image
             * @param displayPixels Should pixel information be displayed?
             * @param maxDimX Maximum numbers printed in X direction
             * @param maxDimY Maximum numbers printed in Y direction
             * @param maxDimZ Maximum numbers printed in Z direction
             * @return Image
             */
            const CpuImage& print(const std::string& title = "", const bool displayPixels = true, int maxDimX = 28, int maxDimY = 28, int maxDimZ = 8) const {
                using namespace std;
                using namespace karabo::util;
                const size_t siz = m_cimg.size();
                size_t msiz = siz * sizeof (TPix);
                size_t mdisp = msiz < 8 * 1024 ? 0 : (msiz < 8 * 1024 * 1024 ? 1 : 2);
                mdisp == 0 ? msiz = msiz : (mdisp == 1 ? (msiz = msiz >> 10) : (msiz = msiz >> 20));
                std::string unit;
                mdisp == 0 ? unit = "b " : (mdisp == 1 ? unit = "Kb" : unit = "Mb");
                string type = "type = Image<" + Types::convert<FromTypeInfo, ToLiteral > (typeid (TPix)) + ">";
                if (!title.empty()) cout << title << ": ";
                cout << type << ", size = (" << m_cimg.width() << ", " << m_cimg.height() << ", " << m_cimg.depth() << "), data = " << msiz << " " << unit << endl;
                cout << "Header:\n" << getHeader();

                if (!m_cimg.is_empty()) {
                    ci::CImg<double> st = m_cimg.get_stats();
                    Statistics statistics(st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[8], st[9], st[10]);
                    // Display pixels
                    if (displayPixels) {
                        //int nDigits = 1;
                        //if (statistics.getMax() <= 0) ceil(log10(-1 * statistics.getMax() + 1));
                        //else nDigits = ceil(log10(st[1] + 1));

                        int dimX = m_cimg.width();
                        int maxX = maxDimX;
                        int printX = maxX;
                        if (dimX > maxX) printX = maxX / 2;

                        int dimY = m_cimg.height();
                        int maxY = maxDimY;
                        int printY = maxY;
                        if (dimY > maxY) printY = maxY / 2;

                        int dimZ = m_cimg.depth();
                        int maxZ = maxDimZ;
                        int printZ = maxZ;
                        if (dimZ > maxZ) printZ = maxZ / 2;

                        for (int z = 0; z < dimZ && z < maxZ; ++z) {
                            int idz = z;
                            if (z >= printZ) idz = dimZ - (maxZ - z);
                            if (z == printZ) cout << ".\n.\n.\n\n";
                            if (dimZ > 1) cout << "-z(" << idz << ")-" << endl;
                            for (int y = 0; y < dimY && y < maxY; ++y) {
                                int idy = y;
                                if (y >= printY) idy = dimY - (maxY - y);
                                if (y == printY) cout << ".\n.\n.\n";
                                for (int x = 0; x < dimX && x < maxX; ++x) {
                                    int idx = x;
                                    if (x >= printX) idx = dimX - (maxX - x);
                                    if (x == printX) cout << "... ";
                                    //cout << karabo::util::toString(m_cimg(idx, idy, idz), nDigits) << " ";
                                    cout << karabo::util::toString(m_cimg(idx, idy, idz)) << " ";
                                }
                                cout << endl;
                            }
                            cout << endl;
                        }
                    }
                    // Display statistics
                    statistics.print();
                    cout << endl;
                }
                return *this;
            }

            /***************************************
             *              Display                *
             ***************************************/

            void display(const std::string& title) {
                m_cimg.display(title.c_str());
            }

            void displayAndKeep(const std::string& title) {
                boost::shared_ptr<ci::CImgDisplay> d = boost::shared_ptr<ci::CImgDisplay > (new ci::CImgDisplay(m_cimg, title.c_str()));
                d->show();
                m_displays.push_back(d);
            }

            void display3dVectors(const std::string& title) {
                if (dimZ() != 1) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Expecting 3d vector type data");
                if (dimY() != 3 && dimX() == 3) {
                    ci::CImg<TPix> tmp = m_cimg.get_permute_axes("yxzc");
                    ci::CImg<unsigned char>().display_object3d(title.c_str(), tmp);
                } else if (dimY() == 3) {
                    ci::CImg<unsigned char>().display_object3d(title.c_str(), m_cimg);
                } else {
                    throw KARABO_IMAGE_DIMENSION_EXCEPTION("Expecting 3d vector type data");
                }
            }

            void displayAndKeep3dVectors(const std::string& title) {
                CImgDisplayPointer d = CImgDisplayPointer(new ci::CImgDisplay());
                if (dimZ() != 1) throw KARABO_IMAGE_DIMENSION_EXCEPTION("Expecting 3d vector type data");
                if (dimY() != 3 && dimX() == 3) {
                    ci::CImg<TPix> tmp = m_cimg.get_permute_axes("yxzc");
                    ci::CImg<unsigned char>().display_object3d(*d, tmp);
                } else if (dimY() == 3) {
                    ci::CImg<unsigned char>().display_object3d(*d, m_cimg);
                } else {
                    throw KARABO_IMAGE_DIMENSION_EXCEPTION("Expecting 3d vector type data");
                }
                m_displays.push_back(d);
            }

            void display3dVolume(const std::string& title, const float isoValue) {
                ci::CImgList<TPix> primitives;
                ci::CImg<TPix> vectors = m_cimg.get_isosurface3d(primitives, isoValue);
                ci::CImg<unsigned char>().display_object3d(title.c_str(), vectors, primitives);
            }

            void display3dVolume(const std::string& title) {
                float isoValue = getStatistics().getMean();
                display3dVolume(title, isoValue);
            }

            void displayAndKeep3dVolume(const std::string& title, const float isoValue) {
                CImgDisplayPointer d = CImgDisplayPointer(new ci::CImgDisplay());
                // Fuck the title -> seems to be a bug in CImg
                ci::CImgList<TPix> primitives;
                ci::CImg<TPix> vectors = m_cimg.get_isosurface3d(primitives, isoValue);
                ci::CImg<unsigned char>().display_object3d(*d, vectors, primitives);
                m_displays.push_back(d);
            }

            void displayAndKeep3dVolume(const std::string& title) {
                float isoValue = getStatistics().getMean();
                displayAndKeep3dVolume(title, isoValue);
            }


        public: // functions

            const ci::CImg<TPix>& getCImg() const {
                return m_cimg;
            }

            ci::CImg<TPix>& getCImg() {
                return m_cimg;
            }

            explicit CpuImage(const ci::CImg<TPix>& cimg) {
                ci::CImg<TPix>& tmp = const_cast<ci::CImg<TPix>&> (cimg);
                tmp.move_to(m_cimg);
            }

        private: // functions

            void updateHeader(const CpuImage& image) const {
                CpuImage& tmp = const_cast<CpuImage&> (image);
                tmp.m_header.set("__dimX", tmp.m_cimg.width());
                tmp.m_header.set("__dimY", tmp.m_cimg.height());
                tmp.m_header.set("__dimZ", tmp.m_cimg.depth());
            }
        };

        template <class T>
        std::vector<boost::shared_ptr<cimg_library::CImgDisplay> > CpuImage<T>::m_displays;

        typedef CpuImage<char> CpuImgC;
        typedef CpuImage<short> CpuImgS;
        typedef CpuImage<int> CpuImgI;
        typedef CpuImage<float> CpuImgF;
        typedef CpuImage<double> CpuImgD;

        typedef karabo::io::Input<CpuImgD> InputCpuImgD;
        typedef karabo::io::Output<CpuImgD> OutputCpuImgD;

        typedef karabo::io::Input<CpuImgI> InputCpuImgI;
        typedef karabo::io::Output<CpuImgI> OutputCpuImgI;

        typedef karabo::util::Hash Config;


    }
}

#endif
