#ifndef EXFEL_XIP_CPUIMAGE_HH
#define EXFEL_XIP_CPUIMAGE_HH

#include "AbstractImage.hh"
#include "CImg.h"


//#include "../core/cudaImage.h"
//#include "../util/util.h"
namespace exfel {

    namespace xip {

        //! Data specialization for CPU computing

        template<class T>
        class EXFEL_API Image : public AbstractImage {


            //friend class CudaImage<T> ;

        public:

            /***************************************
             *            Constructors             *
             ***************************************/

            //! Standard Data Constructor
            /*!
             * @param dx image width
             * @param dy image height
             * @param dz image depth
             */
            explicit CpuImage(const unsigned int dx = 0, const unsigned int dy = 1, const unsigned int dz = 1);

            //! Data Constructor
            /*
             * @param dx image width
             * @param dy image height
             * @param dz image depth
             * @param value default value to fill the image
             */
            explicit CpuImage(const int dx, const int dy, const int dz, const T& value);

            //! Data Constructor
            /*
             * Given an array (motif) of values, those will be used to fill the image
             * @param dx image width
             * @param dy image height
             * @param dz image depth
             * @param values vector of values (motif) to fill the image
             */
            explicit CpuImage(const int dx, const int dy, const int dz, const vector<T>& values);

            //! Simple copy constructor
            CpuImage<T> (const Image<T>& image);

            //! Foreign copy Constructor
            CpuImage<T> (const CudaImage<T>& image);

            //! Copy Constructor form other type
            template<class Ts>
            CpuImage<T> (const Image<Ts>& image);

            template<class Ts>
            CpuImage<T> (const CudaImage<Ts>& image) {
                m_width = 0;
                _height = 0;
                _depth = 0;
                m_data = NULL;
                _isValid = false;

                Image<T>(CudaImage<T>(image)).swap(*this);
            }

            /***************************************
             *             Destructor              *
             ***************************************/

            virtual ~Image();

            /***************************************
             *       Public Member Functions        *
             ***************************************/
            //! This function fills an image with values
            const Image& fill(const T& value);

            //! Given an array (motif) of values, those will be used to fill the image
            const Image& fill(const vector<T>& values);

            //! Get pixel type

            inline TEMPLATE_TYPE getPixelType() const {
                return getTemplateType<T> ();
            }

            //! Puts image to console
            void format() const;

            //! Data swapping
            void swap(Image<T>& image);

            /*
             * *********************************************************
             * neighbour interpolated
             * *********************************************************
             */

            //! give a 1d neighbour interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param y source y-value determinate the line for 1d interpolation
             * @param z source z-value determinate the line for 1d interpolation
             * @return 1d neighbour interpolated value
             */
            inline T neighbourPix1D(const REAL ffx, const int y = 0, const int z = 0) const;

            //! give a 2d neighbour interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param z source z-value give the plane for 2d interpolation
             * @return 2d neighbour interpolated value
             */
            inline T neighbourPix2D(const REAL ffx, const REAL ffy = 0, const int z = 0) const;

            //! give a 3d neighbour interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param ffz source z-value of the to interpolated point
             * @return 3d neighbour interpolated value
             */
            inline T neighbourPix3D(const REAL ffx, const REAL ffy = 0, const REAL ffz = 0) const;

            //! basic cubic interpolation function
            /*!
             * @param p0 is the value on point f(-1)
             * @param p1 is the value on point f( 0)
             * @param p2 is the value on point f( 1)
             * @param p3 is the value on point f( 2)
             * @param dp is the displace of the to interpolated value
             * @return cubic interpolated value
             */

            //! give a 1d neighbour interpolated value with  Border condition Nearest neighbors
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param y source y-value determinate the line for 1d interpolation
             * @param z source z-value determinate the line for 1d interpolation
             * if ffx outside the image ranges then the nearest pixel value are usesd
             * to interpolate the result
             * @return 1d neighbour interpolated value
             */
            inline T neighbourPix1D_Nearest(const REAL ffx, const int y = 0, const int z = 0) const;

            //! give a 2d neighbour interpolated value with  Border condition Nearest neighbors
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param z source z-value give the plane for 2d interpolation
             * if ffx or ffy outside the image ranges then the nearest pixel value are usesd
             * to interpolate the result
             * @return 2d neighbour interpolated value
             */
            inline T neighbourPix2D_Nearest(const REAL ffx, const REAL ffy = 0, const int z = 0) const;

            //! give a 3d neighbour interpolated value with Border condition Nearest neighbors
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param ffz source z-value of the to interpolated point
             * if ffx, ffy or ffz outside the image ranges then the nearest pixel value are usesd
             * to interpolate the result
             * @return 3d neighbour interpolated value
             */
            inline T neighbourPix3D_Nearest(const REAL ffx, const REAL ffy = 0, const REAL ffz = 0) const;

            //! give a 1d neighbour interpolated value with  Border condition Nearest neighbors
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param y source y-value determinate the line for 1d interpolation
             * @param z source z-value determinate the line for 1d interpolation
             * if ffx outside the image ranges then the nearest pixel value are usesd
             * to interpolate the result
             * @return 1d neighbour interpolated value
             */
            inline T neighbourPix1D_Repeat(const REAL ffx, const int y = 0, const int z = 0) const;

            //! give a 2d neighbour interpolated value with Border condition Repeat Pattern
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param z source z-value give the plane for 2d interpolation
             * if ffx or ffy outside the image ranges then the nearest pixel value are usesd
             * to interpolate the result
             * @return 2d neighbour interpolated value
             */
            inline T neighbourPix2D_Repeat(const REAL ffx, const REAL ffy = 0, const int z = 0) const;

            //! give a 3d neighbour interpolated value with Border condition Nearest neighbors
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param ffz source z-value of the to interpolated point
             * if ffx, ffy or ffz outside the image ranges then the nearest pixel value are usesd
             * to interpolate the result
             * @return 3d neighbour interpolated value
             */
            inline T neighbourPix3D_Repeat(const REAL ffx, const REAL ffy = 0, const REAL ffz = 0) const;

            /*
             * *********************************************************
             * linear interpolated
             * *********************************************************
             */

