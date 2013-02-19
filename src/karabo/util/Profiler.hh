/* 
 * File:   profiler.hh
 * Author: djelloul.boukhelef@xfel.eu
 *
 * Created on February 22, 2012, 8:10 AM
 */

#ifndef PROFILER_HH
#define	PROFILER_HH

#include <ctime>
#include <string>
#include <map>
#include <vector>
#include <iostream>


#include "Timer.hh"
#include "karaboDll.hh"

namespace karabo {
    namespace util {

        class KARABO_DECLSPEC Profiler {
            
            // Period of time 

            struct Period {
                std::string m_name;
                timestamp m_startTime;
                timestamp m_endTime;
                bool m_open;
                int m_level;

                Period();

                Period(const std::string name, int level = 0);

                Period(const std::string name, timestamp start, int level = 0);

                Period(const std::string name, timestamp start, timestamp end, int level = 0);
            };

            std::vector< Period > profile;
            std::multimap<std::string, size_t> mapperiods;
            std::string m_name;

        public:

            // Initialize a profile with called "name"
            Profiler(const std::string profilename);
            
            const std::string getName() const {
                return m_name;
            }

            std::string& getName() {
                return m_name;
            }
            
            void setName(const std::string name){
                m_name = name;
                profile[0].m_name = name;
            }


            // Start an new period with a given name
            // Empty name means that the period is appended to the previous one
            
            void start(const std::string periodname = std::string("")) ;
            
            // Stops the last period
            // TODO: implement interleaving periods (ie. reentrance)
            
            void stop(const std::string periodname = std::string("")) ;

            // Reset the content of the profiler
            void reset() ;

            // Return the global time for this profiler include idle time (ie. between periods)
            timestamp getGlobalTime() ;

            // Return the total time of a period at "position"
            // This will sum up the all the sub-periods (ie, unnamed period) the come right after it.
            timestamp getTime(size_t position) ;
            
            // Return the total time of the period called "periodname"
            // This will sum up the all the sub-periods (ie, unnamed period) the come right after it.
            timestamp getTime(std::string periodname) ;

            // Return the detailed time profile for the period called "periodname"
            std::vector<timestamp> getTimeDetails(std::string periodname);

            // Returns the effective time for this profiler.
            // This function excludes idle time (ie. between periods)
            timestamp getEffectiveTime();

            // Serialize the profile into standard ostream
            std::string report(int level=0xFFFF) const;
            
            // Serialize the profile into standard ostream
            
            friend std::ostream& operator <<(std::ostream& os, const Profiler & pr);

        private:
            int m_openPeriodsCount;
        };


    }
}

#endif	/* PROFILER_HH */
