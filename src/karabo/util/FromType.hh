/* 
 * File:   FromType.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013 9:12 PM
 * 
 */

#ifndef KARABO_UTIL_FROMTYPE_HH
#define	KARABO_UTIL_FROMTYPE_HH

#include "Types.hh"


namespace karabo {

    namespace util {

        template <class Impl>
        class FromType {


            typedef typename Impl::ArgumentType ArgumentType;

            FromType();
            virtual ~FromType();

        public:

            // Concept that must be implemented

            static Types::ReferenceType from(const ArgumentType& type) {
                return Impl::from(type);
            }


        };
    }
}

#endif