            //! basic linear interpolation function
            /*!
             * @param p0 is the value on point f( 0)
             * @param p1 is the value on point f( 1)
             * @param dp is the displace of the to interpolated value
             * @return cubic interpolated value
             */
            inline T linear(const T p0, const T p1, const T dp) const;

            //! give a 1d linear interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param y source y-value determinate the line for 1d interpolation
             * @param z source z-value determinate the line for 1d interpolation
             * @return linear interpolated value
             */
            inline T linearPix1D(const REAL ffx, const int y = 0, const int z = 0) const;

            //! give a 2d linear interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param z source z-value give the plane for 2d interpolation
             * @return bilinear interpolated value
             */
            inline T linearPix2D(const REAL ffx, const REAL ffy = 0, const int z = 0) const;

            //! give a 3d linear interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param ffz source z-value of the to interpolated point
             * @return trilinear interpolated value
             */
            inline T linearPix3D(const REAL ffx, const REAL ffy = 0, const REAL ffz = 0) const;

            //! give a 1d linear interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param y source y-value determinate the line for 1d interpolation
             * @param z source z-value determinate the line for 1d interpolation
             * @return linear interpolated value
             */
            inline T linearPix1D_Nearest(const REAL ffx, const int y = 0, const int z = 0) const;

            //! give a 2d linear interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param z source z-value give the plane for 2d interpolation
             * @return bilinear interpolated value
             */
            inline T linearPix2D_Nearest(const REAL ffx, const REAL ffy = 0, const int z = 0) const;

            //! give a 3d linear interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param ffz source z-value of the to interpolated point
             * @return trilinear interpolated value
             */
            inline T linearPix3D_Nearest(const REAL ffx, const REAL ffy = 0, const REAL ffz = 0) const;

            //! give a 1d linear interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param y source y-value determinate the line for 1d interpolation
             * @param z source z-value determinate the line for 1d interpolation
             * @return linear interpolated value
             */
            inline T linearPix1D_Repeat(const REAL ffx, const int y = 0, const int z = 0) const;

            //! give a 2d linear interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param z source z-value give the plane for 2d interpolation
             * @return bilinear interpolated value
             */
            inline T linearPix2D_Repeat(const REAL ffx, const REAL ffy = 0, const int z = 0) const;

            //! give a 3d linear interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param ffz source z-value of the to interpolated point
             * @return trilinear interpolated value
             */
            inline T linearPix3D_Repeat(const REAL ffx, const REAL ffy = 0, const REAL ffz = 0) const;

            /*
             * *********************************************************
             * cubic interpolated
             * *********************************************************
             */

            //! basic cubic interpolation function
            /*!
             * @param p0 is the value on point f(-1)
             * @param p1 is the value on point f( 0)
             * @param p2 is the value on point f( 1)
             * @param p3 is the value on point f( 2)
             * @param dp is the displace of the to interpolated value
             * @return cubic interpolated value
             */
            inline T cubic(const T p0, const T p1, const T p2, const T p3, const T dp) const;

            //! basic cubic interpolation function
            /*!
             * @param p0 is the value on point f(-1)
             * @param p1 is the value on point f( 0)
             * @param p2 is the value on point f( 1)
             * @param p3 is the value on point f( 2)
             * @param dp is the displace of the to interpolated value
             * @return cubic interpolated value
             */
            inline T cubic_bspline(const T p0, const T p1, const T p2, const T p3, const T dp) const;

            //! give a 1d cubic interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param y source y-value determinate the line for 1d interpolation
             * @param z source z-value determinate the line for 1d interpolation
             * @return cubic interpolated value
             */
            inline T cubicPix1D(const REAL ffx, const int y = 0, const int z = 0) const;

            //! give a 2d cubic interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param z source z-value give the plane for 2d interpolation
             * @return bicubic interpolated value
             */
            inline T cubicPix2D(const REAL ffx, const REAL ffy = 0, const int z = 0) const;

            //! give a 3d cubic interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param ffz source z-value of the to interpolated point
             * @return tricubic interpolated value
             */
            inline T cubicPix3D(const REAL ffx, const REAL ffy = 0, const REAL ffz = 0) const;

            //! give a 1d cubic interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param y source y-value determinate the line for 1d interpolation
             * @param z source z-value determinate the line for 1d interpolation
             * @return cubic interpolated value
             */
            inline T cubicPix1D_Nearest(const REAL ffx, const int y = 0, const int z = 0) const;

            //! give a 2d cubic interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param z source z-value give the plane for 2d interpolation
             * @return bicubic interpolated value
             */
            inline T cubicPix2D_Nearest(const REAL ffx, const REAL ffy = 0, const int z = 0) const;

            //! give a 3d cubic interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param ffz source z-value of the to interpolated point
             * @return tricubic interpolated value
             */
            inline T cubicPix3D_Nearest(const REAL ffx, const REAL ffy = 0, const REAL ffz = 0) const;
            //! give a 1d cubic interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param y source y-value determinate the line for 1d interpolation
             * @param z source z-value determinate the line for 1d interpolation
             * @return cubic interpolated value
             */
            inline T cubicPix1D_Repeat(const REAL ffx, const int y = 0, const int z = 0) const;

            //! give a 2d cubic interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param z source z-value give the plane for 2d interpolation
             * @return bicubic interpolated value
             */
            inline T cubicPix2D_Repeat(const REAL ffx, const REAL ffy = 0, const int z = 0) const;

            //! give a 3d cubic interpolated value
            /*!
             * @param ffx source x-value of the to interpolated point
             * @param ffy source y-value of the to interpolated point
             * @param ffz source z-value of the to interpolated point
             * @return tricubic interpolated value
             */
            inline T cubicPix3D_Repeat(const REAL ffx, const REAL ffy = 0, const REAL ffz = 0) const;

            //! creates a circular mask
            void circularMask(REAL fraction = 1.0);
            //! creates and returns a circular mask
            Image<T>* getCircularMask(REAL fraction = 1.0) const;

            //! Data translation
            /*! This function translates the COORDINATE SYSTEM along the given axis.
             * i.e. a translation of mx = 1 would move the origin of the coordinate system 1px to the right
             * thus the image appears to be moved to the left
             * Unknown pixels are filled with 0's
             */
            void translate(const REAL mx, const REAL my, const REAL mz, const INTERPOLATION_TYPE interp = LINEAR);
            Image<T>* getTranslate(const REAL mx, const REAL my, const REAL mz, const INTERPOLATION_TYPE interp = LINEAR) const;


