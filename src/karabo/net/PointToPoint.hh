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
#include <boost/asio.hpp>
#include <karabo/util/Hash.hh>
#include "Connection.hh"
#include "Channel.hh"
#include "EventLoop.hh"

namespace karabo {
    namespace net {

        typedef boost::function<bool (const std::string&,
                                      const karabo::util::Hash::Pointer&,
                                      const karabo::util::Hash::Pointer&) > ConsumeHandler;

        /**
         * @class PointToPoint
         * @brief This class implements a point-to-point (p2p) messaging interface for Karabo
         * 
         * This class implements a point-to-point (p2p) messaging interface for Karabo. When
         * using this interface the signaling side (producer) is always the server, the
         * slot side (consumer) is always the client.
         */
        class PointToPoint {

        public:

            typedef boost::shared_ptr<karabo::net::PointToPoint> Pointer;

            PointToPoint();

            virtual ~PointToPoint();

            /**
             * Return a string specifying the host and port the p2p interface is connected to
             * @return 
             */
            std::string getConnectionString() const;

            /**
             * Return the boost::asio::io_service of Karabo's EventLoop
             * @return 
             */
            boost::asio::io_service& getIOService() {
                return karabo::net::EventLoop::getIOService();
            }

            /**
             * Connect a signal on a SignalSlotable instance to a slot on another SignalSlotable 
             * instance using the PointToPoint interface. If both instances run in the same
             * process a shortcut is used and no tcp traffic is generated.
             * 
             * @param signalInstanceId SignalSlotable instance the signal is on
             * @param slotInstanceId SignalSlotable instance the slot is on
             * @param signalConnectionString connection string as given by getConnectionString identifying the server (signal side)
             * @param handler on the consumer (slot) used for consuming messages
             */
            void connect(const std::string& signalInstanceId, const std::string& slotInstanceId,
                         const std::string& signalConnectionString, const karabo::net::ConsumeHandler& handler);

            /**
             * Disconnect a PointToPoint connection established between a signal instance and a slot instance
             * @param signalInstanceId
             * @param slotInstanceId
             */
            void disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId);

            /**
             * Publish a message to a slot instance consisting of a message header and body, with
             * a priority assigned to it
             * @param slotInstanceId slotInstanceId to post the message to
             * @param header of the message
             * @param body of the message
             * @param prio of the message, ranging from 0-9 where 9 is the highest priority.
             * @return 
             */
            bool publish(const std::string& slotInstanceId,
                         const karabo::util::Hash::Pointer& header,
                         const karabo::util::Hash::Pointer& body,
                         int prio);
            
            /**
             * Publish a message to slot instances having a slot connected to those specified
             * in registered slots
             * @param registeredSlots a std::map where the key is the slot instance and the value is a std::set
             *        of slots that should be registered and connected to on this instance
             * @param header of the message
             * @param message of the message 
             * @param prio of the message, ranging from 0-9 where 9 is the highest priority.
             */
            void publishIfConnected(std::map<std::string, std::set<std::string> >& registeredSlots,
                                    const karabo::util::Hash::Pointer& header,
                                    const karabo::util::Hash::Pointer& message, int prio);


        private:

            class Producer;
            class Consumer;

            boost::shared_ptr<Producer> m_producer;
            boost::shared_ptr<Consumer> m_consumer;

        };

    }
}

#endif	/* KARABO_NET_POINTTOPOINT_HH */

