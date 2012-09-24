/*
 * $Id: ArrayView.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IO_ARRAYVIEW_HH
#define	EXFEL_IO_ARRAYVIEW_HH

#include <vector>
#include <cassert>
#include "ArrayDimensions.hh"
#include <boost/shared_array.hpp>
#include <karabo/util/Exception.hh>

namespace exfel {
    namespace io {

        /**
         * This template can be used to interpret C array of type T as a multi dimensional array.
         * It is a wrapper around C array or boost::shared_array defined as a continues block of memory and requires C type storage layout 
         * (as opposite to the fortran storage layout)
         * The ArrayView can be used with existing arrays by simply passing to the constructor a C pointer or boost:shared_array 
         * reference or it can allocate memory using boost::shared_array. 
         * The ArrayView is used primarily to help with storing and retrieving data to/from hdf5 file and therefore 
         * its main feature is a possibility to define and discover dimensionality at runtime. It does not aim to make the 
         * navigation interface (indexing) easier for multi dimensional arrays apart from cases where rank is equal to 1 or 2. 
         * For that purpose it is recommended to use boost::multi_array_ref or boost::const_multi_array_ref or other specialized containers.
         * The reason why the boost multi_arrays are not used for IO is that these require the rank to be defined at compilation time.
         */
        template<typename T>
        class ArrayView {
        public:

            ArrayView() : m_ptr(0), m_isShared(false) {
                m_dims = ArrayDimensions();
            }

            /**
             * Construct ArrayView from raw pointer
             * @param ptr pointer to allocated array
             * @param dims description of dimensions
             */
            explicit ArrayView(T* ptr, ArrayDimensions dims)
            : m_dims(dims), m_ptr(ptr), m_isShared(false) {
            }

            /**
             * Construct ArrayView from std::vector. ArrayView is valid until vector is not modified in terms of changing its size.
             * Since the memory is managed by the vector ArrayView life time is limited to the life time of the vector.
             * @param vec reference to the vector
             * @param dims description of dimensions
             */
            explicit ArrayView(std::vector<T>& vec, ArrayDimensions dims)
            : m_dims(dims), m_isShared(false) {

                size_t vs = vec.size();
                if (vs == 0) {
                    throw PARAMETER_EXCEPTION("Dimensions is not defined");
                }

                size_t s = 1;
                for (size_t i = 0; i < dims.size(); ++i) {
                    s *= dims[i];
                }
                if (s != vs) {
                    throw PARAMETER_EXCEPTION("array dimensions does not agree with the size of the vector");
                }

                m_ptr = &vec[0];
            }

            /**
             * Construct ArrayView from std::vector and interpret it as 1-dimensional array of std::vector size() 
             * ArrayView is valid until vector is not modified in terms of changing its size.
             * Since the memory is managed by the vector ArrayView life time is limited to the life time of the vector.
             * @param vec reference to the vector
             */

            explicit ArrayView(std::vector<T>& vec)
            : m_isShared(false) {

                this->m_dims.resize(1);
                this->m_dims[0] = static_cast<unsigned long long> (vec.size());

                m_ptr = &vec[0];
            }

            /**
             * Construct ArrayView from existing boost::shared_array
             * @param sharedArray reference to a boost::shared_array
             * @param dims description of dimensions
             */

            explicit ArrayView(boost::shared_array<T> sharedArray, ArrayDimensions dims) :
            m_dims(dims), m_isShared(true) {
                m_sharedArray = sharedArray;
                m_ptr = sharedArray.get();
            }

            /**
             * Construct ArrayView and allocate memory block using boost::shared_array.
             * Post condition: isShared == true
             * The ArrayView object is always valid.             
             * @param dims description of dimensions
             */            
            explicit ArrayView(ArrayDimensions dims) :
            m_dims(dims), m_isShared(true) {
                m_sharedArray = boost::shared_array<T > (new T[ dims.getNumberOfElements() ]);
                m_ptr = m_sharedArray.get();
            }

            explicit ArrayView(T* ptr, int ndims, size_t* dims) : m_ptr(ptr), m_isShared(false) {
                this->m_dims.resize(ndims);
                for (int i = 0; i < ndims; ++i) {
                    this->m_dims[i] = dims[i];
                }
            }

            explicit ArrayView(T* ptr, size_t nx) : m_ptr(ptr), m_isShared(false) {

                if (nx <= 0) {
                    throw PARAMETER_EXCEPTION("array dimensions must be greater than zero");
                }

                this->m_dims.resize(1);
                this->m_dims[0] = nx;
            }

            explicit ArrayView(T* ptr, size_t nx, size_t ny) : m_ptr(ptr), m_isShared(false) {

                if (nx <= 0 || ny <= 0) {
                    throw PARAMETER_EXCEPTION("array dimensions must be greater than zero");
                }

                this->m_dims.resize(2);
                this->m_dims[0] = nx;
                this->m_dims[1] = ny;
            }

            explicit ArrayView(T* ptr, size_t nx, size_t ny, size_t nz) : m_ptr(ptr), m_isShared(false) {
                if (nx <= 0 || ny <= 0 || nz <= 0) {
                    throw PARAMETER_EXCEPTION("array dimensions must be greater than zero");
                }
                this->m_dims.resize(3);
                this->m_dims[0] = nx;
                this->m_dims[1] = ny;
                this->m_dims[2] = nz;
            }

            inline int getNumDims() const {
                return m_dims.size();
            }

            /**
             * Get the number of elements in underlaying storage (as it was 1-dim array)
             * @return number of elements
             */
            unsigned long long getSize() const {
                if (m_dims.size() == 0) return 0L;
                unsigned long long size = m_dims[0];
                for (size_t i = 1; i < m_dims.size(); ++i) {
                    size *= m_dims[i];
                }
                return size;
            }

            /**
             * Get description of dimensions
             * @return ArrayDimensions
             */
            inline const ArrayDimensions getDims() const {
                return m_dims;
            }

            /**
             * Get pointer to the first element in continues memory block.
             * The same can be achieved using operator in like that: &av[0] assuming av is an object of ArrayView class.
             * @return pointer of type T
             */
            T* data() const {
                return m_ptr;
            }

            /**
             * Get reference to a specified element in the array. 
             * @param i index of an array as it was one dimensional array.
             * @return reference to the value 
             */

            T& operator[](size_t i) {
                return m_ptr[i];
            }

            /**
             * Get const reference to a specified element in the array. 
             * @param i index of an array as it was one dimensional array.
             * @return const reference to the value 
             */

            const T& operator[](size_t i) const {
                return m_ptr[i];
            }

            void getVectorOfArrayViews(std::vector<ArrayView<T> >& vec) {
                vec.resize(m_dims[0]);
                size_t rank = getNumDims();
                if (rank <= 1) {
                    throw LOGIC_EXCEPTION("Cannot convert ArrayView to vector of ArrayViews. Number of dimensions is too low.");
                }
                std::vector<unsigned long long> dimsVec(rank - 1, 0);
                for (size_t i = 1; i < rank; ++i) {
                    dimsVec[i - 1] = m_dims[i];
                }
                ArrayDimensions dims(dimsVec);
                unsigned long long sizeReduced = dims.getNumberOfElements();

                for (size_t i = 0; i < m_dims[0]; ++i) {
                    vec[i] = ArrayView<T > ((m_ptr + (i * sizeReduced)), dims);
                    vec[i].m_isShared = this->isShared();
                    if (isShared()) {
                        vec[i].m_sharedArray = this->m_sharedArray;
                    }
                }

            }

            /**
             * Convert ArrayView of type T to a 1-dim array of ArrayViews of rank-1. This function is needed for implementation
             * of IO buffers.
             * For example ArrayView<int> av[4][5][6] is converted to ArrayView<ArrayView<T> > with rank 1 and size 4. 
             * Each element is an ArrayView<int> of dims [5][6].
             * @return ArrayView to ArrayViews of type T
             */
            ArrayView<ArrayView<T> > indexable() {
                // TODO think about better name

                boost::shared_array<ArrayView<T> > resultStorage(new ArrayView<T>[m_dims[0] ]);
                ArrayView< ArrayView<T> >resultArrayView(resultStorage, m_dims[0]);
                size_t rank = getNumDims();
                if (rank <= 1) {
                    throw LOGIC_EXCEPTION("Cannot convert ArrayView to ArrayView of ArrayViews. Number of dimensions is too low.");
                }

                std::vector<unsigned long long> dimsVec(rank - 1, 0);
                for (size_t i = 1; i < rank; ++i) {
                    dimsVec[i - 1] = m_dims[i];
                }
                ArrayDimensions dims(dimsVec);
                unsigned long long sizeReduced = dims.getNumberOfElements();

                for (size_t i = 0; i < m_dims[0]; ++i) {
                    resultStorage[i] = ArrayView<T > ((m_ptr + (i * sizeReduced)), dims);
                    resultStorage[i].m_isShared = this->isShared();
                    if (isShared()) {
                        resultStorage[i].m_sharedArray = this->m_sharedArray;
                    }
                }
                return resultArrayView;
            }

            /**
             * Get information if the ArrayView is a wrapper around boost::shared_array.
             * @return true if the ArrayView is a wrapper around boost::shared_array, faalse otherwise.
             */
            bool isShared() {
                return m_isShared;
            }

        private:

            ArrayDimensions m_dims;
            T* m_ptr;
            boost::shared_array<T> m_sharedArray;
            bool m_isShared;

        };

    }
}

#endif