            //! Data transformation
            /*! This function transforms the input image given the matrix mat. It is possible to translate, rotate, and scale
             * in one step. This method doesn't change the image pixel frame, i.e. image pixels which are rotated out of the original frame are
             discarded. Unknown pixels are filled with zeros.
             * @param mat A 4x4 transformation matrix
             * @param INTERPOLATION_TYPE The interpolation to use
             * @param borderCondition Border condition.
             *   borderCondition may be :
             *     - 0 : Zero border condition.
             *     - 1 : Nearest neighbour.
             *     - 2 : Repeat Pattern (like fftw).
             */
            void transform(const math::Mat4x4& mat, const INTERPOLATION_TYPE interp = LINEAR, const int borderCondition = 0);
            Image<T>* getTransform(const math::Mat4x4& mat, const INTERPOLATION_TYPE interp = LINEAR, const int borderCondition = 0) const;

            //! Rotates the image inplace. Does fast rotation for 0�, 90�, 180�, 270�, 360�
            /*!
             * @param angle in degree. Range is from 0 to 360.
             * @param interpolationType defines type of interpolation. Defaults to bilinear interpolation.
             * @param dir defines rotation direction. clockwise (CW) or counterclockwise (CCW). Defaults to CW.
             */
            void rotateZ(REAL angle, const INTERPOLATION_TYPE interpolationType = LINEAR);

            //! Creates a copy of the image and rotates it. Does fast rotation for 0�, 90�, 180�, 270�, 360�
            /*!
             * @param angle in degree. Range is from 0 to 360.
             * @param interpolationType defines type of interpolation. Defaults to bilinear interpolation.
             * @param dir defines rotation direction. clockwise (CW) or counterclockwise (CCW). Defaults to CW.
             * @return A new rotated image.
             */
            Image<T>* getRotateZ(REAL angle, const INTERPOLATION_TYPE interpolationType = LINEAR) const;

            //! Three dimensional inplace rotation.
            /*!
             * @param alpha in degree. Range is from 0 to 360.
             * @param beta in degree. Range is from 0 to 360.
             * @param gamma in degree. Range is from 0 to 360.
             * @param interpolationType defines type of interpolation. Defaults to bilinear interpolation.
             * @return A new rotated image.
             */
            void rotate(REAL alpha, REAL beta, REAL gamma, INTERPOLATION_TYPE interpolationType = LINEAR);

            //! Creates a copy of the image and make a three dimensional rotation.
            /*!
             * @param alpha in degree. Range is from 0 to 360.
             * @param beta in degree. Range is from 0 to 360.
             * @param gamma in degree. Range is from 0 to 360.
             * @param interpolationType defines type of interpolation. Defaults to bilinear interpolation.
             * @return A new rotated image.
             */
            Image<T>* getRotate(REAL alpha, REAL beta, REAL gamma, INTERPOLATION_TYPE interpolationType = LINEAR) const;

            //! Creates a copy of the image and make a three dimensional rotation.
            /*!
             * @param alpha in degree. Range is from 0 to 360.
             * @param beta in degree. Range is from 0 to 360.
             * @param gamma in degree. Range is from 0 to 360.
             * @param interpolationType defines type of interpolation. Defaults to bilinear interpolation.
             * @return A new rotated image.
             */
            //    template<INTERPOLATION_TYPE INTERP>
            //    CpuImage<T>* _getRotate(REAL alpha, REAL beta, REAL gamma) const;

            //! Data resizing in per-cent
            //void resize(const int pdx=-100, const int pdy=-100, const int pdz=-100, const int interp=1);

            //! Copy data resizing in per-cent
            //CpuImage<T>* getResize(const int pdx=-100, const int pdy=-100, const int pdz=-100, const int interp=1) const;

            // Statistics
            StatisticalData* getStatistics(const bool computeVariance = true) const;

            //! Find interpolated maximum pixel location
            PeakData getPeak() const;

            //! Find some interpolated maximum pixel locations
            /*!
             * @param number the number of peaks to find
             * @return all peaks in a vector struct
             */
            vector<PeakData> getPeakList(const int number) const;

            //! Range normalization
            void normalizeRange(const REAL& a, const REAL& b);
            Image<T>* getNormalizeRange(const REAL& a, const REAL& b) const;

            //! Variance normalization
            void normalizeVariance(const REAL sigma);
            Image<T>* getNormalizeVariance(const REAL sigma) const;

            //! 1D cropping (from left to right)
            void crop(const int x0, const int x1);
            Image<T>* getCrop(const int x0, const int x1) const;

            //! 2D cropping (from upper-left to lower-right)
            void crop(const int x0, const int y0, const int x1, const int y1);
            Image<T>* getCrop(const int x0, const int y0, const int x1, const int y1) const;

            //! 3D cropping (from upper-left-front to lower-right-bottom)
            void crop(const int x0, const int y0, const int z0, const int x1, const int y1, const int z1);
            Image<T>* getCrop(const int x0, const int y0, const int z0, const int x1, const int y1, const int z1) const;

            //! Coarsing
            /*!
             * Coarses square blocks (a=factor) of pixels by replacing it with average pixel
             */
            void coarse(const int factor);
            Image<T>* getCoarse(const int factor) const;

            void pad(const int left, const int right = 0, const int up = 0, const int down = 0, const int front = 0, const int back = 0);
            Image<T>* getPad(const int left, const int right = 0, const int up = 0, const int down = 0, const int front = 0, const int back = 0) const;

            void stitch(const AbstractImage& image, POSITION_TYPE position);
            Image<T>* getStitch(const AbstractImage& image, POSITION_TYPE position) const;

            void power(const int pow = 2);
            Image<T>* getPower(const int pow = 2) const;

            void sqrt(void);
            Image<T>* getSqrt(void) const;

            //! Slicing of 3D images
            void slice2D(const int idz);
            Image<T>* getSlice2D(const int idz) const;
            void setSlice2D(const AbstractImage& image, const int idz);

