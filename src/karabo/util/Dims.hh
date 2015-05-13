/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_DIMS_HH
#define	KARABO_UTIL_DIMS_HH

#include <vector>
#include <string>
#include "Configurator.hh"

namespace karabo {
    namespace util {

        /**
         * Describes array dimensions. 
         */
        class Dims {

            typedef unsigned long long ull64;

            std::vector<ull64> m_vec;
            ull64 m_numberOfElements;
            size_t m_rank;

        public:

            Dims() : m_vec(std::vector<ull64>()) {
                calculate();
            }

            Dims(const std::vector<ull64>& vec) : m_vec(vec) {
                calculate();
            }

            Dims(ull64 x1Size) : m_vec(std::vector<ull64>(1, x1Size)) {
                calculate();
            }

            Dims(ull64 x1Size, ull64 x2Size) : m_vec(std::vector<ull64>(2, 0)) {
                m_vec[0] = x1Size;
                m_vec[1] = x2Size;
                calculate();
            }

            Dims(ull64 x1Size, ull64 x2Size, ull64 x3Size) : m_vec(std::vector<ull64>(3, 0)) {
                m_vec[0] = x1Size;
                m_vec[1] = x2Size;
                m_vec[2] = x3Size;
                calculate();
            }

            Dims(ull64 x1Size, ull64 x2Size, ull64 x3Size, ull64 x4Size) : m_vec(std::vector<ull64>(4, 0)) {
                m_vec[0] = x1Size;
                m_vec[1] = x2Size;
                m_vec[2] = x3Size;
                m_vec[3] = x4Size;
                calculate();
            }

            virtual ~Dims() {
            }

            std::size_t rank() const {
                return m_rank;
            }

            /*
             * returns total number of elements
             */
            ull64 size() const {
                return m_numberOfElements;
            }

            /*
             * Pre:  idx >= 0 && idx < rank()
             */
            ull64 extentIn(size_t idx) const {
                return m_vec[idx];
            }

            const std::vector<ull64>& toVector() const {
                return m_vec;
            }

            void fromVector(const std::vector<ull64>& vec) {
                m_vec = vec;
                calculate();
            }

            KARABO_DEPRECATED ull64 x() const {
                if (m_rank >= 1) return m_vec[0];
                return 0;
            }

            KARABO_DEPRECATED ull64 y() const {
                if (m_rank >= 2) return m_vec[1];
                return 1;
            }

            KARABO_DEPRECATED ull64 z() const {
                if (m_rank >= 3) return m_vec[2];
                return 1;
            }
            
            ull64 x1() const {
                if (m_rank >= 1) return m_vec[0];
                return 0;
            }

            ull64 x2() const {
                if (m_rank >= 2) return m_vec[1];
                return 1;
            }

            ull64 x3() const {
                if (m_rank >= 3) return m_vec[2];
                return 1;
            }
            
             ull64 x4() const {
                if (m_rank >= 4) return m_vec[3];
                return 1;
            }
             
             void reverse() {
                 std::reverse(m_vec.begin(), m_vec.end());
             }




        private:

            /**
             * calculate rank and number of elements in array.
             */
            void calculate() {
                m_rank = m_vec.size();
                if (m_rank == 0) {
                    m_numberOfElements = 0;
                    return;
                }
                m_numberOfElements = m_vec[0];
                //std::clog << "calculate numEl: " << m_numberOfElements << std::endl;
                for (size_t i = 1; i < m_rank; i++) {
                    m_numberOfElements *= static_cast<ull64> (m_vec[i]);
                    //std::clog << "calculate i=" << i << " numEl= " << m_numberOfElements << std::endl;
                }
            }


        };


    }
}



#endif	

