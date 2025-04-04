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
 * File:   Timestamp.cc
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#include "Timestamp.hh"

namespace karabo {
    namespace data {


        Timestamp::Timestamp() : m_epochstamp(Epochstamp()), m_trainstamp(Trainstamp()) {}


        Timestamp::Timestamp(const Epochstamp& e, const Trainstamp& t) : m_epochstamp(e), m_trainstamp(t) {}


        Timestamp::~Timestamp() {}


        bool Timestamp::hashAttributesContainTimeInformation(const Hash::Attributes& attributes) {
            return (Epochstamp::hashAttributesContainTimeInformation(attributes) &&
                    Trainstamp::hashAttributesContainTimeInformation(attributes));
        }


        Timestamp Timestamp::fromHashAttributes(const Hash::Attributes& attributes) {
            return Timestamp(Epochstamp::fromHashAttributes(attributes), Trainstamp::fromHashAttributes(attributes));
        }


        std::string Timestamp::toIso8601(TIME_UNITS precision, bool extended) const {
            return m_epochstamp.toIso8601(precision, extended);
        }


        std::string Timestamp::toIso8601Ext(TIME_UNITS precision, bool extended) const {
            return m_epochstamp.toIso8601Ext(precision, extended);
        }


        std::string Timestamp::toFormattedString(const std::string& format, const std::string& localTimeZone) const {
            return m_epochstamp.toFormattedString(format, localTimeZone);
        }


        std::string Timestamp::toFormattedStringLocale(const std::string& localeName, const std::string& format,
                                                       const std::string& localTimeZone) const {
            return m_epochstamp.toFormattedStringLocale(localeName, format, localTimeZone);
        }


        double Timestamp::toTimestamp() const {
            return m_epochstamp.toTimestamp();
        }


        void Timestamp::toHashAttributes(Hash::Attributes& attributes) const {
            m_epochstamp.toHashAttributes(attributes);
            m_trainstamp.toHashAttributes(attributes);
        }


        bool operator==(const Timestamp& lhs, const Timestamp& rhs) {
            return (lhs.m_epochstamp == rhs.m_epochstamp && lhs.m_trainstamp == rhs.m_trainstamp);
        }


        bool operator!=(const Timestamp& lhs, const Timestamp& rhs) {
            return (lhs.m_epochstamp != rhs.m_epochstamp || lhs.m_trainstamp != rhs.m_trainstamp);
        }


        std::ostream& operator<<(std::ostream& out, const Timestamp& timestamp) {
            out << "Epoch: " << timestamp.getEpochstamp() << " Train ID: " << timestamp.getTrainstamp();
            return out;
        }
    } // namespace data
} // namespace karabo
