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
#include "SignalSlotable.hh"

namespace exfel {
    namespace core {

        using namespace std;
        using namespace exfel::util;
        using namespace exfel::net;

        string SignalSlotable::m_hostId = boost::asio::ip::host_name();
        std::set<int> SignalSlotable::m_reconnectIntervals = std::set<int>();


        SignalSlotable::SignalSlotable() : m_instanceId(getClassInfo().getClassId()) {
        }


        SignalSlotable::SignalSlotable(const BrokerConnection::Pointer& connection, const string& instanceId, int heartbeatRate)
        : m_instanceId(instanceId), m_connection(connection), m_timeToLive(heartbeatRate) {
            initReconnectIntervals();
            registerDefaultSignalsAndSlots();
            startTrackingSystem();
        }


        void SignalSlotable::expectedParameters(exfel::util::Config& expected) {

            CHOICE_ELEMENT<BrokerConnection > (expected).key("connection")
                    .displayedName("BrokerConnection")
                    .description("The connection to the communication layer")
                    .assignmentOptional().defaultValue("Jms")
                    .init()
                    .commit();

            INT32_ELEMENT(expected).key("heartbeatRate")
                    .displayedName("heartbeatRate")
                    .description("Heartbeats will be send at the configured rate (e.g. 10 = a heartbeat every 10th second) [s]")
                    .minInc(5)
                    .assignmentOptional().defaultValue(5)
                    .advanced()
                    .commit();
        }


        void SignalSlotable::configure(const exfel::util::Config& input) {
            try {
                m_connection = BrokerConnection::createChoice("connection", input);
            } catch (...) {
                if (input.get<Config > ("connection").has("Jms")) {
                    if (input.getFromPath<string > ("connection.Jms.hostname") != "localhost") {
                        cout << "    \"Trying to use local broker (if available)\"" << endl;
                        Config modifiedInput = input;
                        modifiedInput.setFromPath("connection.Jms.hostname", "localhost");
                        m_connection = BrokerConnection::createChoice("connection", modifiedInput);
                    }
                }
            }
            input.get("heartbeatRate", m_timeToLive);
            initReconnectIntervals();
            registerDefaultSignalsAndSlots();
            startTrackingSystem();
        }


        void SignalSlotable::initReconnectIntervals() {
            if (m_reconnectIntervals.empty()) {
                for (int i = 0; i <= 50; ++i) {
                    int x = (int) (1 + 0.005 * i * i * i * i);
                    m_reconnectIntervals.insert(-x);
                }
            }
        }


        void SignalSlotable::registerDefaultSignalsAndSlots() {

            m_connection->start();
            m_signalService = m_connection->createService();

            /**
             * Emits a "still-alive" signal
             * @param networkId
             * @param timeToLive
             */
            SIGNAL2("signalHeartbeat", string, int)

            /**
             * Listener for heartbeats
             * @param networkId
             * @param timeToLive
             */
            SLOT2(slotHeartbeat, string, int)

            /**
             * Emits a global ping request
             */
            SIGNAL0("signalPing");

            /**
             * Emits a connect request
             * @param signalId
             * @param slotId
             */
            SIGNAL2("signalConnect", string, string)

            /**
             * Signals a successful connection
             * @param signalId
             * @param slotId
             */
            SIGNAL2("signalConnected", string, string)

            /**
             * Emits a disconnect request
             * @param signalId
             * @param slotId
             */
            SIGNAL2("signalDisconnect", string, string)

            /**
             * Signals a successful disconnection
             * @param signalId
             * @param slotId
             */
            SIGNAL2("signalDisconnected", string, string)

            /**
             * Emits as answer to a ping request
             * @param networkId
             */
            SIGNAL1("signalGotPinged", string)

            // Register networkId invariant ping slot
            GLOBAL_SLOT0(slotPing)

            /**
             * Connects signal to slot
             * @param signalId
             * @param slotId
             */
            GLOBAL_SLOT2(slotConnect, string, string)

            /**
             * Disconnects signal and slot
             * @param signalId
             * @param slotId
             */
            GLOBAL_SLOT2(slotDisconnect, string, string)

            GLOBAL_SLOT0(slotTryReconnectNow)

            SLOT0(slotShowSignalsAndSlots)
            SIGNAL2("signalAvailableSignalsAndSlots", vector<string>, vector<string>)
            SLOT2(slotReceiveSignalsAndSlots, vector<string>, vector<string>)

            // Default connects
            connectN("signalConnect-STRING-STRING", "slotConnect-STRING-STRING");
            connectN("signalPing", "slotPing");
            connectN("signalConnected-STRING-STRING", "slotConnected-STRING-STRING");
            connectN("signalGotPinged-STRING", "slotGotPinged-STRING");
        }


