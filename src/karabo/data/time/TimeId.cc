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
/*
 * File:   TimeId.cc
 *
 * Created on June 19, 2013, 3:22 PM
 */

#include "TimeId.hh"

namespace karabo {
    namespace data {


        TimeId::TimeId() : m_timeId(0) {}


        TimeId::TimeId(const unsigned long long timeId) : m_timeId(timeId) {}


        TimeId::~TimeId() {}


        bool TimeId::hashAttributesContainTimeInformation(const Hash::Attributes& attributes) {
            return attributes.has("tid");
        }


        TimeId TimeId::fromHashAttributes(const Hash::Attributes& attributes) {
            unsigned long long tid;

            try {
                auto& element = attributes.getNode("tid");
                tid = element.getValue<decltype(tid), long long, unsigned int, int>();
                return tid;

            } catch (const Exception& e) {
                KARABO_RETHROW_AS(
                      KARABO_PARAMETER_EXCEPTION("Provided attributes do not contain proper timeId information"));
            }
            return TimeId(tid);
        }


        void TimeId::toHashAttributes(Hash::Attributes& attributes) const {
            attributes.set("tid", m_timeId);
        }

        std::ostream& operator<<(std::ostream& out, const TimeId& trainstamp) {
            out << trainstamp.getTid();
            return out;
        }

        bool operator==(const TimeId& lhs, const TimeId& rhs) {
            return lhs.m_timeId == rhs.m_timeId;
        }

        bool operator!=(const TimeId& lhs, const TimeId& rhs) {
            return lhs.m_timeId != rhs.m_timeId;
        }

    } // namespace data
} // namespace karabo
