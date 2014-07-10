/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2011, 2:25 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <sys/types.h>

#include <karabo/webAuth/Authenticator.hh>

#include "SignalSlotable.hh"

namespace karabo {
    namespace xms {

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::net;
        using namespace karabo::webAuth;

        // Static initializations
        std::set<int> SignalSlotable::m_reconnectIntervals = std::set<int>();


        SignalSlotable::SignalSlotable() {
        }


        SignalSlotable::SignalSlotable(const string& instanceId,
                                       const BrokerConnection::Pointer& connection) {
            init(instanceId, connection);
        }


        SignalSlotable::SignalSlotable(const std::string& instanceId,
                                       const std::string& brokerType,
                                       const karabo::util::Hash& brokerConfiguration) {
            karabo::net::BrokerConnection::Pointer connection = karabo::net::BrokerConnection::create(brokerType, brokerConfiguration);
            init(instanceId, connection);
        }


        SignalSlotable::~SignalSlotable() {
        }


        void SignalSlotable::init(const std::string& instanceId,
                                  const karabo::net::BrokerConnection::Pointer& connection) {


            m_defaultAccessLevel = KARABO_DEFAULT_ACCESS_LEVEL;
            m_connection = connection;
            m_instanceId = instanceId;

            // Sanify instanceId (e.g. dots are bad)
            sanifyInstanceId(m_instanceId);

            // Create the managing ioService object
            m_ioService = m_connection->getIOService();
            // Start connection (and take the default channel for signals)
            m_signalChannel = m_connection->start();
            // Create request channel
            m_requestChannel = m_connection->createChannel();

            // Check, whether we are unique in the distributed system
            std::pair<bool, std::string> result = isValidInstanceId(instanceId);
            if (result.first) {
                initReconnectIntervals();
                registerDefaultSignalsAndSlots();
                startTrackingSystem();
            } else {
                throw KARABO_SIGNALSLOT_EXCEPTION(result.second);
            }
        }


        bool SignalSlotable::login(const std::string& username, const std::string& password, const std::string& provider) {

            string brokerHostname = getConnection()->getBrokerHostname();
            int brokerPort = getConnection()->getBrokerPort();
            string brokerTopic = getConnection()->getBrokerTopic();

            m_authenticator = Authenticator::Pointer(new Authenticator(username, password, provider, boost::asio::ip::host_name(), brokerHostname, brokerPort, brokerTopic));

            if (username == "god" && godEncode(password) == 749) {
                std::cout << "Bypassing authentication service and entering god mode..., full access granted" << std::endl;
                m_defaultAccessLevel = 1000;
                return true;
            }
            try {
                if (m_authenticator->login()) {
                    m_defaultAccessLevel = m_authenticator->getDefaultAccessLevelId();
                    //m_accessList = m_authenticator->getAccessList();
                    //m_sessionToken = m_authenticator->getSessionToken();
                    return true;
                } else {
                    return false;
                }
            } catch (karabo::util::NetworkException&) {
                karabo::util::Exception::clearTrace();
                KARABO_LOG_FRAMEWORK_ERROR << "Could not contact the authentication service, falling back to in-build access level";
                m_defaultAccessLevel = KARABO_DEFAULT_ACCESS_LEVEL;
                return true;
            }
        }


        bool SignalSlotable::logout() {
            return m_authenticator->logout();
        }


        void SignalSlotable::sanifyInstanceId(std::string& instanceId) const {
            for (std::string::iterator it = instanceId.begin(); it != instanceId.end(); ++it) {
                if ((*it) == '.') (*it) = '-';
            }
        }


        std::pair<bool, std::string> SignalSlotable::isValidInstanceId(const std::string& instanceId) {

            Hash instanceInfo;
            try {
                Requestor(m_requestChannel, m_instanceId).call("*", "slotPing", instanceId, true, false).timeout(100).receive(instanceInfo);
            } catch (const karabo::util::TimeoutException&) {
                Exception::clearTrace();
                return std::make_pair(true, "");
            }
            string foreignHost("unknown");
            if (instanceInfo.has("host")) instanceInfo.get("host", foreignHost);
            string msg("Another instance with the same ID is already online (on host: " + foreignHost + ")");
            return std::make_pair(false, msg);
        }


        void SignalSlotable::initReconnectIntervals() {
            if (m_reconnectIntervals.empty()) {
                for (int i = 0; i <= 50; ++i) {
                    int x = static_cast<int> (1 + 0.005 * i * i * i * i);
                    m_reconnectIntervals.insert(-x);
                }
            }
        }


        void SignalSlotable::registerDefaultSignalsAndSlots() {

            // Emits a "still-alive" signal
            SIGNAL3("signalHeartbeat", string /*instanceId*/, int /*heartbeatIntervalInSec*/, Hash /*instanceInfo*/)

            // Signals a successful connection
            //SIGNAL4("signalConnected", string /*signalInstanceId*/, string /*signalFunction*/, string /*slotInstanceId*/, string /*slotFunction*/)

            // Signals a successful disconnection
            SIGNAL4("signalDisconnected", string /*signalInstanceId*/, string /*signalFunction*/, string /*slotInstanceId*/, string /*slotFunction*/)

            // Emits as answer to a ping request
            SIGNAL1("signalGotPinged", string /*instanceId*/)

            // Listener for heartbeats
            SLOT3(slotHeartbeat, string /*instanceId*/, int /*heartbeatIntervalInSec*/, Hash /*instanceInfo*/)

            // Global ping listener
            GLOBAL_SLOT3(slotPing, string /*callersInstanceId*/, bool /*replyIfSame*/, bool /*trackPingedInstance*/)

            // Listener for ping answers
            SLOT2(slotPingAnswer, string /*instanceId*/, Hash /*instanceInfo*/)

            // Connects signal to slot
            SLOT4(slotConnect, string /*signalFunction*/, string /*slotInstanceId*/, string /*slotFunction*/, int /*connectionType*/)

            // Replies whether slot exists on this instance
            SLOT4(slotHasSlot, string /*signalInstanceId*/, string /*signalFunction*/, string /*slotFunction*/, int /*connectionType*/)

            // Disconnects signal from slot
            SLOT3(slotDisconnect, string /*signalFunction*/, string /*slotInstanceId*/, string /*slotFunction*/)

            // Globally gives up tracking of this connection
            GLOBAL_SLOT1(slotStopTrackingExistenceOfConnection, string /*connectionId*/)

            // Function request
            SLOT1(slotGetAvailableFunctions, string /*functionType*/)

            //GLOBAL_SLOT0(slotTryReconnectNow)

            // Provides information about p2p connectivity
            SLOT2(slotGetOutputChannelInformation, string /*ioChannelId*/, int /*pid*/)

            SLOT3(slotConnectToOutputChannel, string /*inputChannelName*/, karabo::util::Hash /*outputChannelInfo */, bool /*connect/disconnect*/)

        }


