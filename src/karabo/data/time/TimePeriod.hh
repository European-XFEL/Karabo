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
 * File:   TimePeriod.hh
 * Author: boukhelef
 *
 * Created on April 28, 2013, 11:02 PM
 */

#ifndef TIMEPERIOD_HH
#define TIMEPERIOD_HH

#include "Epochstamp.hh"
#include "TimeDuration.hh"
#include "karabo/data/types/Hash.hh"

namespace karabo {
    namespace data {

        /**
         * This class models a time period expressed by two time points, i.e. start and stop.
         *
         * The default constructor create a time period of length zero, i.e. start=stop.
         */

        class TimePeriod {
           public:
            /**
             * Default constructor creates and empty time period
             */
            TimePeriod();

            /**
             * Constructs a time period from Hash. Start and stop timestamps are stored
             * under the two reserved keys "KRB_start" and "KRB_stop", respectively.
             * @param hash Hash object ("KRB_start", any, "KRB_stop", any)
             */
            TimePeriod(const karabo::data::Hash& hash);

            /**
             * Constructs a time period from two given timestamps
             * @param start Epochstamp object
             * @param stop Epochstamp object
             */
            TimePeriod(const Epochstamp& start, const Epochstamp& stop);
            virtual ~TimePeriod();

            /**
             * Return the time duration (i.e. length) of a time period.
             * @return TimeDuration object
             */
            TimeDuration getDuration() const;

            /**
             * Get the start (resp. stop) timestamp
             * @return Epochstamp object
             */
            Epochstamp getStart() const;
            Epochstamp getStop() const;

            /**
             * Set the start (resp. stop) timestamp. By default, it set it to the current system epoch timestamp.
             * @param tm Epochstamp (by default, use the current system timestamp)
             */
            void start(const Epochstamp& tm = Epochstamp());
            void stop(const Epochstamp& tm = Epochstamp());

            /**
             * Check if period is still open (i.e. not yet stopped)
             * @return bool
             */
            bool isOpen() const;

            /**
             * Check if time point (timestamp) is before, within, or after a time period.
             * @param tm Epochstamp object
             * @return bool
             */
            bool before(const Epochstamp& tm) const;
            bool contain(const Epochstamp& tm) const;
            bool after(const Epochstamp& tm) const;

            /**
             * Serialize time period to and from Hash object.
             * @param hash Hash object
             */
            void fromHash(const karabo::data::Hash& hash);
            void toHash(karabo::data::Hash& hash);

            operator karabo::data::Hash() {
                karabo::data::Hash hash;
                toHash(hash);
                return hash;
            }

           private:
            bool m_Open;
            Epochstamp m_Start;
            Epochstamp m_Stop;
        };

    } // namespace data
} // namespace karabo
#endif /* TIMEPERIOD_HH */
