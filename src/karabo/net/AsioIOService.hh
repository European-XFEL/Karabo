/* 
 * File:   AsioIOService.hh
 * Author: WP76 <wp76@xfel.eu>
 *
 * Created on June 2, 2011, 6:45 PM
 */

#ifndef EXFEL_NET_ASIOIOSERVICE_HH
#define	EXFEL_NET_ASIOIOSERVICE_HH

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include "AbstractIOService.hh"

namespace exfel {
    namespace net {

        typedef boost::shared_ptr<boost::asio::io_service> BoostIOServicePointer;
        typedef boost::shared_ptr<boost::asio::io_service::work> WorkPointer;

        class AsioIOService : public AbstractIOService {
        public:

            EXFEL_CLASSINFO(AsioIOService, "Asio", "1.0")

            AsioIOService() : m_ioservice(new boost::asio::io_service) {
            }

            virtual ~AsioIOService() {
            }

            void run() {
                if (!m_ioservice)
                    throw PARAMETER_EXCEPTION("AsioIOService is not configured");
                m_ioservice->run();
            }

            void work() {
                if (!m_ioservice)
                    throw PARAMETER_EXCEPTION("AsioIOService is not configured");
                m_work = WorkPointer(new boost::asio::io_service::work(*m_ioservice));
                m_ioservice->run();
            }

            void stop() {
                if (!m_ioservice)
                    throw PARAMETER_EXCEPTION("AsioIOService is not configured");
                m_ioservice->stop();
            }

            BoostIOServicePointer getBoostIOService() {
                return m_ioservice;
            }

        private:
            BoostIOServicePointer m_ioservice;
            WorkPointer m_work;
        };
    }
}

#endif	/* EXFEL_NET_ASIOIOSERVICE_HH */

