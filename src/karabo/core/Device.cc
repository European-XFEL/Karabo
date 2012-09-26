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
#include <karabo/util/Time.hh>

#include "Device.hh"

namespace karabo {
    namespace core {

        using namespace log4cpp;
        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;
        using namespace karabo::net;

        std::map<std::string, int> Device::m_instanceCountPerDeviceServer;
        boost::mutex Device::m_instanceCountMutex;

        Device::~Device() {
            decreaseInstanceCount();
        }

        void Device::expectedParameters(karabo::util::Schema& expected) {

            CHOICE_ELEMENT<BrokerConnection > (expected).key("connection")
                    .displayedName("Connection")
                    .description("The connection to the communication layer of the distributed system")
                    .assignmentOptional().defaultValue("Jms")
                    .advanced()
                    .init()
                    .commit();

            STRING_ELEMENT(expected).key("devSrvInstId")
                    .displayedName("Device-Server Instance Id")
                    .description("The device-server instance id, on which this device-instance is running on")
                    .assignmentInternal().defaultValue("")
                    .commit();

            STRING_ELEMENT(expected).key("devInstId")
                    .displayedName("Device Instance Id")
                    .description("Device Instance Id uniquely identifies a device instance in the distributed system")
                    .assignmentOptional().noDefaultValue()
                    .init()
                    .advanced()
                    .commit();
            
            STRING_ELEMENT(expected).key("devClaId")
                    .displayedName("Device Class Id")
                    .description("The (factory)-name of the class of this device")
                    .readOnly()
                    .commit();
            
            STRING_ELEMENT(expected).key("state")
                    .displayedName("State")
                    .description("The current state the device is in")
                    .assignmentOptional().defaultValue("uninitialized")
                    .readOnly()
                    .commit();
        }

        void Device::configure(const karabo::util::Hash& input) {
            try {

                // Speed access to own classId
                m_classId = getClassInfo().getClassId();
                
                // Speed access to device-server instance
                if (input.has("devSrvInstId")) {
                    input.get("devSrvInstId", m_devSrvInstId);
                } else {
                    m_devSrvInstId = "";
                }

                // Increase instance count
                increaseInstanceCount();

                // Construct needed for splitting the parameters (validate function needs this)
                Hash tmp(m_classId, input);

                // Set device instance 
                string devInstId;
                if (input.has("devInstId")) {
                    input.get("devInstId", devInstId);
                } else { // No devInstId given
                    devInstId = generateDefaultDeviceInstanceId();
                    tmp.setFromPath(m_classId + ".devInstId", devInstId);
                }

                // Setup logger
                m_log = &(karabo::log::Logger::logger(devInstId));

                // Split the configuration parameters into three pots
                m_initialParameters = m_expectedInitialParameters.validate(tmp, true, false, true).get<Hash > (m_classId);
                m_reconfigurableParameters = m_expectedReconfigurableParameters.validate(tmp, true, false, true).get<Hash > (m_classId);
                m_monitoredParameters = m_expectedMonitoredParameters.validate(tmp, true, false, true).get<Hash > (m_classId);

                // Instantiate connection
                BrokerConnection::Pointer connection = BrokerConnection::createChoice("connection", input);

                // Initialize the SignalSlotable instance
                init(connection, devInstId);

                SIGNAL4("signalErrorFound", string, string, string, string); // timeStamp, shortMsg, longMsg, instanceId
                SIGNAL2("signalBadReconfiguration", string, string); // shortMsg, instanceId 
                SIGNAL2("signalNoTransition", string, string);
                SIGNAL3("signalChanged", karabo::util::Hash, string, string); // changeHash, instanceId, classId
                SIGNAL4("signalWarning", string, string, string, string); // timeStamp, warnMsg, instanceId, priority
                SIGNAL4("signalAlarm", string, string, string, string); // timeStamp, alarmMsg, instanceId, priority
                SIGNAL3("signalSchemaUpdated", string, string, string); // schema, instanceId, classId
                SIGNAL2("signalDeviceInstanceGone", string, string) /* DeviceServerInstanceId, deviceInstanceId */

                SLOT1(slotReconfigure, karabo::util::Hash)
                SLOT0(slotRefresh)
                SLOT1(slotGetSchema, bool); // onlyCurrentState
                SLOT0(slotKillDeviceInstance)

                // Hard-coded connects (for global slots with this name)
                connectN("", "signalChanged", "*", "slotChanged");
                connectN("", "signalBadReconfiguration", "*", "slotBadReconfiguration");
                connectN("", "signalNoTransition", "*", "slotNoTransition");
                connectN("", "signalErrorFound", "*", "slotErrorFound");
                connectN("", "signalWarning", "*", "slotWarning");
                connectN("", "signalAlarm", "*", "slotAlarm");
                connectN("", "signalSchemaUpdated", "*", "slotSchemaUpdated");
                connectN("", "signalDeviceInstanceGone", "*", "slotDeviceInstanceGone");

                log() << Priority::INFO << "Starting up " << m_classId << " on networkId " << getInstanceId();

                if (m_devSrvInstId == boost::asio::ip::host_name()) {
                    std::stringstream stream;
                    Hash config("Xsd.indentation", -1);
                    Format<Schema>::create(config)->convert(m_allExpectedParameters, stream);
                    call("*", "slotNewStandaloneDeviceInstanceAvailable", boost::asio::ip::host_name(), tmp, getInstanceId(), stream.str());
                }
                
                set("devClaId", m_classId);


            } catch (const Exception& e) {
                RETHROW;
            }
        }

