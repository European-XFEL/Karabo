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
 * File:   Timestamp.hh
 * Author: WP76
 *
 * Created on June 19, 2013, 3:22 PM
 */

#ifndef KARABO_DATA_TIME_TIMESTAMP_HH
#define KARABO_DATA_TIME_TIMESTAMP_HH

#include "Epochstamp.hh"
#include "TimeId.hh"


namespace karabo {
    namespace data {

        /**
         * This class expresses a time point and holds it in form of an Epochstamp and TimeId
         */
        class Timestamp {
            Epochstamp m_epochstamp;
            TimeId m_timeId;

           public:
            using enum TIME_UNITS;

            Timestamp();

            Timestamp(const Epochstamp& e, const TimeId& t);

            /**
             * Return Epochstamp part of the timestamp
             * @return
             */
            inline const Epochstamp& getEpochstamp() const {
                return m_epochstamp;
            }

            /**
             * Return TimeId part of the timestamp
             * @return
             */
            inline const TimeId& getTimeId() const {
                return m_timeId;
            }

            /**
             * Return the seconds entry of the timestamp
             * @return
             */
            inline const unsigned long long& getSeconds() const {
                return m_epochstamp.getSeconds();
            }

            /**
             * Return the fractional seconds entry of the timestamp
             * @return
             */
            inline const unsigned long long& getFractionalSeconds() const {
                return m_epochstamp.getFractionalSeconds();
            }

            /**
             * Return the train id entry of the timestamp
             * @return
             */
            inline const unsigned long long& getTid() const {
                return m_timeId.getTid();
            }

            static bool hashAttributesContainTimeInformation(const Hash::Attributes& attributes);

            /**
             * Creates an Timestamp from three Hash attributes
             * This function throws in case the attributes do no provide the correct information
             * @param attributes Hash attributes
             * @return Timestamp object
             */
            static Timestamp fromHashAttributes(const Hash::Attributes& attributes);

            /**
             * Generates a sting (respecting ISO-8601) for object time for INTERNAL usage ("%Y%m%dT%H%M%S%f" =>
             * "20121225T132536.789333[123456789123]")
             *
             * @param precision - Indicates the precision of the fractional seconds (e.g. MILLISEC, MICROSEC, NANOSEC,
             * PICOSEC, FEMTOSEC, ATTOSEC) [Default: MICROSEC]
             * @param extended - "true" returns ISO8601 extended string; "false" returns ISO8601 compact string
             * [Default: false]
             * @return ISO 8601 formatted string (extended or compact)
             */
            std::string toIso8601(TIME_UNITS precision = MICROSEC, bool extended = false) const;

            /**
             * Generates a sting (respecting ISO-8601) for object time for EXTERNAL usage ("%Y%m%dT%H%M%S%f%z" =>
             * "20121225T132536.789333[123456789123]Z")
             *
             * @param precision - Indicates the precision of the fractional seconds (e.g. MILLISEC, MICROSEC, NANOSEC,
             * PICOSEC, FEMTOSEC, ATTOSEC) [Default: MICROSEC]
             * @param extended - "true" returns ISO8601 extended string; "false" returns ISO8601 compact string
             * [Default: false]
             * @return ISO 8601 formatted string with "Z" in the string end ("Z" means the date time zone is using
             * Coordinated Universal Time - UTC)
             */
            std::string toIso8601Ext(TIME_UNITS precision = MICROSEC, bool extended = false) const;

            /**
             * Formats to specified format time stored in the object
             *
             * @param format The format of the time point (visit strftime for more info:
             * http://www.cplusplus.com/reference/ctime/strftime/) [Default: "%Y-%b-%d %H:%M:%S"]
             * @param localTimeZone - String that represents an ISO8601 time zone [Default: "Z" == UTC]
             * @return formated string in the specified Time Zone
             */
            std::string toFormattedString(const std::string& format = std::string("%Y-%b-%d %H:%M:%S"),
                                          const std::string& localTimeZone = std::string("Z")) const;

            /**
             * Formats to specified format time stored in the object
             *
             * @param localeName - String that represents the locale to be used [Default: "" == System locale]
             * @param format The format of the time point (visit strftime for more info:
             * http://www.cplusplus.com/reference/ctime/strftime/) [Default: "%Y-%b-%d %H:%M:%S"]
             * @param localTimeZone - String that represents an ISO8601 time zone [Default: "Z" == UTC]
             * @return formated string in the specified Time Zone
             */
            std::string toFormattedStringLocale(const std::string& localeName = std::string(""),
                                                const std::string& format = std::string("%Y-%b-%d %H:%M:%S"),
                                                const std::string& localTimeZone = std::string("Z")) const;

            /**
             * Generates a timestamp as double with seconds.fractional format (fractional precision == MICROSEC)
             * Function necessary to use in graphs plotting in Python code (MICROSEC precision is enough)
             *
             * @return A double value with the decimal point indicating fractions of seconds
             */
            double toTimestamp() const;

            /**
             * Formats as Hash attributes
             * @param attributes container to which the time point information is added
             */
            void toHashAttributes(Hash::Attributes& attributes) const;

            virtual ~Timestamp();

            /**
             * Compare if the Epochstamp and TimeId of this Timestamp are the same as those of other
             * @param other
             * @return
             */
            friend bool operator==(const Timestamp& lhs, const Timestamp& rhs);

            /**
             * Compare if the Epochstamp and/or TimeId of this Timestamp are not the same of other
             * @param other
             * @return
             */
            friend bool operator!=(const Timestamp& lhs, const Timestamp& rhs);

           private:
        };

        std::ostream& operator<<(std::ostream&, const Timestamp& timestamp);

    } // namespace data
} // namespace karabo

#endif
