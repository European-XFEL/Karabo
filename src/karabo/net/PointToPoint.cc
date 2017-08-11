#include <sstream>
#include <string>
#include <unordered_set>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "karabo/log.hpp"
#include "karabo/net/TcpChannel.hh"
#include "utils.hh"
#include "PointToPoint.hh"
#include "EventLoop.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/xms/SignalSlotable.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::log;

namespace karabo {
    namespace net {


        PointToPoint::PointToPoint() : m_serverPort(0), m_localUrl(""), m_customReadHandler() {
        }


        PointToPoint::~PointToPoint() {
        }


        void PointToPoint::start(const CustomP2PReadHandler& handler) {
            m_customReadHandler = handler;
            Connection::Pointer connection = Connection::create(Hash("Tcp.port", 0, "Tcp.type", "server"));
            m_serverPort = connection->startAsync(bind_weak(&PointToPoint::onConnectServer, this, _1, _2, connection));
            m_localUrl = "tcp://" + karabo::net::bareHostName() + ":" + toString(m_serverPort);
        }


        // Bookkeeping stuff

        const std::string& PointToPoint::getLocalUrl() const {
            return m_localUrl;
        }


        void PointToPoint::updateUrl(const std::string& newInstanceId, const std::string& remoteUrl) {
            boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            auto it = m_instanceIdToUrl.find(newInstanceId);
            if (it != m_instanceIdToUrl.end()) {
                // found -- clean up old info
                const std::string& oldUrl = it->second;
                auto idsIter = m_urlToInstanceIdSet.find(oldUrl);
                if (idsIter != m_urlToInstanceIdSet.end()) idsIter->second.erase(newInstanceId);
                if (idsIter->second.empty()) {
                    // Don't keep entry with the empty id's set
                    m_urlToInstanceIdSet.erase(idsIter);
                    // No reference left to the old connection
                    // This will close obsolete connection (by destroying their shared pointers)
                    m_mapOpenConnections.erase(oldUrl);
                }
            }
            m_instanceIdToUrl[newInstanceId] = remoteUrl;
            m_urlToInstanceIdSet[remoteUrl].insert(newInstanceId);
        }


        void PointToPoint::updateServerId(const std::string& url, const std::string& serverId) {
            boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            m_urlToServerId[url] = serverId;
        }


        void PointToPoint::eraseUrl(const std::string& instanceId) {
            boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            auto it = m_instanceIdToUrl.find(instanceId);
            if (it == m_instanceIdToUrl.end()) return;  // nothing to erase
            std::string url = it->second;
            m_instanceIdToUrl.erase(it);

            auto idsIter = m_urlToInstanceIdSet.find(url);
            if (idsIter != m_urlToInstanceIdSet.end()) {
                auto& ids = idsIter->second;
                // Remove instanceId from set
                ids.erase(instanceId);
                if (!ids.empty()) return;  // Keep open connection
                m_urlToInstanceIdSet.erase(idsIter); // no entries left
                auto urlToServerIdIter = m_urlToServerId.find(url);
                if (urlToServerIdIter != m_urlToServerId.end()) m_urlToServerId.erase(urlToServerIdIter);
            }
            m_mapOpenConnections.erase(url);
        }


        std::string PointToPoint::eraseOpenConnection(const karabo::net::Channel::Pointer& channel) {
            std::string url = "";
            boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            // Check log connections first
            for (MapUrlToConnection::iterator i = m_mapLogConnections.begin(); i != m_mapLogConnections.end(); ++i) {
                if (i->second.first.get() == channel.get()) {
                    m_mapLogConnections.erase(i);
                    return "";   // No reconnection for loggers
                }
            }
            for (MapUrlToConnection::iterator it = m_mapOpenConnections.begin(); it != m_mapOpenConnections.end(); ++it) {
                if (it->second.first.get() == channel.get()) {
                    url = it->first;
                    m_mapOpenConnections.erase(it);
                    break;
                }
            }
            if (!url.empty()) {
                // Check if we still have URL registered
                auto idsIter = m_urlToInstanceIdSet.find(url);
                if (idsIter != m_urlToInstanceIdSet.end()) {
                    // Check if set of instanceIds is not empty and we can pick up some remoteInstanceId
                    if (!idsIter->second.empty()) {                    
                        // return the first remoteInstanceId
                        return *(idsIter->second.begin());
                    }
                }
            }
            return "";
        }


