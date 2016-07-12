/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_GUISERVERDEVICE_HH
#define	KARABO_CORE_GUISERVERDEVICE_HH

#include <karabo/core/ProjectManager.hh>
#include <karabo/net/Channel.hh>
#include <karabo/net/Connection.hh>
#include <karabo/xms/InputChannel.hh>

#include "Device.hh"
#include "OkErrorFsm.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    namespace core {

        class GuiServerDevice : public karabo::core::Device<> {

            struct NetworkConnection {
                std::string name;
                karabo::net::Channel::Pointer channel;
	    };

            typedef std::multimap<karabo::xms::InputChannel::Pointer, NetworkConnection> NetworkMap;
            
            enum QueueBehaviorsTypes { REMOVE_OLDEST = 3, LOSSLESS };

            karabo::net::IOService::Pointer m_ioService;
            karabo::net::Connection::Pointer m_dataConnection;

            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_serializer;
            std::map<karabo::net::Channel::Pointer, std::set<std::string> > m_channels;
            mutable boost::mutex m_channelMutex;
            mutable boost::mutex m_monitoredDevicesMutex;
            mutable boost::mutex m_networkMutex;

            karabo::net::BrokerConnection::Pointer m_loggerConnection;
            karabo::net::BrokerIOService::Pointer m_loggerIoService;
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

            void onError(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& errorMessage);

            void onGuiError(const karabo::util::Hash& hash);

            void onConnect(karabo::net::Channel::Pointer channel);

            void onRead(karabo::net::Channel::Pointer channel, karabo::util::Hash& info);

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

        };
    }
}

#endif
