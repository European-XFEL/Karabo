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

#include "SignalSlotable.hh"


namespace karabo {
    namespace xms {

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::net;

        // Static initializations
        std::set<int> SignalSlotable::m_reconnectIntervals = std::set<int>();


        SignalSlotable::SignalSlotable() {
        }


        SignalSlotable::SignalSlotable(const BrokerConnection::Pointer& connection, const string& instanceId, int heartbeatRate) {
            init(connection, instanceId, heartbeatRate);
        }


        SignalSlotable::~SignalSlotable() {
        }


        void SignalSlotable::init(const karabo::net::BrokerConnection::Pointer& connection, const std::string& instanceId, int heartbeatRate) {

            m_connection = connection;
            m_instanceId = instanceId;
            m_timeToLive = heartbeatRate;

            // Create the managing ioService object
            m_ioService = m_connection->getIOService();
            // Start connection (and take the default channel for signals)
            m_signalChannel = m_connection->start();
            // Create request channel
            m_requestChannel = m_connection->createChannel();

            initReconnectIntervals();
            registerDefaultSignalsAndSlots();
            startTrackingSystem();
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

            // TODO See whether we can do this later
            //m_signalChannel = m_connection->start();

            /**
             * Emits a "still-alive" signal
             * @param networkId
             * @param timeToLive
             */
            SIGNAL2("signalHeartbeat", string, int)

            /**
             * Signals a successful connection
             * @param signalInstanceId
             * @param signalFunction
             * @param slotInstanceId
             * @param slotFunction
             */
            SIGNAL4("signalConnected", string, string, string, string)

            /**
             * Signals a successful disconnection
             * @param signalInstanceId
             * @param signalFunction
             * @param slotInstanceId
             * @param slotFunction
             */
            SIGNAL4("signalDisconnected", string, string, string, string)

            /**
             * Emits as answer to a ping request
             * @param networkId
             */
            SIGNAL1("signalGotPinged", string)

            /**
             * Listener for heartbeats
             * @param networkId
             * @param timeToLive
             */
            SLOT2(slotHeartbeat, string, int)

            // Register networkId invariant ping slot
            GLOBAL_SLOT2(slotPing, string, bool) // instanceId of caller, only reply if same instance id

            SLOT2(slotPingAnswer, string, Hash) // instanceId, instanceInfo

            /**
             * Connects signal to slot
             * @param signalFunction
             * @param slotInstanceId
             * @param slotFunction
             */
            SLOT4(slotConnect, string, string, string, int) // slotInstanceId, slotFunction, signalFunction, connectionType

            SLOT4(slotHasSlot, string, string, string, int) // signalInstanceId, signalFunction, slotFunction, connectionType

            /**
             * Disconnects signal from slot
             * @param signalFunction
             * @param slotInstanceId
             * @param slotFunction
             */
            SLOT3(slotDisconnect, string, string, string)

            GLOBAL_SLOT1(slotStopTrackingExistenceOfConnection, string)

            SLOT1(slotGetAvailableFunctions, string) // what type of function

            //GLOBAL_SLOT0(slotTryReconnectNow)
            SLOT2(slotGetOutputChannelInformation, string, int)

        }


