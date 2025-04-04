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
 * File:   TrainStamp.cc
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#include "Trainstamp.hh"

namespace karabo {
    namespace data {


        Trainstamp::Trainstamp() : m_trainId(0) {}


        Trainstamp::Trainstamp(const unsigned long long trainId) : m_trainId(trainId) {}


        Trainstamp::~Trainstamp() {}


        bool Trainstamp::hashAttributesContainTimeInformation(const Hash::Attributes& attributes) {
            return attributes.has("tid");
        }


        Trainstamp Trainstamp::fromHashAttributes(const Hash::Attributes& attributes) {
            unsigned long long tid;

            try {
                auto& element = attributes.getNode("tid");
                tid = element.getValue<decltype(tid), long long, unsigned int, int>();
                return tid;

            } catch (const Exception& e) {
                KARABO_RETHROW_AS(
                      KARABO_PARAMETER_EXCEPTION("Provided attributes do not contain proper trainId information"));
            }
            return Trainstamp(tid);
        }


        void Trainstamp::toHashAttributes(Hash::Attributes& attributes) const {
            attributes.set("tid", m_trainId);
        }

        std::ostream& operator<<(std::ostream& out, const Trainstamp& trainstamp) {
            out << trainstamp.getTrainId();
            return out;
        }

        bool operator==(const Trainstamp& lhs, const Trainstamp& rhs) {
            return lhs.m_trainId == rhs.m_trainId;
        }

        bool operator!=(const Trainstamp& lhs, const Trainstamp& rhs) {
            return lhs.m_trainId != rhs.m_trainId;
        }

    } // namespace data
} // namespace karabo