        void SignalSlotable::startTrackingSystem() {
            // Countdown and finally timeout registered heartbeats
            boost::thread(boost::bind(&SignalSlotable::letConnectionSlowlyDieWithoutHeartbeat, this));
        }


        void SignalSlotable::runEventLoop(bool emitHeartbeat) {
            if (emitHeartbeat) {
                // Send heartbeat and sleep for m_pulse seconds
                while (true) {
                    emit("signalHeartbeat", getNetworkId(), m_timeToLive);
                    boost::this_thread::sleep(boost::posix_time::seconds(m_timeToLive));
                }
            } else {
                // Just sleep
                while (true) {
                    boost::this_thread::sleep(boost::posix_time::seconds(m_timeToLive));
                }
            }
        }


        void SignalSlotable::slotPing() {
            cout << getClassInfo().getClassId() << " on " << getHostId() << " got pinged" << endl;
            emit("signalGotPinged", getNetworkId());
        }


        void SignalSlotable::showSignalsAndSlots(const std::string& networkId) {
            SIGNAL0("showSignalsAndSlots")
            connectN("", "showSignalsAndSlots", networkId, "slotShowSignalsAndSlots");
            connectN(networkId, "signalAvailableSignalsAndSlots-VECTOR_STRING-VECTOR_STRING",
                     "", "slotReceiveSignalsAndSlots-VECTOR_STRING-VECTOR_STRING");
            emit("showSignalsAndSlots");
        }


        void SignalSlotable::ping() const {
            emit("signalPing");
        }


        void SignalSlotable::slotShowSignalsAndSlots() {
            std::vector<string> signals(m_signalFunctions.size());
            std::vector<string> slots(m_slotServices.size());
            int i = 0;
            for (Hash::const_iterator it = m_signalFunctions.begin(); it != m_signalFunctions.end(); ++it, ++i) {
                signals[i] = it->first;
                //cout << it->first << endl;
            }
            i = 0;
            for (SlotServicesConstIt it = m_slotServices.begin(); it != m_slotServices.end(); ++it, ++i) {
                slots[i] = it->first;
                //cout << it->first << endl;
            }
            emit("signalAvailableSignalsAndSlots", signals, slots);
        }


        void SignalSlotable::slotReceiveSignalsAndSlots(const std::vector<string>& signals, const std::vector<string>& slots) {
            for (size_t i = 0; i < signals.size(); ++i) {
                cout << signals[i] << endl;
            }
            for (size_t i = 0; i < signals.size(); ++i) {
                cout << slots[i] << endl;
            }
        }


        void SignalSlotable::slotHeartbeat(const std::string& networkId, const int& timeToLive) {
            refreshTimeToLiveForConnectedSlot(networkId, timeToLive);
        }


        const std::string& SignalSlotable::getHostId() const {
            return m_hostId;
        }


        void SignalSlotable::setDefaultHostId(const std::string& defaultHostId) {
            m_hostId = defaultHostId;
        }


        void SignalSlotable::setHostId(const string& hostId) {

            // Change the hostId
            m_hostId = hostId;

            // Re-Register default signal/slot for location independent connection establishment
            SIGNAL2("signalHeartbeat", string, int)
            SIGNAL2("signalConnect", string, string)
            SIGNAL2("signalConnected", string, string)
            SIGNAL1("signalGotPinged", string)
            SLOT2(slotConnect, string, string)
            connectN("signalConnect-STRING-STRING", "slotConnect-STRING-STRING");
            connectN("signalConnected-STRING-STRING", "slotConnected-STRING-STRING");
            connectN("signalGotPinged-STRING", "slotGotPinged-STRING");
            // TODO One may want to stop former onConnects with every call to this function...
        }


        const std::string& SignalSlotable::getInstanceId() const {
            return m_instanceId;
        }


        void SignalSlotable::setInstanceId(const std::string& instanceId) {
            m_instanceId = instanceId;
        }


        std::string SignalSlotable::getNetworkId() const {
            return m_hostId + "/" + m_instanceId;
        }


