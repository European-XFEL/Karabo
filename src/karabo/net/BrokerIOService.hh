/* 
 * File:   BrokerIOService.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 2, 2011, 11:16 AM
 */

#ifndef KARABO_NET_BROKERIOSERVICE_HH
#define	KARABO_NET_BROKERIOSERVICE_HH

#include <boost/shared_ptr.hpp>
#include <karabo/util/ClassInfo.hh>
#include "AbstractIOService.hh"

namespace karabo {
    namespace net {

        class BrokerConnection;

        class BrokerIOService {
        public:

            KARABO_CLASSINFO(BrokerIOService, "BrokerIOService", "1.0")

            friend class karabo::net::BrokerConnection;

            typedef boost::shared_ptr<BrokerIOService> Pointer;

            virtual ~BrokerIOService() {
            }

            void run() {
                if (m_service)
                    m_service->run();
            }
            
            void work() {
              if (m_service) {
                m_service->work();
              }
            }
            
            void stop() {
              if(m_service) {
                m_service->stop();
              }
            }
            
            template <class T>
            boost::shared_ptr<T> castTo() {
              return boost::static_pointer_cast<T>(m_service);
            }

        private:

            AbstractIOService::Pointer m_service;

            void setService(const std::string& classId) {
                if (!m_service)
                    m_service = AbstractIOService::createDefault(classId);
                else {
                    if (classId != m_service->getClassInfo().getClassId()) {
                        throw LOGIC_EXCEPTION("Service was set to " + m_service->getClassInfo().getClassId() + " before. Cannot be used with " + classId + " now.");
                    } else {
                        //OK, another connection wants to use us
                    }
                }
            }
        };
    }
}

#endif	/* KARABO_NET_IOSERVICE_HH */