        std::string PointToPoint::findUrlById(const std::string& remoteInstanceId) {
            boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            auto it = m_instanceIdToUrl.find(remoteInstanceId);
            if (it == m_instanceIdToUrl.end()) return "";
            return it->second;
        }


        void PointToPoint::insertToLocalMap(const std::string& instanceId, karabo::xms::SignalSlotable* signalSlotable) {
            boost::unique_lock<boost::shared_mutex> lock(m_localMapMutex);
            m_localMap[instanceId] = signalSlotable;
        }


        void PointToPoint::eraseFromLocalMap(const std::string& instanceId) {
            boost::unique_lock<boost::shared_mutex> lock(m_localMapMutex);
            m_localMap.erase(instanceId);
        }


        bool PointToPoint::isInLocalMap(const std::string& instanceId) {
            boost::shared_lock<boost::shared_mutex> lock(m_localMapMutex);
            if (m_localMap.find(instanceId) == m_localMap.end()) return false;
            return true;
        }


        /**
         * Passive connection: we simply wait for client to connect us.
         *
         * @param e
         * @param channel
         */
        void PointToPoint::onConnectServer(const ErrorCode& e,
                                           const Channel::Pointer& channel,
                                           const Connection::Pointer& connection) {
            try {
                boost::shared_ptr<PointToPoint> guard = shared_from_this();
                if (e) {
                    channelErrorHandler(e, channel);
                    return;
                }
                // We can accept more connections
                connection->startAsync(bind_weak(&PointToPoint::onConnectServer, this, _1, _2, connection));
                // Read client side info about open connection before registration
                channel->readAsyncHashPointer(bind_weak(&PointToPoint::onConnectServerBottomHalf, this, _1, channel, connection, _2));
            } catch (const boost::bad_weak_ptr&) {
            }
        }


        void PointToPoint::onConnectServerBottomHalf(const ErrorCode& e,
                                                     const karabo::net::Channel::Pointer& channel,
                                                     const karabo::net::Connection::Pointer& connection,
                                                     const karabo::util::Hash::Pointer& message) {
            if (e) {
                channelErrorHandler(e, channel);
                return;
            }

            channel->setAsyncChannelPolicy(3, "REMOVE_OLDEST");
            channel->setAsyncChannelPolicy(4, "LOSSLESS");

            if (!message || !message->has("url") || !message->has("ids")) return;
                
            // Get client URL to register an open connection
            const string& remoteUrl = message->get<string>("url");
            vector<string> remoteIds;
            if (message->has("ids")) message->get("ids", remoteIds);
            bool isLog = false;
            if (message->has("log")) message->get("log", isLog);
            if (!isLog) {
                boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                // Clean from old entries with the same value : remoteUrl
                // because what we got is most up-to-date information
                for (MapInstanceIdToUrl::iterator it = m_instanceIdToUrl.begin(); it!=m_instanceIdToUrl.end(); ++it) {
                    if (it->second == remoteUrl) m_instanceIdToUrl.erase(it);
                }
                for (auto& remoteInstanceId : remoteIds) m_instanceIdToUrl[remoteInstanceId] = remoteUrl;
                m_urlToInstanceIdSet[remoteUrl] = set<string>(remoteIds.begin(), remoteIds.end());
                m_mapOpenConnections[remoteUrl] = std::make_pair(channel, connection);
            } else {
                m_mapLogConnections[remoteUrl] = std::make_pair(channel, connection);
            }
            // Prepare to read from client
            channel->readAsyncHashPointerHashPointer(bind_weak(&PointToPoint::onP2PMessage, this, _1, channel, _2, _3));
        }


        bool PointToPoint::connect(const std::string& remoteInstanceId) {
            if (this->isInLocalMap(remoteInstanceId)) return false;

            string remoteUrl = "";
            {
                boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                auto it = m_instanceIdToUrl.find(remoteInstanceId);
                if (it == m_instanceIdToUrl.end()) return false;
                remoteUrl = it->second;
                if (m_mapOpenConnections.find(remoteUrl) != m_mapOpenConnections.end()) return true;   // already connected
            }

            Connection::Pointer connection = Connection::create(Hash("Tcp", Hash("type", "client", "url", remoteUrl)));
            Channel::Pointer channel = connection->start();  // synchronous call
            if (!channel) return false;

            finalizeConnectionProtocol(remoteUrl, channel, connection);
            return true;
        }


