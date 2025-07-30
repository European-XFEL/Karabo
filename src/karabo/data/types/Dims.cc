/*
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

#include "Dims.hh"


namespace karabo {
    namespace data {

        void Dims::calculate() {
            m_rank = m_vec.size();
            if (m_rank == 0) {
                m_numberOfElements = 0;
                return;
            }
            m_numberOfElements = m_vec[0];
            for (size_t i = 1; i < m_rank; i++) {
                m_numberOfElements *= static_cast<ull64>(m_vec[i]);
            }
        }

        bool operator==(const Dims& lhs, const Dims& rhs) {
            if (lhs.size() != rhs.size()) return false;
            for (size_t i = 0; i != lhs.rank(); ++i) {
                if (lhs.extentIn(i) != rhs.extentIn(i)) {
                    return false;
                }
            }
            return true;
        }

        std::ostream& operator<<(std::ostream& os, const Dims& dims) {
            os << '(';
            if (dims.m_rank != 0) {
                os << dims.m_vec[0];
                for (size_t i = 1; i < dims.m_rank; ++i) {
                    os << ',' << dims.m_vec[i];
                }
            }
            os << ')';
            return os;
        }
    } // namespace data
} // namespace karabo
