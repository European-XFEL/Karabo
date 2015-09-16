/* 
 * File:   IOService.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 2, 2011, 11:16 AM
 */

#ifndef KARABO_NET_IOSERVICE_HH
#define	KARABO_NET_IOSERVICE_HH

#include <boost/shared_ptr.hpp>
#include <karabo/util/Factory.hh>
#include "AbstractIOService.hh"

namespace karabo {
    namespace net {

        class Connection;
        class BrokerConnection;

        class IOService {

        public:

            KARABO_CLASSINFO(IOService, "IOService", "1.0")

            friend class Connection;
            friend class BrokerConnection;

            virtual ~IOService() {
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
                if (m_service) {
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

#endif

