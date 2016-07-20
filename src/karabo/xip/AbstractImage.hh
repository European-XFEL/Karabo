/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 26, 2012, 8:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_ABSTRACTIMAGE_HH
#define KARABO_XIP_ABSTRACTIMAGE_HH

#include <karabo/util/Configurator.hh>
#include <karabo/util/Types.hh>
#include <karabo/util/FromTypeInfo.hh>
#include <karabo/util/ToLiteral.hh>

#include "Statistics.hh"

namespace karabo {

    namespace xip {

        struct AbstractImageType {

            template <class T>
            static std::string classId() {
                using namespace karabo::util;
                return "AbstractImage" + Types::convert<FromTypeInfo, ToLiteral > (typeid (T));
            }
        };

        /**
         * Image class (computing done on CPU)
         */
        template<class TPix>
        class AbstractImage {

            public:

            KARABO_CLASSINFO(AbstractImage, AbstractImageType::classId<TPix>(), "1.0")

            /***************************************
             *        In-Place Construction        *
             ***************************************/

            virtual AbstractImage& assign() {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& assign(const size_t dx, const size_t dy = 1, const size_t dz = 1) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& assign(const int dx, const int dy, const int dz, const TPix& value) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& assign(const int dx, const int dy, const int dz, const std::string& values, const bool repeatValues) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& assign(const TPix * const dataBuffer, const int dx, const int dy, const int dz) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& assign(const std::vector<TPix>& dataBuffer, const int dx, const int dy, const int dz) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
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
            inline AbstractImage& assign(const AbstractImage<UPix>& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& assign(const AbstractImage& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            /***************************************
             *             Destructor              *
             ***************************************/

            virtual ~AbstractImage() {
            }

            /***************************************
             *         Special functions           *
             ***************************************/

            virtual void swap(AbstractImage& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
            }

            virtual void swap(const AbstractImage& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
            }

            /**
             * Moves the content of the instance image into another one in a way that memory copies are avoided if possible.
             * 
             * CAVEAT: The instance image is always empty after a call to this function.
             * 
             * @param image The image this instance should be moved into
             * @return The new image
             */
            virtual AbstractImage& moveTo(AbstractImage& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual inline AbstractImage& clear() {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& read(const std::string& filename) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual const AbstractImage& write(const std::string& filename, const int number = -1) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual size_t offset(const size_t x, const size_t y = 0, const size_t z = 0) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            /***************************************
             *      Instance Characteristics       *
             ***************************************/

            virtual const int dimensionality() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual bool isEmpty() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual size_t dimX() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual size_t dimY() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual size_t dimZ() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual const karabo::util::Hash& getHeader() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static karabo::util::Hash h;
                return h;
            }

            virtual void setHeader(const karabo::util::Hash& header) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");

            }

            // This is a case where C++ should allow templated virtual functions!
            // I think not supporting this is a shortcoming in the language!

            virtual void setHeaderParam(const std::string& key, const std::string& value) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");

            }

            virtual void setHeaderParam(const std::string& key, const bool value) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");

            }

            virtual void setHeaderParam(const std::string& key, const int value) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");

            }

            virtual void setHeaderParam(const std::string& key, const double value) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");

            }

            virtual size_t size() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual size_t byteSize() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual std::string pixelType() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual Statistics getStatistics() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return Statistics();
            }

            /***************************************
             *              Operators              *
             ***************************************/

