/*
 * $Id: Traits.hh 2621 2010-11-21 14:58:29Z wrona $
 *
 * File:   Traits.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 14, 2010, 1:45 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_TRAITS_HH
#define	EXFEL_UTIL_TRAITS_HH

namespace exfel {
  namespace util {

    template<class Class, class Argument, void (Class::*)(const Argument&)>
    struct Empty {
    };

    template<class Class, class Argument>
    char checkForConfigureFunction(Empty<Class, Argument, &Class::configure >*);
    
    template<class Class, class Argument>
    float checkForConfigureFunction(...);

    template<class Class, class Argument>
    class ConfigureTraits {
    public:
      const static bool hasFunction = (sizeof (checkForConfigureFunction<Class, Argument > (0)) == 1);
    };
   
  }
}



#endif	/* EXFEL_UTIL_TRAITS_HH */

