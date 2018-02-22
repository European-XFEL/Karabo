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
            sanitize(m_Seconds, m_Fractions);
        }


        TimeDuration::TimeDuration(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions) :
            m_Seconds(days*DAY + hours*HOUR + minutes*MINUTE + seconds),
            m_Fractions(fractions) {
            sanitize(m_Seconds, m_Fractions);
        }


        TimeDuration::~TimeDuration() {
        }


        TimeDuration& TimeDuration::set(const TimeValue seconds, const TimeValue fractions) {
            m_Seconds = seconds;
            m_Fractions = fractions;
            sanitize(m_Seconds, m_Fractions);
            return *this;
        }


        TimeDuration & TimeDuration::set(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions) {
            m_Seconds = days * DAY + hours * HOUR + minutes * MINUTE + seconds;
            m_Fractions = fractions;
            sanitize(m_Seconds, m_Fractions);
            return *this;
        }


        TimeDuration& TimeDuration::add(TimeValue seconds, TimeValue fractions) {
            sanitize(seconds, fractions);
            m_Seconds += seconds;
            m_Fractions += fractions;
            if (m_Fractions > m_oneSecondInAtto) {
                ++m_Seconds;
                m_Fractions -= m_oneSecondInAtto;
            }
            return *this;
        }


        TimeDuration & TimeDuration::add(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions) {
            m_Seconds += days * DAY + hours * HOUR + minutes * MINUTE + seconds;
            m_Fractions += fractions;
            sanitize(m_Seconds, m_Fractions);
            return *this;
        }


        TimeDuration& TimeDuration::sub(const TimeValue seconds, const TimeValue fractions) {
            m_Seconds -= seconds;
            if (m_Fractions < fractions) {
                m_Fractions = (m_oneSecondInAtto - fractions) + m_Fractions;
                --m_Seconds;
            } else {
                m_Fractions -= fractions;
            }
            return *this;
        }


        TimeDuration & TimeDuration::sub(const int days, const int hours, const int minutes, const TimeValue seconds, const TimeValue fractions) {
            m_Seconds -= days * DAY + hours * HOUR + minutes * MINUTE + seconds;
            if (m_Fractions < fractions) {
                m_Fractions = (m_oneSecondInAtto - fractions) + m_Fractions;
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
            while (zeros-- > 0) multiplier *= 10ULL;
            return m_Fractions / multiplier; // unsigned long long dividing
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
            sanitize(m_Seconds, m_Fractions);
        }


        void TimeDuration::toHash(karabo::util::Hash& hash) {
            hash.set<unsigned long long>("seconds", getSeconds());
            hash.set<unsigned long long>("fractions", getFractions(ATTOSEC));
        }


        TimeDuration& TimeDuration::operator*=(TimeValue factor) {
            // Do not care about overflowing seconds - that's billions of years...
            m_Seconds *= factor;

            const std::pair<TimeValue, TimeValue> fractions = safeMultiply(m_Fractions, factor);
            // Treat non-overflow part...
            m_Fractions = fractions.second % m_oneSecondInAtto;
            m_Seconds += (fractions.second / m_oneSecondInAtto);
            // ... and add overflow if needed:
            if (fractions.first) {
                // Calculate duration of single overflow
                TimeValue attos = TimeValue(0ull) - 1ull; // all bits are set, i.e. maximum TimeValue (which is unsigned)!
                TimeValue seconds = 0ull;
                sanitize(seconds, attos);
                // + 1 since overflow is one more than all 64 bits set
                // (no harm if attos + 1 == 1 second - constructor sanitizes)
                TimeDuration overflow(seconds, attos + 1);
                // Multiply with number of overflows and add
                overflow *= fractions.first; // recursion...
                (*this) += overflow;
            }

            return *this;
        }


        unsigned long long safeAddToFirst(unsigned long long& first, unsigned long long second) {
            // Add 'second' to 'first' and return overflow bits shifted by 64 bit to the right

            unsigned long long overflow = 0ull;

            const unsigned long long allBits = -1; // all bits set!
            if (first > allBits - second) {
                ++overflow; // overflow is only one bit
            }
            first += second; // if overflow, remaining bits are OK

            return overflow;
        }


        std::pair<unsigned long long, unsigned long long>
        safeMultiply(unsigned long long a, unsigned long long b) {
            // Inspired by Norman Ramsey's answer at
            // https://stackoverflow.com/questions/1815367/multiplication-of-large-numbers-how-to-catch-overflow

            const unsigned long long maskHigh = (1ull << 32ull) - 1ull; // only lower 32-bits are set

            const unsigned long long aHigh = a >> 32ull;
            const unsigned long long bHigh = b >> 32ull;
            const unsigned long long aLow = a & maskHigh;
            const unsigned long long bLow = b & maskHigh;

            // Now the result is 2**64 * aHigh * bHigh + 2**32 * (aHigh * bLow + aLow * bHigh) + aLow * bLow,
            // so calculate each term:
            unsigned long long low = aLow * bLow;
            const unsigned long long mid1 = aHigh * bLow;
            const unsigned long long mid2 = aLow * bHigh;
            unsigned long long high = aHigh * bHigh;

            // Add high parts of mid1 and mid2 to high:
            high += (mid1 >> 32ull); // Shifting by 32 bits divides by 2**32 while adding it to high implicitly...
            high += (mid2 >> 32ull); // ...multiplies by 2**64, so overall we multiply by 2**32 as desired.

            // Add low parts of mid1 to low, taking care of overflow:
            const unsigned long long mid1Low = mid1 << 32ull; // Shift 32 bits to multiply with 2**32 as desired.
            high += safeAddToFirst(low, mid1Low);

            // Add low parts of mid2 to low, taking care of overflow:
            const unsigned long long mid2Low = mid2 << 32ull; // Shift 32 bits to multiply with 2**32 as desired.
            high += safeAddToFirst(low, mid2Low);

            return std::make_pair(high, low);
        }
    }
}