        void SignalSlotable::runEventLoop(int heartbeatInterval, const karabo::util::Hash& instanceInfo) {

            m_heartbeatInterval = heartbeatInterval;
            m_instanceInfo = instanceInfo;

            // Inject the heartbeat interval to instanceInfo
            m_instanceInfo.set("heartbeatInterval", heartbeatInterval);

            call("*", "slotInstanceNew", m_instanceId, m_instanceInfo);

            KARABO_LOG_FRAMEWORK_INFO << "Instance starts up with id: " << m_instanceId;

            if (m_heartbeatInterval > 0) {
                m_sendHeartbeats = true;
                // Send heartbeat and sleep for m_heartbeatInterval seconds
                boost::thread heartbeatThread(boost::bind(&karabo::xms::SignalSlotable::emitHeartbeat, this));
                m_ioService->work(); // blocks
                m_sendHeartbeats = false;
                heartbeatThread.interrupt(); // interrupt sleeping in heartbeatThread
                heartbeatThread.join();
            } else {
                m_ioService->work(); // blocks
            }
            stopTrackingSystem();
        }


        void SignalSlotable::stopTrackingSystem() {
            m_doTracking = false;
            m_trackingThread.join();
        }


        void SignalSlotable::stopEventLoop() {
            m_ioService->stop();
            call("*", "slotInstanceGone", m_instanceId, m_instanceInfo);
        }


        void SignalSlotable::setSenderInfo(const karabo::util::Hash& senderInfo) {
            m_senderInfo = senderInfo;
        }


        const karabo::util::Hash& SignalSlotable::getSenderInfo() const {
            return m_senderInfo;
        }


        BrokerConnection::Pointer SignalSlotable::getConnection() const {
            return m_connection;
        }


        void SignalSlotable::startTrackingSystem() {
            // Countdown and finally timeout registered heartbeats
            m_doTracking = true;
            m_trackingThread = boost::thread(boost::bind(&SignalSlotable::letConnectionSlowlyDieWithoutHeartbeat, this));
        }


        void SignalSlotable::emitHeartbeat() {
            //----------------- make this thread sensible to external interrupts
            boost::this_thread::interruption_enabled(); // enable interruption +
            boost::this_thread::interruption_requested(); // request interruption = we need both!
            while (m_sendHeartbeats) {
                {
                    boost::this_thread::disable_interruption di; // disable interruption in this block
                    emit("signalHeartbeat", getInstanceId(), m_heartbeatInterval, m_instanceInfo);
                }
                // here the interruption enabled again
                try {
                    boost::this_thread::sleep(boost::posix_time::seconds(m_heartbeatInterval));
                } catch (const boost::thread_interrupted&) {
                }
            }
        }


        const std::vector<std::pair<std::string, karabo::util::Hash> >& SignalSlotable::getAvailableInstances(bool activateTracking) {
            m_availableInstances.clear();
            call("*", "slotPing", m_instanceId, false, activateTracking);
            // The function slotPingAnswer will be called by all instances available now
            // Lets wait a fair amount of time - huaaah this is bad isn't it :-(
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            if (activateTracking) {
                for (size_t i = 0; i < m_availableInstances.size(); ++i) {
                    const string& instanceId = m_availableInstances[i].first;
                    const Hash& instanceInfo = m_availableInstances[i].second;

                    if (instanceId == m_instanceId) continue;

                    m_heartbeatMutex.lock();
                    if (!m_trackedComponents.has(instanceId)) addTrackedComponent(instanceId, instanceInfo);
                    m_trackedComponents.set(instanceId + ".isExplicitlyTracked", true);
                    m_heartbeatMutex.unlock();

                    m_connectMutex.lock();
                    m_signalInstances["signalHeartbeat"]->registerSlot(instanceId, "slotHeartbeat");
                    m_connectMutex.unlock();

                    registerConnectionForTracking(m_instanceId, "signalHeartbeat", instanceId, "slotHeartbeat", TRACK);
                }
            }
            return m_availableInstances;
        }


        std::pair<bool, std::string> SignalSlotable::exists(const std::string& instanceId) {
            string hostname;
            Hash instanceInfo;
            try {
                this->request("*", "slotPing", instanceId, true, false).timeout(300).receive(instanceInfo);
            } catch (const karabo::util::TimeoutException&) {
                return std::make_pair(false, hostname);
            }
            if (instanceInfo.has("host")) instanceInfo.get("host", hostname);
            return std::make_pair(true, hostname);
        }


        void SignalSlotable::slotPing(const std::string& instanceId, bool replyIfInstanceIdIsDuplicated, bool trackPingedInstance) {

            if (replyIfInstanceIdIsDuplicated) {
                //cout << "Got asked whether I am called " << instanceId << endl;
                if (instanceId == m_instanceId) {
                    reply(m_instanceInfo);
                }
            } else {
                //cout << "Got pinged from " << instanceId << endl;
                call(instanceId, "slotPingAnswer", m_instanceId, m_instanceInfo);
            }

            if (trackPingedInstance && instanceId != m_instanceId) {
                m_connectMutex.lock();
                m_signalInstances["signalHeartbeat"]->registerSlot(instanceId, "slotHeartbeat");
                m_connectMutex.unlock();
            }
        }


        std::vector<string> SignalSlotable::getAvailableSignals(const std::string& instanceId) {
            std::vector<string> signals;
            try {
                request(instanceId, "slotGetAvailableFunctions", "signals").timeout(100).receive(signals);
            } catch (const karabo::util::TimeoutException&) {
                karabo::util::Exception::clearTrace();
                cout << "ERROR:  The requested instanceId \"" << instanceId << "\" is currently not available." << endl;
            }
            return signals;
        }


