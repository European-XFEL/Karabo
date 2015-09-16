/* 
 * File:   AbstractIOService.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 3, 2011, 12:04 PM
 */

#ifndef KARABO_NET_ABSTRACTIOSERVICE_HH
#define	KARABO_NET_ABSTRACTIOSERVICE_HH

#include <boost/shared_ptr.hpp>
#include <karabo/util/Configurator.hh>
#include "karabo/util/karaboDll.hh"

namespace karabo {
    namespace net {

        class AbstractIOService {
        public:

            KARABO_CLASSINFO(AbstractIOService, "AbstractIOService", "1.0")
            
            virtual void run() = 0;
            
            virtual void work() = 0;
            
            virtual void stop() = 0;
            
            virtual void post(const boost::function<void()>& handler) = 0;
            
        protected:
            virtual ~AbstractIOService() {
            }
        };
    }
}

// TODO Windows
//KARABO_REGISTER_FACTORY_BASE_HH(karabo::net::AbstractIOService, TEMPLATE_NET, DECLSPEC_NET)

#endif	/* KARABO_NET_ABSTRACTIOSERVICE_HH */

