/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 25, 2011, 8:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_GPUIMAGE_HH
#define KARABO_XIP_GPUIMAGE_HH

#include <cuda_runtime_api.h>
#include <boost/swap.hpp>
#include <karabo/util/Factory.hh>

#include "CpuImage.hh"
#include "Statistics.hh"
#include "GpuImage.cuh"

#define CUDA_SAFE_CALL(cudaCall) \
{ \
  cudaError_t error; \
  if ((error = (cudaCall)) != cudaSuccess) { \
    throw CUDA_EXCEPTION(cudaGetErrorString(error)); \
  } \
}

namespace karabo {

    namespace xip {

        /**
         * GpuImage class (computing done on GPU)
         */
        template<class TPix>
        class KARABO_API GpuImage {


            template <class UPix> friend class GpuImage;

            unsigned int m_dimX;
            unsigned int m_dimY;
            unsigned int m_dimZ;

            karabo::util::Hash m_header;
            TPix* m_data;

        public:

            KARABO_CLASSINFO(GpuImage, "GpuImage", "1.0")

            /***************************************
             *            Constructors             *
             ***************************************/

            GpuImage() : m_dimX(0), m_dimY(0), m_dimZ(0), m_data(0) {
            }

            explicit GpuImage(const std::string& filename) :
                m_dimX(0), m_dimY(0), m_dimZ(0), m_data(0) {
                CpuImage<TPix> cpuImg(filename);
                GpuImage gpuImg(cpuImg);
                this->swap(gpuImg);
            }

            explicit GpuImage(const unsigned int dx, const unsigned int dy = 1, const unsigned int dz = 1) :
                m_dimX(dx), m_dimY(dy), m_dimZ(dz), m_data(0) {
                const size_t bytes = byteSize();
                if (bytes) {
                    CUDA_SAFE_CALL(cudaMalloc((void**) &m_data, bytes))
                }
            }

            GpuImage(const int dx, const int dy, const int dz, const TPix& value) :
                m_dimX(dx), m_dimY(dy), m_dimZ(dz), m_data(0) {
                const size_t bytes = byteSize();
                if (bytes) {
                    CUDA_SAFE_CALL(cudaMalloc((void**) &m_data, bytes))

                    // Fill the data-array with value
                    fill(value);
                }
            }

            GpuImage(const unsigned int dx, const unsigned int dy, const unsigned int dz, const std::string& values, const bool repeatValues) :
                m_dimX(dx), m_dimY(dy), m_dimZ(dz), m_data(0) {
                CpuImage<TPix> cpuImg(dx, dy, dz, values, repeatValues); // Construction through cpu image
                GpuImage gpuImg(cpuImg);
                this->swap(gpuImg);
            }

            // IMPORTANT: Host data buffer !!

            GpuImage(const TPix * const hostDataBuffer, const int dx, const int dy, const int dz) :
                m_dimX(dx), m_dimY(dy), m_dimZ(dz), m_data(0) {
                const unsigned int bytes = byteSize();
                if (bytes && hostDataBuffer) {
                    CUDA_SAFE_CALL(cudaMalloc((void**) &m_data, bytes))
                }
                CUDA_SAFE_CALL(cudaMemcpy(m_data, hostDataBuffer, bytes, cudaMemcpyHostToDevice))
            }

            GpuImage(const std::vector<TPix>& hostDataBuffer, const unsigned int dx, const unsigned int dy, const unsigned int dz) :
                m_dimX(dx), m_dimY(dy), m_dimZ(dz), m_data(0) {
                const unsigned int bytes = byteSize();
                if (bytes && !hostDataBuffer.empty()) {
                    CUDA_SAFE_CALL(cudaMalloc((void**) &m_data, bytes))
                }
                CUDA_SAFE_CALL(cudaMemcpy(m_data, &hostDataBuffer[0], bytes, cudaMemcpyHostToDevice))
            }

            /***************************************
             *          Copy-Constructors          *
             ***************************************/