            //! This function is to add up all pixel along the x axes
            void sumAlongX();
            //! This function is to add up all pixel along the x axes
            /*!
             * @return the summation image
             */
            Image<T>* getSumAlongX() const;

            //! This function is to add up all pixel along the y axes
            void sumAlongY();
            //! This function is to add up all pixel along the y axes
            /*!
             * @return the summation image
             */
            Image<T>* getSumAlongY() const;

            //! This function is to add up all pixel along the y axes
            void sumAlongZ();
            //! This function is to add up all pixel along the y axes
            /*!
             * @return the summation image
             */
            Image<T>* getSumAlongZ() const;


            //! This function is to smear the Image along the x axes
            /*!
             * @param size is the count how long to smear
             */
            void smearAlongX(const int size);

            //! This function is to smear the Image along the x axes
            /*!
             * @param size is the count how long to smear
             * @return the smear image
             */
            Image<T>* getSmearAlongX(const int size) const;

            //! This function is to smear the Image along the y axes
            /*!
             * @param size is the count how long to smear
             */
            virtual void smearAlongY(const int size);

            //! This function is to smear the Image along the y axes
            /*!
             * @param size is the count how long to smear
             * @return the smear image
             */
            Image<T>* getSmearAlongY(const int size) const;

            //! This function is to smear the Image along the z axes
            /*!
             * @param size is the count how long to smear
             */
            void smearAlongZ(const int size);

            //! This function is to smear the Image along the z axes
            /*!
             * @param size is the count how long to smear
             * @return the smear image
             */
            Image<T>* getSmearAlongZ(const int size) const;


            //! Scale an image.
            /*!
             *  \param newX new size along the X-axis.
             *  \param newY new size along the Y-axis.
             *  \param newZ new size along the Z-axis.
             *  if the new size are zero then don't change the size.
             *  \param interp Method of interpolation
             *  \param borderCondition Border condition type.
             *   borderCondition can be :
             *     - 0 : Zero border condition.
             *     - 1 : Nearest neighbors.
             *     - 2 : Repeat Pattern (like fftw).
             */
            void scale(const int newX = 0, const int newY = 0, const int newZ = 0, const INTERPOLATION_TYPE interp = LINEAR, const int borderCondition = 0);
            Image<T>* getScale(const int newX = 0, const int newY = 0, const int newZ = 0, const INTERPOLATION_TYPE interp = LINEAR, const int borderCondition = 0);

            //! sums up all values of an image
            /*!
             * @return sum of all values
             */
            void getSum(T& sum) const;

            /***************************************
             *            Inlines             *
             ***************************************/

            //! Data type identification

            /*!
             * Possible results:
             * CPU_IMAGE
             * CUDA_IMAGE
             * BOOST_IMAGE
             */
            inline IMAGE_TYPE getImageType() const {
                return CPU_IMAGE;
            }

            //! Get pixel ( CAVEAT maybe slow!! )

            inline void getPixel(T& pixelValue, const int x, const int y = 0, const int z = 0) const {
                pixelValue = m_data[offset(x, y, z)];
            }

            //! This function is to swap quadrans
            void swapQuadrants();

            //! This function is to swap quadrans
            /*!
             * @return the new image
             */
            Image<T>* getSwapQuadrants() const;

            //! Data dimensionality

            inline const int dim() const {
                // zero image
                if ((m_width <= 1) && (_height <= 1) && (_depth <= 1)) return 0;

                // three dimensional image
                if ((m_width > 1) && (_height > 1) && (_depth > 1)) return 3;

                // two dimensional image
                if (((m_width > 2) && (_height > 1)) || ((m_width > 1) && (_depth > 1)) || ((_height > 1) && (_depth > 1))) return 2;

                // one dimensioanl image
                return 1;
            }

            //! X dimension

            inline const int dimX() const {
                return m_width;
            }
            //! Y dimension

            inline const int dimY() const {
                return _height;
            }
            //! Z dimension

            inline const int dimZ() const {
                return _depth;
            }

            //! Clear

            inline void clear() {
                Image<T> ().swap(*this);
            }

            //! Returns total number of pixels in image

            inline const size_t size() const {
                return m_width * _height * _depth;
            }

            //! Returns size of image in bytes

            inline const size_t byteSize() const {
                return size() * sizeof (T);
            }

            //! Offset function for data pointer position

            inline const long offset(const int x = 0) const {
                return x;
            }

            inline const long offset(const int x, const int y) const {
                return x + m_width * y;
            }

            inline const long offset(const int x, const int y, const int z) const {
                return x + m_width * (y + _height * z);
            }

            //! Returns pointer to data array

            inline T* ptr(const int x = 0) const {
                return &m_data[x];
            }

            //! Returns pointer to data array

            inline T* ptr(const int x, const int y) const {
                return m_data + offset(x, y);
            }

            //! Returns pointer to data array

            inline T* ptr(const int x, const int y, const int z) const {
                return m_data + offset(x, y, z);
            }

            //! Returns void pointer to data array

            inline void* pixelPtr(const int x = 0, const int y = 0, const int z = 0) {
                return m_data + offset(x, y, z);
            }

            //! Sets the value of an individual pixel

            inline void setPixel(const T& pixelValue, const int x, const int y = 0, const int z = 0) {
                m_data[offset(x, y, z)] = pixelValue;
            }

            /***************************************
             *            Basic Operators          *
             ***************************************/

            inline T& operator()(const int x, const int y = 0, const int z = 0) const {
                return m_data[offset(x, y, z)];
            }

            inline T& operator[](const unsigned long off) const {
                return m_data[off];
            }

            //! Like the copy constructor
            Image<T>& operator=(const Image<T>& image);

            //! Like the copy constructor
            template<class Ts>
            Image<T>& operator=(const Image<Ts>& image);

            const bool operator==(const AbstractImage& image);
            const bool operator!=(const AbstractImage& image);

            /***************************************
             *       Arithmetic Operators          *
             ***************************************/