        void PointToPoint::finalizeConnectionProtocol(const std::string& remoteUrl,
                                                      const Channel::Pointer& channel,
                                                      const Connection::Pointer& connection) {
            channel->setAsyncChannelPolicy(3, "REMOVE_OLDEST");
            channel->setAsyncChannelPolicy(4, "LOSSLESS");

            {   // Register server connection
                boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                m_mapOpenConnections[remoteUrl] = std::make_pair(channel, connection);
            }
            // Inform server about my local URL and local instanceIds
            vector<string> ids;
            {
                boost::shared_lock<boost::shared_mutex> lock(m_localMapMutex);
                for (const auto& i : m_localMap) ids.push_back(i.first);
            }
            channel->writeAsync(Hash("url", m_localUrl, "ids", ids, "log", (!m_customReadHandler? false : true)), 4);
            // Prepare to read from server
            channel->readAsyncHashPointerHashPointer(bind_weak(&PointToPoint::onP2PMessage, this, _1, channel, _2, _3));
        }


        bool PointToPoint::connectAsync(const std::string& remoteInstanceId) {
            if (this->isInLocalMap(remoteInstanceId)) return false;

            string remoteUrl = "";
            {
                boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                auto it = m_instanceIdToUrl.find(remoteInstanceId);
                if (it == m_instanceIdToUrl.end()) return false;
                remoteUrl = it->second;
                if (m_mapOpenConnections.find(remoteUrl) != m_mapOpenConnections.end()) return true;   // already connected
            }
            Connection::Pointer connection = Connection::create(Hash("Tcp", Hash("type", "client", "url", remoteUrl)));
            connection->startAsync(bind_weak(&PointToPoint::onConnectClient, this, _1, _2, connection, Hash("url", remoteUrl)));
            return true;
        }


        void PointToPoint::onConnectClient(const ErrorCode& e,
                                           const Channel::Pointer& channel,
                                           const Connection::Pointer& connection,
                                           const Hash& config) {
            if (e) {
                channelErrorHandler(e, channel);
                return;
            }
                        
            const string& remoteUrl = config.get<string>("url");
            finalizeConnectionProtocol(remoteUrl, channel, connection);
        }


        bool PointToPoint::isConnected(const std::string& remoteInstanceId) {
            boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            const auto& it = m_instanceIdToUrl.find(remoteInstanceId);
            if (it == m_instanceIdToUrl.end()) return false;
            const std::string& remoteUrl = it->second;
            if (m_mapOpenConnections.find(remoteUrl) == m_mapOpenConnections.end()) return false;
            return true;
        }


        void PointToPoint::filterConnectedAndGroupByUrl(SlotMap& input, std::map<std::string, SlotMap>& resultMap) {
            resultMap.clear();
            for (SlotMap::iterator it = input.begin(); it != input.end(); ++it) {
                const string& remoteInstanceId = it->first;
                const std::set<std::string>& remoteSlots = it->second;
                string remoteUrl = "";
                {
                    boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                    const auto& iter = m_instanceIdToUrl.find(remoteInstanceId);
                    if (iter == m_instanceIdToUrl.end()) continue;
                    remoteUrl = iter->second;
                    if (m_mapOpenConnections.find(remoteUrl) == m_mapOpenConnections.end()) continue;   // not connected
                }
                resultMap[remoteUrl][remoteInstanceId] = remoteSlots;
                input.erase(it);
            }
        }


