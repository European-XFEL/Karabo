/*
 * $Id: Time.hh 6711 2012-07-05 10:15:39Z heisenb $
 *
 * File:   Time.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 17, 2010, 1:43 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_TIME_HH
#define	KARABO_UTIL_TIME_HH

#include <boost/date_time/local_time/local_time.hpp>

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package packageName
     */
    namespace util {

        /**
         * The Time class.
         * More detailed comments
         */
        class Time {

        public:

            /**
             * Default constructor.
             */
            Time();

            virtual ~Time();

            static unsigned long long getMsSinceEpoch();

            //static std::string getCurrentDateTime(const std::string& format="%Y-%m-%d %H:%M:%S");

        protected:

        private:

            static boost::posix_time::ptime m_epoch;

        };

        /**
         * Placeholder for later timestamp
         */
        class Timestamp {
            
            static boost::posix_time::ptime m_epoch;
            boost::posix_time::ptime m_now;
            unsigned long long m_msSinceEpoch;
            
            
        public:
            
            Timestamp() : m_now(boost::posix_time::microsec_clock::universal_time()) {
                 boost::posix_time::time_duration diff = m_now - m_epoch;
                m_msSinceEpoch = diff.total_milliseconds();
            }

            unsigned long long getAsTrainId() const {
                return 0;
            }

            unsigned long long getAsMsSinceEpoch() const {
                return m_msSinceEpoch;
            }

        };

    } // namespace util
} // namespace karabo

#endif	/* KARABO_UTIL_TIME_HH */