        std::vector<string> SignalSlotable::getAvailableSlots(const std::string& instanceId) {
            std::vector<string> slots;
            try {
                request(instanceId, "slotGetAvailableFunctions", "slots").timeout(100).receive(slots);
            } catch (const karabo::util::TimeoutException&) {
                karabo::util::Exception::clearTrace();
                cout << "ERROR:  The requested instanceId \"" << instanceId << "\" is currently not available." << endl;
            }
            return slots;
        }


        const SignalSlotable::SlotInstancePointer& SignalSlotable::getSenderInfo(const std::string& slotFunction) {
            SlotInstancesConstIt it = m_slotInstances.find(slotFunction);
            if (it == m_slotInstances.end()) throw KARABO_SIGNALSLOT_EXCEPTION("No slot-object could be found for slotFunction \"" + slotFunction + "\"");
            return it->second;
        }


        void SignalSlotable::slotGetAvailableFunctions(const std::string& type) {
            std::vector<string> functions;
            if (type == "signals") {
                for (SignalInstancesConstIt it = m_signalInstances.begin(); it != m_signalInstances.end(); ++it) {
                    const string& function = it->first;
                    functions.push_back(function);
                }
            } else if (type == "slots") {
                for (SlotInstancesConstIt it = m_slotInstances.begin(); it != m_slotInstances.end(); ++it) {
                    const string& function = it->first;
                    std::cout << "|" << function << "|" << std::endl;
                    // Filter out service slots // TODO finally update to last set of those
                    if (function == "slotConnect" || function == "slotDisconnect" || function == "slotGetAvailableFunctions" ||
                            function == "slotHasSlot" || function == "slotHeartbeat" || function == "slotPing" || function == "slotPingAnswer" ||
                            function == "slotStopTrackingExistenceOfConnection") {
                        continue;
                    }
                    functions.push_back(it->first);
                }
            }
            reply(functions);
        }


        void SignalSlotable::slotPingAnswer(const std::string& instanceId, const karabo::util::Hash& hash) {
            m_availableInstances.push_back(std::make_pair(instanceId, hash));
        }


        void SignalSlotable::slotHeartbeat(const std::string& networkId, const int& heartbeatInterval, const Hash& instanceInfo) {
            refreshTimeToLiveForConnectedSlot(networkId, heartbeatInterval, instanceInfo);
        }


        const std::string& SignalSlotable::getInstanceId() const {
            return m_instanceId;
        }


        void SignalSlotable::updateInstanceInfo(const karabo::util::Hash& update) {
            m_instanceInfo.merge(update);
            call("*", "slotInstanceUpdated", m_instanceId, m_instanceInfo);
        }


        const karabo::util::Hash& SignalSlotable::getInstanceInfo() const {
            return m_instanceInfo;
        }


        void SignalSlotable::autoConnectAllSignals(const karabo::util::Hash& config, const std::string signalRegularExpression) {
            try {
                for (Hash::const_iterator it = config.begin(); it != config.end(); ++it) {
                    const boost::regex e(signalRegularExpression);
                    if (boost::regex_match(it->getKey(), e)) {
                        const vector<string>& connects = it->getValue<vector<string> >();
                        for (size_t i = 0; i < connects.size(); ++i) {
                            connect(it->getKey(), connects[i], RECONNECT);
                        }
                    }
                }
            } catch (...) {
                KARABO_RETHROW;
            }
        }


        void SignalSlotable::autoConnectAllSlots(const karabo::util::Hash& config, const std::string slotRegularExpression) {
            try {
                for (Hash::const_iterator it = config.begin(); it != config.end(); ++it) {
                    const boost::regex e(slotRegularExpression);
                    if (boost::regex_match(it->getKey(), e)) {
                        const vector<string>& connects = it->getValue<vector<string> >();
                        for (size_t i = 0; i < connects.size(); ++i) {
                            cout << "AutoConnect:" << connects[i] << endl;
                            connect(connects[i], it->getKey(), RECONNECT);
                        }
                    }
                }
            } catch (...) {
                KARABO_RETHROW;
            }
        }


        void SignalSlotable::trackExistenceOfInstance(const std::string& instanceId, const karabo::util::Hash& instanceInfo) {
            if (instanceId == m_instanceId) return;
            {
                boost::mutex::scoped_lock lock(m_heartbeatMutex);
                if (!m_trackedComponents.has(instanceId)) {
                    addTrackedComponent(instanceId, instanceInfo);
                }
                m_trackedComponents.set(instanceId + ".isExplicitlyTracked", true);
            }
            connect(instanceId, "signalHeartbeat", "", "slotHeartbeat", TRACK, false);
        }


        void SignalSlotable::stopTrackingExistenceOfInstance(const std::string& instanceId) {
            boost::mutex::scoped_lock lock (m_heartbeatMutex);
            if (m_trackedComponents.has(instanceId)) {
                const vector<Hash>& connections = m_trackedComponents.get<std::vector<Hash> >(instanceId + ".connections");
                for (size_t i = 0; i < connections.size(); ++i) {
                    const string& signalInstanceId = connections[i].get<string>("signalInstanceId");
                    const string& signalFunction = connections[i].get<string>("signalFunction");
                    const string& slotInstanceId = connections[i].get<string>("slotInstanceId");
                    const string& slotFunction = connections[i].get<string>("slotFunction");
                    m_heartbeatMutex.unlock();
                    cout << "LOW LEVEL DEBUG: Disconnecting: " << signalInstanceId << ":" << signalFunction << " <-> " << slotInstanceId << ":" << slotFunction << endl;
                    disconnect(signalInstanceId, signalFunction, slotInstanceId, slotFunction, false);
                    m_heartbeatMutex.lock();
                }
                m_trackedComponents.erase(instanceId);
            } 
        }


        void SignalSlotable::registerInstanceNotAvailableHandler(const InstanceNotAvailableHandler& instanceNotAvailableCallback) {
            m_instanceNotAvailableHandler = instanceNotAvailableCallback;
        }