        void Device::increaseInstanceCount() {
            boost::mutex::scoped_lock lock(m_instanceCountMutex);
            m_instanceCountPerDeviceServer[m_devSrvInstId]++;
        }

        void Device::decreaseInstanceCount() {
            boost::mutex::scoped_lock lock(m_instanceCountMutex);
            m_instanceCountPerDeviceServer[m_devSrvInstId]--;
        }
        
        void Device::slotGetSchema(const bool& onlyCurrentState) {
            if (onlyCurrentState) {
                const string& currentState = get<string > ("state");
                reply(getStateDependentSchema(currentState));
            } else {
                reply(m_allExpectedParameters);
            }
        }
        

        karabo::util::Schema Device::getFullSchema() const {
            if (!m_injectedExpectedParameters.empty())
                return Schema(m_allExpectedParameters).addExternalSchema(m_injectedExpectedParameters);
            else return m_allExpectedParameters;
        }

        std::string Device::generateDefaultDeviceInstanceId() {
            boost::mutex::scoped_lock lock(m_instanceCountMutex);
            string index = String::toString(m_instanceCountPerDeviceServer[m_devSrvInstId]);
            // Prepare shortened Device-Server name
            vector<string> tokens;
            if (m_devSrvInstId.empty()) return boost::asio::ip::host_name() + "/" + m_classId + "/" + index;
            boost::split(tokens, m_devSrvInstId, boost::is_any_of("/"));
            string domain(tokens.front() + "-" + tokens.back());
            return domain + "/" + m_classId + "/" + index;
        }

        log4cpp::Category& Device::log() {
            return (*m_log);
        }

        Hash Device::getCurrentConfiguration() const {
            Hash ret;
            Hash& config = ret.bindReference<Hash > (m_classId);
            config.update(m_initialParameters);
            config.update(m_reconfigurableParameters);
            config.update(m_monitoredParameters);
            return ret;
        }

        Hash Device::getInitialParameters() const {
            return m_initialParameters.flatten();
        }

        Hash Device::getReconfigurableParameters() const {
            return m_reconfigurableParameters.flatten();
        }

        Hash Device::getMonitorableParameters() const {
            return m_monitoredParameters.flatten();
        }

        void Device::reconfigure(const std::string& instanceId, const Hash& configuration) {
            call(instanceId, "slotReconfigure", configuration);
        }

        void Device::errorFoundAction(const std::string& shortMessage, const std::string& detailedMessage) {
            triggerErrorFound(shortMessage, detailedMessage);
        }

        void Device::updateCurrentState(const std::string& state) {
            set("state", state);
            // Reply new state to interested event initiators
            reply(state);
        }
        
        void Device::slotKillDeviceInstance() {
            log() << Priority::INFO << "Device is going down...";
            onKill(); // Give devices a chance to react
            emit("signalDeviceInstanceGone", m_devSrvInstId, m_instanceId);
            stopEventLoop();
            log() << Priority::INFO << "dead.";
        }

        void Device::slotRefresh() {
            Hash all(m_initialParameters);
            all.update(m_reconfigurableParameters);
            all.update(m_monitoredParameters);
            emit("signalChanged", all, m_instanceId, m_classId);
            reply(all);
        }

