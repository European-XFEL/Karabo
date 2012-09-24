/*
 * $Id: Time.cc 6711 2012-07-05 10:15:39Z heisenb $
 *
 * File:   Time.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 17, 2010, 1:43 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <stdio.h>
#include <time.h>

#include "Time.hh"
#include <time.h>


namespace exfel {
  namespace util {

    using namespace boost::posix_time;
    using namespace boost::gregorian;

    ptime Time::m_epoch = ptime(date(1970, 1, 1));
        
    unsigned long long Time::getMsSinceEpoch() {
      ptime now(microsec_clock::universal_time());
      time_duration diff = now - m_epoch;
      return diff.total_milliseconds();
    }
    
    // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
    std::string Time::getCurrentDateTime(const std::string& format) {
        time_t     now = time(0);
        struct tm  tstruct;
        char       buf[80];
        tstruct = *localtime(&now);
        // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
        // for more information about date/time format
        strftime(buf, sizeof(buf), format.c_str(), &tstruct);
        
        return buf;
    }
    
  } // namespace packageName
} // namespace exfel
