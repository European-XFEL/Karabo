/* 
 * File:   TimePeriod.cc
 * Author: boukhelef
 * 
 * Created on April 28, 2013, 11:02 PM
 */

#include "TimePeriod.hh"

namespace karabo {
    namespace util {


        TimePeriod::TimePeriod() : m_Open(false) {
        }


        TimePeriod::TimePeriod(const karabo::util::Hash& hash) {
            this->fromHash(hash);
        }

        TimePeriod::TimePeriod(const Epochstamp& start, const Epochstamp& stop) :
        m_Open(false), m_Start(start), m_Stop(stop) {
        }


        TimePeriod::~TimePeriod() {
        }


        TimeDuration TimePeriod::getDuration() const {
            return m_Open ? TimeDuration(-1, -1) : m_Stop - m_Start;
        }


        Epochstamp TimePeriod::getStart() const {
            return m_Start;
        }


        Epochstamp TimePeriod::getStop() const {
            return m_Stop;
        }


        void TimePeriod::start(const Epochstamp& tm) {
            m_Start = tm;
            m_Open = true;
        }


        void TimePeriod::stop(const Epochstamp& tm) {
            m_Open = false;
            m_Stop = tm;
        }


        bool TimePeriod::isOpen() const {
            return m_Open;
        }


        bool TimePeriod::before(const Epochstamp& tm) const {
            return !(m_Open || m_Stop > tm);
        }


        bool TimePeriod::contain(const Epochstamp& tm) const {
            return (tm >= m_Start && (m_Open || tm <= m_Stop));
        }


        bool TimePeriod::after(const Epochstamp& tm) const {
            return (m_Start >= tm);
        }


        void TimePeriod::fromHash(const karabo::util::Hash& hash) {
            m_Start = Epochstamp::fromHashAttributes(hash.getAttributes("KRB_start"));
            m_Stop = Epochstamp::fromHashAttributes(hash.getAttributes("KRB_stop"));
            m_Open = hash.has("KRB_open") ? hash.get<bool>("KRB_open") : false;
        }


        void TimePeriod::toHash(karabo::util::Hash& hash) {
            Hash::Attributes now;
            hash.set("KRB_start", "");
            m_Start.toHashAttributes(now);
            hash.setAttributes("KRB_start", now);

            hash.set("KRB_stop", "");
            m_Stop.toHashAttributes(now);
            hash.setAttributes("KRB_stop", now);

            hash.set("KRB_open", m_Open);
        }

    }
}