            //! Operator+= with value
            Image<T>& operator+=(const T& value);
            //! Operator+= with another image
            Image<T>& operator+=(const AbstractImage& image);
            //! Operator+ with value
            Image<T>* operator+(const T& value) const;
            //! Operator+ with image
            Image<T>* operator+(const AbstractImage& image) const;
            //! Operator-= with value
            Image<T>& operator-=(const T& value);
            //! Operator-= with image
            Image<T>& operator-=(const AbstractImage& image);
            //! Operator- with value
            Image<T>* operator-(const T& value) const;
            //! Operator- with image
            Image<T>* operator-(const AbstractImage& image) const;
            //! Operator*= with value
            Image<T>& operator*=(const T& value);
            //! Operator*= with another image
            Image<T>& operator*=(const AbstractImage& image);
            //! Operator* with value
            Image<T>* operator*(const T& value) const;
            //! Operator* with image
            Image<T>* operator*(const AbstractImage& image) const;
            //! Operator/= with value
            Image<T>& operator/=(const T& value);
            //! Operator/= with another image
            Image<T>& operator/=(const AbstractImage& image);
            //! Operator/ with value
            Image<T>* operator/(const T& value) const;
            //! Operator/ with image
            Image<T>* operator/(const AbstractImage& image) const;

            inline void set(const int x, const int y, const int z, const T& value) {
                m_data[x + m_width * (y + _height * z)] = value;
            }

            //! Type identification
            CLASS_TYPE getType() const;

            //! Cloning

            AbstractImage* clone() {
                return new Image<T> (*this);
            }

        };

        //! 1d neighbour interpolation

        template<class T>
        inline T Image<T>::neighbourPix1D(const REAL ffx, const int y, const int z) const {
            const int tmp = (int) floor(ffx); // f(0)

            return this->operator()(tmp, y, z);
        }

        //! 2d neighbour interpolation

        template<class T>
        inline T Image<T>::neighbourPix2D(const REAL ffx, const REAL ffy, const int z) const {
            const int tmpx = (int) floor(ffx); // f(0)
            const int tmpy = (int) floor(ffy); // f(0)

            return this->operator()(tmpx, tmpy, z);
        }

        //! 3d neighbour interpolation

        template<class T>
        inline T Image<T>::neighbourPix3D(const REAL ffx, const REAL ffy, const REAL ffz) const {
            const int tmpx = (int) floor(ffx); // f(0)
            const int tmpy = (int) floor(ffy); // f(0)
            const int tmpz = (int) floor(ffz); // f(0)

            return this->operator()(tmpx, tmpy, tmpz);
        }

        //! 1d neighbour interpolation

        template<class T>
        inline T Image<T>::neighbourPix1D_Nearest(const REAL ffx, const int y, const int z) const {
            int tmpx = (int) floor(ffx); // f(0)

            if (tmpx >= m_width) tmpx = m_width - 1;
            if (tmpx < 0) tmpx = 0;

            return this->operator()(tmpx, y, z);
        }

        //! 2d neighbour interpolation

        template<class T>
        inline T Image<T>::neighbourPix2D_Nearest(const REAL ffx, const REAL ffy, const int z) const {
            int tmpy = (int) floor(ffy); // f(0)

            if (tmpy >= _height) tmpy = _height - 1;
            if (tmpy < 0) tmpy = 0;

            return neighbourPix1D_Nearest(ffx, tmpy, z);
        }

        //! 3d neighbour interpolation

        template<class T>
        inline T Image<T>::neighbourPix3D_Nearest(const REAL ffx, const REAL ffy, const REAL ffz) const {
            int tmpz = (int) floor(ffz); // f(0)

            if (tmpz >= _depth) tmpz = _depth - 1;
            if (tmpz < 0) tmpz = 0;

            return neighbourPix2D_Nearest(ffx, ffy, tmpz);
        }

        //! 1d neighbour interpolation

        template<class T>
        inline T Image<T>::neighbourPix1D_Repeat(const REAL ffx, const int y, const int z) const {
            int tmpx = (int) floor(ffx); // f(0)

            if (tmpx >= m_width) tmpx -= m_width;
            if (tmpx < 0) tmpx += m_width;

            return this->operator()(tmpx, y, z);
        }

        //! 2d neighbour interpolation

        template<class T>
        inline T Image<T>::neighbourPix2D_Repeat(const REAL ffx, const REAL ffy, const int z) const {
            int tmpy = (int) floor(ffy); // f(0)

            if (tmpy >= _height) tmpy -= _height;
            if (tmpy < 0) tmpy += _height;

            return neighbourPix1D_Repeat(ffx, tmpy, z);

        }

        //! 3d neighbour interpolation

        template<class T>
        inline T Image<T>::neighbourPix3D_Repeat(const REAL ffx, const REAL ffy, const REAL ffz) const {
            int tmpz = (int) floor(ffz); // f(0)

            if (tmpz >= _depth) tmpz -= _depth;
            if (tmpz < 0) tmpz += _depth;

            return neighbourPix2D_Repeat(ffx, ffy, tmpz);
        }

        //! basic linear interpolation function

        template<class T>
        inline T Image<T>::linear(const T p0, const T p1, const T dp) const {
            return p0 + dp * (p1 - p0);
        }

        //! 1d linear interpolation

        template<class T>
        inline T Image<T>::linearPix1D(const REAL ffx, const int y, const int z) const {
            const int tmp = (int) floor(ffx); // f(0)
            const int x[2] = {tmp, tmp + 1};

            T w[2] = {(T) 0.0, (T) 0.0};
            T dp = (T) (ffx - x[0]);

            for (int i = 0; i < 2; i++) {
                if ((x[i] >= 0) && (x[i] < m_width)) {
                    w[i] = this->operator()(x[i], y, z);
                }
            }

            return linear(w[0], w[1], dp);
        }

        //! 2d bilinear interpolation

        template<class T>
        inline T Image<T>::linearPix2D(const REAL ffx, const REAL ffy, const int z) const {
            const int tmpx = (int) floor(ffx); // f(0)
            const int x[2] = {tmpx, tmpx + 1};

            const int tmpy = (int) floor(ffy); // f(0)
            const int y[2] = {tmpy, tmpy + 1};

            const T dpx = (T) (ffx - x[0]);
            const T dpy = (T) (ffy - y[0]);

            T v[2] = {(T) 0.0, (T) 0.0};

            for (int i = 0; i < 2; i++) {
                if ((y[i] >= 0) && (y[i] < _height)) {
                    T w[2] = {(T) 0.0, (T) 0.0};
                    for (int j = 0; j < 2; j++) {
                        if ((x[j] >= 0) && (x[j] < m_width)) {
                            w[j] = this->operator()(x[j], y[i], z);
                        }
                    }
                    v[i] = linear(w[0], w[1], dpx);
                }
            }

            return linear(v[0], v[1], dpy);
        }