        void SignalSlotable::registerInstanceAvailableAgainHandler(const InstanceAvailableAgainHandler& instanceAvailableAgainCallback) {
            m_instanceAvailableAgainHandler = instanceAvailableAgainCallback;
        }


        KARABO_DEPRECATED void SignalSlotable::instanceNotAvailable(const std::string& instanceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << instanceId << "\" not available anymore";
        }


        void SignalSlotable::instanceAvailableAgain(const std::string& instanceId) {
            KARABO_LOG_FRAMEWORK_DEBUG << "Instance \"" << instanceId << "\" available again";
        }


        bool SignalSlotable::connectChannels(std::string outputInstanceId, const std::string& outputName, std::string inputInstanceId, const std::string& inputName, const bool isVerbose) {

            if (outputInstanceId.empty()) outputInstanceId = m_instanceId;
            if (inputInstanceId.empty()) inputInstanceId = m_instanceId;

            bool outputChannelExists = false;
            bool inputChannelExists = false;
            karabo::util::Hash outputChannelInfo;

            if (outputInstanceId == m_instanceId) { // Local output channel
                outputChannelInfo = slotGetOutputChannelInformation(outputName, static_cast<int> (getpid()));
                if (!outputChannelInfo.empty()) outputChannelExists = true;

            } else { // Remote output channel
                try {
                    this->request(outputInstanceId, "slotGetOutputChannelInformation", outputName, static_cast<int> (getpid())).timeout(1000).receive(outputChannelExists, outputChannelInfo);
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    inputChannelExists = false;
                }
            }

            if (outputChannelExists) {

                if (inputInstanceId == m_instanceId) {// Local input channel
                    inputChannelExists = slotConnectToOutputChannel(inputName, outputChannelInfo, true);

                } else {
                    try {
                        this->request(inputInstanceId, "slotConnectToOutputChannel", inputName, outputChannelInfo, true).timeout(1000).receive(inputChannelExists);
                    } catch (const karabo::util::TimeoutException&) {
                        karabo::util::Exception::clearTrace();
                        inputChannelExists = false;
                    }
                }
            }

            if (outputChannelExists && inputChannelExists) {
                if (isVerbose) cout << "INFO   : Channel connection successfully established." << endl;
                return true;
            }
            if (isVerbose) cout << "ERROR   : Channel connection could not be established." << endl;
            return false;
        }


        bool SignalSlotable::slotConnectToOutputChannel(const std::string& inputName, const karabo::util::Hash& outputChannelInfo, bool connect) {
            try {
                // Loop channels
                InputChannels::const_iterator it = m_inputChannels.find(inputName);
                if (it != m_inputChannels.end()) {
                    if (connect) it->second->connect(outputChannelInfo); // Synchronous
                    else it->second->disconnect(outputChannelInfo);
                    reply(true);
                    return true;
                }
                return false;
            } catch (const Exception& e) {
                reply(false);
                return false;
            }
        }


        bool SignalSlotable::disconnectChannels(std::string outputInstanceId, const std::string& outputName, std::string inputInstanceId, const std::string& inputName, const bool isVerbose) {

            if (outputInstanceId.empty()) outputInstanceId = m_instanceId;
            if (inputInstanceId.empty()) inputInstanceId = m_instanceId;

            bool outputChannelExists = false;
            bool inputChannelExists = false;

            // Need to get the outputChannelInfo (containing amongst others, port and host)
            karabo::util::Hash outputChannelInfo;

            if (outputInstanceId == m_instanceId) { // Local output channel
                outputChannelInfo = slotGetOutputChannelInformation(outputName, static_cast<int> (getpid()));
                if (!outputChannelInfo.empty()) outputChannelExists = true;

            } else { // Remote output channel
                try {
                    this->request(outputInstanceId, "slotGetOutputChannelInformation", outputName, static_cast<int> (getpid())).timeout(1000).receive(outputChannelExists, outputChannelInfo);
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    inputChannelExists = false;
                }
            }

            if (outputChannelExists) {

                if (inputInstanceId == m_instanceId) { // Local input channel
                    inputChannelExists = slotConnectToOutputChannel(inputName, outputChannelInfo, false);

                } else {
                    try {
                        this->request(inputInstanceId, "slotConnectToOutputChannel", inputName, outputChannelInfo, false).timeout(1000).receive(inputChannelExists);
                    } catch (const karabo::util::TimeoutException&) {
                        karabo::util::Exception::clearTrace();
                        inputChannelExists = false;
                    }
                }
            }

            if (outputChannelExists && inputChannelExists) {
                if (isVerbose) cout << "INFO   : Channel connection successfully released." << endl;
                return true;
            }
            if (isVerbose) cout << "ERROR   : Channel connection could not be released." << endl;
            return false;
        }


        bool SignalSlotable::connect(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, ConnectionType connectionType, const bool isVerbose) {

            if (signalInstanceId.empty()) signalInstanceId = m_instanceId;
            if (slotInstanceId.empty()) slotInstanceId = m_instanceId;

            bool signalExists = tryToConnectToSignal(signalInstanceId, signalFunction, slotInstanceId, slotFunction, connectionType, isVerbose);
            bool slotExists = tryToFindSlot(signalInstanceId, signalFunction, slotInstanceId, slotFunction, connectionType, isVerbose);

            bool connectionEstablished = false;
            if (signalExists && slotExists) {
                //if (slotInstanceId != "*") emit("signalConnected", signalInstanceId, signalFunction, slotInstanceId, slotFunction);
                connectionEstablished = true;
                if (isVerbose) cout << "INFO  : Connection successfully established." << endl;
            } else if (signalExists) {
                if (isVerbose) cout << "WARN  : Connection not yet established, but will automatically be if slot appears." << endl;
            } else {
                if (isVerbose) cout << "ERROR : Connection could not be established." << endl;
            }
            return connectionEstablished;
        }


