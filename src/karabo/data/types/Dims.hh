/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef KARABO_DATA_TYPES_DIMS_HH
#define KARABO_DATA_TYPES_DIMS_HH

#include <algorithm>
#include <ostream>
#include <vector>

#include "karaboDll.hh"


namespace karabo {
    namespace data {

        /**
         * @class Dims
         * @brief A class describing array dimensions
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

            virtual ~Dims() {}

            /**
             * Return the rank of the dimensions
             * @return
             */
            std::size_t rank() const {
                return m_rank;
            }

            /**
             * Return the total number of elements in the array
             * @return
             */
            ull64 size() const {
                return m_numberOfElements;
            }

            /**
             * Return the extend of the array in the dimension identified by idx
             * @param idx, needs to be >= 0 and , rank
             * @return
             */
            ull64 extentIn(size_t idx) const {
                return m_vec[idx];
            }

            friend std::ostream& operator<<(std::ostream& os, const Dims& hash);

            /**
             * Return a std::vector holding the dimension sizes
             * @return
             */
            const std::vector<ull64>& toVector() const {
                return m_vec;
            }

            /**
             * Create a dimension object from a vector
             * @param vec
             */
            void fromVector(const std::vector<ull64>& vec) {
                m_vec = vec;
                calculate();
            }

            /**
             * Return size of first dimension
             * @return
             */
            ull64 x1() const {
                if (m_rank >= 1) return m_vec[0];
                return 0;
            }

            /**
             * Return size of second dimension
             * @return
             */
            ull64 x2() const {
                if (m_rank >= 2) return m_vec[1];
                return 1;
            }

            /**
             * Return size of third dimension
             * @return
             */
            ull64 x3() const {
                if (m_rank >= 3) return m_vec[2];
                return 1;
            }

            /**
             * Return size of fourth dimension
             * @return
             */
            ull64 x4() const {
                if (m_rank >= 4) return m_vec[3];
                return 1;
            }

            /**
             * Reverse dimension sizes
             */
            void reverse() {
                std::reverse(m_vec.begin(), m_vec.end());
            }

           private:
            /**
             * calculate rank and number of elements in array.
             */
            void calculate();
        };

        bool operator==(const Dims& lhs, const Dims& rhs);

        std::ostream& operator<<(std::ostream& os, const Dims& dims);
    } // namespace data
} // namespace karabo


#endif
