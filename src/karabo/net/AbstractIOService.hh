/* 
 * File:   AbstractIOService.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 12:04 PM
 */

#ifndef EXFEL_NET_ABSTRACTIOSERVICE_HH
#define	EXFEL_NET_ABSTRACTIOSERVICE_HH

#include <boost/shared_ptr.hpp>
#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include "netdll.hh"

namespace exfel {
    namespace net {

        class AbstractIOService {
        public:

            EXFEL_CLASSINFO(AbstractIOService, "AbstractIOService", "1.0")
            EXFEL_FACTORY_BASE_CLASS

            virtual void run() = 0;
            
            virtual void work() = 0;
            
            virtual void stop() = 0;
            
        protected:
            virtual ~AbstractIOService() {
            }
        };
    }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::net::AbstractIOService, TEMPLATE_NET, DECLSPEC_NET)

#endif	/* EXFEL_NET_ABSTRACTIOSERVICE_HH */