        //! 3d trilinear interpolation

        template<class T>
        inline T Image<T>::linearPix3D(const REAL ffx, const REAL ffy, const REAL ffz) const {
            const int tmpx = (int) floor(ffx); // f(0)
            const int x[2] = {tmpx, tmpx + 1};

            const int tmpy = (int) floor(ffy); // f(0)
            const int y[2] = {tmpy, tmpy + 1};

            const int tmpz = (int) floor(ffz); // f(0)
            const int z[2] = {tmpz, tmpz + 1};

            const T dpx = (T) (ffx - x[0]);
            const T dpy = (T) (ffy - y[0]);
            const T dpz = (T) (ffz - z[0]);

            T w[2] = {(T) 0.0, (T) 0.0};
            for (int i = 0; i < 2; i++) {
                if ((z[i] >= 0) && (z[i] < _depth)) {
                    T v[2] = {(T) 0.0, (T) 0.0};
                    for (int j = 0; j < 2; j++) {
                        if ((y[j] >= 0) && (y[j] < _height)) {
                            T p[2] = {(T) 0.0, (T) 0.0};
                            for (int k = 0; k < 2; k++) {
                                if ((x[k] >= 0) && (x[k] < m_width)) {
                                    p[k] = this->operator()(x[k], y[j], z[i]);
                                }
                            }
                            v[j] = linear(p[0], p[1], dpx);
                        }
                    }
                    w[i] = linear(v[0], v[1], dpy);
                } // end if (( z[i]
            }

            return linear(w[0], w[1], dpz);
        }

        //! 1d linear interpolation

        template<class T>
        inline T Image<T>::linearPix1D_Nearest(const REAL ffx, const int y, const int z) const {
            const int tmp = (int) floor(ffx); // f(0)
            const T dpx = (T) (ffx - tmp);

            int x[2] = {tmp, tmp + 1};
            T w[2] = {(T) 0.0, (T) 0.0};

            if (x[0] < 0) {
                x[0] = 0;
            } else if (x[0] >= m_width) {
                x[0] = m_width - 1;
            }

            if (x[1] < 0) {
                x[1] = 0;
            } else if (x[1] >= m_width) {
                x[1] = m_width - 1;
            }

            w[0] = this->operator()(x[0], y, z);
            w[1] = this->operator()(x[1], y, z);

            return linear(w[0], w[1], dpx);
        }

        //! 2d bilinear interpolation

        template<class T>
        inline T Image<T>::linearPix2D_Nearest(const REAL ffx, const REAL ffy, const int z) const {
            const int tmpy = (int) floor(ffy); // f(0)
            const T dpy = (T) (ffy - tmpy);

            int y[2] = {tmpy, tmpy + 1};
            if (y[0] < 0) {
                y[0] = 0;
            } else if (y[0] >= _height) {
                y[0] = _height - 1;
            }

            if (y[1] < 0) {
                y[1] = 0;
            } else if (y[1] >= _height) {
                y[1] = _height - 1;
            }

            return linear(linearPix1D_Nearest(ffx, y[0], z), linearPix1D_Nearest(ffx, y[0], z), dpy);
        }

        //! 3d trilinear interpolation

        template<class T>
        inline T Image<T>::linearPix3D_Nearest(const REAL ffx, const REAL ffy, const REAL ffz) const {
            const int tmpz = (int) floor(ffz); // f(0)
            const T dpz = (T) (ffz - tmpz);
            int z[2] = {tmpz, tmpz + 1};

            if (z[0] < 0) {
                z[0] = 0;
            } else if (z[0] >= _depth) {
                z[0] = _depth - 1;
            }

            if (z[1] < 0) {
                z[1] = 0;
            } else if (z[1] >= _depth) {
                z[1] = _depth - 1;
            }

            return linear(linearPix2D_Nearest(ffx, ffy, z[0]), linearPix2D_Nearest(ffx, ffy, z[1]), dpz);
        }

        //! 1d linear interpolation

        template<class T>
        inline T Image<T>::linearPix1D_Repeat(const REAL ffx, const int y, const int z) const {
            const int tmp = (int) floor(ffx); // f(0)
            const T dpx = (T) (ffx - tmp);

            int x[2] = {tmp, tmp + 1};
            T w[2] = {(T) 0.0, (T) 0.0};

            if (x[0] < 0) {
                x[0] += m_width;
            } else if (x[0] >= m_width) {
                x[0] -= m_width;
            }

            if (x[1] < 0) {
                x[1] += m_width;
            } else if (x[1] >= m_width) {
                x[1] -= m_width;
            }

            w[0] = this->operator()(x[0], y, z);
            w[1] = this->operator()(x[1], y, z);

            return linear(w[0], w[1], dpx);
        }

        //! 2d bilinear interpolation

        template<class T>
        inline T Image<T>::linearPix2D_Repeat(const REAL ffx, const REAL ffy, const int z) const {
            const int tmpy = (int) floor(ffy); // f(0)
            const T dpy = (T) (ffy - tmpy);

            int y[2] = {tmpy, tmpy + 1};
            if (y[0] < 0) {
                y[0] += _height;
            } else if (y[0] >= _height) {
                y[0] -= _height;
            }

            if (y[1] < 0) {
                y[1] += _height;
            } else if (y[1] >= _height) {
                y[1] -= _height;
            }

            return linear(linearPix1D_Repeat(ffx, y[0], z), linearPix1D_Repeat(ffx, y[0], z), dpy);
        }

        //! 3d trilinear interpolation

        template<class T>
        inline T Image<T>::linearPix3D_Repeat(const REAL ffx, const REAL ffy, const REAL ffz) const {
            const int tmpz = (int) floor(ffz); // f(0)
            const T dpz = (T) (ffz - tmpz);
            int z[2] = {tmpz, tmpz + 1};

            if (z[0] < 0) {
                z[0] += _depth;
            } else if (z[0] >= _depth) {
                z[0] -= _depth;
            }

            if (z[1] < 0) {
                z[1] += _depth;
            } else if (z[1] >= _depth) {
                z[1] -= _depth;
            }

            return linear(linearPix2D_Repeat(ffx, ffy, z[0]), linearPix2D_Repeat(ffx, ffy, z[1]), dpz);
        }

