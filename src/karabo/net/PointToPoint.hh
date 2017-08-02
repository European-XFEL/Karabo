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

    namespace xms {
        class SignalSlotable;  // forward declaration
    }

    namespace net {


        class PointToPoint : public boost::enable_shared_from_this<PointToPoint> {

        public:

            // Local map simply repeats the map in SignalSlotable
            typedef std::map<std::string, karabo::xms::SignalSlotable*> LocalMap;
            // In SignalSlotable there is no serverId. We use url instead
            // mapping the instanceId to url
            typedef std::map<std::string, std::string> MapInstanceIdToUrl;
            // mapping url to set of instanceIds
            typedef std::map<std::string, std::set<std::string> > MapUrlToInstanceIdSet;
            // mapping url to connection (pair of channel and connection pointers)
            typedef std::map<std::string, std::pair<karabo::net::Channel::Pointer, karabo::net::Connection::Pointer> > MapUrlToConnection;
            // This repeates map defined in karabo::xms::Signal class
            typedef std::map<std::string, std::set<std::string> > SlotMap;

            KARABO_CLASSINFO(PointToPoint, "PointToPoint", "2.1")
            
            PointToPoint();

            virtual ~PointToPoint();

            /**
             * Return a string specifying the host and port the p2p interface is connected to
             * @return 
             */
            std::string getLocalUrl() const;

            /**
             * Actively requests to establish and register a P2P connection to the remote server asynchronously
             *  where the "remoteInstanceId" is running.
             * 
             * @param remoteInstanceId remote SignalSlotable instanceId on the remote server
             * @return false if argument is not known (not in our bookkeeping structure) or is not "remote"
             *         true if already connected or initiated successfully
             */
            bool connectAsync(const std::string& remoteInstanceId);

            /**
             * Try to connect synchronously to the remote device server running remoteInstanceId.
             *
             * @param remoteInstanceId
             * @return false if local or unknown or connection failed, otherwise returns true
             */
            bool connect(const std::string& remoteInstanceId);

            /**
             * Checks if we can access remoteInstanceId via point-to-point
             * @param remoteInstanceId
             * @return true if connected and false if not.
             */
            bool isConnected(const std::string& remoteInstanceId);

            /**
             * Disconnect or de-register remoteInstanceIda
             * @param remoteInstanceId
             */
            void disconnect(const std::string& remoteInstanceId) {
            }

            /**
             * Publish a message to a slot instance consisting of a message header and body, with
             * a priority assigned to it
             * @param slotInstanceId slotInstanceId to post the message to
             * @param header of the message
             * @param body of the message
             * @param prio of the message, ranging from 0-9 where 9 is the highest priority.
             * @return 
             */
            bool publish(const std::string& remoteInstanceId,
                         const karabo::util::Hash::Pointer& header,
                         const karabo::util::Hash::Pointer& body,
                         int prio);
            
            /**
             * Filter the input map of slot instanceIds to set of slot functions and create output
             * map that groups all SlotMap entries with the same URL. The entries that got to the output
             * will be erased from the input. Used for routing messages via P2P or broker
             * @param in  map between instanceId and set of slot functions
             * @param out map between URL and SlotMap entries with the this URL
             */
            void filterConnectedAndGroupByUrl(SlotMap& in, std::map<std::string, SlotMap>& out);
            
            /**
             * Update InstanceIdToUrl map
             * @param remoteInstanceId
             * @param remoteUrl - connection string
             */
            void updateUrl(const std::string& remoteInstanceId, const std::string& remoteUrl);

            /**
             * Erase entry from InstanceIdToUrl map
             * @param instanceId
             */
            void eraseUrl(const std::string& instanceId);

            /**
             * Find URL (connection string) by remoteInstanceId
             * @param remoteInstanceId
             * @return URL
             */
            std::string findUrlById(const std::string& remoteInstanceId);

            /**
             * Register SignalSlotable in local map
             * @param localId  instanceId
             * @param ss       and related SignalSlotable pointer
             */
            void insertToLocalMap(const std::string& localId, karabo::xms::SignalSlotable* ss);

            /**
             * Remove SignalSlotable entry from the local map
             * @param instanceId
             */
            void eraseFromLocalMap(const std::string& instanceId);

            /**
             * Check that instanceId is in the local map.
             * @param true if instanceId entry is in the local map
             */
            bool isInLocalMap(const std::string& instanceId);

            std::string allMapsToString();

        private:

            /**
             * Passive connection: we simply wait for client to connect us.
             * Then, following the protocol: the client should send us the configuration Hash:
             *    "instanceId"    : client instanceId
             *    "instanceInfo"  : client instanceInfo
             *    "instanceIds"   : all instanceIds on client's side application (device server)
             * @param e
             * @param channel
             */
            void onConnectServer(const ErrorCode& e,
                                 const karabo::net::Channel::Pointer& channel,
                                 const karabo::net::Connection::Pointer& connection);

            void onConnectServerBottomHalf(const ErrorCode& e,
                                           const karabo::net::Channel::Pointer& channel,
                                           const karabo::net::Connection::Pointer& connection,
                                           const karabo::util::Hash::Pointer& message);

            void onConnectClient(const ErrorCode& e,
                                 const Channel::Pointer& channel,
                                 const Connection::Pointer& connection,
                                 const karabo::util::Hash& config);

            void onP2PMessage(const ErrorCode& e,
                              const karabo::net::Channel::Pointer& channel,
                              const karabo::util::Hash::Pointer& header,
                              const karabo::util::Hash::Pointer& message);

            void channelErrorHandler(const ErrorCode& ec, const Channel::Pointer& channel);

        private:

            int m_serverPort;
            std::string m_localUrl;
            boost::shared_mutex m_pointToPointMutex;
            MapInstanceIdToUrl m_instanceIdToUrl;
            MapUrlToInstanceIdSet m_urlToInstanceIdSet;
            MapUrlToConnection m_mapOpenConnections;
            LocalMap m_localMap;
            boost::shared_mutex m_localMapMutex;
        };

    }
}

#endif	/* KARABO_NET_POINTTOPOINT_HH */

