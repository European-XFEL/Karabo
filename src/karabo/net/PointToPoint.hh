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

            struct Producer {
                // Producer typedefs ...
                typedef std::map<Channel::Pointer, std::set<std::string> > ChannelToSlotInstanceIds;
            
            private:
            
                unsigned int m_port;
                Connection::Pointer m_connection;
                ChannelToSlotInstanceIds m_registeredChannels;
                boost::mutex m_registeredChannelsMutex;
                
            public:
                
                KARABO_CLASSINFO(Producer, "PointToPointProducer", "1.0")
                
                Producer();

                ~Producer();

                IOService::Pointer getIOService() const {
                    return m_connection->getIOService();
                }
                
                unsigned int getPort() const {
                    return m_port;
                }
                
                bool publish(const std::string& slotInstanceId,
                        const karabo::util::Hash::Pointer& header,
                        const karabo::util::Hash::Pointer& body,
                        int prio);

                void publishIfConnected(std::map<std::string, std::set<std::string> >& registeredSlots,
                        const karabo::util::Hash::Pointer& header,
                        const karabo::util::Hash::Pointer& message, int prio);

                void connectHandler(const karabo::net::Channel::Pointer& channel);

                void onSubscribe(const karabo::net::Channel::Pointer& channel, const std::string& subscription);

                void channelErrorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec);

                void stop();
                
            private:
                void updateHeader(const karabo::util::Hash::Pointer& header,
                        const std::map<std::string, std::set<std::string> >& registeredSlots) const;

            };

            struct Consumer {
                typedef std::map<std::string, ConsumeHandler> SlotInstanceIds;

                // Mapping signalInstanceId into signalConnectionString like "tcp://host:port" and set of pairs of slotInstanceId and ConsumeHandler
                typedef std::map<std::string, std::pair<std::string, SlotInstanceIds> > ConnectedInstances;
                // Mapping connectionString into (Connection, Channel) pointers
                typedef std::map<std::string, std::pair<karabo::net::Connection::Pointer, karabo::net::Channel::Pointer> > OpenConnections;

            private:
                
                OpenConnections m_openConnections; // key: connection_string 
                ConnectedInstances m_connectedInstances;
                boost::mutex m_connectedInstancesMutex;

            public:
                
                KARABO_CLASSINFO(Consumer, "PointToPointConsumer", "1.0")
                        
                Consumer();

                virtual ~Consumer();

                void connectionErrorHandler(const std::string& signalInstanceId,
                        const std::string& signalConnectionString,
                        const karabo::net::Connection::Pointer& connection,
                        const karabo::net::ErrorCode& ec);

                void channelErrorHandler(const std::string& signalInstanceId,
                        const std::string& signalConnectionString,
                        const karabo::net::Connection::Pointer& connection,
                        const karabo::net::Channel::Pointer& channel,
                        const karabo::net::ErrorCode& ec);

                void connect(const karabo::net::IOService::Pointer& svc,
                        const std::string& signalInstanceId,
                        const std::string& slotInstanceId,
                        const std::string& signalConnectionString,
                        const karabo::net::ConsumeHandler& handler);

                void disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId);

                void consume(const karabo::net::Channel::Pointer& channel,
                        karabo::util::Hash::Pointer& header,
                        karabo::util::Hash::Pointer& body);
                
            private:
                
                void storeTcpConnectionInfo(const std::string& signalConnectionString,
                        const karabo::net::Connection::Pointer& connection,
                        const karabo::net::Channel::Pointer& channel) {
                    if (m_openConnections.find(signalConnectionString) == m_openConnections.end())
                        m_openConnections[signalConnectionString] = std::make_pair(connection, channel);
                }

                void storeSignalSlotConnectionInfo(const std::string& slotInstanceId,
                        const std::string& signalInstanceId,
                        const std::string& signalConnectionString,
                        const ConsumeHandler& handler);

                void connectHandler(const std::string& subscription,
                        const std::string& instanceId,
                        const std::string& connectionString,
                        const karabo::net::ConsumeHandler& handler,
                        const karabo::net::Connection::Pointer& connection,
                        const karabo::net::Channel::Pointer& channel);

            };

            Producer m_producer;
            Consumer m_consumer;

            // Producer stuff ...
            karabo::net::IOService::Pointer m_svc;
            boost::thread m_thread;



        public:

            typedef boost::shared_ptr<karabo::net::PointToPoint> Pointer;

            PointToPoint();

            virtual ~PointToPoint();

            std::string getConnectionString() const;

            karabo::net::IOService::Pointer getIOService() const {
                return m_svc;
            }

            void connect(const std::string& signalInstanceId, const std::string& slotInstanceId,
                    const std::string& signalConnectionString, const karabo::net::ConsumeHandler& handler) {
                m_consumer.connect(m_svc, signalInstanceId, slotInstanceId, signalConnectionString, handler);
            }

            void disconnect(const std::string& signalInstanceId, const std::string& slotInstanceId) {
                m_consumer.disconnect(signalInstanceId, slotInstanceId);
            }

            bool publish(const std::string& slotInstanceId,
                    const karabo::util::Hash::Pointer& header,
                    const karabo::util::Hash::Pointer& body,
                    int prio) {
                return m_producer.publish(slotInstanceId, header, body, prio);
            }

            void publishIfConnected(std::map<std::string, std::set<std::string> >& registeredSlots,
                    const karabo::util::Hash::Pointer& header,
                    const karabo::util::Hash::Pointer& message, int prio) {
                m_producer.publishIfConnected(registeredSlots, header, message, prio);
            }

        private:

            void consume(const karabo::net::Channel::Pointer& channel, karabo::util::Hash::Pointer& header, karabo::util::Hash::Pointer& body) {
                m_consumer.consume(channel, header, body);
            }

        };
    }
}

#endif	/* KARABO_NET_POINTTOPOINT_HH */

