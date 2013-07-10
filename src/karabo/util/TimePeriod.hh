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

        class TimePeriod {

        public:
            TimePeriod();
            TimePeriod(const karabo::util::Hash& hash);
            TimePeriod(const Epochstamp& start, const Epochstamp& stop);
            virtual ~TimePeriod();

            TimeDuration getDuration() const;

            Epochstamp getStart() const;
            Epochstamp getStop() const;

            void start(const Epochstamp& tm = Epochstamp());
            void stop(const Epochstamp& tm = Epochstamp());

            bool isOpen() const;

            bool before(const Epochstamp& tm) const;
            bool contain(const Epochstamp& tm) const;
            bool after(const Epochstamp& tm) const;

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