            virtual const TPix& operator()(const size_t x, const size_t y = 0, const size_t z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix& operator()(const size_t x, const size_t y = 0, const size_t z = 0) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual const TPix& operator[](const size_t offset) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix& operator[](const size_t offset) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            /**
             * Operator() Pixel-Buffer access [const]
             * @return Address of pixel buffer
             */
            virtual operator const TPix*() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            /**
             * Operator() 
             * @return Address of pixel buffer
             */
            virtual operator TPix*() {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            /**
             * Operator=()
             * Assignment operator. Fill all pixels of the instance image with the same value.
             * The image size is not modified.
             * @param val
             * @return Image
             */
            virtual AbstractImage& operator=(const TPix& val) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            /**
             * Operator=().
             * Assignment operator.
             * Replace the image content by the content of the argument image.
             *
             * @param image Another image
             * @return Image 
             */
            virtual AbstractImage& operator=(const AbstractImage& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            /**************************
             *        Operator +      *
             **************************/

            virtual AbstractImage& operator+=(const TPix& value) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& operator+=(const AbstractImage& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            //virtual AbstractImage& operator++() {
            //}

            virtual AbstractImage& operator+(const TPix& value) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& operator+(const AbstractImage& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            /**************************
             *        Operator -      *
             **************************/

            virtual AbstractImage& operator-=(const TPix& value) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& operator-=(const AbstractImage& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& operator-(const TPix& value) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            virtual AbstractImage& operator-(const AbstractImage& image) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return *this;
            }

            /***************************************
             *             Pixel Access            *  
             ***************************************/

            virtual const TPix* pixelPointer() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual TPix* pixelPointer() {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            /**
             * Read a pixel value with Dirichlet boundary conditions.
             * @param value
             * @return 
             */
            virtual TPix& at(const int offset, const TPix beyondBorderValue) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix at(const int offset, const TPix beyondBorderValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            //! Read a pixel value with Neumann boundary conditions.

            virtual TPix& at(const int offset) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix at(const int offset) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            //! Read a pixel value with Dirichlet boundary conditions for the first coordinates (\c x).

            virtual TPix& atX(const int x, const int y, const int z, const TPix beyondBoundaryValue) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix atX(const int x, const int y, const int z, const TPix beyondBoundaryValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            //! Read a pixel value with Neumann boundary conditions for the first coordinates (\c x).

            virtual TPix& atX(const int x, const int y = 0, const int z = 0) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix atX(const int x, const int y = 0, const int z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual TPix& atXY(const int x, const int y, const int z, const TPix beyondBoundaryValue) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix atXY(const int x, const int y, const int z, const TPix beyondBoundaryValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual TPix& atXY(const int x, const int y = 0, const int z = 0) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix atXY(const int x, const int y = 0, const int z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual TPix& atXYZ(const int x, const int y, const int z, const TPix beyondBoundaryValue) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix atXYZ(const int x, const int y, const int z, const TPix beyondBoundaryValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            //! Read a pixel value with Neumann boundary conditions for the first coordinates (\c x).

            virtual TPix& atXYZ(const int x, const int y = 0, const int z = 0) {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                static TPix t;
                return t;
            }

            virtual TPix atXYZ(const int x, const int y = 0, const int z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double linearAtX(const float fx, const int y, const int z, const double beyondBoundaryValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double linearAtX(const float fx, const int y = 0, const int z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double linearAtXY(const float fx, const float y, const int z, const double beyondBoundaryValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double linearAtXY(const float fx, const float y = 0, const int z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double linearAtXYZ(const float fx, const float y, const float z, const double beyondBoundaryValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double linearAtXYZ(const float fx, const float y = 0, const float z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double cubicAtX(const float fx, const int y, const int z, const double beyondBoundaryValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double cubicAtX(const float fx, const int y = 0, const int z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double cubicAtXY(const float fx, const float y, const int z, const double beyondBoundaryValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double cubicAtXY(const float fx, const float y = 0, const int z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double cubicAtXYZ(const float fx, const float y, const float z, const double beyondBoundaryValue) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double cubicAtXYZ(const float fx, const float y = 0, const float z = 0) const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            /***************************************
             *        Convenience Functions        *
             ***************************************/

            /**
             * Computes the sum of all pixels
             * @return total sum of all pixels
             */
            virtual double getSum() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }

            virtual double getMean() const {
                KARABO_NOT_SUPPORTED_EXCEPTION("Function call not supported by the underlying image implementation");
                return 0;
            }



            /***************************************
             *          Value Manipulation         *
             ***************************************/

            /**
             * *this function fills an image with values
             * @param value
             * @return 
             */
            //            AbstractImage& fill(const TPix& value) {
            //                m_cimg.fill(value);
            //                return *this;
            //            }
            //
            //            AbstractImage process(const std::string& processorName, const karabo::util::Hash& configuration = karabo::util::Hash()) {
            //                karabo::util::Hash h(processorName, configuration);
            //                typename SingleProcessor< AbstractImage >::Pointer p = SingleProcessor< AbstractImage>::create(h);
            //                return p->process(*this);
            //            }
            //
            //            AbstractImage& processInPlace(const std::string& processorName, const karabo::util::Hash& configuration = karabo::util::Hash()) {
            //                karabo::util::Hash h(processorName, configuration);
            //                typename SingleProcessor< AbstractImage >::Pointer p = SingleProcessor< AbstractImage>::create(h);
            //                p->processInPlace(*this);
            //                return *this;
            //            }
            //
            //            AbstractImage& randomize(const TPix& valueMin, const TPix& valueMax) {
            //                m_cimg.rand(valueMin, valueMax);
            //                return *this;
            //            }
            //
            //            AbstractImage getRandomize(const TPix& valueMin, const TPix& valueMax) {
            //                return AbstractImage(m_cimg.get_rand(valueMin, valueMax));
            //            }
            //
            //            AbstractImage& permuteAxis(const std::string& order) {
            //                std::string fullOrder = order + "c";
            //                m_cimg.permute_axes(fullOrder.c_str());
            //                return *this;
            //            }
            //
            //            AbstractImage& getPermuteAxis(const std::string& order) {
            //                AbstractImage ret(*this);
            //                return ret.permuteAxis(order);
            //            }

            /**
             * Prints image information to console
             * @param title Any custom title for the current image
             * @param displayPixels Should pixel information be displayed?
             * @param maxDimX Maximum numbers printed in X direction
             * @param maxDimY Maximum numbers printed in Y direction
             * @param maxDimZ Maximum numbers printed in Z direction
             * @return Image
             */
            virtual const AbstractImage& print(const std::string& title = "", const bool displayPixels = true, int maxDimX = 28, int maxDimY = 28, int maxDimZ = 8) const = 0;

            /***************************************
             *              Display                *
             ***************************************/

            virtual void display(const std::string& title) {
            }

            virtual void displayAndKeep(const std::string& title) {
            }

            virtual void display3dVectors(const std::string& title) {
            }

            virtual void displayAndKeep3dVectors(const std::string& title) {
            }

            virtual void display3dVolume(const std::string& title, const float isoValue) {
            }

            virtual void display3dVolume(const std::string& title) {
            }

            virtual void displayAndKeep3dVolume(const std::string& title, const float isoValue) {
            }

            virtual void displayAndKeep3dVolume(const std::string& title) {
            }
        };
    }
}

#endif
