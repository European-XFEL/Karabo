/* 
 * File:   SnmpIOService.hh
 * Author: WP76 <wp76@xfel.eu>
 * 
 * Created on July 15, 2011, 11:46 AM
 */

#ifndef KARABO_NET_SNMPIOSERVICE_HH
#define	KARABO_NET_SNMPIOSERVICE_HH

#include "AbstractIOService.hh"

namespace karabo {
    namespace net {

        class SnmpIOService : public AbstractIOService {
            
        public:

            KARABO_CLASSINFO(SnmpIOService, "Snmp", "1.0")

            SnmpIOService() : m_expectedReplies(0) {
            }

            virtual ~SnmpIOService() {
            }

            void run();
            
            void work();
            
            void stop();
                        
            void increaseReplyCount();
            
            void decreaseReplyCount();
            
            
        private:
            
            int m_expectedReplies;
        };
    }
}

#endif	/* KARABO_NET_SNMPIOSERVICE_HH */