        void Device::slotReconfigure(const karabo::util::Hash& newConfiguration) {

            if (newConfiguration.empty()) return;

            std::pair<bool, std::string> result = validate(newConfiguration);

            if (result.first == true) { // is a valid reconfiguration
                // Automatically flatten - for user convenience
                //m_incomingValidatedReconfiguration = m_incomingValidatedReconfiguration.flatten();

                // Give device-implementer a chance to specifically react on reconfiguration event by polymorphically calling back
                try {
                    onReconfigure(m_incomingValidatedReconfiguration);
                } catch (const karabo::util::Exception& e) {
                    onException(e.userFriendlyMsg(), e.detailedMsg());
                    reply(false, e.userFriendlyMsg());
                    return;
                }

                // Merge reconfiguration with current state
                applyReconfiguration(m_incomingValidatedReconfiguration);
            }
            reply(result.first, result.second);
        }

        std::pair<bool, std::string> Device::validate(const karabo::util::Hash& newConfiguration) {
            // Retrieve the current state of the device instance
            const string& currentState = get<string > ("state");
            Schema& whiteList = getStateDependentSchema(currentState);
            Hash config(m_classId, newConfiguration); // Validator needs classId as root item
            log() << Priority::DEBUG << "Incoming reconfiguration:\n" << newConfiguration;
            try {
                
                config = whiteList.validate(config, false, false, false, true); 

            } catch (const ParameterException& e) {
                string errorText =  e.userFriendlyMsg() + " in state: \"" + currentState + "\"";
                log() << Priority::ERROR << errorText;
                return make_pair(false, errorText);
            }
            m_incomingValidatedReconfiguration = config.get<Hash > (m_classId);
            log() << Priority::DEBUG << "Validated reconfiguration:\n" << m_incomingValidatedReconfiguration;
            return make_pair(true, "");
        }
        
        Schema& Device::getStateDependentSchema(const std::string& currentState) {
            boost::mutex::scoped_lock lock(m_stateDependendSchemaMutex);
            // Check cache, whether a special set of state-dependent expected parameters was created before
            map<string, Schema>::iterator it = m_stateDependendSchema.find(currentState);
            if (it == m_stateDependendSchema.end()) { // No
                it = m_stateDependendSchema.insert(make_pair(currentState, Device::expectedParameters(m_classId, karabo::util::WRITE, currentState))).first; // New one
                if (!m_injectedExpectedParameters.empty()) it->second.addExternalSchema(m_injectedExpectedParameters);
            }
            return it->second;
        }

        void Device::applyReconfiguration(const karabo::util::Hash& user) {
            boost::mutex::scoped_lock lock(m_objectStateChangeMutex);
            //m_incomingValidatedReconfiguration = m_incomingValidatedReconfiguration.unflatten();
            m_reconfigurableParameters.update(m_incomingValidatedReconfiguration);
            log() << Priority::DEBUG << "After user interaction:\n" << user;
            emit("signalChanged", m_incomingValidatedReconfiguration, getInstanceId(), m_classId);
            m_incomingValidatedReconfiguration.clear(); // No pending reconfiguration anymore
            log() << Priority::DEBUG << "Current state:\n" << m_reconfigurableParameters;
        }

        void Device::noStateTransition(const std::string& typeId, int state) {
            string eventName(typeId);
            boost::regex re(".*\\d+(.+Event).*");
            boost::smatch what;
            bool result = boost::regex_search(typeId, what, re);
            if (result && what.size() == 2) {
                eventName = what.str(1);
            }
            ostringstream msg;
            msg << "Current state of device \"" << m_classId << "\" does not allow any transition for event \"" << eventName << "\"";
            log() << Priority::DEBUG << msg.str();
            emit("signalNoTransition", msg.str(), getInstanceId());
        }

        void Device::triggerErrorFound(const std::string& shortMessage, const std::string& detailedMessage) const {
            emit("signalErrorFound", karabo::util::Time::getCurrentDateTime(), shortMessage, detailedMessage, getInstanceId());
        }

        void Device::triggerWarning(const std::string& warningMessage, const std::string& priority) const {
            emit("signalWarning", karabo::util::Time::getCurrentDateTime(), warningMessage, getInstanceId(), priority);
        }

        void Device::triggerAlarm(const std::string& alarmMessage, const std::string& priority) const {
            emit("signalAlarm", karabo::util::Time::getCurrentDateTime(), alarmMessage, getInstanceId(), priority);
        }
    }
}
