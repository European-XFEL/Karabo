#include <sstream>
#include <string>
#include <unordered_set>

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "karabo/log.hpp"
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


        PointToPoint::PointToPoint() : m_serverPort(0), m_localUrl("") {
            Connection::Pointer connection = Connection::create(Hash("Tcp.port", 0, "Tcp.type", "server"));
            m_serverPort = connection->startAsync(boost::bind(&PointToPoint::onConnectServer, this, _1, _2, connection));
            m_localUrl = "tcp://" + karabo::net::bareHostName() + ":" + toString(m_serverPort);
        }


        PointToPoint::~PointToPoint() {
        }


        // Bookkeeping stuff

        std::string PointToPoint::getLocalUrl() const {
            return m_localUrl;
        }


        void PointToPoint::updateUrl(const std::string& newInstanceId, const std::string& remoteUrl) {
            boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            auto it = m_instanceIdToUrl.find(newInstanceId);
            if (it != m_instanceIdToUrl.end()) {
                // found -- clean up old info
                const std::string& oldUrl = it->second;
                auto ii = m_urlToInstanceIdSet.find(oldUrl);
                if (ii != m_urlToInstanceIdSet.end()) ii->second.erase(newInstanceId);
                if (ii->second.empty()) {
                    // Don't keep entry with the empty id's set
                    m_urlToInstanceIdSet.erase(ii);
                    // No reference left to the old connection
                    auto iii = m_mapOpenConnections.find(oldUrl);
                    // This will close obsolete connection (by destroying their shared pointers)
                    if (iii != m_mapOpenConnections.end()) m_mapOpenConnections.erase(iii);
                }
            }
            m_instanceIdToUrl[newInstanceId] = remoteUrl;
            auto ii = m_urlToInstanceIdSet.find(remoteUrl);
            if (ii == m_urlToInstanceIdSet.end()) {
                m_urlToInstanceIdSet[remoteUrl] = std::set<std::string>();
                ii = m_urlToInstanceIdSet.find(remoteUrl);
            }
            auto& ids = ii->second;
            ids.insert(newInstanceId);
        }


        void PointToPoint::eraseUrl(const std::string& instanceId) {
            boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            auto it = m_instanceIdToUrl.find(instanceId);
            if (it == m_instanceIdToUrl.end()) return;
            const std::string& url = it->second;
            auto ii = m_urlToInstanceIdSet.find(url);
            if (ii == m_urlToInstanceIdSet.end()) {
                auto ir = m_mapOpenConnections.find(url);
                // This should not ever happen ...
                if (ir != m_mapOpenConnections.end()) m_mapOpenConnections.erase(ir);
                return;
            }
            auto& ids = ii->second;
            ids.erase(instanceId);
            if (ids.empty()) {
                m_urlToInstanceIdSet.erase(ii);
                auto ir = m_mapOpenConnections.find(url);
                if (ir != m_mapOpenConnections.end()) m_mapOpenConnections.erase(ir);
            }
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
                channel->getConnection()->startAsync(bind_weak(&PointToPoint::onConnectServer, this, _1, _2, connection));
                channel->readAsyncHashPointer(bind_weak(&PointToPoint::onConnectServerBottomHalf, this, _1, channel, connection, _2));
            } catch (const std::exception&) {
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

            try {
                const string& remoteUrl = message->get<string>("url");
                {
                    boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                    m_mapOpenConnections[remoteUrl] = std::make_pair(channel, connection);
                }
                channel->readAsyncHashPointerHashPointer(bind_weak(&PointToPoint::onP2PMessage, this, _1, channel, _2, _3));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "PointToPoint::onConnectServerBottomHalf : exception -- " << e.what()
                        << "\n**** message was ...\n" << message;
            }
        }


        bool PointToPoint::connect(const std::string& remoteInstanceId) {
            if (this->isInLocalMap(remoteInstanceId)) return false;

            string remoteUrl = "";
            {
                boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                auto it = m_instanceIdToUrl.find(remoteInstanceId);
                if (it == m_instanceIdToUrl.end()) return false;
                remoteUrl = it->second;
                auto ii = m_mapOpenConnections.find(remoteUrl);
                if (ii != m_mapOpenConnections.end()) return true;   // already connected
            }

            Connection::Pointer connection = Connection::create(Hash("Tcp", Hash("type", "client", "url", remoteUrl)));
            Channel::Pointer channel = connection->start();  // synchronous call
            if (!channel) return false;

            channel->setAsyncChannelPolicy(3, "REMOVE_OLDEST");
            channel->setAsyncChannelPolicy(4, "LOSSLESS");

            {
                boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                m_mapOpenConnections[remoteUrl] = std::make_pair(channel, connection);
            }
            channel->writeAsync(Hash("url", m_localUrl), 4);
            channel->readAsyncHashPointerHashPointer(bind_weak(&PointToPoint::onP2PMessage, this, _1, channel, _2, _3));
            return true;
        }


        bool PointToPoint::connectAsync(const std::string& remoteInstanceId) {
            if (this->isInLocalMap(remoteInstanceId)) return false;

            string remoteUrl = "";
            {
                boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);

                auto it = m_instanceIdToUrl.find(remoteInstanceId);
                if (it == m_instanceIdToUrl.end()) return false;

                remoteUrl = it->second;

                auto ii = m_mapOpenConnections.find(remoteUrl);
                if (ii != m_mapOpenConnections.end()) return true;   // already connected
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
            
            channel->setAsyncChannelPolicy(3, "REMOVE_OLDEST");
            channel->setAsyncChannelPolicy(4, "LOSSLESS");

            const string& remoteUrl = config.get<string>("url");
            {
                boost::unique_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                m_mapOpenConnections[remoteUrl] = std::make_pair(channel, connection);
            }
            channel->writeAsync(Hash("url", m_localUrl), 4);
            channel->readAsyncHashPointerHashPointer(bind_weak(&PointToPoint::onP2PMessage, this, _1, channel, _2, _3));
        }


        bool PointToPoint::isConnected(const std::string& remoteInstanceId) {
            boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
            const auto& it = m_instanceIdToUrl.find(remoteInstanceId);
            if (it == m_instanceIdToUrl.end()) return false;
            const std::string& remoteUrl = it->second;
            const auto& ir = m_mapOpenConnections.find(remoteUrl);
            if (ir == m_mapOpenConnections.end()) return false;
            return true;
        }


        void PointToPoint::filterConnectedAndGroupByUrl(SlotMap& input, std::map<std::string, SlotMap>& resultMap) {
            resultMap.clear();
            for (SlotMap::iterator it = input.begin(); it != input.end(); ++it) {
                const string& remoteInstanceId = it->first;
                const std::set<std::string>& remoteValue = it->second;
                string remoteUrl = "";
                {
                    boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                    const auto& ii = m_instanceIdToUrl.find(remoteInstanceId);
                    if (ii == m_instanceIdToUrl.end()) continue;
                    remoteUrl = ii->second;
                    const auto& ir = m_mapOpenConnections.find(remoteUrl);
                    if (ir == m_mapOpenConnections.end()) continue;   // not connected
                }
                auto ik = resultMap.find(remoteUrl);
                if (ik == resultMap.end()) {
                    resultMap[remoteUrl] = SlotMap();
                    ik = resultMap.find(remoteUrl);
                }
                ik->second[remoteInstanceId] = remoteValue;
                input.erase(it);
            }
        }


        void PointToPoint::onP2PMessage(const ErrorCode& e, const karabo::net::Channel::Pointer& channel,
                                        const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& message) {
            if (e) {
                channelErrorHandler(e, channel);
                return;
            }

            // We don't need to check the existence of "slotInstanceIds" in header. It is always there!
            const string& slotInstanceIds = header->get<string>("slotInstanceIds");
            // slotInstanceIds is defined like
            // |slotInstanceId| or |*| or |slotInstanceId1||slotInstanceId2||...||slotInstanceIdN|
            KARABO_LOG_FRAMEWORK_DEBUG << "SignalSlotable::onP2pMessage  slotInstanceIds : \"" << slotInstanceIds << "\"";
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
                    for (auto& i : m_localMap) {
                        if (stripped == i.first) {
                            EventLoop::getIOService().post(bind_weak(&karabo::xms::SignalSlotable::processEvent, i.second,
                                                                     header, message, i.second->getEpochMillis()));
                        }
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
            channel->readAsyncHashPointerHashPointer(bind_weak(&PointToPoint::onP2PMessage, this, _1, channel, _2, _3));
        }


        void PointToPoint::channelErrorHandler(const ErrorCode& ec, const Channel::Pointer& channel) {
            KARABO_LOG_FRAMEWORK_ERROR << "Channel Exception #"<< ec.value() << " -- " << ec.message();
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
                auto ir = m_mapOpenConnections.find(remoteUrl);
                if (ir == m_mapOpenConnections.end()) return false;
                channel = ir->second.first;
            }
            channel->writeAsync(*header, *body, prio);
            return true;
        }


        void PointToPoint::printMaps() {
            vector<string> localIds;
            {
                boost::shared_lock<boost::shared_mutex> lock(m_localMapMutex);
                for (const auto& id : m_localMap) localIds.push_back(id.first);
            }
            KARABO_LOG_FRAMEWORK_DEBUG << "Local map ..." << toString(localIds);
            vector<string> remoteIds;
            {
                boost::shared_lock<boost::shared_mutex> lock(m_pointToPointMutex);
                for (const auto& id : m_instanceIdToUrl) remoteIds.push_back(id.first);
            }
            KARABO_LOG_FRAMEWORK_DEBUG << "Remote map ..." << toString(remoteIds);
            ostringstream oss;
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
            KARABO_LOG_FRAMEWORK_DEBUG << "URLs .......\n" << oss.str();
        }
    }
}
