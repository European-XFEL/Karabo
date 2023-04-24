/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   TimeProfiler.hh
 * Author: boukhelef
 *
 * Created on June 18, 2013, 9:21 PM
 */

#ifndef TIMEPROFILER_HH
#define TIMEPROFILER_HH

#include <stack>
#include <string>

#include "Hash.hh"
#include "TimePeriod.hh"

namespace karabo {
    namespace util {

        class TimeProfiler {
           public:
            /**
             * Constructor creates a profiler with a given name
             * @param name profiler's name
             */
            TimeProfiler(const std::string& name);
            TimeProfiler(const karabo::util::Hash& hash);
            virtual ~TimeProfiler();

            /**
             * Returns the profiler name
             * @return std::string profiler's name
             */
            const std::string& getName() const;

            /**
             * Initialize the profiler internal structure
             */
            void open();

            /**
             * Finalize the profiler internal structure
             */
            void close();

            /**
             * Start a new unnamed period (i.e. detail) and append it to the current open period.
             * Unnamed periods are leaves, thus do cover other sub-periods.
             */
            void startPeriod();

            /**
             * Start an new period with the given name. Named periods can be nested,
             * i.e. named periods can cover other named and anonymous periods
             * @param periodname period's name
             */
            void startPeriod(const std::string& periodname);

            /**
             * Close the last open period
             */
            void stopPeriod();

            /**
             * Stops period "periodname" and all nested periods
             * @param periodname period's name
             */
            void stopPeriod(const std::string& periodname);

            /**
             * Return the time period period "periodname" as Hash
             * @param periodname period's name
             * @return TimePeriod object
             */
            const TimePeriod getPeriod(const std::string& periodname) const;

            /**
             * Returns the overall profiler period, i.e. from open to close.
             * @return TimePeriod object
             */
            const TimePeriod getPeriod() const;

            /**
             * Return the time period period "periodname" as Hash
             * @param periodname period's name
             * @return Hash object
             */
            const karabo::util::Hash& getPeriodAsHash(const std::string& periodname) const;

            /**
             * Returns the overall profiler period.
             * @return Hash object
             */
            const karabo::util::Hash& getPeriodAsHash() const;

            /**
             * Serialize the profiler into Hash object.
             */
            operator karabo::util::Hash();

            /**
             * Serialize the profiler into string using specific time format.
             * @param format time format
             * @param level deepest level
             * @return string object holding the string representation
             */
            std::string format(const std::string& fmt, int level = std::numeric_limits<int>::max()) const;

            /**
             * Serialize the profiler as SQL insert query, in preparation to be inserted into database.
             * @return string object holding the SQL query string
             */
            std::string sql() const;

            /**
             * Serialize the profiler into ostream object using default time format,
             * i.e X.Y (where X is total seconds, and Y is fraction in nanoseconds)
             * @param os output stream object
             * @param format time format
             * @param level deepest level
             */
            void serialize(std::ostream& os, int level = std::numeric_limits<int>::max()) const;
            friend std::ostream& operator<<(std::ostream& os, const TimeProfiler& profiler);

           private:
            std::string m_name;
            karabo::util::Hash m_periods;
            std::stack<karabo::util::Hash*> m_stack;

            static void sql(std::ostream& os, const std::string& name, const karabo::util::Hash& period,
                            const int parent_key);

            static void compact(karabo::util::Hash& period);
        };
    } // namespace util
} // namespace karabo

#endif /* TIMEPROFILER_HH */
