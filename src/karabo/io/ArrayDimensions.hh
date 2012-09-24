/*
 * $Id: ArrayDimensions.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_IO_ARRAYDIMENSIONS_HH
#define	EXFEL_IO_ARRAYDIMENSIONS_HH

#include <vector>
#include <karabo/util/Factory.hh>
#include "iodll.hh"

namespace exfel {
    namespace io {

        /**
         * Describes array dimensions. 
         */
        class ArrayDimensions : public std::vector<unsigned long long> {
            typedef unsigned long long ull64;
        public:

            EXFEL_CLASSINFO(ArrayDimensions, "ArrayDimensions", "1.0")

            ArrayDimensions() : std::vector<ull64>() {
            }

            ArrayDimensions(const std::vector<ull64>& vec) : std::vector<ull64>(vec) {
            }

            ArrayDimensions(ull64 xSize) : std::vector<ull64>(1, xSize) {
            }

            ArrayDimensions(ull64 xSize, ull64 ySize) : std::vector<ull64>(2, 0) {
                (*this)[0] = xSize;
                (*this)[1] = ySize;
            }

            ArrayDimensions(ull64 xSize, ull64 ySize, ull64 zSize) : std::vector<ull64>(3, 0) {
                (*this)[0] = xSize;
                (*this)[1] = ySize;
                (*this)[2] = zSize;
            }


            ArrayDimensions(const ArrayDimensions& o) : std::vector<ull64>(o) {
            }

            virtual ~ArrayDimensions() {
            }

            /**
             * Get the number of elements in array for all dimensions.
             * 1-d ->  equal to dim[0]
             * 2-d ->  equal to dim[0] x dim[1]
             * n-d ->  equal to dim[0] x ... x dim[n-1]
             */
            ull64 getNumberOfElements() const {
                size_t ndims = size();
                if (ndims == 0) return 0;
                unsigned long long totalNumber = (*this)[0];
                for (size_t i = 1; i < ndims; i++) {
                    totalNumber *= static_cast<unsigned long long> ((*this)[i]);
                }
                return totalNumber;
            }

            size_t getRank() const {
                return this->size();
            }

            std::vector<ull64>& toVector() {
                return *this;
            }

            const std::vector<ull64>& toVector() const {
                return *this;
            }


        };


    }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::ArrayDimensions, TEMPLATE_IO, DECLSPEC_IO)

#endif	