        bool SignalSlotable::tryToConnectToSignal(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const int& connectionType, const bool isVerbose) {

            bool signalExists = false;

            if (signalInstanceId == m_instanceId) { // Local signal requested
                boost::mutex::scoped_lock lock(m_connectMutex);
                SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
                if (it != m_signalInstances.end()) { // Signal found
                    signalExists = true;
                    // Register new slotId to local signal
                    it->second->registerSlot(slotInstanceId, slotFunction);
                    if (connectionType != NO_TRACK) {
                        // Equip own heartbeat with slot information
                        m_signalInstances["signalHeartbeat"]->registerSlot(slotInstanceId, "slotHeartbeat");
                        registerConnectionForTracking(m_instanceId, signalFunction, slotInstanceId, slotFunction, connectionType);
                    }
                } else {
                    signalExists = false;
                    if (isVerbose) cout << "WARN  : The requested signal \"" << signalFunction << "\" is currently not available on this (local) instance \"" << m_instanceId << "\"." << endl;
                }
            } else { // Remote signal requested
                try {
                    startRequest().call(signalInstanceId, "slotConnect", signalFunction, slotInstanceId, slotFunction, connectionType).timeout(100).receive(signalExists);
                    if (!signalExists) {
                        if (isVerbose) cout << "WARN  : The requested signal \"" << signalFunction << "\" is currently not available on signal-instanceId \"" << signalInstanceId << "\"." << endl;
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    signalExists = false;
                    if (isVerbose) cout << "WARN  : The requested signal-instanceId \"" << signalInstanceId << "\" is currently not available." << endl;
                }
            }
            return signalExists;
        }


        void SignalSlotable::slotConnect(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType) {
            boost::mutex::scoped_lock lock(m_connectMutex);
            SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
            if (it != m_signalInstances.end()) {
                it->second->registerSlot(slotInstanceId, slotFunction);
                if (connectionType != NO_TRACK) {
                    // Equip own heartbeat with slot information
                    m_signalInstances["signalHeartbeat"]->registerSlot(slotInstanceId, "slotHeartbeat");
                    //trackExistenceOfConnection(m_instanceId, signalFunction, slotInstanceId, slotFunction, connectionType);
                }
                //cout << "LOW-LEVEL-DEBUG: Established remote connection of signal \"" << signalFunction << "\" to slot \"" << slotFunction << "\"." << endl;
                reply(true);
            } else {
                reply(false);
            }
        }


        bool SignalSlotable::tryToFindSlot(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType, const bool isVerbose) {

            if (slotInstanceId == "*") return true; // GLOBAL slots may or may not exist

            bool slotExists = false;

            if (slotInstanceId == m_instanceId) { // Local slot requested
                boost::mutex::scoped_lock lock(m_connectMutex);
                SlotInstancesConstIt it = m_slotInstances.find(slotFunction);
                if (it != m_slotInstances.end()) { // Slot found
                    slotExists = true;
                    if (connectionType != NO_TRACK) {
                        // Equip own heartbeat with slot information
                        m_signalInstances["signalHeartbeat"]->registerSlot(signalInstanceId, "slotHeartbeat");
                        registerConnectionForTracking(signalInstanceId, signalFunction, m_instanceId, slotFunction, connectionType);
                    }
                } else {
                    slotExists = false;
                    if (isVerbose) cout << "WARN  : The requested slot \"" << slotFunction << "\" is currently not available on this (local) instance \"" << m_instanceId << "\"." << endl;
                }
            } else {// Remote slot requested
                try {
                    startRequest().call(slotInstanceId, "slotHasSlot", signalInstanceId, signalFunction, slotFunction, connectionType).timeout(100).receive(slotExists);
                    if (!slotExists) {
                        if (isVerbose) cout << "WARN  : The requested slot \"" << slotFunction << "\" is currently not available on slot-instanceId \"" << slotInstanceId << "\"." << endl;
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    slotExists = false;
                    if (isVerbose) cout << "WARN  : The requested slot-instanceId \"" << slotInstanceId << "\" is currently not available." << endl;
                }
            }
            return slotExists;
        }


        void SignalSlotable::slotHasSlot(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotFunction, const int& connectionType) {
            if (m_slotInstances.find(slotFunction) != m_slotInstances.end()) {
                if (connectionType != NO_TRACK) {
                    m_signalInstances["signalHeartbeat"]->registerSlot(signalInstanceId, "slotHeartbeat");
                    registerConnectionForTracking(signalInstanceId, signalFunction, m_instanceId, slotFunction, connectionType);
                }
                reply(true);
            } else {
                reply(false);
            }
        }


        bool SignalSlotable::connect(const std::string& signal, const std::string& slot, ConnectionType connectionType, const bool isVerbose) {
            std::pair<std::string, std::string> signalPair = splitIntoInstanceIdAndFunctionName(signal);
            std::pair<std::string, std::string> slotPair = splitIntoInstanceIdAndFunctionName(slot);
            return connect(signalPair.first, signalPair.second, slotPair.first, slotPair.second, connectionType, isVerbose);
        }


        void SignalSlotable::connectN(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string & slotSignature) {
            connect(signalInstanceId, signalSignature, slotInstanceId, slotSignature, NO_TRACK);
        }


        void SignalSlotable::connectT(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string & slotSignature) {
            connect(signalInstanceId, signalSignature, slotInstanceId, slotSignature, TRACK);
        }


        void SignalSlotable::connectR(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string & slotSignature) {
            connect(signalInstanceId, signalSignature, slotInstanceId, slotSignature, RECONNECT);
        }


        void SignalSlotable::connectN(const std::string& signalId, const std::string & slotId) {
            connect(signalId, slotId, NO_TRACK);
        }


        void SignalSlotable::connectT(const std::string& signalId, const std::string & slotId) {
            connect(signalId, slotId, TRACK);
        }


        void SignalSlotable::connectR(const std::string& signalId, const std::string & slotId) {
            connect(signalId, slotId, RECONNECT);
        }


        void SignalSlotable::registerConnectionForTracking(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType) {

            // Signal or slot is remote
            if (signalInstanceId != m_instanceId || slotInstanceId != m_instanceId) {

                string connectionId(signalInstanceId + signalFunction + slotInstanceId + slotFunction);

                if (!isConnectionTracked(connectionId)) {

                    Hash connection;
                    connection.set("signalInstanceId", signalInstanceId);
                    connection.set("signalFunction", signalFunction);
                    connection.set("slotInstanceId", slotInstanceId);
                    connection.set("slotFunction", slotFunction);
                    connection.set("connectionType", connectionType);
                    connection.set("connectionId", connectionId);

                    // Signal is remote
                    if (signalInstanceId != m_instanceId) {
                        boost::mutex::scoped_lock lock(m_heartbeatMutex);
                        if (m_trackedComponents.has(signalInstanceId)) {
                            m_trackedComponents.get<vector<Hash> >(signalInstanceId + ".connections").push_back(connection);
                        }
                    }

                    // Slot is on remote component
                    if (slotInstanceId != m_instanceId) {
                        boost::mutex::scoped_lock lock(m_heartbeatMutex);
                        if (m_trackedComponents.has(slotInstanceId)) {
                            m_trackedComponents.get<vector<Hash> >(slotInstanceId + ".connections").push_back(connection);
                        }
                    }
                }
            }
        }


        bool SignalSlotable::isConnectionTracked(const std::string & connectionId) {

            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                const vector<Hash>& connections = it->getValue<Hash>().get<vector<Hash> >("connections");
                for (size_t j = 0; j < connections.size(); ++j) {
                    const Hash& connection = connections[j];
                    if (connection.get<string > ("connectionId") == connectionId) return true;
                }
            }
            return false;
        }


        void SignalSlotable::addTrackedComponent(const std::string & instanceId, const karabo::util::Hash& instanceInfo) {
            Hash h;
            h.set("connections", vector<Hash > ());
            h.set("instanceInfo", instanceInfo);
            // Initialize the countdown with the heartbeat interval of the remote signal
            h.set("countdown", instanceInfo.get<int>("heartbeatInterval"));
            h.set("isExplicitlyTracked", false);
            m_trackedComponents.set(instanceId, h);
        }


        bool SignalSlotable::disconnect(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const bool isVerbose) {

            if (signalInstanceId.empty()) signalInstanceId = m_instanceId;
            if (slotInstanceId.empty()) slotInstanceId = m_instanceId;

            bool signalExists = tryToDisconnectFromSignal(signalInstanceId, signalFunction, slotInstanceId, slotFunction, isVerbose);
            //bool slotExists = tryToFindSlot(slotInstanceId, slotFunction, isVerbose);

            bool connectionDestroyed = false;
            if (signalExists) {
                emit("signalDisconnected", signalInstanceId, signalFunction, slotInstanceId, slotFunction);
                connectionDestroyed = true;
                if (isVerbose) cout << "INFO  : Connection successfully released." << endl;
            } else {
                if (isVerbose) cout << "ERROR : Connection could not be released (signal was not found)." << endl;
            }

            string connectionId(signalInstanceId + signalFunction + slotInstanceId + slotFunction);
            call("*", "slotStopTrackingExistenceOfConnection", connectionId);

            return connectionDestroyed;
        }


        bool SignalSlotable::tryToDisconnectFromSignal(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, const bool isVerbose) {

            bool signalExists = false;

            if (signalInstanceId == m_instanceId) { // Local signal requested
                boost::mutex::scoped_lock lock(m_connectMutex);
                SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
                if (it != m_signalInstances.end()) { // Signal found
                    signalExists = true;
                    // Unregister slotId from local signal
                    it->second->unregisterSlot(slotInstanceId, slotFunction);
                } else {
                    signalExists = false;
                    if (isVerbose) cout << "WARN  : The requested signal \"" << signalFunction << "\" is currently not available on this (local) instance \"" << m_instanceId << "\"." << endl;
                }
            } else { // Remote signal requested
                try {
                    startRequest().call(signalInstanceId, "slotDisconnect", signalFunction, slotInstanceId, slotFunction).timeout(100).receive(signalExists);
                    if (!signalExists) {
                        if (isVerbose) cout << "WARN  : The requested signal \"" << signalFunction << "\" is currently not available on signal-instanceId \"" << signalInstanceId << "\"." << endl;
                    }
                } catch (const karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    signalExists = false;
                    if (isVerbose) cout << "WARN  : The requested signal-instanceId \"" << signalInstanceId << "\" is currently not available." << endl;
                }
            }

            return signalExists;
        }


        void SignalSlotable::slotDisconnect(const std::string& signalFunction, const std::string& slotInstanceId, const std::string & slotFunction) {
            boost::mutex::scoped_lock lock(m_connectMutex);
            SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
            if (it != m_signalInstances.end()) {
                it->second->unregisterSlot(slotInstanceId, slotFunction);
                reply(true);
            } else {
                reply(false);
            }
        }


        void SignalSlotable::slotStopTrackingExistenceOfConnection(const std::string & connectionId) {

            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                vector<Hash>& connections = it->getValue<Hash>().get<vector<Hash> >("connections");
                for (vector<Hash>::iterator jt = connections.begin(); jt != connections.end(); ++jt) {
                    const Hash& connection = *jt;
                    if (connection.get<string > ("connectionId") == connectionId) {
                        connections.erase(jt);
                        break;
                    }
                }
            }
        }


