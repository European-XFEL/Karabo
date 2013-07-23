/* 
 * File:   TimeProfiler.hh
 * Author: boukhelef
 *
 * Created on June 18, 2013, 9:21 PM
 */

#ifndef TIMEPROFILER_HH
#define	TIMEPROFILER_HH

#include <stack>
#include <string>

#include <karabo/util/Hash.hh>
#include "TimePeriod.hh"

namespace karabo {
    namespace util {

        class TimeProfiler {

        public:
            TimeProfiler(const std::string& name);
            TimeProfiler(const karabo::util::Hash& hash);
            virtual ~TimeProfiler();

            // Returns the profiler name
            const std::string& getName() const;

            // Initialize/finalize the profiler internal structure
            void open();
            void close();

            // Start a new detail and append it to the current open period
            // Anonymous periods are leaves and cannot cover sub-periods
            void startPeriod();

            // Start an new period with the given name
            // Named periods can be nested, ie named periods can cover other named and anonymous periods
            void startPeriod(const std::string& periodname);

            // Stops the last period
            void stopPeriod();

            // Stops period "periodname" and all nested periods
            void stopPeriod(const std::string& periodname);

            // Return the time period period "periodname" as Hash
            const TimePeriod getPeriod(const std::string& periodname) const;

            // Returns the overall profiler period.
            const TimePeriod getPeriod() const;

            // Return the time period period "periodname" as Hash
            const karabo::util::Hash& getPeriodAsHash(const std::string& periodname) const;

            // Returns the overall profiler period.
            const karabo::util::Hash& getPeriodAsHash() const;

            operator karabo::util::Hash();
            // Serialize the profiler into string using specific time format
            std::string format(const std::string& fmt, int level = std::numeric_limits<int>::max()) const;
            std::string sql() const;

            // Serialize the profiler into ostream object using default time format, i.e X.Y (where X is total seconds, and Y is fraction in nanoseconds)
            void serialize(std::ostream& os, int level = std::numeric_limits<int>::max()) const;
            friend std::ostream& operator <<(std::ostream& os, const TimeProfiler & profiler);

        private:
            std::string m_name;
            std::stack<karabo::util::Hash*> m_stack;
            karabo::util::Hash m_periods;

            static void sql(std::ostream& os, const std::string& name, const karabo::util::Hash& period, const int parent_key);

            static void compact(karabo::util::Hash& period);
        };
    }
}

#endif	/* TIMEPROFILER_HH */
