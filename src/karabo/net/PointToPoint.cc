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

using namespace std;
using namespace karabo::util;
using namespace karabo::log;

namespace karabo {
    namespace net {


        PointToPoint::PointToPoint() : m_serverPort(0), m_localUrl(""), m_globalHandler() {
            // No bind_weak in constructor possible yet... call "second" constructor via EventLoop
            EventLoop::getIOService().post(boost::bind(&PointToPoint::init, this));
        }


        void PointToPoint::init() {
            try {
                // We get an exception if the object is destroyed
                boost::shared_ptr<PointToPoint> guard = shared_from_this();
                // Here bind_weak is possible - second constructor
                Connection::Pointer connection = Connection::create(Hash("Tcp.port", 0, "Tcp.type", "server"));
                m_serverPort = connection->startAsync(bind_weak(&PointToPoint::onConnectServer, this, _1, _2, connection));
                m_localUrl = "tcp://" + karabo::net::bareHostName() + ":" + toString(m_serverPort);
            } catch (const std::exception&) {
            }
        }


        PointToPoint::~PointToPoint() {
        }


        void PointToPoint::registerP2PReadHandler(const GlobalP2PReadHandler& handler) {
            m_globalHandler = handler;
        }


        // Bookkeeping stuff

        std::string PointToPoint::getLocalUrl() const {
            return m_localUrl;
        }


        void PointToPoint::updateUrl(const std::string& newInstanceId, const std::string& remoteUrl) {
            boost::mutex::scoped_lock lock(m_pointToPointMutex);
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
            boost::mutex::scoped_lock lock(m_pointToPointMutex);
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
            boost::mutex::scoped_lock lock(m_pointToPointMutex);
            auto it = m_instanceIdToUrl.find(remoteInstanceId);
            if (it == m_instanceIdToUrl.end()) return "";
            return it->second;
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
            if (e) {
                channelErrorHandler(e, channel);
                return;
            }
            // We can accept more connections
            channel->getConnection()->startAsync(bind_weak(&PointToPoint::onConnectServer, this, _1, _2, connection));
            channel->readAsyncHashPointer(bind_weak(&PointToPoint::onConnectServerBottomHalf, this, _1, channel, connection, _2));
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
                    boost::mutex::scoped_lock lock(m_pointToPointMutex);
                    m_mapOpenConnections[remoteUrl] = std::make_pair(channel, connection);
                }
                channel->readAsyncHashPointerHashPointer(bind_weak(&PointToPoint::onP2PMessage, this, _1, channel, _2, _3));
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "PointToPoint::onConnectServerBottomHalf : exception -- " << e.what()
                        << "\n**** message was ...\n" << message;
            }
        }


        bool PointToPoint::connect(const std::string& remoteInstanceId) {

            boost::mutex::scoped_lock lock(m_pointToPointMutex);

            auto it = m_instanceIdToUrl.find(remoteInstanceId);
            if (it == m_instanceIdToUrl.end()) return false;
            const std::string& remoteUrl = it->second;

            auto ii = m_mapOpenConnections.find(remoteUrl);
            if (ii != m_mapOpenConnections.end()) return true;   // already connected

            Connection::Pointer connection = Connection::create(Hash("Tcp", Hash("type", "client", "url", remoteUrl)));
            Channel::Pointer channel = connection->start();
            if (!channel) return false;
            this->onConnectClient(boost::system::error_code(), channel, connection, Hash("url", remoteUrl));
            return true;
        }


        bool PointToPoint::connectAsync(const std::string& remoteInstanceId) {

            boost::mutex::scoped_lock lock(m_pointToPointMutex);

            auto it = m_instanceIdToUrl.find(remoteInstanceId);
            if (it == m_instanceIdToUrl.end()) return false;
            const std::string& remoteUrl = it->second;

            auto ii = m_mapOpenConnections.find(remoteUrl);
            if (ii != m_mapOpenConnections.end()) return true;   // already connected

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
                boost::mutex::scoped_lock lock(m_pointToPointMutex);
                m_mapOpenConnections[remoteUrl] = std::make_pair(channel, connection);
            }
            channel->writeAsync(Hash("url", m_localUrl), 4);
            channel->readAsyncHashPointerHashPointer(bind_weak(&PointToPoint::onP2PMessage, this, _1, channel, _2, _3));
        }


        bool PointToPoint::isConnected(const std::string& remoteInstanceId) {
            boost::mutex::scoped_lock lock(m_pointToPointMutex);
            const auto& it = m_instanceIdToUrl.find(remoteInstanceId);
            if (it == m_instanceIdToUrl.end()) return false;
            const std::string& remoteUrl = it->second;
            const auto& ir = m_mapOpenConnections.find(remoteUrl);
            if (ir == m_mapOpenConnections.end()) return false;
            return true;
        }


        void PointToPoint::filterConnectedAndGroupByUrl(SlotMap& input, std::map<std::string, SlotMap>& resultMap) {
            resultMap.clear();
            boost::mutex::scoped_lock lock(m_pointToPointMutex);
            for (SlotMap::iterator it = input.begin(); it != input.end(); ++it) {
                const string& remoteInstanceId = it->first;
                const std::set<std::string>& remoteValue = it->second;
                const auto& ii = m_instanceIdToUrl.find(remoteInstanceId);
                if (ii == m_instanceIdToUrl.end()) continue;
                const std::string& remoteUrl = ii->second;
                const auto& ir = m_mapOpenConnections.find(remoteUrl);
                if (ir == m_mapOpenConnections.end()) continue;   // not connected
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

            m_globalHandler(header, message);
        }


        void PointToPoint::channelErrorHandler(const ErrorCode& ec, const Channel::Pointer& channel) {
            KARABO_LOG_FRAMEWORK_ERROR << "Channel Exception #"<< ec.value() << " -- " << ec.message();
        }


        bool PointToPoint::publish(const std::string& remoteInstanceId,
                                             const karabo::util::Hash::Pointer& header,
                                             const karabo::util::Hash::Pointer& body,
                                             int prio) {
            boost::mutex::scoped_lock lock(m_pointToPointMutex);
            auto it = m_instanceIdToUrl.find(remoteInstanceId);
            if (it == m_instanceIdToUrl.end()) return false;
            const string& remoteUrl = it->second;
            auto ir = m_mapOpenConnections.find(remoteUrl);
            if (ir == m_mapOpenConnections.end()) return false;
            ir->second.first->writeAsync(*header, *body, prio);
            return true;
        }
    }
}
