/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_CORE_GUISERVERDEVICE_HH
#define	KARABO_CORE_GUISERVERDEVICE_HH

#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>
#include <karabo/net/BrokerConnection.hh>
#include <karabo/net/BrokerChannel.hh>

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

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_textSerializer;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;
            std::map<karabo::net::Channel::Pointer, std::set<std::string> > m_channels;
            boost::mutex m_channelMutex;

            karabo::net::BrokerConnection::Pointer m_loggerConnection;
            karabo::net::BrokerIOService::Pointer m_loggerIoService;
            karabo::net::BrokerChannel::Pointer m_loggerChannel;

        public:

            KARABO_CLASSINFO(GuiServerDevice, "GuiServerDevice", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            GuiServerDevice(const karabo::util::Hash& input);

            virtual ~GuiServerDevice() {
            }


            void okStateOnEntry();


        private: // Functions

            void onError(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& errorMessage);

            void onConnect(karabo::net::Channel::Pointer channel);

            void onRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body);

            void onLogin(karabo::net::Channel::Pointer channel, const std::string& body);

            void onReconfigure(const karabo::util::Hash& header, const std::string& body);

            void onExecute(const karabo::util::Hash& header, const std::string& body);

            void onRefreshInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header);

            void onInitDevice(const karabo::util::Hash& header, const std::string& body);

            void onKillServer(const karabo::util::Hash& header, const std::string& body);

            void onKillDevice(const karabo::util::Hash& header, const std::string& body);

            void onNewVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header);

            void onRemoveVisibleDevice(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header);
            
            void onGetClassSchema(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body);



            void sendSystemTopology(karabo::net::Channel::Pointer channel);

            void registerConnect(const karabo::net::Channel::Pointer& channel);
            

            void instanceNewHandler(const karabo::util::Hash& topologyEntry);

            void instanceUpdatedHandler(const karabo::util::Hash& topologyEntry);

            void instanceGoneHandler(const std::string& instanceId);


            void preprocessImageData(karabo::util::Hash& modified);

            void slotChanged(const karabo::util::Hash& what, const std::string& instanceId);

            void slotSchemaUpdated(const karabo::util::Schema& description, const std::string& deviceId);

            void logHandler(karabo::net::BrokerChannel::Pointer channel, const std::string& logMessage, const karabo::util::Hash& header);

            void slotNotification(const std::string& type, const std::string& shortMessage, const std::string& detailedMessage, const std::string& deviceId);



        };

    }
}

#endif