        void SignalSlotable::connectionNotAvailable(const std::string& instanceId, const std::vector<karabo::util::Hash>& connections) {
            KARABO_LOG_FRAMEWORK_WARN << m_instanceId << " says: Instance \"" << instanceId << "\" is not available";
            KARABO_LOG_FRAMEWORK_DEBUG << "The following connection will thus not work:";
            for (size_t i = 0; i < connections.size(); ++i) {
                const Hash& connection = connections[i];
                const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                const string& signalFunction = connection.get<string > ("signalFunction");
                const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                const string& slotFunction = connection.get<string > ("slotFunction");
                KARABO_LOG_FRAMEWORK_DEBUG << "\"" << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")";
            }
        }


        void SignalSlotable::connectionAvailableAgain(const std::string& instanceId, const std::vector<karabo::util::Hash>& connections) {
            KARABO_LOG_FRAMEWORK_INFO << "Previously unavailable instance \"" << instanceId << "\" is now available";
            KARABO_LOG_FRAMEWORK_DEBUG << "The following connections are (re-)established: ";
            for (size_t i = 0; i < connections.size(); ++i) {
                const Hash& connection = connections[i];
                const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                const string& signalFunction = connection.get<string > ("signalFunction");
                const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                const string& slotFunction = connection.get<string > ("slotFunction");
                KARABO_LOG_FRAMEWORK_DEBUG << "\"" << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")";
            }
        }