        void SignalSlotable::autoConnectAllSignals(const exfel::util::Config& config, const std::string signalRegularExpression) {
            try {
                for (Config::const_iterator it = config.begin(); it != config.end(); ++it) {
                    const boost::regex e(signalRegularExpression);
                    if (boost::regex_match(it->first, e)) {
                        const vector<string>& connects = config.get<vector<string> >(it);
                        for (size_t i = 0; i < connects.size(); ++i) {
                            connect(it->first, connects[i], RECONNECT);
                        }
                    }
                }
            } catch (...) {
                RETHROW;
            }
        }


        void SignalSlotable::autoConnectAllSlots(const exfel::util::Config& config, const std::string slotRegularExpression) {
            try {
                for (Config::const_iterator it = config.begin(); it != config.end(); ++it) {
                    const boost::regex e(slotRegularExpression);
                    if (boost::regex_match(it->first, e)) {
                        const vector<string>& connects = config.get<vector<string> >(it);
                        for (size_t i = 0; i < connects.size(); ++i) {
                            cout << "AutoConnect:" << connects[i] << endl;
                            connect(connects[i], it->first, RECONNECT);
                        }
                    }
                }
            } catch (...) {
                RETHROW;
            }
        }


        void SignalSlotable::trackExistenceOfComponent(const std::string& networkId) {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            if (!m_trackedComponents.has(networkId)) {
                addTrackedComponent(networkId);
            }
            m_trackedComponents.setFromPath(networkId + ".isExplicitlyTracked", true);
            connectN(networkId + "/signalHeartbeat-STRING-INT32", "slotHeartbeat-STRING-INT32");
        }


        void SignalSlotable::connectN(const std::string& signalNetworkId, const std::string& signalSignature, const std::string& slotNetworkId, const std::string& slotSignature) {
            connect(signalNetworkId, signalSignature, slotNetworkId, slotSignature, NO_TRACK);
        }


        void SignalSlotable::connectT(const std::string& signalNetworkId, const std::string& signalSignature, const std::string& slotNetworkId, const std::string& slotSignature) {
            connect(signalNetworkId, signalSignature, slotNetworkId, slotSignature, TRACK);
        }