            GpuImage(const GpuImage& image) {
                m_dimX = image.m_dimX;
                m_dimY = image.m_dimY;
                m_dimZ = image.m_dimZ;
                m_header = image.m_header;
                m_data = 0;

                const unsigned int bytes = byteSize();
                if (bytes) {
                    CUDA_SAFE_CALL(cudaMalloc((void **) &m_data, bytes));
                }
                CUDA_SAFE_CALL(cudaMemcpy(m_data, image.m_data, bytes, cudaMemcpyDeviceToDevice));
            }

            GpuImage(const CpuImage<TPix>& image) :
                m_dimX(0), m_dimY(0), m_dimZ(0), m_data(0) {
                GpuImage gpuImg(image.pixelPointer(), image.dimX(), image.dimY(), image.dimZ());
                this->swap(gpuImg);
            }

            template <class TImage>
            TImage to() {
                TImage tmp(this->dimX(), this->dimY(), this->dimZ());
                CUDA_SAFE_CALL(cudaMemcpy(tmp.pixelPointer(), this->pixelPointer(), this->byteSize(), cudaMemcpyDeviceToHost));
                return tmp;
            }

            /***************************************
             *        In-Place Construction        *
             ***************************************/

            inline GpuImage& assign() {
                if (m_data) {
                    CUDA_SAFE_CALL(cudaFree(m_data))
                }
                m_header.clear();
                m_dimX = m_dimY = m_dimZ = m_data = 0;
                return *this;
            }

            inline GpuImage& assign(const unsigned int dx = 0, const unsigned int dy = 1, const unsigned int dz = 1) {
                const unsigned int siz = dx * dy * dz;
                if (!siz) return assign();
                const unsigned int currentSize = size();
                if (siz != currentSize) {
                    if (m_data) {
                        CUDA_SAFE_CALL(cudaFree(m_data))
                    }
                    m_header.clear();
                    m_dimX = m_dimY = m_dimZ = m_data = 0;
                    CUDA_SAFE_CALL(cudaMalloc((void **) &m_data, siz * sizeof (TPix)));
                }
                m_dimX = dx;
                m_dimY = dy;
                m_dimZ = dz;
                return *this;
            }

            inline GpuImage& assign(const int dx, const int dy, const int dz, const TPix& value) {
                return assign(dx, dy, dz).fill(value);
            }

            inline GpuImage& assign(const int dx, const int dy, const int dz, const std::string& values, const bool repeatValues) {
                CpuImage<TPix> cpuImg(dx, dy, dz, values, repeatValues);
                GpuImage gpuImg(cpuImg);
                this->swap(gpuImg);
                return *this;
            }

            inline GpuImage& assign(const TPix * const dataBuffer, const int dx, const int dy, const int dz) {
                const unsigned int siz = dx * dy * dz;
                if (!dataBuffer || !siz) return assign();
                const unsigned int currentSize = size();
                if (dataBuffer == m_data && siz == currentSize) {
                    return assign(dx, dy, dz);
                } else {
                    TPix* newData = 0;
                    CUDA_SAFE_CALL(cudaMalloc((void **) &newData, siz * sizeof (TPix)));
                    m_header.clear();
                    m_dimX = m_dimY = m_dimZ = m_data = 0;
                    CUDA_SAFE_CALL(cudaMemcpy(newData, dataBuffer, siz * sizeof (TPix), cudaMemcpyHostToDevice));
                    CUDA_SAFE_CALL(cudaFree(m_data))
                    m_data = newData;
                    m_dimX = dx;
                    m_dimY = dy;
                    m_dimZ = dz;
                }
                return *this;
            }

            inline GpuImage& assign(const std::vector<TPix>& dataBuffer, const int dx, const int dy, const int dz) {
                return assign(&dataBuffer[0], dx, dy, dz);
            }

            /***************************************
             *     In-Place Copy-Construction      *
             ***************************************/

            inline GpuImage& assign(const GpuImage& image) {
                GpuImage tmp(image);
                this->swap(image);
                return *this;
            }

            inline GpuImage& assign(const CpuImage<TPix>& image) {
                GpuImage gpuImg(image);
                this->swap(gpuImg);
                return *this;
            }

            /***************************************
             *             Destructor              *
             ***************************************/

            virtual ~GpuImage() {
                if (m_data) {
                    CUDA_SAFE_CALL(cudaFree(m_data))
                }
            }

