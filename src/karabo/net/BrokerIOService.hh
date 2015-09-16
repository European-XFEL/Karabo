/* 
 * File:   BrokerIOService.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 2, 2011, 11:16 AM
 */

#ifndef KARABO_NET_BROKERIOSERVICE_HH
#define	KARABO_NET_BROKERIOSERVICE_HH

#include <boost/shared_ptr.hpp>
#include <karabo/util/Factory.hh>
#include "AbstractIOService.hh"

namespace karabo {
    namespace net {

        class BrokerConnection;

        class BrokerIOService {
        public:

            KARABO_CLASSINFO(BrokerIOService, "BrokerIOService", "1.0")

            friend class karabo::net::BrokerConnection;

            virtual ~BrokerIOService() {
            }
            
            
            /**
             * Runs all registered handlers once.
             * If the same handler should stay active it must be re-registered whilst running has not returned.
             * Re-registration of a handler can for example be done in the handlers function body.
             * New handlers can be registered whilst run() has not returned.
             * The run() function will automatically return if no handlers are registered anymore.
             */
            void run() {
                if (m_service)
                    m_service->run();
            }
            
            
            /**
             * Work will block until stop() is called.
             * All handlers will automatically be re-registered!
             * Registration of new handlers is always possible. 
             */
            void work() {
              if (m_service) {
                m_service->work();
              }
            }
            
            /**
             * Will stop work();
             */
            void stop() {
              if(m_service) {
                m_service->stop();
              }
            }
            
            void post(const boost::function<void()>& handler) {
                if (m_service) {
                    m_service->post(handler);
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
                    m_service = karabo::util::Factory<AbstractIOService>::create(classId);
                else {
                    if (classId != m_service->getClassInfo().getClassId()) {
                        throw KARABO_LOGIC_EXCEPTION("Service was set to " + m_service->getClassInfo().getClassId() + " before. Cannot be used with " + classId + " now.");
                    } else {
                        //OK, another connection wants to use us
                    }
                }
            }
        };
    }
}

#endif	/* KARABO_NET_IOSERVICE_HH */

