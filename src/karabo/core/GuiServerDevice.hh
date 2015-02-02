/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_GUISERVERDEVICE_HH
#define	KARABO_CORE_GUISERVERDEVICE_HH

#include <karabo/net/Channel.hh>
#include <karabo/net/Connection.hh>

#include "Device.hh"
#include "OkErrorFsm.hh"

/**
 * The main karabo namespace
 */
namespace karabo {

    namespace core {

        class GuiServerDevice : public karabo::core::Device<OkErrorFsm> {

	    struct NetworkConnection {
		std::string name;
	        karabo::net::Channel::Pointer channel;
	    };

	    typedef std::multimap<karabo::io::Input<karabo::util::Hash>::Pointer, NetworkConnection> NetworkMap;

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

        public:

            KARABO_CLASSINFO(GuiServerDevice, "GuiServerDevice", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            GuiServerDevice(const karabo::util::Hash& input);

            virtual ~GuiServerDevice();
            
            void okStateOnEntry();


        private: // Functions

            void onError(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& errorMessage);

            void onGuiError(const karabo::util::Hash& hash);

            void onConnect(karabo::net::Channel::Pointer channel);

            void onRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onLogin(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onReconfigure(const karabo::util::Hash& info);

            void onExecute(const karabo::util::Hash& info);

            void onInitDevice(const karabo::util::Hash& info);

            void onGetDeviceConfiguration(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onKillServer(const karabo::util::Hash& info);

            void onKillDevice(const karabo::util::Hash& info);

            void onStartMonitoringDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onStopMonitoringDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);
            
            void onGetClassSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onGetDeviceSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onGetPropertyHistory(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onSubscribeNetwork(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onNetworkData(const karabo::io::Input<karabo::util::Hash>::Pointer &input);
            
            void onGetAvailableProjects(karabo::net::Channel::Pointer channel);
            
            void slotAvailableProjects(const std::vector<std::string>& projects);
            
            void onLoadProject(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);
            
            void slotProjectLoaded(const std::string& projectName, const std::vector<char>& data);
            
            void onSaveProject(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);
            
            void slotProjectSaved(const std::string& projectName, bool success);
            
            void onCloseProject(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);
            
            void slotProjectClosed(const std::string& projectName, bool success);
            
            void propertyHistory(karabo::net::Channel::Pointer channel, const std::string& deviceId, const std::string& property, const std::vector<karabo::util::Hash>& data);

            void registerConnect(const karabo::net::Channel::Pointer& channel);
            
            void sendSystemTopology(karabo::net::Channel::Pointer channel);

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceUpdatedHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void deviceChangedHandler(const std::string& instanceId, const karabo::util::Hash& what);

            void classSchemaHandler(const std::string& serverId, const std::string& classId, const karabo::util::Schema& classSchema);
            
            void schemaUpdatedHandler(const std::string& deviceId, const karabo::util::Schema& schema);

            void logHandler(karabo::net::BrokerChannel::Pointer channel, const karabo::util::Hash::Pointer& header, const std::string& logMessage);

            void slotNotification(const std::string& type, const std::string& shortMessage, const std::string& detailedMessage, const std::string& deviceId);
            
            void slotLoggerMap(const karabo::util::Hash& loggerMap);
            
        };
    }
}

#endif