            /***************************************
             *         Special functions           *
             ***************************************/

            void swap(GpuImage& image) {
                boost::swap(m_dimX, image.m_dimX);
                boost::swap(m_dimY, image.m_dimY);
                boost::swap(m_dimZ, image.m_dimZ);
                m_header.swap(image.m_header);
                boost::swap(m_data, image.m_data);
            }

            void swap(const GpuImage& image) {
                GpuImage& tmp = const_cast<GpuImage&> (image);
                this->swap(tmp);
            }

            inline GpuImage& clear() {
                return assign();
            }

            GpuImage& read(const std::string& filename) {
                GpuImage tmp(filename);
                this->swap(tmp);
                return *this;
            }

            const GpuImage& write(const std::string& filename, const int number = -1) const {
                karabo::util::Hash h("AnyFormat.filename", filename, "AnyFormat.number", number);
                boost::shared_ptr<Output<CpuImage<TPix> > > out = Output<CpuImage<TPix> >::create(h);
                CpuImage<TPix> tmp(*this);
                out->write(tmp);
                return *this;
            }

            //! Offset function for data pointer position

            inline size_t offset(const unsigned int x = 0) const {
                return x;
            }

            inline size_t offset(const unsigned int x, const unsigned int y) const {
                return x + m_dimX * y;
            }

            inline size_t offset(const unsigned int x, const unsigned int y, const unsigned int z) const {
                return x + m_dimX * (y + m_dimY * z);
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

            inline int dimX() const {
                return (int) m_dimX;
            }

            inline int dimY() const {
                return (int) m_dimY;
            }

            inline int dimZ() const {
                return (int) m_dimZ;
            }

            const karabo::util::Hash& getHeader() const {
                updateHeader(*this);
                return m_header;
            }

            void setHeader(const karabo::util::Hash& header) {
                m_header = header;
            }

            inline size_t size() const {
                return m_dimX * m_dimY * m_dimZ;
            }

            inline size_t byteSize() const {
                return size() * sizeof (TPix);
            }

            inline const TPix* pixelPointer() const {
                return m_data;
            }

            inline TPix* pixelPointer() {
                return m_data;
            }

            inline std::string pixelType() const {
                return karabo::util::Types::getTypeAsString<TPix, karabo::util::Types::FORMAT_INTERN > ();
            }

            /***************************************
             *              Operators              *
             ***************************************/

            inline const TPix& operator()(const unsigned int x, const unsigned int y = 0, const unsigned int z = 0) const {
                return m_data[offset(x, y, z)];
            }

            inline TPix& operator()(const unsigned int x, const unsigned int y = 0, const unsigned int z = 0) {
                return m_data[offset(x, y, z)];
            }


            /***************************************
             *          Value Manipulation         *
             ***************************************/

            /**
             * *this function fills an image with values
             * @param value
             * @return 
             */
            GpuImage& fill(const TPix& value) {
                cudaFill(karabo::util::Types::getTypeAsId<TPix > (), m_data, (unsigned int) size(), &value);
                return *this;
            }



            /***************************************
             *          Value Manipulation         *
             ***************************************/

            /**
             * Prints image information to console
             * @param title Any custom title for the current image
             * @param displayPixels Should pixel information be displayed?
             * @param maxDimX Maximum numbers printed in X direction
             * @param maxDimY Maximum numbers printed in Y direction
             * @param maxDimZ Maximum numbers printed in Z direction
             * @return GpuImage
             */
            const GpuImage& print(const std::string& title = "", const bool displayPixels = true, int maxDimX = 28, int maxDimY = 28, int maxDimZ = 8) const {
                CpuImage<TPix> cpuImage(*this);
                cpuImage.print(title, displayPixels, maxDimX, maxDimY, maxDimZ);
                return *this;
            }


        private:

            void updateHeader(const GpuImage& image) const {
                GpuImage& tmp = const_cast<GpuImage&> (image);
                tmp.m_header.set("dimX", tmp.m_cimg.width());
                tmp.m_header.set("dimY", tmp.m_cimg.height());
                tmp.m_header.set("dimZ", tmp.m_cimg.depth());
            }
        };
    }
}

#endif
