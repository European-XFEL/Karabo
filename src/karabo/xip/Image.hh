/*
 * $Id$
 *
 * File:   Image.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 23, 2012, 12:42 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_IMAGE_HH
#define	KARABO_XIP_IMAGE_HH

#include "Environment.hh"


namespace karabo {
    namespace xip {

        enum ImageType {


            CPU,
            GPU
        };

        template <class TPix>
        class Image {


            typedef typename boost::shared_ptr<AbstractImage<TPix> > AbstractImagePointer;
            typedef typename boost::shared_ptr<Environment<TPix> > EnvironmentPointer;

            static EnvironmentPointer m_cpuEnvironment;
            static EnvironmentPointer m_cudaEnvironment;
            AbstractImagePointer m_img;

        public:

            /***************************************
             *            Constructors             *
             ***************************************/

            explicit Image(const AbstractImagePointer& img) : m_img(img) {
            }

            explicit Image(const int imageType) {
                if (imageType == CPU) m_img = initCpu()->image();
                else m_img = initCuda()->image();
            }

            Image(const int imageType, const std::string& filename) {
                if (imageType == CPU) m_img = initCpu()->image(filename);
                else m_img = initCuda()->image(filename);
            }

            Image(const int imageType, const size_t dx, const size_t dy = 1, const size_t dz = 1) {
                if (imageType == CPU) m_img = initCpu()->image(dx, dy, dz);
                else m_img = initCuda()->image(dx, dy, dz);
            }

            Image(const int imageType, const size_t dx, const size_t dy, const size_t dz, const TPix& value) {
                if (imageType == CPU) m_img = initCpu()->image(dx, dy, dz, value);
                else m_img = initCuda()->image(dx, dy, dz, value);
            }

            Image(const int imageType, const size_t dx, const size_t dy, const size_t dz, const std::string& values, const bool repeatValues) {
                if (imageType == CPU) m_img = initCpu()->image(dx, dy, dz, values, repeatValues);
                else m_img = initCuda()->image(dx, dy, dz, values, repeatValues);
            }

            Image(const int imageType, const TPix * const dataBuffer, const size_t dx, const size_t dy, const size_t dz) {
                if (imageType == CPU) m_img = initCpu()->image(dataBuffer, dx, dy, dz);
                else m_img = initCuda()->image(dataBuffer, dx, dy, dz);
            }

            Image(const int imageType, const std::vector<TPix>& dataBuffer, const size_t dx, const size_t dy, const size_t dz) {
                if (imageType == CPU) m_img = initCpu()->image(dataBuffer, dx, dy, dz);
                else m_img = initCuda()->image(dataBuffer, dx, dy, dz);
            }

            Image(const int imageType, const karabo::util::Hash& header) {
                if (imageType == CPU) m_img = initCpu()->image(header);
                else m_img = initCuda()->image(header);
            }

            Image(const int imageType, const karabo::util::Hash& header, const TPix& value) {
                if (imageType == CPU) m_img = initCpu()->image(header, value);
                else m_img = initCuda()->image(header, value);
            }

            /***************************************
             *          Copy-Constructors          *
             ***************************************/




            /***************************************
             *        In-Place Construction        *
             ***************************************/



            /***************************************
             *     In-Place Copy-Construction      *
             ***************************************/


            /***************************************
             *             Destructor              *
             ***************************************/


            /***************************************
             *         Special functions           *
             ***************************************/

            //inline void setImageType();

            /***************************************
             *      Instance Characteristics       *
             ***************************************/

            inline const int dimensionality() const {
                return m_img->dimensionality();
            }

            inline bool isEmpty() const {
                return m_img->isEmpty();
            }

            inline size_t dimX() const {
                return m_img->dimX();
            }

            inline size_t dimY() const {
                return m_img->dimY();
            }

            inline size_t dimZ() const {
                return m_img->dimZ();
            }

            const karabo::util::Hash& getHeader() const {
                return m_img->getHeader();
            }

            void setHeader(const karabo::util::Hash& header) {
                m_img->setHeader(header);
            }

            void setHeaderParam(const std::string& key, const char* const& value) {
                m_img->setHeaderParam(key, std::string(value));
            }

            void setHeaderParam(const std::string& key, const std::string& value) {
                m_img->setHeaderParam(key, value);
            }

            void setHeaderParam(const std::string& key, const bool value) {
                m_img->setHeaderParam(key, value);
            }

            void setHeaderParam(const std::string& key, const int value) {
                m_img->setHeaderParam(key, value);
            }

            void setHeaderParam(const std::string& key, const double value) {
                m_img->setHeaderParam(key, value);
            }

            inline size_t size() const {
                return m_img->size();
            }

            inline size_t byteSize() const {
                return m_img->byteSize();
            }

            inline std::string pixelType() const {
                return m_img->pixelType();
            }

            Statistics getStatistics() const {
                return m_img->getStatistics();
            }

            Image& print(const std::string& title = "", const bool displayPixels = true, int maxDimX = 28, int maxDimY = 28, int maxDimZ = 8) {
                m_img->print();
                return *this;
            }

            /***************************************
             *              Operators              *
             ***************************************/

            inline const TPix& operator[](const size_t offset) const {
                return (*m_img)[offset];
            }

            inline TPix& operator[](const size_t offset) {
                return (*m_img)[offset];
            }

            inline const TPix& operator()(const size_t x, const size_t y = 0, const size_t z = 0) const {
                return (*m_img)(x, y, z);
            }

            inline TPix& operator()(const size_t x, const size_t y = 0, const size_t z = 0) {
                return (*m_img)(x, y, z);
            }

            /***************************************
             *        Convenience Functions        *
             ***************************************/

            /**
             * Computes the sum of all pixels
             * @return total sum of all pixels
             */
            inline double getSum() const {
                return m_img->getSum();
            }

            inline double getMean() const {
                return m_img->getMean();
            }



        private:

            const EnvironmentPointer& initCpu() {
                if (!m_cpuEnvironment) {
                    m_cpuEnvironment = karabo::util::Factory<Environment<TPix> >::create("cpu");
                    //m_cpuEnvironment->printInfo();
                }
                return m_cpuEnvironment;
            }

            const EnvironmentPointer& initCuda() {
                if (!m_cudaEnvironment) {
                    try {
                        m_cudaEnvironment = karabo::util::Factory<Environment<TPix> >::create("cuda");
                        //m_cudaEnvironment->printInfo();
                    } catch (karabo::util::Exception&) {
                        std::cout << "Falling back to CPU version" << std::endl;
                        m_cudaEnvironment = initCpu();
                    }
                }
                return m_cudaEnvironment;
            }

        };

        template<class TPix>
        typename Image<TPix>::EnvironmentPointer Image<TPix>::m_cpuEnvironment = typename Image<TPix>::EnvironmentPointer();

        template<class TPix>
        typename Image<TPix>::EnvironmentPointer Image<TPix>::m_cudaEnvironment = typename Image<TPix>::EnvironmentPointer();

        typedef Image<float> ImageF;
        typedef Image<double> ImageD;
    }
}



#endif

