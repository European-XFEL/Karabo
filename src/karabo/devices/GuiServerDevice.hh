/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_GUISERVERDEVICE_HH
#define	KARABO_CORE_GUISERVERDEVICE_HH

#include "karabo/devices/ProjectManager.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/xms/InputChannel.hh"

#include "karabo/core/Device.hh"
#include "karabo/core/OkErrorFsm.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    namespace devices {

        class GuiServerDevice : public karabo::core::Device<> {

            struct NetworkConnection {

                std::string name;
                karabo::net::Channel::Pointer channel;
            };

            typedef std::multimap<karabo::xms::InputChannel::Pointer, NetworkConnection> NetworkMap;

            enum QueueBehaviorsTypes {

                REMOVE_OLDEST = 3, LOSSLESS
            };

            karabo::net::Connection::Pointer m_dataConnection;

            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_serializer;
            std::map<karabo::net::Channel::Pointer, std::set<std::string> > m_channels;
            mutable boost::mutex m_channelMutex;
            mutable boost::mutex m_monitoredDevicesMutex;
            mutable boost::mutex m_networkMutex;

            karabo::net::BrokerConnection::Pointer m_loggerConnection;
            karabo::net::BrokerChannel::Pointer m_loggerChannel;
            std::map<std::string, int> m_monitoredDevices;
            NetworkMap m_networkConnections;

            karabo::net::BrokerConnection::Pointer m_guiDebugConnection;
            karabo::net::BrokerChannel::Pointer m_guiDebugChannel;

            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::const_iterator ConstChannelIterator;
            typedef std::map< karabo::net::Channel::Pointer, std::set<std::string> >::iterator ChannelIterator;

            karabo::util::Hash m_loggerMap;
            karabo::util::Hash m_loggerInput;

        public:

            KARABO_CLASSINFO(GuiServerDevice, "GuiServerDevice", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            GuiServerDevice(const karabo::util::Hash& input);

            virtual ~GuiServerDevice();

            void initialize();


        private: // Functions

            void safeClientWrite(const karabo::net::Channel::Pointer channel, const karabo::util::Hash& message, int prio = LOSSLESS);

            void safeAllClientsWrite(const karabo::util::Hash& message, int prio = LOSSLESS);

            void onError(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel);

            void onGuiError(const karabo::util::Hash& hash);

            void onConnect(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel);

            void onRead(const karabo::net::ErrorCode& e, karabo::net::Channel::Pointer channel, karabo::util::Hash& info);

            void onLogin(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onReconfigure(const karabo::util::Hash& info);

            void onExecute(const karabo::util::Hash& info);

            void onInitDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void initReply(karabo::net::Channel::Pointer channel, const std::string& deviceId, bool success, const std::string& message);

            void onGetDeviceConfiguration(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onKillServer(const karabo::util::Hash& info);

            void onKillDevice(const karabo::util::Hash& info);

            void onStartMonitoringDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onStopMonitoringDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onGetClassSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onGetDeviceSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onGetPropertyHistory(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void propertyHistory(karabo::net::Channel::Pointer channel, const std::string& deviceId, const std::string& property, const std::vector<karabo::util::Hash>& data);

            void onSubscribeNetwork(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onNetworkData(const karabo::xms::InputChannel::Pointer& input);

            void onGetAvailableProjects(karabo::net::Channel::Pointer channel);

            void availableProjects(karabo::net::Channel::Pointer channel,
                                   const karabo::util::Hash& projects);

            void onNewProject(karabo::net::Channel::Pointer channel,
                              const karabo::util::Hash& info);

            void projectNew(karabo::net::Channel::Pointer channel,
                            const std::string& projectName, bool success,
                            const std::vector<char>& data);

            void onLoadProject(karabo::net::Channel::Pointer channel,
                               const karabo::util::Hash& info);

            void projectLoaded(karabo::net::Channel::Pointer channel,
                               const std::string& projectName,
                               const karabo::util::Hash& metaData,
                               const std::vector<char>& data);

            void onSaveProject(karabo::net::Channel::Pointer channel,
                               const karabo::util::Hash& info);

            void projectSaved(karabo::net::Channel::Pointer channel,
                              const std::string& projectName, bool success,
                              const std::vector<char>& data);

            void onCloseProject(karabo::net::Channel::Pointer channel,
                                const karabo::util::Hash& info);

            void projectClosed(karabo::net::Channel::Pointer channel,
                               const std::string& projectName,
                               bool success,
                               const std::vector<char>& data);

            void registerConnect(const karabo::net::Channel::Pointer& channel);

            void sendSystemTopology(karabo::net::Channel::Pointer channel);
            void sendSystemVersion(karabo::net::Channel::Pointer channel);

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceUpdatedHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void deviceChangedHandler(const std::string& instanceId, const karabo::util::Hash& what);

            void classSchemaHandler(const std::string& serverId, const std::string& classId, const karabo::util::Schema& classSchema);

            void schemaUpdatedHandler(const std::string& deviceId, const karabo::util::Schema& schema);

            void logHandler(const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);

            void slotNotification(const std::string& type, const std::string& shortMessage, const std::string& detailedMessage, const std::string& deviceId);

            void slotLoggerMap(const karabo::util::Hash& loggerMap);

            void onInputChannelConnected(const karabo::xms::InputChannel::Pointer& input, const karabo::net::Channel::Pointer& channel, const std::string& channelName);

            void logErrorHandler(karabo::net::BrokerChannel::Pointer channel, const std::string& info);

            /**
             * A slot called by alarm service devices if they want to notify of an alarm update
             * @param alarmServiceId: the instance id of the service device
             * @param type: the type of the update: can be alarmUpdate, alarmInit
             * @param updateRows: the rows which should be updated. This is a Hash of Hashes, were
             * the unique row indices, as managed by the alarm service are keys, and the values are
             * Hashes of the form updateType->entry
             *
             * updateType is a string of any of the following: init, update, delete,
             *  acknowledgeable, deviceKilled, refuseAcknowledgement
             *
             * entry is a Hash with the following entries:
             *
             *  timeOfFirstOccurrence -> string: timestamp of first occurrence of alarm
             *  trainOfFirstOccurrence -> unsigned long long: train id of first occurrence of alarm
             *  timeOfOccurrence -> string: timestamp of most resent occurrence of alarm
             *  trainOfOccurrence -> unsigned long long: train id of most resent occurrence of alarm
             *  needsAcknowledging -> bool: does the alarm require acknowledging
             *  acknowledgeable -> bool: can the alarm be acknowledged
             *  deviceId -> string: deviceId of device that raised the alarm
             *  property -> string: property the alarm refers to
             *  id -> the unique row id
             *
             * it send the following Hash to the client (lossless)
             *
             * Hash h("type", type, "instanceId", alarmServiceId, "rows", updateRows);
             */
            void slotAlarmSignalsUpdate(const std::string& alarmServiceId, const std::string& type, const karabo::util::Hash& updateRows);

            /**
             * Called if the client wants to acknowledge an alarm, by sending a message of type "acknowledgeAlarm"
             * @param channel: the channel the client is connected to
             * @param info: the message from the client. Should contain the unique row ids of the alarms
             * to acknowledge. It is a Hash of Hashes of the same form as described for slotAlarmSignalsUpdate,
             * where the keys give the unique row indices
             */
            void onAcknowledgeAlarm(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Called if a client sends a message of type "requestAlarms"
             * @param channel: the channel the calling client is connected to
             * @param info: message passed from the client. It is a Hash that needs to contain
             * a string field "alarmInstanceId", which either contains a specifiy alarm service's
             * instance id, or an empty string. In the latter case a request for current alarms
             * is forwarded to all known alarm services, otherwise to the specific one. Replies
             * from the alarm services trigger calling "onRequestedAlarmsReply" asynchroniously.
             */
            void onRequestAlarms(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Callback executed upon reply from an alarm service to "onRequestAlarms".
             * @param channel: the client channel the request came from, bound by "onRequestAlarms"
             * @param reply: reply from the alarm service, expected to contain fields
             *
             * instanceId -> string: instance id of the replying alarm service
             * alarms -> Nested Hash of form given in "slotAlarmSignalsUpdate"
             *
             * It sends out a Hash of form:
             * Hash h("type", "alarmInit", "instanceId", reply.get<std::string>("instanceId"), "rows", reply.get<Hash>("alarms"));
             */
            void onRequestedAlarmsReply(karabo::net::Channel::Pointer channel, const karabo::util::Hash& reply);


            /**
             * Executed when the gui requests an update to schema attributes via the updateAttributes signal.
             * @param channel: gui channel that requested the update
             * @param info: updated attributes, expected to be of form Hash("instanceId", str, "updates", vector<Hash>) where
             * each entry in updates is of the form Hash("path", str, "attribute", str, "value", valueType)
             */
            void onUpdateAttributes(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            /**
             * Callback for onUpdateAttributes
             * @param channel: gui channel that requested the update
             * @param reply: reply from the device that performed the attribute update. Is of form
             * Hash("success" bool, "instanceId", str, "updatedSchema", Schema, "requestedUpdate", vector<Hash>)
             * where success indicates a successful update, instanceId the device that performed the update
             * updatedSchema the new valid schema, regardless of success or not, and requestedUpdates the
             * original update request, as received through onUpdateAttributes
             */
            void onRequestedAttributeUpdate(karabo::net::Channel::Pointer channel, const karabo::util::Hash& reply);

            /**
             * Checks if an instance at instanceId is an alarmService and connects to its signals if it is.
             * @param topologyEntry: the topology Hash, from which the class of instanceId will be deduced
             */
            void connectPotentialAlarmService(const karabo::util::Hash& topologyEntry);

            /**
             * Returns the instance type and instance id from a topology entry
             * @param topologyEntry: a Hash of the topology format
             * @param type: string which will afterwards contain type
             * @param instanceId: string which will be filled with the instance id
             */
            void typeAndInstanceFromTopology(const karabo::util::Hash& topologyEntry, std::string& type, std::string& instanceId);

        };
    }
}

#endif
