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

#include <karabo/util/Factory.hh>

#include "Statistics.hh"

namespace karabo {

    namespace xip {

        struct AbstractImageType {

            template <class T>
            static std::string classId() {
                return "AbstractImage" + karabo::util::Types::getTypeAsString<T, karabo::util::Types::FORMAT_INTERN > ();
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
            }

            virtual AbstractImage& assign(const size_t dx, const size_t dy = 1, const size_t dz = 1) {
            }

            virtual AbstractImage& assign(const int dx, const int dy, const int dz, const TPix& value) {
            }

            virtual AbstractImage& assign(const int dx, const int dy, const int dz, const std::string& values, const bool repeatValues) {
            }

            virtual AbstractImage& assign(const TPix * const dataBuffer, const int dx, const int dy, const int dz) {
            }

            virtual AbstractImage& assign(const std::vector<TPix>& dataBuffer, const int dx, const int dy, const int dz) {
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
            }

            virtual AbstractImage& assign(const AbstractImage& image) {
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
            }

            virtual void swap(const AbstractImage& image) {
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
            }

            virtual inline AbstractImage& clear() {
            }

            virtual AbstractImage& read(const std::string& filename) {
            }

            virtual const AbstractImage& write(const std::string& filename, const int number = -1) const {
            }

            virtual size_t offset(const size_t x, const size_t y = 0, const size_t z = 0) {
            }

            /***************************************
             *      Instance Characteristics       *
             ***************************************/

            virtual const int dimensionality() const {
            }

            virtual bool isEmpty() const {
            }

            virtual size_t dimX() const {
            }

            virtual size_t dimY() const {
            }

            virtual size_t dimZ() const {
            }

            virtual const karabo::util::Hash& getHeader() const {
            }

            virtual void setHeader(const karabo::util::Hash& header) {
            }

            virtual size_t size() const {
            }

            virtual size_t byteSize() const {
            }

            virtual std::string pixelType() const {
            }

            virtual Statistics getStatistics() const {
            }

            /***************************************
             *              Operators              *
             ***************************************/

            virtual const TPix& operator()(const size_t x, const size_t y = 0, const size_t z = 0) const {
            }

            virtual TPix& operator()(const size_t x, const size_t y = 0, const size_t z = 0) {
            }

            virtual const TPix& operator[](const size_t offset) const {
            }

            virtual TPix& operator[](const size_t offset) {
            }

            /**
             * Operator() Pixel-Buffer access [const]
             * @return Address of pixel buffer
             */
            virtual operator const TPix*() const {
            }

            /**
             * Operator() 
             * @return Address of pixel buffer
             */
            virtual operator TPix*() {
            }

            /**
             * Operator=()
             * Assignment operator. Fill all pixels of the instance image with the same value.
             * The image size is not modified.
             * @param val
             * @return Image
             */
            virtual AbstractImage& operator=(const TPix& val) {
            }

            /**
             * Assignment operator.
             * If \p expression is a formula or a list of values, the image pixels are filled
             * according to the expression and the image size is not modified.
             * If \p expression is a filename, the image is replaced by the input file data
             * (so image size is modified).
             * @param expression
             * @return Image
             */
            virtual AbstractImage& operator=(const std::string& expression) {
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
            }

            /**
             * Operator+=();
             */
            virtual AbstractImage& operator+=(const TPix& value) {
            }

            virtual AbstractImage& operator+=(const std::string& expression) {
            }

            virtual AbstractImage& operator+=(const AbstractImage& image) {
            }

            virtual AbstractImage& operator++() {
            }

            virtual AbstractImage& operator+(const TPix& value) const {
            }

            virtual AbstractImage& operator+(const std::string& expression) {
            }

            virtual AbstractImage& operator+(const AbstractImage& image) {
            }

            /***************************************
             *             Pixel Access            *  
             ***************************************/

            virtual const TPix* pixelPointer() const {
            }

            virtual TPix* pixelPointer() {
            }

            /**
             * Read a pixel value with Dirichlet boundary conditions.
             * @param value
             * @return 
             */
            virtual TPix& at(const int offset, const TPix beyondBorderValue) {
            }

            virtual TPix at(const int offset, const TPix beyondBorderValue) const {
            }

            //! Read a pixel value with Neumann boundary conditions.

            virtual TPix& at(const int offset) {
            }

            virtual TPix at(const int offset) const {
            }

            //! Read a pixel value with Dirichlet boundary conditions for the first coordinates (\c x).

            virtual TPix& atX(const int x, const int y, const int z, const TPix beyondBoundaryValue) {
            }

            virtual TPix atX(const int x, const int y, const int z, const TPix beyondBoundaryValue) const {
            }

            //! Read a pixel value with Neumann boundary conditions for the first coordinates (\c x).

            virtual TPix& atX(const int x, const int y = 0, const int z = 0) {
            }

            virtual TPix atX(const int x, const int y = 0, const int z = 0) const {
            }

            virtual TPix& atXY(const int x, const int y, const int z, const TPix beyondBoundaryValue) {
            }

            virtual TPix atXY(const int x, const int y, const int z, const TPix beyondBoundaryValue) const {
            }

            virtual TPix& atXY(const int x, const int y = 0, const int z = 0) {
            }

            virtual TPix atXY(const int x, const int y = 0, const int z = 0) const {
            }

            virtual TPix& atXYZ(const int x, const int y, const int z, const TPix beyondBoundaryValue) {
            }

            virtual TPix atXYZ(const int x, const int y, const int z, const TPix beyondBoundaryValue) const {
            }

            //! Read a pixel value with Neumann boundary conditions for the first coordinates (\c x).

            virtual TPix& atXYZ(const int x, const int y = 0, const int z = 0) {

            }

            virtual TPix atXYZ(const int x, const int y = 0, const int z = 0) const {

            }

            virtual double linearAtX(const float fx, const int y, const int z, const double beyondBoundaryValue) const {

            }

            virtual double linearAtX(const float fx, const int y = 0, const int z = 0) const {

            }

            virtual double linearAtXY(const float fx, const float y, const int z, const double beyondBoundaryValue) const {

            }

            virtual double linearAtXY(const float fx, const float y = 0, const int z = 0) const {

            }

            virtual double linearAtXYZ(const float fx, const float y, const float z, const double beyondBoundaryValue) const {

            }

            virtual double linearAtXYZ(const float fx, const float y = 0, const float z = 0) const {

            }

            virtual double cubicAtX(const float fx, const int y, const int z, const double beyondBoundaryValue) const {

            }

            virtual double cubicAtX(const float fx, const int y = 0, const int z = 0) const {

            }

            virtual double cubicAtXY(const float fx, const float y, const int z, const double beyondBoundaryValue) const {

            }

            virtual double cubicAtXY(const float fx, const float y = 0, const int z = 0) const {

            }

            virtual double cubicAtXYZ(const float fx, const float y, const float z, const double beyondBoundaryValue) const {

            }

            virtual double cubicAtXYZ(const float fx, const float y = 0, const float z = 0) const {

            }

            /***************************************
             *        Convenience Functions        *
             ***************************************/

            /**
             * Computes the sum of all pixels
             * @return total sum of all pixels
             */
            virtual double getSum() const {
            }

            virtual double getMean() const {
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
            virtual AbstractImage& print(const std::string& title = "", const bool displayPixels = true, int maxDimX = 28, int maxDimY = 28, int maxDimZ = 8) {
            }

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
