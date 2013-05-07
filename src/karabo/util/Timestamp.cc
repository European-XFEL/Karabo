/*
 *
 * File:   Timestamp.cc
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 23, 2013, 11:53 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Timestamp.hh"
#include "Exception.hh"


namespace karabo {
    namespace util {

        using namespace boost::posix_time;
        using namespace boost::gregorian;
        using namespace std;

        const locale inputs[] = {
            locale(locale::classic(), new time_input_facet("%m/%d/%Y")),
            locale(locale::classic(), new time_input_facet("%Y-%m-%dT%H:%M:%S%f%q")), //ISO-8601
            locale(locale::classic(), new time_input_facet("%Y-%m-%d %H:%M:%S%f")),
            locale(locale::classic(), new time_input_facet("%Y-%m-%d %H:%M:%S")),
            locale(locale::classic(), new time_input_facet("%Y%m%dT%H%M%S%f%q")), // UNIVERSAL
            locale(locale::classic(), new time_input_facet("%Y%m%d%H%M%S")),
            locale(locale::classic(), new time_input_facet("%Y%m%d%H%M")),
            locale(locale::classic(), new time_input_facet("%Y%m%d"))
        };
        const size_t formats = sizeof (inputs) / sizeof (inputs[0]);

        ptime Timestamp::m_epoch = ptime(date(1970, 1, 1));


        /**
         * Constructors.
         */
        Timestamp::Timestamp() : m_timePoint(boost::posix_time::microsec_clock::universal_time()) {
            boost::posix_time::time_duration diff = m_timePoint - m_epoch;
            m_msSinceEpoch = diff.total_milliseconds();
        }


        Timestamp::Timestamp(const boost::posix_time::ptime pt) : m_timePoint(pt) {
            boost::posix_time::time_duration diff = m_timePoint - m_epoch;
            m_msSinceEpoch = diff.total_milliseconds();
        }


        Timestamp::Timestamp(const std::string& timeStr) : m_timePoint(Timestamp::getUniversalString2PTime(timeStr)) {
            boost::posix_time::time_duration diff = m_timePoint - m_epoch;
            m_msSinceEpoch = diff.total_milliseconds();
        }


        Timestamp::~Timestamp() {
        }


        /*
         * Getters and Setters
         */
        void Timestamp::setTime(const boost::posix_time::ptime& timePoint) {
            this->m_timePoint = timePoint;
            boost::posix_time::time_duration diff = timePoint - m_epoch;
            this->m_msSinceEpoch = diff.total_milliseconds();
        }


        void Timestamp::setTime(const std::string& timePoint) {
            boost::posix_time::ptime now = getStringFormated2PTime(timePoint);
            Timestamp::setTime(now);
        }


        boost::posix_time::ptime Timestamp::getTime() const {
            return m_timePoint;
        }


        unsigned long long Timestamp::getMsSinceEpoch() const {
            return m_msSinceEpoch;
        }


        unsigned long long Timestamp::getTrainId() const {
            //return 0;
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("not yet implemented");
        }

        /*
         * Class logic functions
         */


        /*
         * Returns timestamp string in "UNIVERSAL" format
         * "UNIVERSAL" => "%Y%m%dT%H%M%S%f%q"
         */
        std::string Timestamp::toString() {
            return toStandardString("UNIVERSAL");
        }


        /*
         * Returns timestamp string in "ISO 8601" format
         * "ISO 8601"  => "%Y-%m-%dT%H:%M:%S.%f%q"
         */
        std::string Timestamp::toIsoString() {
            return toStandardString("ISO8601");
        }


        /*
         * Returns timestamp string in "Human Readable" format
         * "Human Readable" => "%Y-%b-%d %H:%M:%S"
         */
        std::string Timestamp::toFormattedString(const std::string& format) {
            //boost::shared_ptr<boost::posix_time::time_facet> facet(new boost::posix_time::time_facet(format.c_str()));
            boost::posix_time::time_facet* facet = new boost::posix_time::time_facet(format.c_str());
            std::string timestampStr = getPTime2String(this->m_timePoint, facet);
            return timestampStr;
        }


        /*
         * Returns timestamp string in "ISO 8601" or "UNIVERSAL" format
         * "ISO 8601"  => "%Y-%m-%dT%H:%M:%S.%f%q"
         * "UNIVERSAL" => "%Y%m%dT%H%M%S%f%q"
         */
        std::string Timestamp::toStandardString(const std::string& format) {
            std::string timestampStr;

            if (format == "ISO8601") {
                timestampStr = to_iso_extended_string(this->m_timePoint);
            } else if (format == "UNIVERSAL") {
                timestampStr = to_iso_string(this->m_timePoint);
            } else {
                timestampStr = "Error: Not-a-Timestamp";
            }
            return timestampStr;
        }


        /*
         * Returns timestamp string in "ANY SPECIFIED" format
         * "DEFAULT" example => "10/31/2005 08:00:00 PM"
         */
        std::string Timestamp::getPTime2String(const boost::posix_time::ptime pt, const boost::posix_time::time_facet* facet) {
            //std::string Timestamp::getPTime2String(const boost::posix_time::ptime pt, const boost::shared_ptr<boost::posix_time::time_facet>& facet) {
            std::ostringstream datetime_ss;

            // special_locale takes ownership of the p_time_output facet
            std::locale special_locale(std::locale(""), facet /*.get()*/);

            datetime_ss.imbue(special_locale);

            datetime_ss << pt;

            // return timestamp as string
            return datetime_ss.str();
        }


        unsigned long long Timestamp::calculateAndReturnMsSinceEpoch(const boost::posix_time::ptime now) {
            time_duration diff = now - m_epoch;
            return diff.total_milliseconds();
        }


        /*
         * Returns ptime receiving a string as input in "UNIVERSAL" format
         * "UNIVERSAL" => "%Y%m%dT%H%M%S%f%q" (example: "20121225T132536.789333")
         */
        boost::posix_time::ptime Timestamp::getUniversalString2PTime(const std::string& timestampStr) {
            boost::posix_time::ptime pt = boost::posix_time::from_iso_string(timestampStr);
            return pt;
        }


        boost::posix_time::ptime Timestamp::getStringFormated2PTime(const std::string& timestampStr) {

            throw KARABO_NOT_SUPPORTED_EXCEPTION("To solve Warning problem!");

            //            for (size_t i = 0; i < formats; ++i) {
            //                std::istringstream ss(timestampStr);
            //                ss.imbue(inputs[i]);
            //                boost::posix_time::ptime this_time;
            //
            //                // Warning here!
            //                ss >> this_time;
            //
            //                if (this_time != boost::posix_time::not_a_date_time)
            //                    return this_time;
            //            }
            //
            //            // If a match format is not find
            //            throw KARABO_NOT_SUPPORTED_EXCEPTION("Not a valid format");
        }


        boost::posix_time::ptime Timestamp::getStringFormated2PTime(const std::string& timestampStr, const boost::posix_time::time_input_facet* facet) {

            throw KARABO_NOT_SUPPORTED_EXCEPTION("To solve Warning problem!");

            //            const boost::posix_time::time_input_facet* finalTimestampFormat;
            //            if (facet == NULL) {
            //                return Timestamp::getStringFormated2PTime(timestampStr);
            //            } else {
            //                finalTimestampFormat = facet;
            //            }
            //
            //            boost::posix_time::ptime pt;
            //
            //            std::istringstream iss(timestampStr);
            //            std::locale official_format = std::locale(iss.getloc(), finalTimestampFormat);
            //            iss.imbue(official_format);
            //
            //            // Warning here!
            //            iss >> pt;
            //
            //            return pt;
        }


        time_t Timestamp::ptime_to_time_t(const boost::posix_time::ptime t) {
            static boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
            return (t - epoch).ticks() / boost::posix_time::time_duration::ticks_per_second();
        }


        boost::posix_time::ptime Timestamp::local_ptime_from_utc_time_t(std::time_t const t) {
            using boost::date_time::c_local_adjustor;
            using boost::posix_time::from_time_t;
            using boost::posix_time::ptime;
            return c_local_adjustor<ptime>::utc_to_local(from_time_t(t));
        }

    } // namespace packageName
} // namespace karabo
