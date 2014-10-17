/* 
 * File:   TimeDuration.cc
 * Author: boukhelef
 * 
 * Created on April 27, 2013, 10:55 PM
 */

#include "TimeDuration.hh"

namespace karabo {
    namespace util {

        std::string TimeDuration::DEFAULT_FORMAT("%s.%N");


        TimeDuration::TimeDuration() :
        m_Seconds(0ULL),
        m_Fractions(0ULL) {
        }


        TimeDuration::TimeDuration(const karabo::util::Hash& hash) {
            fromHash(hash);
        }


        TimeDuration::TimeDuration(const TimeValue seconds, const TimeValue fractions) :
        m_Seconds(seconds),
        m_Fractions(fractions) {
        }


        TimeDuration::TimeDuration(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions) :
        m_Seconds(days*DAY + hours*HOUR + minutes*MINUTE + seconds),
        m_Fractions(fractions) {
        }


        TimeDuration::~TimeDuration() {
        }


        TimeDuration& TimeDuration::set(const TimeValue seconds, const TimeValue fractions) {
            m_Seconds = seconds;
            m_Fractions = fractions;
            return *this;
        }


        TimeDuration & TimeDuration::set(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions) {
            m_Seconds = days * DAY + hours * HOUR + minutes * MINUTE + seconds;
            m_Fractions = fractions;
            return *this;
        }


        TimeDuration& TimeDuration::add(const TimeValue seconds, const TimeValue fractions) {
            m_Seconds += seconds;
            m_Fractions += fractions;
            unsigned long long onesec = 1000000000000000000ULL; // with 18 zeros
            if (m_Fractions > onesec) {
                ++m_Seconds;
                m_Fractions -= onesec;
            }
            return *this;
        }


        TimeDuration & TimeDuration::add(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions) {
            m_Seconds += days * DAY + hours * HOUR + minutes * MINUTE + seconds;
            m_Fractions += fractions;
            unsigned long long onesec = 1000000000000000000ULL; // with 18 zeros
            if (m_Fractions > onesec) {
                ++m_Seconds;
                m_Fractions -= onesec;
            }
            return *this;
        }


        TimeDuration& TimeDuration::sub(const TimeValue seconds, const TimeValue fractions) {
            m_Seconds -= seconds;
            unsigned long long onesec = 1000000000000000000ULL; // with 18 zeros
            if (m_Fractions < fractions) {
                m_Fractions = (onesec - fractions) + m_Fractions;
                --m_Seconds;
            } else {
                m_Fractions -= fractions;
            }
            return *this;
        }


        TimeDuration & TimeDuration::sub(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions) {
            m_Seconds -= days * DAY + hours * HOUR + minutes * MINUTE + seconds;
            unsigned long long onesec = 1000000000000000000ULL; // with 18 zeros
            if (m_Fractions < fractions) {
                m_Fractions = (onesec - fractions) + m_Fractions;
                --m_Seconds;
            } else {
                m_Fractions -= fractions;
            }
            return *this;
        }


        bool TimeDuration::isNull() const {
            return (m_Seconds == 0ull) && (m_Fractions == 0ull);
        }


        uint64_t TimeDuration::getDays() const {
            return m_Seconds / DAY;
        }


        uint64_t TimeDuration::getHours() const {
            return (m_Seconds / HOUR) % 24;

        }


        TimeValue TimeDuration::getTotalHours() const {
            return m_Seconds / HOUR;
        }


        uint64_t TimeDuration::getMinutes() const {
            return (m_Seconds / MINUTE) % 60;
        }


        TimeValue TimeDuration::getTotalMinutes() const {
            return m_Seconds / MINUTE;
        }


        TimeValue TimeDuration::getSeconds() const {
            return m_Seconds % 60;
        }


        TimeValue TimeDuration::getTotalSeconds() const {
            return m_Seconds;
        }


        TimeValue TimeDuration::getFractions(const TIME_UNITS unit) const {
            int zeros = int(unit);
            unsigned long long multiplier = 1ULL;
            while(zeros-->0) multiplier *= 10ULL;
            return m_Fractions / multiplier;  // unsigned long long dividing
        }


        std::string TimeDuration::format(const std::string& fmt) const {
            std::ostringstream oss;
            oss.fill('0');
            for (char const * ptr = fmt.c_str(); *ptr; ++ptr) {
                if (*ptr == '%') {
                    ++ptr;
                    switch (*ptr) {
                        case 'd':
                        case 'D': oss << getDays();
                            break;
                        case 'H': oss.width(2);
                            oss << getHours();
                            break;
                        case 'M':oss.width(2);
                            oss << getMinutes();
                            break;
                        case 'S': oss.width(2);
                            oss << getSeconds();
                            break;
                        case 'h': oss.width(0);
                            oss << getHours();
                            break;
                        case 'm': oss.width(0);
                            oss << getMinutes();
                            break;
                        case 's': oss.width(0);
                            oss << getSeconds();
                            break;
                        default:
                        {
                            int w;
                            TIME_UNITS p = ATTOSEC;
                            switch (*ptr) {
                                case 'l': w = 0, p = MILLISEC;
                                    break;
                                case 'u': w = 0, p = MICROSEC;
                                    break;
                                case 'n': w = 0, p = NANOSEC;
                                    break;
                                case 'p': w = 0, p = PICOSEC;
                                    break;
                                case 'f': w = 0, p = FEMTOSEC;
                                    break;
                                case 'a': w = 0, p = ATTOSEC;
                                    break;
                                case 'L': w = 3, p = MILLISEC;
                                    break;
                                case 'U': w = 6, p = MICROSEC;
                                    break;
                                case 'N': w = 9, p = NANOSEC;
                                    break;
                                case 'P': w = 12, p = PICOSEC;
                                    break;
                                case 'F': w = 15, p = FEMTOSEC;
                                    break;
                                case 'A': w = 18, p = ATTOSEC;
                                    break;
                                default: throw "Unrecognized format";
                            }
                            oss.width(w);
                            oss << getFractions(p);
                        }
                    }
                } else {
                    oss << *ptr;
                }
            }

            return oss.str();
        }


        void TimeDuration::fromHash(const karabo::util::Hash& hash) {
            m_Seconds = hash.get<unsigned long long>("seconds");
            m_Fractions = hash.get<unsigned long long>("fractions");
        }


        void TimeDuration::toHash(karabo::util::Hash& hash) {
            hash.set<unsigned long long>("seconds", getSeconds());
            hash.set<unsigned long long>("fractions", getFractions(ATTOSEC));
        }
    }
}