        //! basic cubic interpolation function

        template<class T>
        inline T Image<T>::cubic(const T p0, const T p1, const T p2, const T p3, const T dp) const {

            T dp2 = dp * dp;
            T v0 = p3 - p2 - p0 + p1;
            T v1 = p0 - p1 - v0;
            T v2 = p2 - p0;
            T v3 = p1;

            return (T) (v0 * dp * dp2 + v1 * dp2 + v2 * dp + v3);
        }

        //! basic cubic bspline interpolation function

        template<class T>
        inline T Image<T>::cubic_bspline(const T p0, const T p1, const T p2, const T p3, const T dp) const {
            T dp2 = dp * dp;
            T dp3 = dp2 * dp;

            T v0 = (p3 - (T) 3 * p2 + (T) 3 * p1 - p0) / (T) 6;
            T v1 = ((T) 3 * p0 - (T) 6 * p1 + (T) 3 * p2) / (T) 6;
            T v2 = (-(T) 3 * p0 + (T) 3 * p2) / (T) 6;
            T v3 = (p0 + (T) 4 * p1 + p2) / (T) 6;


            return (T) (dp3 * v0 + dp2 * v1 + dp * v2 + v3);
        }

        //! 1d cubic interpolation

        template<class T>
        inline T Image<T>::cubicPix1D(const REAL ffx, const int y, const int z) const {
            const int tmp = (int) floor(ffx); // f(0)
            const int x[4] = {tmp - 1, tmp, tmp + 1, tmp + 2};

            T w[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};
            T dp = (T) (ffx - x[1]);

            for (int i = 0; i < 4; i++) {
                if ((x[i] >= 0) && (x[i] < m_width)) {
                    w[i] = this->operator()(x[i], y, z);
                }
            }

            return (T) cubic(w[0], w[1], w[2], w[3], dp);
        }
        //! 2d bicubic interpolation

        template<class T>
        inline T Image<T>::cubicPix2D(const REAL ffx, const REAL ffy, const int z) const {
            const int tmpx = (int) floor(ffx); // f(0)
            const int x[4] = {tmpx - 1, tmpx, tmpx + 1, tmpx + 2};

            const int tmpy = (int) floor(ffy); // f(0)
            const int y[4] = {tmpy - 1, tmpy, tmpy + 1, tmpy + 2};

            T dpx = (T) (ffx - x[1]);
            T dpy = (T) (ffy - y[1]);

            T v[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};

            for (int i = 0; i < 4; i++) {
                if ((y[i] >= 0) && (y[i] < _height)) {
                    T w[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};
                    for (int j = 0; j < 4; j++) {
                        if ((x[j] >= 0) && (x[j] < m_width)) {
                            w[j] = this->operator()(x[j], y[i], z);
                        }
                    }
                    v[i] = cubic(w[0], w[1], w[2], w[3], dpx);
                }
            }

            return cubic(v[0], v[1], v[2], v[3], dpy);
        }

        //! 3d tricubic interpolation

        template<class T>
        inline T Image<T>::cubicPix3D(const REAL ffx, const REAL ffy, const REAL ffz) const {
            const int tmpx = (int) floor(ffx); // f(0)
            const int x[4] = {tmpx - 1, tmpx, tmpx + 1, tmpx + 2};

            const int tmpy = (int) floor(ffy); // f(0)
            const int y[4] = {tmpy - 1, tmpy, tmpy + 1, tmpy + 2};

            const int tmpz = (int) floor(ffz); // f(0)
            const int z[4] = {tmpz - 1, tmpz, tmpz + 1, tmpz + 2};

            T dpx = (T) (ffx - x[1]);
            T dpy = (T) (ffy - y[1]);
            T dpz = (T) (ffz - z[1]);

            T w[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};
            for (int i = 0; i < 4; i++) {
                if ((z[i] >= 0) && (z[i] < _depth)) {
                    T v[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};
                    for (int j = 0; j < 4; j++) {
                        if ((y[j] >= 0) && (y[j] < _height)) {
                            T p[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};
                            for (int k = 0; k < 4; k++) {
                                if ((x[k] >= 0) && (x[k] < m_width)) {
                                    p[k] = this->operator()(x[k], y[j], z[i]);
                                }
                            }
                            v[j] = cubic(p[0], p[1], p[2], p[3], dpx);
                        }
                    }
                    w[i] = cubic(v[0], v[1], v[2], v[3], dpy);
                } // end if (( z[i]
            }

            return (T) cubic(w[0], w[1], w[2], w[3], dpz);
        }

        //! 1d cubic interpolation

        template<class T>
        inline T Image<T>::cubicPix1D_Nearest(const REAL ffx, const int y, const int z) const {
            const int tmp = (int) floor(ffx); // f(0)
            const T dpx = (T) (ffx - tmp);

            int x[4] = {tmp - 1, tmp, tmp + 1, tmp + 2};
            T w[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};

            for (int i = 0; i < 4; i++) {
                if (x[i] < 0) {
                    x[i] = 0;
                } else if (x[i] >= m_width) {
                    x[i] = m_width - 1;
                }
                w[i] = this->operator()(x[i], y, z);
            }

            return (T) cubic(w[0], w[1], w[2], w[3], dpx);
        }
        //! 2d bicubic interpolation

        template<class T>
        inline T Image<T>::cubicPix2D_Nearest(const REAL ffx, const REAL ffy, const int z) const {
            const int tmpy = (int) floor(ffy); // f(0)
            const T dpy = (T) (ffy - tmpy);

            int y[4] = {tmpy - 1, tmpy, tmpy + 1, tmpy + 2};
            T w[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};

            for (int i = 0; i < 4; i++) {
                if (y[i] < 0) {
                    y[i] = 0;
                } else if (y[i] >= _height) {
                    y[i] = _height - 1;
                }
                w[i] = cubicPix1D_Nearest(ffx, y[i], z);
            }
            return (T) cubic(w[0], w[1], w[2], w[3], dpy);
        }