        string SignalSlotable::fetchInstanceId(const std::string & signalOrSlotId) const {
            return signalOrSlotId.substr(0, signalOrSlotId.find_last_of('/'));
        }


        std::pair<std::string, std::string> SignalSlotable::splitIntoInstanceIdAndFunctionName(const std::string& signalOrSlotId, const char sep) const {
            size_t pos = signalOrSlotId.find_last_of(sep);
            if (pos == std::string::npos) return std::make_pair("", signalOrSlotId);
            std::string instanceId = signalOrSlotId.substr(0, pos);
            std::string functionName = signalOrSlotId.substr(pos);
            return std::make_pair(instanceId, functionName);
        }


        void SignalSlotable::refreshTimeToLiveForConnectedSlot(const std::string& instanceId, int countdown, const karabo::util::Hash & instanceInfo) {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            if (m_trackedComponents.has(instanceId)) {
                Hash& entry = m_trackedComponents.get<Hash > (instanceId);
                entry.set("instanceInfo", instanceInfo);
                int& oldCountDown = entry.get<int >("countdown");
                if (oldCountDown <= 0) {
                    if (entry.get<bool>("isExplicitlyTracked") == true) {
                        m_heartbeatMutex.unlock();
                        // Polymorphic call DEPRECATE
                        //instanceAvailableAgain(instanceId);
                        // Regular callback
                        if (m_instanceAvailableAgainHandler) m_instanceAvailableAgainHandler(instanceId, instanceInfo);
                        m_heartbeatMutex.lock();
                    }
                    const vector<Hash >& connections = entry.get<vector<Hash> > ("connections");
                    for (size_t i = 0; i < connections.size(); ++i) {
                        const Hash& connection = connections[i];
                        if (connection.get<int>("connectionType") == RECONNECT) {
                            // Active re-connection requests are only needed for remote signals
                            const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                            if (signalInstanceId != m_instanceId) {
                                const string& signalFunction = connection.get<string > ("signalFunction");
                                const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                                const string& slotFunction = connection.get<string > ("slotFunction");
                                m_heartbeatMutex.unlock();
                                //cout << "LOW-LEVEL-DEBUG: Trying to reconnect: " << jt->first << " <-> " << kt->first << endl;
                                bool success = connect(signalInstanceId, signalFunction, slotInstanceId, slotFunction, NO_TRACK, false);
                                m_heartbeatMutex.lock();
                                if (!success) {
                                    // TODO remove from connections
                                    //cout << "Re-appeared instance \"" << signalInstanceId << "\" does not carry signalFunction \"" << signalFunction << "\" anymore. Gave up on trying to re-connect." << endl;
                                }
                            }
                        }
                    }
                    if (!connections.empty()) connectionAvailableAgain(instanceId, connections);
                }
                //cout << "LOW-LEVEL-DEBUG: Countdown refresh for " << instanceId << ": old = " << oldCountDown << " new = " << countdown << endl;
                oldCountDown = countdown;
            } else {
                //cout << "LOW-LEVEL-DEBUG: Got refresh request for unregistered heartbeat of " << instanceId << endl;
            }
        }


