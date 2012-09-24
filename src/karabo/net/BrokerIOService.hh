/* 
 * File:   BrokerIOService.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 2, 2011, 11:16 AM
 */

#ifndef EXFEL_NET_BROKERIOSERVICE_HH
#define	EXFEL_NET_BROKERIOSERVICE_HH

#include <boost/shared_ptr.hpp>
#include <karabo/util/ClassInfo.hh>
#include "AbstractIOService.hh"

namespace exfel {
    namespace net {

        class BrokerConnection;

        class BrokerIOService {
        public:

            EXFEL_CLASSINFO(BrokerIOService, "BrokerIOService", "1.0")

            friend class exfel::net::BrokerConnection;

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

#endif	/* EXFEL_NET_IOSERVICE_HH */