        //! 3d tricubic interpolation

        template<class T>
        inline T Image<T>::cubicPix3D_Nearest(const REAL ffx, const REAL ffy, const REAL ffz) const {
            const int tmpz = (int) floor(ffz); // f(0)
            const T dpz = (T) (ffz - tmpz);

            int z[4] = {tmpz - 1, tmpz, tmpz + 1, tmpz + 2};
            T w[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};

            for (int i = 0; i < 4; i++) {
                if (z[i] < 0) {
                    z[i] = 0;
                } else if (z[i] >= _depth) {
                    z[i] = _depth - 1;
                }
                w[i] = cubicPix2D_Nearest(ffx, ffy, z[i]);
            }
            return (T) cubic(w[0], w[1], w[2], w[3], dpz);
        }

        //! 1d cubic interpolation

        template<class T>
        inline T Image<T>::cubicPix1D_Repeat(const REAL ffx, const int y, const int z) const {
            const int tmp = (int) floor(ffx); // f(0)
            const T dpx = (T) (ffx - tmp);

            int x[4] = {tmp - 1, tmp, tmp + 1, tmp + 2};
            T w[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};

            for (int i = 0; i < 4; i++) {
                if (x[i] < 0) {
                    x[i] += m_width;
                } else if (x[i] >= m_width) {
                    x[i] -= m_width;
                }
                w[i] = this->operator()(x[i], y, z);
            }

            return (T) cubic(w[0], w[1], w[2], w[3], dpx);
        }
        //! 2d bicubic interpolation

        template<class T>
        inline T Image<T>::cubicPix2D_Repeat(const REAL ffx, const REAL ffy, const int z) const {
            const int tmpy = (int) floor(ffy); // f(0)
            const T dpy = (T) (ffy - tmpy);

            int y[4] = {tmpy - 1, tmpy, tmpy + 1, tmpy + 2};
            T w[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};

            for (int i = 0; i < 4; i++) {
                if (y[i] < 0) {
                    y[i] += _height;
                } else if (y[i] >= _height) {
                    y[i] -= _height;
                }
                w[i] = cubicPix1D_Repeat(ffx, y[i], z);
            }
            return (T) cubic(w[0], w[1], w[2], w[3], dpy);
        }

        //! 3d tricubic interpolation

        template<class T>
        inline T Image<T>::cubicPix3D_Repeat(const REAL ffx, const REAL ffy, const REAL ffz) const {
            const int tmpz = (int) floor(ffz); // f(0)
            const T dpz = (T) (ffz - tmpz);

            int z[4] = {tmpz - 1, tmpz, tmpz + 1, tmpz + 2};
            T w[4] = {(T) 0.0, (T) 0.0, (T) 0.0, (T) 0.0};

            for (int i = 0; i < 4; i++) {
                if (z[i] < 0) {
                    z[i] += _depth;
                } else if (z[i] >= _depth) {
                    z[i] -= _depth;
                }
                w[i] = cubicPix2D_Repeat(ffx, ffy, z[i]);
            }
            return (T) cubic(w[0], w[1], w[2], w[3], dpz);
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::neighbourPix1D(const REAL ffx, const int y, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(y) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("complex 1D Neighbour Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::neighbourPix2D(const REAL ffx, const REAL ffy, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("complex 2D Neighbour Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::neighbourPix3D(const REAL ffx, const REAL ffy, const REAL ffz) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(ffz)
                    throw NOT_IMPLEMENTED_ERROR("complex 3D Neighbour Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::neighbourPix1D_Nearest(const REAL ffx, const int y, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(y) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("complex 1D Neighbour Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::neighbourPix2D_Nearest(const REAL ffx, const REAL ffy, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("complex 2D Neighbour Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::neighbourPix3D_Nearest(const REAL ffx, const REAL ffy, const REAL ffz) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(ffz)
                    throw NOT_IMPLEMENTED_ERROR("complex 3D Neighbour Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::neighbourPix1D_Repeat(const REAL ffx, const int y, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(y) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("complex 1D Neighbour Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::neighbourPix2D_Repeat(const REAL ffx, const REAL ffy, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("complex 2D Neighbour Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::neighbourPix3D_Repeat(const REAL ffx, const REAL ffy, const REAL ffz) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(ffz)
                    throw NOT_IMPLEMENTED_ERROR("complex 3D Neighbour Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::linearPix1D(const REAL ffx, const int y, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(y) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("complex 1D Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::linearPix2D(const REAL ffx, const REAL ffy, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::linearPix3D(const REAL ffx, const REAL ffy, const REAL ffz) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(ffz)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::linearPix1D_Nearest(const REAL ffx, const int y, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(y) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("complex 1D Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::linearPix2D_Nearest(const REAL ffx, const REAL ffy, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::linearPix3D_Nearest(const REAL ffx, const REAL ffy, const REAL ffz) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(ffz)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::linearPix1D_Repeat(const REAL ffx, const int y, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(y) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("complex 1D Interpolation not yet implemented!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::linearPix2D_Repeat(const REAL ffx, const REAL ffy, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::linearPix3D_Repeat(const REAL ffx, const REAL ffy, const REAL ffz) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(ffz)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::cubicPix1D(const REAL ffx, const int y, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(y) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::cubicPix2D(const REAL ffx, const REAL ffy, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::cubicPix3D(const REAL ffx, const REAL ffy, const REAL ffz) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(ffz)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::cubicPix1D_Nearest(const REAL ffx, const int y, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(y) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::cubicPix2D_Nearest(const REAL ffx, const REAL ffy, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::cubicPix3D_Nearest(const REAL ffx, const REAL ffy, const REAL ffz) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(ffz)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::cubicPix1D_Repeat(const REAL ffx, const int y, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(y) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::cubicPix2D_Repeat(const REAL ffx, const REAL ffy, const int z) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(z)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

        template<>
        inline COMPLEX CpuImage<COMPLEX>::cubicPix3D_Repeat(const REAL ffx, const REAL ffy, const REAL ffz) const {
            ALWAYS_UNUSED(ffx) ALWAYS_UNUSED(ffy) ALWAYS_UNUSED(ffz)
                    throw NOT_IMPLEMENTED_ERROR("If you want complex 3D rotation, go and implement it yourself!");
        }

    }
} //#namespace cow

#endif
