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

        public:

            KARABO_CLASSINFO(GuiServerDevice, "GuiServerDevice", "1.0")

            static void expectedParameters(karabo::util::Schema& expected);

            GuiServerDevice(const karabo::util::Hash& input);

            virtual ~GuiServerDevice() {
            }


            void okStateOnEntry();


        private: // Members

            karabo::net::IOService::Pointer m_ioService;
            karabo::net::Connection::Pointer m_dataConnection;

            karabo::io::TextSerializer<karabo::util::Hash>::Pointer m_textSerializer;
            karabo::io::BinarySerializer<karabo::util::Hash>::Pointer m_binarySerializer;
            std::map<karabo::net::Channel::Pointer, std::set<std::string> > m_channels;
            boost::mutex m_channelMutex;

            karabo::net::BrokerConnection::Pointer m_loggerConnection;
            karabo::net::BrokerIOService::Pointer m_loggerIoService;
            karabo::net::BrokerChannel::Pointer m_loggerChannel;



        private: // Functions

            void onError(karabo::net::Channel::Pointer channel, const karabo::net::ErrorCode& errorMessage);

            void onConnect(karabo::net::Channel::Pointer channel);

            void onRead(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header, const std::string& body);

            void onLogin(karabo::net::Channel::Pointer channel, const std::string& body);

            void onRefreshInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header);

            void onReconfigure(const karabo::util::Hash& header, const std::string& body);

            void onInitDevice(const karabo::util::Hash& header, const std::string& body);

            void onKillDeviceServerInstance(const karabo::util::Hash& header, const std::string& body);

            void onKillDeviceInstance(const karabo::util::Hash& header, const std::string& body);

            void onCreateNewDeviceClassPlugin(const karabo::util::Hash& header, const std::string& body);

            void onSlotCommand(const karabo::util::Hash& header, const std::string& body);

            void onNewVisibleDeviceInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header);

            void onRemoveVisibleDeviceInstance(karabo::net::Channel::Pointer channel, const karabo::util::Hash& header);

            void sendCurrentIds(karabo::net::Channel::Pointer channel);

            void registerConnect(const karabo::net::Channel::Pointer& channel);

            void slotNewNode(const karabo::util::Hash& row);

            void slotNewDeviceServerInstance(const karabo::util::Hash& row);

            void slotNewDeviceClass(const karabo::util::Hash& row);

            void slotNewDeviceInstance(const karabo::util::Hash& row);

            void slotUpdateDeviceServerInstance(const karabo::util::Hash& row);

            void slotUpdateDeviceInstance(const karabo::util::Hash& row);

            //            void slotNoTransition(const std::string& what, const std::string& who);
            //
            //            void slotBadReconfiguration(const std::string& what, const std::string& who);
            //
            //            void slotConnected(const std::string& signal, const std::string& slot);
            //
            void slotChanged(const karabo::util::Hash& what, const std::string& instanceId, const std::string& classId);

            void onLog(karabo::net::BrokerChannel::Pointer channel, const std::string& logMessage, const karabo::util::Hash& header);

            void slotErrorFound(const std::string& timeStamp, const std::string& shortMessage, const std::string& detailedMessage, const std::string& instanceId);

            void slotWarning(const std::string& timeStamp, const std::string& warnMessage, const std::string& instanceId, const std::string& priority);

            void slotAlarm(const std::string& timeStamp, const std::string& alarmMessage, const std::string& instanceId, const std::string& priority);

            void slotSchemaUpdatedToGui(const std::string& schema, const std::string& instanceId, const std::string& classId);

        };

    }
}

#endif	/* KARABO_GUISERVER_GUISERVERDEVICE_HH */