        void SignalSlotable::connectR(const std::string& signalNetworkId, const std::string& signalSignature, const std::string& slotNetworkId, const std::string& slotSignature) {
            connect(signalNetworkId, signalSignature, slotNetworkId, slotSignature, RECONNECT);
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


        void SignalSlotable::connect(const std::string& signal, const std::string& slot, ConnectionType connectionType) {
            string signalId(specifySignature(signal));
            string slotId(specifySignature(slot));
            {
                boost::mutex::scoped_lock lock(m_connectMutex);
                SignalInstancesConstIt it = m_signalInstances.find(signalId);
                if (it != m_signalInstances.end()) { // Signal found to be local to this component
                    // Register new slotId to local signal
                    it->second->registerSlot(slotId);
                    emit("signalConnected", signalId, slotId);
                    //cout << "LOW-LEVEL-DEBUG: SIGNAL \"" << signalId << "\" got connected to SLOT \"" << slotId << "\"" << endl;

                } else { // Signal is not registered here
                    // Send a connect request
                    emit("signalConnect", signalId, slotId);
                }
            }
            if (connectionType != NO_TRACK) {
                // Track the existence of the connection from now on
                trackExistenceOfConnection(signalId, slotId, connectionType);
            }
        }


        void SignalSlotable::trackExistenceOfConnection(const std::string& signalId, const std::string& slotId, ConnectionType connectionType) {
            string signalNetworkId = fetchNetworkId(signalId);
            string slotNetworkId = fetchNetworkId(slotId);
            string ownNetworkId = getNetworkId();

            // Signal is on remote component
            if (signalNetworkId != ownNetworkId) {
                boost::mutex::scoped_lock lock(m_heartbeatMutex);
                if (!m_trackedComponents.has(signalNetworkId)) {
                    addTrackedComponent(signalNetworkId);
                }
                Hash& slots = m_trackedComponents.getFromPath<Hash > (signalNetworkId + ".slots");
                if (slots.has(slotId)) {
                    slots.get< AssocType > (slotId).insert(AssocEntry(signalId, connectionType));
                } else {
                    AssocType newEntry;
                    newEntry.insert(AssocEntry(signalId, connectionType));
                    slots.set(slotId, newEntry);
                }
                // Connect remote signalHeartbeat to local slotHeartbeat
                connectN(signalNetworkId + "/signalHeartbeat-STRING-INT32", "slotHeartbeat-STRING-INT32");
            }
            // Slot is on remote component
            if (slotNetworkId != ownNetworkId) {
                boost::mutex::scoped_lock lock(m_heartbeatMutex);
                if (!m_trackedComponents.has(slotNetworkId)) {
                    addTrackedComponent(slotNetworkId);
                }
                Hash& signals = m_trackedComponents.getFromPath<Hash > (slotNetworkId + ".signals");
                if (signals.has(signalId)) {
                    signals.get< AssocType > (signalId).insert(AssocEntry(slotId, connectionType));
                } else {
                    AssocType newEntry;
                    newEntry.insert(AssocEntry(slotId, connectionType));
                    signals.set(signalId, newEntry);
                }
                // Connect remote signalHeartbeat to local slotHeartbeat
                connectN(slotNetworkId + "/signalHeartbeat-STRING-INT32", "slotHeartbeat-STRING-INT32");
            }
        }


        void SignalSlotable::disconnect(const std::string& signal, const std::string& slot) {
            string signalId(specifySignature(signal));
            string slotId(specifySignature(slot));

            // Stop any tracking of the existence of this connection
            stopTrackingExistenceOfConnection(signalId, slotId);

            {
                boost::mutex::scoped_lock lock(m_connectMutex);
                SignalInstancesConstIt it = m_signalInstances.find(signalId);
                if (it != m_signalInstances.end()) { // Signal found to be local to this component
                    // Register new slotId to local signal
                    it->second->unregisterSlot(slotId);
                    emit("signalDisconnected", signalId, slotId);
                } else { // Signal is not registered here
                    // Send a disconnect request
                    emit("signalDisconnect", signalId, slotId);
                }
            }
        }


        void SignalSlotable::stopTrackingExistenceOfConnection(const std::string& signalId, const std::string& slotId) {
            string signalNetworkId = fetchNetworkId(signalId);
            string slotNetworkId = fetchNetworkId(slotId);
            string ownNetworkId = getNetworkId();

            // Signal is on remote component
            if (signalNetworkId != ownNetworkId) {
                boost::mutex::scoped_lock lock(m_heartbeatMutex);
                if (m_trackedComponents.has(signalNetworkId)) {
                    Hash& slots = m_trackedComponents.getFromPath<Hash > (signalNetworkId + ".slots");
                    if (slots.has(slotId)) {
                        cout << "LOW_LEVEL_DEBUG: Removed tracking of connection: " << signalId << " <-> " << slotId << endl;
                        slots.erase(slotId);
                    }
                    if (slots.empty() && slots.get<bool>("isExplicitlyTracked") == false) {
                        cout << "LOW_LEVEL_DEBUG: Removed " << signalNetworkId << " from tracking list" << endl;
                        m_trackedComponents.erase(signalNetworkId);
                    }
                }
            }
            // Slot is on remote component
            if (slotNetworkId != ownNetworkId) {
                boost::mutex::scoped_lock lock(m_heartbeatMutex);
                if (m_trackedComponents.has(slotNetworkId)) {
                    Hash& signals = m_trackedComponents.getFromPath<Hash > (slotNetworkId + ".signals");
                    if (signals.has(signalId)) {
                        cout << "LOW_LEVEL_DEBUG: Removed tracking of connection: " << signalId << " <-> " << slotId << endl;
                        signals.erase(signalId);
                    }
                    if (signals.empty() && signals.get<bool>("isExplicitlyTracked") == false) {
                        cout << "LOW_LEVEL_DEBUG: Removed " << slotNetworkId << " from tracking list" << endl;
                        m_trackedComponents.erase(slotNetworkId);
                    }
                }
            }
        }


        void SignalSlotable::addTrackedComponent(const std::string& networkId) {
            Hash h;
            h.set("signals", Hash());
            h.set("slots", Hash());
            h.set("timeToLive", 4);
            h.set("isExplicitlyTracked", false);
            m_trackedComponents.set(networkId, h);
        }


        void SignalSlotable::connectionNotAvailable(const std::string& slotNetworkId, const exfel::util::Hash& affectedSignals, const exfel::util::Hash& affectedSlots) {
            std::cout << "Device \"" << slotNetworkId << "\" is not available, the following connection will thus not work: " << std::endl;
            std::cout << affectedSignals << affectedSlots << std::endl;
        }


        void SignalSlotable::connectionAvailableAgain(const std::string& slotNetworkId, const exfel::util::Hash& affectedSignals, const exfel::util::Hash& affectedSlots) {
            std::cout << "Previously unavailble device \"" << slotNetworkId << "\" is now available, the following connections are established: " << std::endl;
            std::cout << affectedSignals << affectedSlots << std::endl;
        }


        void SignalSlotable::connect(const std::string& signalNetworkId, const std::string& signalSignature,
                                     const std::string& slotNetworkId, const std::string& slotSignature, ConnectionType connectionType) {
            string signal, slot;
            if (signalNetworkId.empty()) {
                signal = signalSignature;
            } else {
                signal = signalNetworkId + "/" + signalSignature;
            }
            if (slotNetworkId.empty()) {
                slot = slotSignature;
            } else {
                slot = slotNetworkId + "/" + slotSignature;
            }
            connect(signal, slot, connectionType);
        }


        void SignalSlotable::slotConnect(const string& fullSignalSignature, const string& fullSlotSignature) {
            boost::mutex::scoped_lock lock(m_connectMutex);
            SignalInstancesConstIt it = m_signalInstances.find(fullSignalSignature);
            if (it != m_signalInstances.end()) {
                it->second->registerSlot(fullSlotSignature);
                emit("signalConnected", fullSignalSignature, fullSlotSignature);
                cout << "LOW-LEVEL-DEBUG: Established remote connection of signal \"" << fullSignalSignature << "\" to slot \"" << fullSlotSignature << "\"" << endl;
            }
        }


        void SignalSlotable::slotDisconnect(const std::string& fullSignalSignature, const std::string& fullSlotSignature) {
            boost::mutex::scoped_lock lock(m_connectMutex);
            SignalInstancesConstIt it = m_signalInstances.find(fullSignalSignature);
            if (it != m_signalInstances.end()) {
                it->second->unregisterSlot(fullSlotSignature);
                emit("signalDisconnected", fullSignalSignature, fullSlotSignature);
                cout << "LOW-LEVEL-DEBUG: Remotely disconnected signal \"" << fullSignalSignature << "\" from slot \"" << fullSlotSignature << "\"" << endl;
            }
        }


        string SignalSlotable::specifySignature(const string& signature) {
            vector<string> tokens;
            boost::split(tokens, signature, boost::is_any_of("/"));
            string functionSignature(tokens.back());
            // Remove all white spaces within the function signature
            functionSignature.erase(remove_if(functionSignature.begin(), functionSignature.end(), ::isspace), functionSignature.end());
            if (tokens.size() == 2) {
                return getHostId() + "/" + functionSignature;
            } else if (tokens.size() == 1) {
                return getHostId() + "/" + getInstanceId() + "/" + functionSignature;
            } else {
                return signature;
            }
        }


        string SignalSlotable::fetchNetworkId(const std::string& signalOrSlotId) const {
            return signalOrSlotId.substr(0, signalOrSlotId.find_last_of('/'));
        }


        string SignalSlotable::fetchHostId(const std::string& anyId) const {
            return anyId.substr(0, anyId.find_first_of('/'));
        }


        void SignalSlotable::refreshTimeToLiveForConnectedSlot(const std::string& networkId, int timeToLive) {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            if (m_trackedComponents.has(networkId)) {
                Hash& entry = m_trackedComponents.get<Hash > (networkId);
                int& oldTimeToLive = entry.get<int >("timeToLive");
                if (oldTimeToLive <= 0) {
                    if (entry.get<bool>("isExplicitlyTracked") == true) {
                        componentAvailableAgain(networkId);
                    } else {
                        Hash signals = prepareConnectionNotAvailableInformation(entry.get<Hash > ("signals"));
                        Hash slots = prepareConnectionNotAvailableInformation(entry.get<Hash > ("slots"));
                        connectionAvailableAgain(networkId, signals, slots);
                    }
                }
                oldTimeToLive = timeToLive;
            } else {
                //cout << "LOW-LEVEL-DEBUG: Got refresh request for unregistered heartbeat of " << networkId << endl;
            }
        }


        void SignalSlotable::letConnectionSlowlyDieWithoutHeartbeat() {
            while (true) {
                {
                    boost::mutex::scoped_lock lock(m_heartbeatMutex);
                    for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                        Hash& entry = m_trackedComponents.get<Hash > (it->first);
                        int& timeToLive = entry.get<int>("timeToLive");
                        if (timeToLive > 0) {
                            timeToLive--;
                        } else if (timeToLive == 0) {
                            timeToLive--;
                            if (entry.get<bool>("isExplicitlyTracked") == true) {
                                componentNotAvailable(it->first);
                            } else {
                                Hash signals = prepareConnectionNotAvailableInformation(entry.get<Hash > ("signals"));
                                Hash slots = prepareConnectionNotAvailableInformation(entry.get<Hash > ("slots"));
                                connectionNotAvailable(it->first, signals, slots);
                            }
                        } else {
                            // Here we can try to reconnect
                            if (m_reconnectIntervals.find(timeToLive) != m_reconnectIntervals.end()) {
                                const Hash& signals = entry.get<Hash > ("signals");
                                for (Hash::const_iterator jt = signals.begin(); jt != signals.end(); ++jt) {
                                    const AssocType& slots = signals.get< AssocType > (jt);
                                    for (AssocTypeConstIterator kt = slots.begin(); kt != slots.end(); ++kt) {
                                        if (kt->second == RECONNECT) {
                                            m_heartbeatMutex.unlock();
                                            //cout << "LOW-LEVEL-DEBUG: Trying to reconnect: " << jt->first << " <-> " << kt->first << endl;
                                            connect(jt->first, kt->first, RECONNECT);
                                            m_heartbeatMutex.lock();
                                        }
                                    }
                                }
                                const Hash& slots = entry.get<Hash > ("slots");
                                for (Hash::const_iterator jt = slots.begin(); jt != slots.end(); ++jt) {
                                    const AssocType& signals = slots.get< AssocType > (jt);
                                    for (AssocTypeConstIterator kt = signals.begin(); kt != signals.end(); ++kt) {
                                        if (kt->second == RECONNECT) {
                                            m_heartbeatMutex.unlock();
                                            //cout << "LOW-LEVEL-DEBUG: Trying to reconnect: " << kt->first << " <-> " << jt->first << endl;
                                            connect(kt->first, jt->first, RECONNECT);
                                            m_heartbeatMutex.lock();
                                        }
                                    }
                                }
                            }
                            timeToLive--;
                            continue;
                        }
                    }
                }
                boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
            }
        }


