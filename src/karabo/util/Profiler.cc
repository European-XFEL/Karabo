/* 
 * File:   profiler.cc
 * Author: djelloul.boukhelef@xfel.eu
 * 
 * Created on February 22, 2012, 8:10 AM
 */

#include <sstream>

#include "Profiler.hh"
using namespace std;

namespace karabo {
    namespace util {


        Profiler::Period::Period() : m_name(std::string("")), m_open(false), m_level(0) {
            m_startTime.epoch = 0L;
            m_endTime.epoch = 0L;
        }


        Profiler::Period::Period(const std::string name, int level) : m_name(name), m_open(false), m_level(level) {
            m_startTime.epoch = 0L;
            m_endTime.epoch = 0L;
        }


        Profiler::Period::Period(const std::string name, timestamp start, int level) : m_name(name), m_startTime(start), m_open(true), m_level(level) {
            m_endTime.epoch = 0L;
        }


        Profiler::Period::Period(const std::string name, timestamp start, timestamp end, int level) : m_name(name), m_startTime(start), m_endTime(end), m_open(false), m_level(level) {
        }

        // Initialize a profile with called "name"


        Profiler::Profiler(const std::string profilename) : m_name(profilename), m_openPeriodsCount(0) {
            // timestamp now = HighResolutionTimer::now();
            //            profile.push_back(Period(m_name, now, now));
        }

        // Start an new period with a given name
        // Empty name means that the period is appended to the previous one


        void Profiler::start(const std::string periodname) {
            profile.push_back(Period(periodname, HighResolutionTimer::now(), m_openPeriodsCount));
            mapperiods.insert(std::pair<std::string, size_t > (profile[profile.size() - 1].m_name, profile.size() - 1));
            m_openPeriodsCount++;
        }

        // Stops the last period
        // TODO: implement interleaving periods (ie. reentrance)


        void Profiler::stop(const std::string periodname) {
            timestamp now = HighResolutionTimer::now();

            for (int pos = profile.size() - 1; pos >-1; --pos) {
                if ((profile[pos].m_name == periodname) || periodname.empty()) {
                    if (profile[pos].m_open) {
                        profile[pos].m_open = false;
                        profile[pos].m_endTime = now;

                        m_openPeriodsCount--;

                        return;
                    }
                }
            }
        }

        // Reset the content of the profiler


        void Profiler::reset() {
            m_openPeriodsCount = 0;
            profile.resize(0);
        }

        // Return the global time for this profiler include idle time (ie. between periods)


        timestamp Profiler::getGlobalTime() {
            timestamp result;

            if (m_openPeriodsCount > 0) {
                result.sec = -1;
                result.nsec = -1;
            } else {
                size_t lastpos = profile.size() - 1;
                result.sec = 0;
                result.nsec = 0;
                if (lastpos >= 0) {
                    result = profile[lastpos].m_endTime - profile[0].m_startTime;
                }
            }

            return result;
        }

        // Return the total time of a period at "position"
        // This will sum up the all the sub-periods (ie, unnamed period) the come right after it.


        timestamp Profiler::getTime(size_t position) {
            if (position < profile.size()) {
                timestamp result = profile[position].m_endTime - profile[position].m_startTime;

                for (size_t i = position + 1; i < profile.size(); ++i) {
                    if (!profile[i].m_name.empty() || (profile[i].m_level > profile[position].m_level))
                        break;

                    timestamp df = profile[i].m_endTime - profile[i].m_startTime;

                    result.sec += df.sec;
                    result.nsec += df.nsec;
                }

                result.sec += result.nsec / 1000000000;
                result.nsec %= 1000000000;

                return result;
            }

            throw "No such period";
        }


        // Return the total time of the period called "periodname"
        // This will sum up the all the sub-periods (ie, unnamed period) the come right after it.


        timestamp Profiler::getTime(std::string periodname) {
            std::vector<timestamp> details = getTimeDetails(periodname);

            timestamp result;
            result.nsec = 0;
            result.sec = 0;

            for (size_t i = 0; i < details.size(); ++i) {
                result.nsec += details[i].nsec;
                result.sec += details[i].sec;
            }
            result.sec += result.nsec / 1000000000;
            result.nsec %= 1000000000;

            return result;
        }

        // Return the detailed time profile for the period called "periodname"


        std::vector<timestamp> Profiler::getTimeDetails(std::string periodname) {
            std::pair<std::multimap<std::string, size_t>::iterator, std::multimap<std::string, size_t>::iterator> ret;

            if (periodname.empty()) {
                periodname = this->m_name;
            }

            ret = mapperiods.equal_range(periodname);

            std::vector<timestamp> result;


            if (ret.first == mapperiods.end()) {
                std::cout << "No such period: " << periodname << std::endl;
                return result;
            }

            for (std::multimap<std::string, size_t>::iterator it = ret.first; it != ret.second; ++it) {
                result.push_back(getTime(it->second));
            }

            return result;
        }

        // Returns the effective time for this profiler.
        // This function excludes idle time (ie. between periods)


        timestamp Profiler::getEffectiveTime() {
            timestamp result;

            if (m_openPeriodsCount > 0) {
                result.sec = -1;
                result.nsec = -1;
            } else {
                size_t lastpos = profile.size() - 1;
                result.sec = 0;
                result.nsec = 0;

                if (lastpos >= 0) {
                    for (size_t i = 0; i < profile.size(); ++i) {
                        if (profile[i].m_level > 0)
                            continue; // TODO: Include lower level periods to allow fine-grained timing 

                        timestamp period = profile[i].m_endTime - profile[i].m_startTime;
                        result.sec += period.sec;
                        result.nsec += period.nsec;
                    }

                    result.sec += result.nsec / 1000000000;
                    result.nsec %= 1000000000;
                }
            }

            return result;
        }

        // output the content of profiler as string


        std::string Profiler::report(int level) const {
            std::ostringstream oss;

            oss << m_name << std::endl;

            std::streamsize saveprec = oss.precision(9);

            for (size_t i = 0; i < profile.size(); ++i) {
                if (profile[i].m_level > level)
                    continue;

                oss.width((profile[i].m_level + 1) * 4);
                oss << "";
                oss.width(0);

                if (profile[i].m_name.empty()) {
                    oss.width(4);
                    oss << "- ";
                    oss.width(0);
                } else {
                    oss << profile[i].m_name << ": ";
                }

                timestamp res(profile[i].m_endTime - profile[i].m_startTime);
                bool ongoing = profile[i].m_open;
                for (size_t j = i + 1; j < profile.size(); ++j, ++i) {
                    if (profile[j].m_level <= level) {
                        break;
                    }
                    if (profile[j].m_open) {
                        //                            oss << "ongoing ...";
                        ongoing = true;
                    } else {
                        res = res + (profile[j].m_endTime - profile[j].m_startTime);
                    }
                }
                if (ongoing) {
                    oss << "ongoing ...\n";
                } else {
                    oss << HighResolutionTimer::format(res, "%s.%n") << std::endl;
                }
            }

            oss.precision(saveprec);

            return oss.str();
        }

        // Serialize the profile into standard ostream


        std::ostream& operator <<(std::ostream& os, const Profiler & pr) {
            return os << pr.report();
        }

    }
}