        void SignalSlotable::runEventLoop(bool emitHeartbeat, const karabo::util::Hash& instanceInfo) {

            updateInstanceInfo(instanceInfo);

            if (emitHeartbeat) {
                m_sendHeartbeats = true;
                // Send heartbeat and sleep for m_timeToLive seconds
                boost::thread heartbeatThread(boost::bind(&karabo::xms::SignalSlotable::emitHeartbeat, this));
                m_ioService->work(); // blocks
                m_sendHeartbeats = false;
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
            call("*", "slotInstanceGone", m_instanceId);
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
            while (m_sendHeartbeats) {
                emit("signalHeartbeat", getInstanceId(), m_timeToLive);
                boost::this_thread::sleep(boost::posix_time::seconds(m_timeToLive));
            }
        }


        const std::vector<std::pair<std::string, karabo::util::Hash> >& SignalSlotable::getAvailableInstances() {
            m_availableInstances.clear();
            call("*", "slotPing", m_instanceId, false);
            // The function slotPingAnswer will be called by all instances available now
            // Lets wait a fair amount of time - huaaah this is bad isn't it :-(
            boost::this_thread::sleep(boost::posix_time::milliseconds(500));
            return m_availableInstances;
        }


        void SignalSlotable::slotPing(const std::string& instanceId, const bool& replyIfInstanceIdIsDuplicated) {
            if (replyIfInstanceIdIsDuplicated) {
                if (instanceId == m_instanceId) {
                    reply(boost::asio::ip::host_name());
                }
            } else {

                call(instanceId, "slotPingAnswer", m_instanceId, m_instanceInfo);
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
                    // Filter out service slots
                    if (function == "slotConnect" || function == "slotDisconnect" || function == "slotGetAvailableFunctions" ||
                            function == "slotHasSlot" || function == "slotHeartbeat" || function == "slotPing" || function == "slotPingAnswer" ||
                            function == "slotRefresh" || function == "slotRefreshAll" || function == "slotStopTrackingExistenceOfConnection") {
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


        void SignalSlotable::slotHeartbeat(const std::string& networkId, const int& timeToLive) {
            refreshTimeToLiveForConnectedSlot(networkId, timeToLive);
        }

        //        void SignalSlotable::setInstanceId(const std::string& instanceId) {
        //            m_instanceId = instanceId;
        //        }


        const std::string& SignalSlotable::getInstanceId() const {
            return m_instanceId;
        }


        void SignalSlotable::updateInstanceInfo(const karabo::util::Hash& update) {
            m_instanceInfo.merge(update);
            call("*", "slotInstanceUpdated", m_instanceId, m_instanceInfo);
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


        void SignalSlotable::trackExistenceOfInstance(const std::string& instanceId) {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            if (!m_trackedComponents.has(instanceId)) {
                addTrackedComponent(instanceId);
            }
            m_trackedComponents.set(instanceId + ".isExplicitlyTracked", true);
            connect(instanceId, "signalHeartbeat", "", "slotHeartbeat", NO_TRACK, false);
        }


        bool SignalSlotable::connect(std::string signalInstanceId, const std::string& signalFunction, std::string slotInstanceId, const std::string& slotFunction, ConnectionType connectionType, const bool isVerbose) {

            if (signalInstanceId.empty()) signalInstanceId = m_instanceId;
            if (slotInstanceId.empty()) slotInstanceId = m_instanceId;

            bool signalExists = tryToConnectToSignal(signalInstanceId, signalFunction, slotInstanceId, slotFunction, connectionType, isVerbose);
            bool slotExists = tryToFindSlot(signalInstanceId, signalFunction, slotInstanceId, slotFunction, connectionType, isVerbose);

            bool connectionEstablished = false;
            if (signalExists && slotExists) {
                if (slotInstanceId != "*") emit("signalConnected", signalInstanceId, signalFunction, slotInstanceId, slotFunction);
                connectionEstablished = true;
                if (isVerbose) cout << "INFO  : Connection successfully established." << endl;
            } else if (signalExists) {
                if (isVerbose) cout << "WARN  : Connection not yet established, but will automatically be if slot appears." << endl;
            } else {
                if (isVerbose) cout << "ERROR : Connection could not be established." << endl;
            }

            if (connectionEstablished && connectionType != NO_TRACK) {
                //trackExistenceOfConnection(signalInstanceId, signalFunction, slotInstanceId, slotFunction, connectionType);
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
                        trackExistenceOfConnection(m_instanceId, signalFunction, slotInstanceId, slotFunction, connectionType);
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
                    trackExistenceOfConnection(m_instanceId, signalFunction, slotInstanceId, slotFunction, connectionType);
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
                        trackExistenceOfConnection(signalInstanceId, signalFunction, m_instanceId, slotFunction, connectionType);
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
                    trackExistenceOfConnection(signalInstanceId, signalFunction, m_instanceId, slotFunction, connectionType);
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


        void SignalSlotable::connectN(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string& slotSignature) {
            connect(signalInstanceId, signalSignature, slotInstanceId, slotSignature, NO_TRACK);
        }


        void SignalSlotable::connectT(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string& slotSignature) {
            connect(signalInstanceId, signalSignature, slotInstanceId, slotSignature, TRACK);
        }


        void SignalSlotable::connectR(const std::string& signalInstanceId, const std::string& signalSignature, const std::string& slotInstanceId, const std::string& slotSignature) {
            connect(signalInstanceId, signalSignature, slotInstanceId, slotSignature, RECONNECT);
        }


        void SignalSlotable::connectN(const std::string& signalId, const std::string& slotId) {
            connect(signalId, slotId, NO_TRACK);
        }


        void SignalSlotable::connectT(const std::string& signalId, const std::string& slotId) {
            connect(signalId, slotId, TRACK);
        }


        void SignalSlotable::connectR(const std::string& signalId, const std::string& slotId) {
            connect(signalId, slotId, RECONNECT);
        }


        void SignalSlotable::trackExistenceOfConnection(const std::string& signalInstanceId, const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction, const int& connectionType) {

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
                        if (!m_trackedComponents.has(signalInstanceId)) addTrackedComponent(signalInstanceId);
                        m_trackedComponents.get<vector<Hash> >(signalInstanceId + ".connections").push_back(connection);
                        // Connect remote signalHeartbeat to local slotHeartbeat
                        //connect(signalInstanceId, "signalHeartbeat", "", "slotHeartbeat", NO_TRACK, false);
                    }

                    // Slot is on remote component
                    if (slotInstanceId != m_instanceId) {
                        boost::mutex::scoped_lock lock(m_heartbeatMutex);
                        if (!m_trackedComponents.has(slotInstanceId)) addTrackedComponent(slotInstanceId);
                        m_trackedComponents.get<vector<Hash> >(slotInstanceId + ".connections").push_back(connection);
                        // Connect remote signalHeartbeat to local slotHeartbeat
                        //connect(slotInstanceId, "signalHeartbeat", "", "slotHeartbeat", NO_TRACK, false);
                    }
                }
            }
        }


        bool SignalSlotable::isConnectionTracked(const std::string& connectionId) {

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


        void SignalSlotable::addTrackedComponent(const std::string& instanceId) {
            Hash h;
            h.set("connections", vector<Hash > ());
            h.set("countDown", 4);
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
                    // Register new slotId to local signal
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


        void SignalSlotable::slotDisconnect(const std::string& signalFunction, const std::string& slotInstanceId, const std::string& slotFunction) {
            boost::mutex::scoped_lock lock(m_connectMutex);
            SignalInstancesConstIt it = m_signalInstances.find(signalFunction);
            if (it != m_signalInstances.end()) {
                it->second->unregisterSlot(slotInstanceId, slotFunction);
                reply(true);
            } else {
                reply(false);
            }
        }


        void SignalSlotable::slotStopTrackingExistenceOfConnection(const std::string& connectionId) {

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
            std::cout << m_instanceId << "says: Instance \"" << instanceId << "\" is not available, the following connection will thus not work: " << std::endl;
            for (size_t i = 0; i < connections.size(); ++i) {
                const Hash& connection = connections[i];
                const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                const string& signalFunction = connection.get<string > ("signalFunction");
                const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                const string& slotFunction = connection.get<string > ("slotFunction");
                cout << "\"" << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")" << endl;
            }
        }


        void SignalSlotable::connectionAvailableAgain(const std::string& instanceId, const std::vector<karabo::util::Hash>& connections) {
            std::cout << "Previously unavailable instance \"" << instanceId << "\" is now available, the following connections are established: " << std::endl;
            for (size_t i = 0; i < connections.size(); ++i) {
                const Hash& connection = connections[i];
                const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                const string& signalFunction = connection.get<string > ("signalFunction");
                const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                const string& slotFunction = connection.get<string > ("slotFunction");
                cout << "\"" << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")" << endl;
            }
        }


        string SignalSlotable::fetchInstanceId(const std::string& signalOrSlotId) const {
            return signalOrSlotId.substr(0, signalOrSlotId.find_last_of('/'));
        }


        std::pair<std::string, std::string> SignalSlotable::splitIntoInstanceIdAndFunctionName(const std::string& signalOrSlotId, const char sep) const {
            size_t pos = signalOrSlotId.find_last_of(sep);
            if (pos == std::string::npos) return std::make_pair("", signalOrSlotId);
            std::string instanceId = signalOrSlotId.substr(0, pos);
            std::string functionName = signalOrSlotId.substr(pos);
            return std::make_pair(instanceId, functionName);
        }


        void SignalSlotable::refreshTimeToLiveForConnectedSlot(const std::string& instanceId, int countDown) {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            if (m_trackedComponents.has(instanceId)) {
                Hash& entry = m_trackedComponents.get<Hash > (instanceId);
                int& oldCountDown = entry.get<int >("countDown");
                if (oldCountDown <= 0) {
                    if (entry.get<bool>("isExplicitlyTracked") == true) instanceAvailableAgain(instanceId);
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

                                //m_heartbeatMutex.lock();
                            }
                        }
                    }
                    connectionAvailableAgain(instanceId, connections);
                }
                oldCountDown = countDown;
            } else {
                //cout << "LOW-LEVEL-DEBUG: Got refresh request for unregistered heartbeat of " << networkId << endl;
            }
        }


        void SignalSlotable::letConnectionSlowlyDieWithoutHeartbeat() {
            while (m_doTracking) {

                m_heartbeatMutex.lock();

                for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                    Hash& entry = it->getValue<Hash>();
                    int& countDown = entry.get<int>("countDown");
                    if (countDown > 0) countDown--;
                    else if (countDown == 0) {
                        countDown--;
                        if (entry.get<bool>("isExplicitlyTracked") == true) instanceNotAvailable(it->getKey());
                        connectionLost(it->getKey(), entry.get<vector<Hash> >("connections"));
                    } else {
                        countDown--;
                        // Here we can try to reconnect
                        if (m_reconnectIntervals.find(countDown) != m_reconnectIntervals.end()) {
                            const vector<Hash >& connections = entry.get<vector<Hash> > ("connections");
                            for (size_t i = 0; i < connections.size(); ++i) {
                                const Hash& connection = connections[i];
                                if (connection.get<int>("connectionType") == RECONNECT) {
                                    m_heartbeatMutex.unlock();
                                    connect(it->getKey(), "signalHeartbeat", "", "slotHeartbeat", NO_TRACK, false);
                                    m_heartbeatMutex.lock();
                                    break;
                                }
                            }
                        }
                    }
                }

                m_heartbeatMutex.unlock();

                boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
            }
        }


        void SignalSlotable::connectionLost(const std::string& instanceId, const std::vector<karabo::util::Hash>& connections) {
            //std::cout << "Instance \"" << instanceId << "\" is not available, the following connection will thus not work: " << std::endl;
            for (size_t i = 0; i < connections.size(); ++i) {
                const Hash& connection = connections[i];
                const string& signalInstanceId = connection.get<string > ("signalInstanceId");
                const string& signalFunction = connection.get<string > ("signalFunction");
                const string& slotInstanceId = connection.get<string > ("slotInstanceId");
                const string& slotFunction = connection.get<string > ("slotFunction");
                //cout << "\"" << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")" << endl;
                if (signalInstanceId == m_instanceId) {
                    cout << "Disconnecting: " << signalFunction << "\" (" << signalInstanceId << ") \t\t <--> \t\t \"" << slotFunction << "\" (" << slotInstanceId << ")" << endl;
                    tryToDisconnectFromSignal(signalInstanceId, signalFunction, slotInstanceId, slotFunction, false);
                }
            }
            connectionNotAvailable(instanceId, connections);
        }


        Hash SignalSlotable::prepareConnectionNotAvailableInformation(const karabo::util::Hash& hash) const {
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
                const int& timeToLive = entry.get<int>("timeToLive");
                if (timeToLive < 0) {
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


        std::pair<bool, karabo::util::Hash> SignalSlotable::digestPotentialReply() {
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
                        bool channelExists;
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
                            channel->connectNow(reply); // Synchronous
                        } else {
                            throw KARABO_IO_EXCEPTION("Could not find outputChannel \"" + channelId + "\" on instanceId \"" + instanceId + "\"");
                        }
                    }
                }
            }
        }


        void SignalSlotable::slotGetOutputChannelInformation(const std::string& ioChannelId, const int& processId) {
            OutputChannels::const_iterator it = m_outputChannels.find(ioChannelId);
            if (it != m_outputChannels.end()) {
                karabo::util::Hash h(it->second->getInformation());
                if (processId == static_cast<int> (getpid())) {
                    h.set("memoryLocation", "local");
                } else {
                    h.set("memoryLocation", "remote");
                }
                reply(true, h);
            } else {
                reply(false, karabo::util::Hash());
            }
        }


    } // namespace xms
} // namespace karabo