        Hash SignalSlotable::prepareConnectionNotAvailableInformation(const exfel::util::Hash& hash) const {
            Hash result;
            for (Hash::const_iterator jt = hash.begin(); jt != hash.end(); ++jt) {
                const AssocType& associates = hash.get< AssocType > (jt);
                vector<string > tmp;
                tmp.reserve(associates.size());
                for (AssocTypeConstIterator kt = associates.begin(); kt != associates.end(); ++kt) {
                    tmp.push_back(kt->first);
                }
                result.set(jt->first, tmp);
            }
            return result;
        }


        void SignalSlotable::slotTryReconnectNow() {
            boost::mutex::scoped_lock lock(m_heartbeatMutex);
            for (Hash::iterator it = m_trackedComponents.begin(); it != m_trackedComponents.end(); ++it) {
                Hash& entry = m_trackedComponents.get<Hash > (it->first);
                const int& timeToLive = entry.get<int>("timeToLive");
                if (timeToLive < 0) {
                    const Hash& signals = entry.get<Hash > ("signals");
                    for (Hash::const_iterator jt = signals.begin(); jt != signals.end(); ++jt) {
                        const AssocType& slots = signals.get< AssocType > (jt);
                        for (AssocTypeConstIterator kt = slots.begin(); kt != slots.end(); ++kt) {
                            if (kt->second == RECONNECT) {
                                m_heartbeatMutex.unlock();
                                connect(jt->first, kt->first, RECONNECT);
                                m_heartbeatMutex.lock();
                            }
                        }
                    }
                    const Hash& slots = entry.get<Hash > ("slots");
                    for (Hash::const_iterator jt = slots.begin(); jt != slots.end(); ++jt) {
                        const AssocType& signals = slots.get< AssocType > (jt);
                        for (AssocTypeConstIterator kt = signals.begin(); kt != signals.end(); ++kt) {
                            if (kt->second == RECONNECT) {
                                m_heartbeatMutex.unlock();
                                connect(kt->first, jt->first, RECONNECT);
                                m_heartbeatMutex.lock();
                            }
                        }
                    }
                }
            }
        }
    } // namespace core
} // namespace exfel