        void PointToPoint::onP2PMessage(const ErrorCode& e, const karabo::net::Channel::Pointer& channel,
                                        const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& message) {
            if (e) {
                channelErrorHandler(e, channel);
                // here we try to re-connect if needed
                string remoteInstanceId = eraseOpenConnection(channel);
                if (remoteInstanceId.empty()) return;
                // We still need to be connected to some instanceId
                EventLoop::getIOService().post(boost::bind(&PointToPoint::connectAsync, shared_from_this(), remoteInstanceId));
            }
            
            if (!m_customReadHandler) {

                // We don't need to check the existence of "slotInstanceIds" in header. It is always there!
                const string& slotInstanceIds = header->get<string>("slotInstanceIds");
                // slotInstanceIds is defined like
                // |slotInstanceId| or |*| or |slotInstanceId1||slotInstanceId2||...||slotInstanceIdN|

                // strip vertical lines
                string stripped = slotInstanceIds.substr(1, slotInstanceIds.size()-2);
                size_t pos = stripped.find_first_of("|");
                if (pos == std::string::npos) {
                    boost::shared_lock<boost::shared_mutex> lock(m_localMapMutex);
                    if (stripped == "*") {
                        // "Broadcast" case:  |*|
                        for (auto& i : m_localMap) {
                            EventLoop::getIOService().post(bind_weak(&karabo::xms::SignalSlotable::processEvent, i.second,
                                                                     header, message, i.second->getEpochMillis()));
                        }
                    } else {
                        // "Specified" slotInstanceId case:  |slotInstaceId|
                        auto it = m_localMap.find(stripped);
                        if (it != m_localMap.end()) {
                            EventLoop::getIOService().post(bind_weak(&karabo::xms::SignalSlotable::processEvent, it->second,
                                                                     header, message, it->second->getEpochMillis()));
                        }
                    }
                } else {
                    // Many slotInstanceIds : |slotInstaceId1||slotInstaceId2||slotInstaceId3|...|slotInstaceIdN|
                    std::vector<std::string> ids;
                    boost::split(ids, stripped, boost::is_any_of("|"), boost::token_compress_on);
                    boost::shared_lock<boost::shared_mutex> lock(m_localMapMutex);
                    for (auto& slotInstanceId : ids) {
                        for (auto& i : m_localMap) {
                            if (slotInstanceId == i.first) {
                                EventLoop::getIOService().post(bind_weak(&karabo::xms::SignalSlotable::processEvent, i.second,
                                                                         header, message, i.second->getEpochMillis()));
                            }
                        }
                    }
                }
            } else {
                m_customReadHandler(header, message);
            }
            channel->readAsyncHashPointerHashPointer(bind_weak(&PointToPoint::onP2PMessage, this, _1, channel, _2, _3));
        }


        void PointToPoint::channelErrorHandler(const ErrorCode& ec, const Channel::Pointer& channel) {
            if (ec.value() != 2) {
                KARABO_LOG_FRAMEWORK_ERROR << "Channel Exception #"<< ec.value() << " -- " << ec.message();
            }
        }


        bool PointToPoint::publish(const std::string& remoteInstanceId,
                                             const karabo::util::Hash::Pointer& header,
                                             const karabo::util::Hash::Pointer& body,
                                             int prio) {
            Channel::Pointer channel;
            {
                boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                auto it = m_instanceIdToUrl.find(remoteInstanceId);
                if (it == m_instanceIdToUrl.end()) return false;
                const string& remoteUrl = it->second;
                auto openConnectionsIter = m_mapOpenConnections.find(remoteUrl);
                if (openConnectionsIter == m_mapOpenConnections.end()) return false;
                channel = openConnectionsIter->second.first;
            }
            channel->writeAsync(*header, *body, prio);
            for (auto& i : m_mapLogConnections) i.second.first->writeAsync(*header, *body, prio);

            return true;
        }


        std::string PointToPoint::allMapsToString() {
            ostringstream oss;
            vector<string> localIds;
            {
                boost::shared_lock<boost::shared_mutex> lock(m_localMapMutex);
                for (const auto& id : m_localMap) localIds.push_back(id.first);
            }
            oss << "Local map   ... [" << toString(localIds) << "]\n";
            vector<string> remoteIds;
            {
                boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                for (const auto& id : m_instanceIdToUrl) remoteIds.push_back(id.first);
            }
            oss << "InstanceIds ... [" << toString(remoteIds) << "]\n";
            oss << "URLs .......\n";
            {
                boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                for (const auto& url : m_urlToInstanceIdSet) {
                    if (m_mapOpenConnections.find(url.first) == m_mapOpenConnections.end()) {
                        oss << "Disconnected URL = \"";
                    } else {
                        oss << "Connected    URL = \"";
                    }
                    oss << url.first << "\" --> [" << toString(url.second) << "]\n";
                }
            }
            return oss.str();
        }


        karabo::util::Hash PointToPoint::queueInfo() {
            Hash info;
            boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            for (auto i : m_mapOpenConnections) {
                const string& url = i.first;
                string serverId = "_none_";
                auto iter = m_urlToServerId.find(url);
                if (iter != m_urlToServerId.end()) serverId = iter->second;
                TcpChannel::Pointer tcpChannel = boost::static_pointer_cast<TcpChannel>(i.second.first);
                info.set(url, Hash("queueInfo", tcpChannel->queueInfo(), "serverId", serverId));
            }
            return info;
        }
    }
}
