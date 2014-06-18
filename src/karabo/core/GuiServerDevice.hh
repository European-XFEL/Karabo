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

            karabo::net::IOService::Pointer m_ioService;
            karabo::net::Connection::Pointer m_dataConnection;

            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_serializer;
            std::map<karabo::net::Channel::Pointer, std::set<std::string> > m_channels;
            boost::mutex m_channelMutex;

            karabo::net::BrokerConnection::Pointer m_loggerConnection;
            karabo::net::BrokerIOService::Pointer m_loggerIoService;
            karabo::net::BrokerChannel::Pointer m_loggerChannel;
            std::map<std::string, int> m_visibleDevices;

        public:

            KARABO_CLASSINFO(GuiServerDevice, "GuiServerDevice", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            GuiServerDevice(const karabo::util::Hash& input);

            virtual ~GuiServerDevice();
            
            void okStateOnEntry();


        private: // Functions

            void onError(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& errorMessage);

            void onConnect(karabo::net::Channel::Pointer channel);

            void onRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onLogin(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onReconfigure(const karabo::util::Hash& info);

            void onExecute(const karabo::util::Hash& info);

            void onInitDevice(const karabo::util::Hash& info);

            void onRefreshInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onKillServer(const karabo::util::Hash& info);

            void onKillDevice(const karabo::util::Hash& info);

            void onNewVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onRemoveVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);
            
            void onGetClassSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onGetDeviceSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void onGetFromPast(karabo::net::Channel::Pointer channel, const karabo::util::Hash& info);

            void slotPropertyHistory(const std::string& deviceId, const std::string& property, const std::vector<karabo::util::Hash>& data);

            void registerConnect(const karabo::net::Channel::Pointer& channel);
            
            void sendSystemTopology(karabo::net::Channel::Pointer channel);

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceUpdatedHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId, const karabo::util::Hash& instanceInfo);

            void deviceChangedHandler(const std::string& instanceId, const karabo::util::Hash& what);
            
            void slotSchemaUpdated(const karabo::util::Schema& description, const std::string& deviceId);

            void logHandler(karabo::net::BrokerChannel::Pointer channel, const std::string& logMessage, const karabo::util::Hash& header);

            void slotNotification(const std::string& type, const std::string& shortMessage, const std::string& detailedMessage, const std::string& deviceId);
            
        };
    }
}

#endif
