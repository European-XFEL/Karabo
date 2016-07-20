/* 
 * File:   PointToPoint.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on February 8, 2016, 2:19 PM
 */

#ifndef KARABO_NET_POINTTOPOINT_HH
#define	KARABO_NET_POINTTOPOINT_HH

#include <map>
#include <set>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <karabo/util/Hash.hh>
#include "Connection.hh"
#include "Channel.hh"

namespace karabo {
    namespace net {

        typedef boost::function<bool (const std::string&,
                                      const karabo::util::Hash::Pointer&,
                                      const karabo::util::Hash::Pointer&) > ConsumeHandler;

        class PointToPoint {


            // Signal side is always a server (producer) side,  Slot side is a client (consumer) side

        public:

            typedef boost::shared_ptr<karabo::net::PointToPoint> Pointer;

            PointToPoint();

            virtual ~PointToPoint();

            std::string getConnectionString() const;

            karabo::net::IOService::Pointer getIOService() const {
                return m_svc;
            }

            void connect(const std::string& signalInstanceId, const std::string& slotInstanceId,
                         const std::string& signalConnectionString, const karabo::net::ConsumeHandler& handler);

            void disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId);

            bool publish(const std::string& slotInstanceId,
                         const karabo::util::Hash::Pointer& header,
                         const karabo::util::Hash::Pointer& body,
                         int prio);

            void publishIfConnected(std::map<std::string, std::set<std::string> >& registeredSlots,
                                    const karabo::util::Hash::Pointer& header,
                                    const karabo::util::Hash::Pointer& message, int prio);

        private:

            void consume(const karabo::net::Channel::Pointer& channel, karabo::util::Hash::Pointer& header,
                         karabo::util::Hash::Pointer& body);

        private:

            class Producer;
            class Consumer;

            boost::shared_ptr<Producer> m_producer;
            boost::shared_ptr<Consumer> m_consumer;

            // Producer stuff ...
            karabo::net::IOService::Pointer m_svc;
            boost::thread m_thread;

        };

    }
}

#endif	/* KARABO_NET_POINTTOPOINT_HH */

