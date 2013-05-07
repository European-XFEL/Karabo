/*
 *
 * File:   Timestamp.cc
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 23, 2013, 11:53 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef TIMESTAMP_HH
#define	TIMESTAMP_HH

#include <boost/date_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>

namespace karabo {

    /**
     * Namespace for package packageName
     */
    namespace util {

        /**
         * The Timestamp class.
         * More detailed comments
         */
        class Timestamp {

            static boost::posix_time::ptime m_epoch;
            boost::posix_time::ptime m_timePoint;
            unsigned long long m_msSinceEpoch;

        public:
            /**
             * Constructors.
             */
            Timestamp();
            Timestamp(const boost::posix_time::ptime pt);
            Timestamp(const std::string& timeStr);
            virtual ~Timestamp();

            /*
             * Getters and Setters
             */
            void setMsSinceEpoch(const unsigned long long msSinceEpoch);
            void setTime(const boost::posix_time::ptime& timePoint);
            void setTime(const std::string& timePoint);
            //
            boost::posix_time::ptime getTime() const;
            unsigned long long getMsSinceEpoch() const;
            unsigned long long getTrainId() const;

            /*
             * Class logic functions
             */
            std::string toString();
            std::string toIsoString();
            std::string toFormattedString(const std::string& format = "%Y-%b-%d %H:%M:%S");

            static unsigned long long calculateAndReturnMsSinceEpoch(const boost::posix_time::ptime now);
            //
            boost::posix_time::ptime getStringFormated2PTime(const std::string& timestampStr, const boost::posix_time::time_input_facet* facet);
            boost::posix_time::ptime getStringFormated2PTime(const std::string& timestampStr);
            boost::posix_time::ptime getUniversalString2PTime(const std::string& timestampStr);

        private:
            std::string toStandardString(const std::string& format);
            std::string getPTime2String(const boost::posix_time::ptime pt, const boost::posix_time::time_facet* facet);

            time_t ptime_to_time_t(const boost::posix_time::ptime t);
            boost::posix_time::ptime local_ptime_from_utc_time_t(std::time_t const t);
        };

    } // namespace packageName
} // namespace karabo

#endif	/* TIMESTAMP_HH */