        void SignalSlotable::letConnectionSlowlyDieWithoutHeartbeat() {

            try {

                while (m_doTracking) {

                    m_heartbeatMutex.lock();

                    for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                        Hash& entry = it->getValue<Hash>();

                        int& countdown = entry.get<int>("countdown");

                        if (countdown > 0) countdown--; // Regular count down

                        else if (countdown == 0) { // Connection lost
                            countdown--;
                            if (entry.get<bool>("isExplicitlyTracked") == true) {
                                Hash instanceInfo;
                                if (entry.has("instanceInfo")) entry.get("instanceInfo", instanceInfo);

                                // NEVER EVER CALL-BACK UNDER LOCK!!
                                m_heartbeatMutex.unlock();
                                if (m_instanceNotAvailableHandler) m_instanceNotAvailableHandler(it->getKey(), instanceInfo); // Inform via callback
                                m_heartbeatMutex.lock();
                            }
                            if (entry.get<vector<Hash> >("connections").size() > 0) {

                                m_heartbeatMutex.unlock();
                                connectionLost(it->getKey(), entry.get<vector<Hash> >("connections")); // This cleans signals, removes dead connections tracking and informs the user
                                m_heartbeatMutex.lock();

                                if (entry.get<vector<Hash> >("connections").empty()) m_trackedComponents.erase(it->getKey());
                            }

                        } else { // Negative count for tracked connections
                            countdown--;
                            // Here we can try to reconnect
                            if (m_reconnectIntervals.find(countdown) != m_reconnectIntervals.end()) {
                                const vector<Hash >& connections = entry.get<vector<Hash> > ("connections");
                                for (size_t i = 0; i < connections.size(); ++i) {
                                    const Hash& connection = connections[i];
                                    if (connection.get<int>("connectionType") == RECONNECT) {
                                        m_heartbeatMutex.unlock();
                                        // If that worked will trigger refreshTimeToLiveForConnectedSlot that re-connects the rest
                                        connect(it->getKey(), "signalHeartbeat", "", "slotHeartbeat", NO_TRACK, false);
                                        m_heartbeatMutex.lock();
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    m_heartbeatMutex.unlock();

                    // We are sleeping twice as long as the count-down ticks (which ticks in seconds)
                    boost::this_thread::sleep(boost::posix_time::seconds(2));
                }

            } catch (const Exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "letConnectionSlowlyDieWithoutHeartbeat triggered an exception: " << e;
            } catch (...) {
                KARABO_LOG_FRAMEWORK_ERROR << "letConnectionSlowlyDieWithoutHeartbeat triggered an unknown exception";
            }

        }


        void SignalSlotable::connectionLost(const std::string& instanceId, std::vector<karabo::util::Hash>& connections) {
            //std::cout << "Instance \"" << instanceId << "\" is not available, the following connection will thus not work: " << std::endl;
            std::vector<Hash> deadConnections;
            for (size_t i = 0; i < connections.size(); ++i) {
                const Hash& connection = connections[i];
                const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                const string& signalFunction = connection.get<string > ("signalFunction");
                const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                const string& slotFunction = connection.get<string > ("slotFunction");
                int connectionType = connection.get<int>("connectionType");
                //cout << "\"" << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")" << endl;

                if (connectionType == NO_TRACK) { // Everything that does not track is deleted

                    if (signalInstanceId == m_instanceId) {
                        // This connection did not make it, will be deleted and signals cleared from dead slots
                        KARABO_LOG_FRAMEWORK_DEBUG << "Disconnecting as result of silent death: " << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")";
                        tryToDisconnectFromSignal(signalInstanceId, signalFunction, slotInstanceId, slotFunction, false);
                    }

                } else { // Tracked connections are kept
                    // This connection survived
                    deadConnections.push_back(connection);
                }
            }
            // Update the connection list
            connections = deadConnections;

            // Polymorphic callback for the user to react DEPRECATE
            connectionNotAvailable(instanceId, deadConnections);

        }


        Hash SignalSlotable::prepareConnectionNotAvailableInformation(const karabo::util::Hash & hash) const {
            Hash result;
            for (Hash::const_iterator jt = hash.begin(); jt != hash.end(); ++jt) {
                const AssocType& associates = jt->getValue<AssocType>();
                vector<string > tmp;
                tmp.reserve(associates.size());
                for (AssocTypeConstIterator kt = associates.begin(); kt != associates.end(); ++kt) {
                    tmp.push_back(kt->first);
                }
                result.set(jt->getKey(), tmp);
            }
            return result;
        }


        void SignalSlotable::slotTryReconnectNow() {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                Hash& entry = m_trackedComponents.get<Hash > (it->getKey());
                const int& countdown = entry.get<int>("countdown");
                if (countdown < 0) {
                    const Hash& signals = entry.get<Hash > ("signals");
                    for (Hash::const_iterator jt = signals.begin(); jt != signals.end(); ++jt) {
                        const AssocType& slots = jt->getValue<AssocType>();
                        for (AssocTypeConstIterator kt = slots.begin(); kt != slots.end(); ++kt) {
                            if (kt->second == RECONNECT) {
                                m_heartbeatMutex.unlock();
                                connect(jt->getKey(), kt->first, RECONNECT);
                                m_heartbeatMutex.lock();
                            }
                        }
                    }
                    const Hash& slots = entry.get<Hash > ("slots");
                    for (Hash::const_iterator jt = slots.begin(); jt != slots.end(); ++jt) {
                        const AssocType& signals = jt->getValue<AssocType>();
                        for (AssocTypeConstIterator kt = signals.begin(); kt != signals.end(); ++kt) {
                            if (kt->second == RECONNECT) {
                                m_heartbeatMutex.unlock();
                                connect(kt->first, jt->getKey(), RECONNECT);
                                m_heartbeatMutex.lock();
                            }
                        }
                    }
                }
            }
        }


        std::pair<bool, karabo::util::Hash > SignalSlotable::digestPotentialReply() {
            boost::mutex::scoped_lock lock(m_replyMutex);
            Replies::iterator it = m_replies.find(boost::this_thread::get_id());
            std::pair<bool, karabo::util::Hash> ret;
            if (it != m_replies.end()) {
                ret = std::make_pair(true, it->second);
                m_replies.erase(it);
            } else {
                ret = std::make_pair(false, karabo::util::Hash());
            }
            return ret;
        }


        void SignalSlotable::connectInputChannels() {
            // Loop channels
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end(); ++it) {
                AbstractInput::Pointer channel = it->second;
                if (channel->needsDeviceConnection()) {
                    // Loop connected outputs
                    std::vector<karabo::util::Hash> outputChannels = channel->getConnectedOutputChannels();
                    for (size_t j = 0; j < outputChannels.size(); ++j) {
                        const std::string& instanceId = outputChannels[j].get<string > ("instanceId");
                        const std::string& channelId = outputChannels[j].get<string > ("channelId");
                        bool channelExists = false;
                        karabo::util::Hash reply;
                        int sleep = 1;
                        int trials = 8;
                        while ((trials--) > 0) {
                            try {
                                this->request(instanceId, "slotGetOutputChannelInformation", channelId, static_cast<int> (getpid())).timeout(1000).receive(channelExists, reply);
                            } catch (karabo::util::TimeoutException&) {
                                karabo::util::Exception::clearTrace();
                                std::cout << "Could not find instanceId \"" + instanceId + "\" for IO connection" << std::endl;
                                std::cout << "Trying again in " << sleep << " seconds." << std::endl;
                                boost::this_thread::sleep(boost::posix_time::seconds(sleep));
                                sleep += 2;
                                continue;
                            }
                            break;
                        }
                        if (channelExists) {
                            channel->connect(reply); // Synchronous
                        } else {
                            throw KARABO_IO_EXCEPTION("Could not find outputChannel \"" + channelId + "\" on instanceId \"" + instanceId + "\"");
                        }
                    }
                }
            }
        }


        karabo::util::Hash SignalSlotable::slotGetOutputChannelInformation(const std::string& ioChannelId, const int& processId) {
            OutputChannels::const_iterator it = m_outputChannels.find(ioChannelId);
            if (it != m_outputChannels.end()) {
                karabo::util::Hash h(it->second->getInformation());
                if (processId == static_cast<int> (getpid())) {
                    h.set("memoryLocation", "local");
                } else {
                    h.set("memoryLocation", "remote");
                }
                reply(true, h);
                return h;
            } else {
                reply(false, karabo::util::Hash());
                return karabo::util::Hash();
            }
        }


        int SignalSlotable::godEncode(const std::string & password) {
            unsigned int code = 0;
            for (size_t i = 0; i < password.size() - 1; ++i) {
                unsigned int asciiValue = static_cast<int> (password[i]) * (i + 1);
                code += asciiValue;
            }
            size_t lastIdx = password.size() - 1;
            unsigned int asciiValue = static_cast<int> (password[lastIdx]) * (lastIdx + 1);
            code += asciiValue;
            code /= password.size();
            return code;
        }


        int SignalSlotable::getAccessLevel(const std::string & deviceId) const {
            boost::mutex::scoped_lock lock(m_accessLevelMutex);
            boost::optional<const Hash::Node&> node = m_accessList.find(deviceId);
            if (node) return node->getValue<int>();
            else return m_defaultAccessLevel;
        }
    } // namespace xms
} // namespace karabo
