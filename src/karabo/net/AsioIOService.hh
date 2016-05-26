/* 
 * File:   AsioIOService.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 2, 2011, 6:45 PM
 */

#ifndef KARABO_NET_ASIOIOSERVICE_HH
#define	KARABO_NET_ASIOIOSERVICE_HH

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include "AbstractIOService.hh"

namespace karabo {
    namespace net {

        typedef boost::shared_ptr<boost::asio::io_service> BoostIOServicePointer;
        typedef boost::shared_ptr<boost::asio::io_service::work> WorkPointer;

        class AsioIOService : public AbstractIOService {

        public:

            KARABO_CLASSINFO(AsioIOService, "Asio", "1.0")

            AsioIOService() : m_ioservice(new boost::asio::io_service) {
            }

            virtual ~AsioIOService() {
            }

            void run() {
                if (!m_ioservice)
                    throw KARABO_PARAMETER_EXCEPTION("AsioIOService is not configured");
                this->runProtected();
            }

            void work() {
                if (!m_ioservice)
                    throw KARABO_PARAMETER_EXCEPTION("AsioIOService is not configured");
                m_work = WorkPointer(new boost::asio::io_service::work(*m_ioservice));
                
                this->runProtected();
            }

            void stop() {
                m_work.reset();
                if (!m_ioservice)
                    throw KARABO_PARAMETER_EXCEPTION("AsioIOService is not configured");
                m_ioservice->stop();
            }

            void post(const boost::function<void()>& handler) {
                if (m_ioservice)
                    m_ioservice->post(handler);
            }
            
            BoostIOServicePointer getBoostIOService() {
                return m_ioservice;
            }

        private:
            /// Run m_ioservice, but catch and log exceptions and start running again.
            void runProtected();

            BoostIOServicePointer m_ioservice;
            WorkPointer m_work;
        };
    }
}

#endif	/* KARABO_NET_ASIOIOSERVICE_HH */

