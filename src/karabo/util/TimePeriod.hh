/* 
 * File:   TimePeriod.hh
 * Author: boukhelef
 *
 * Created on April 28, 2013, 11:02 PM
 */

#ifndef TIMEPERIOD_HH
#define	TIMEPERIOD_HH

#include <karabo/util/Hash.hh>

#include "Epochstamp.hh"
#include "TimeDuration.hh"

namespace karabo {
    namespace util {

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
            TimePeriod(const karabo::util::Hash& hash);
            
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
            void fromHash(const karabo::util::Hash& hash);
            void toHash(karabo::util::Hash& hash);

            operator karabo::util::Hash() {
                karabo::util::Hash hash;
                toHash(hash);
                return hash;
            }

        private:
            bool m_Open;
            Epochstamp m_Start;
            Epochstamp m_Stop;
        };

    }
}
#endif	/* TIMEPERIOD_HH */

