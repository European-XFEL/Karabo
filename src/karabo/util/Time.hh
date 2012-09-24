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

#ifndef EXFEL_UTIL_TIME_HH
#define	EXFEL_UTIL_TIME_HH

#include <boost/date_time/local_time/local_time.hpp>

/**
 * The main European XFEL namespace
 */
namespace exfel {

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
      
      static std::string getCurrentDateTime(const std::string& format="%Y-%m-%d %H:%M:%S");

    protected:

    private:

      static boost::posix_time::ptime m_epoch;
   
    };

  } // namespace util
} // namespace exfel

#endif	/* EXFEL_UTIL_TIME_HH */
