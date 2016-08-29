/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 *
 * Modified to new concepts: April 17, 2015
 */

#ifndef KARABO_XMS_INPUTCHANNEL_HH
#define	KARABO_XMS_INPUTCHANNEL_HH

#include <boost/asio.hpp>
#include <karabo/net.hpp>
#include <karabo/io.hpp>
#include <karabo/log.hpp>
#include <karabo/util.hpp>
#include "Data.hh"
#include "Memory.hh"

/**
 * The main Karabo namespace
 */
namespace karabo {

    namespace xms {

        /**
         * The InputChannel class.
         */
        class InputChannel : public boost::enable_shared_from_this<InputChannel> {

            // Maps outputChannelString to the Hash with connection parameters
            typedef std::map<std::string, karabo::util::Hash> ConnectedOutputChannels;
            // Maps outputChannelString to the TCP (connection, channel) pair
            typedef std::map<std::string, std::pair<karabo::net::Connection::Pointer, karabo::net::Channel::Pointer> > OpenConnections;
            typedef Memory<karabo::util::Hash> MemoryType;

            boost::asio::deadline_timer m_deadline;
            
            /// Callback on available data (per InputChannel)
            boost::function<void (const boost::shared_ptr<InputChannel>&) > m_inputHandler;

            /// Callback on available data (per Data item in InputChannel)
            boost::function<void (const Data&) > m_dataHandler;

            // Callback on end-of-stream
            boost::function<void (const boost::shared_ptr<InputChannel>&) > m_endOfStreamHandler;

            std::string m_instanceId;

            std::string m_dataDistribution;
            unsigned int m_minData;
            bool m_keepDataUntilNew;
            std::string m_onSlowness;

            unsigned int m_channelId;

            boost::mutex m_mutex;
            boost::mutex m_swapBuffersMutex;

            int m_activeChunk;
            int m_inactiveChunk;

            karabo::net::IOService::Pointer m_tcpIoService;
            boost::thread m_tcpIoServiceThread;

            boost::mutex m_outputChannelsMutex;
            ConnectedOutputChannels m_connectedOutputChannels;
            OpenConnections m_openConnections;

            bool m_isEndOfStream;
            bool m_respondToEndOfStream;

            // Tracks channels that send EOS
            std::set<karabo::net::Channel::Pointer> m_eosChannels;

            int m_delayOnInput;

        public:

            KARABO_CLASSINFO(InputChannel, "InputChannel", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected);



            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            InputChannel(const karabo::util::Hash& config);

            virtual ~InputChannel();




            void reconfigure(const karabo::util::Hash& config);

            void setInstanceId(const std::string& instanceId);

            const std::string& getInstanceId() const;

            void registerInputHandler(const boost::function<void (const Self::Pointer&)>& ioInputHandler);

            void registerDataHandler(const boost::function<void (const Data&) >& ioDataHandler);

            void registerEndOfStreamEventHandler(const boost::function<void (const Self::Pointer&)>& endOfStreamEventHandler);

            void triggerIOEvent();

            void triggerEndOfStreamEvent();

            /**
             * Returns a map of between  "output channel string" and "output channel info" Hash
             * outputChannelString (STRING) represented like "instanceId@channelName" or "instanceId:channelName"
             * outputChannelInfo contains connection parameters or is empty, depending on connection state.
             * @return map.
             */
            std::map<std::string, karabo::util::Hash> getConnectedOutputChannels();

            void read(karabo::util::Hash& data, size_t idx = 0);

            karabo::util::Hash::Pointer read(size_t idx = 0);

            template <class T>
            T readData(size_t idx = 0) {
                boost::mutex::scoped_lock lock(m_swapBuffersMutex);
                return T(MemoryType::read(idx, m_channelId, m_activeChunk));
            }

            size_t size();

            unsigned int getMinimumNumberOfData() const;

            void connect(const karabo::util::Hash& outputChannelInfo);

            void disconnect(const karabo::util::Hash& outputChannelInfo);

            void disconnect(const std::string& connectionString);

            karabo::util::Hash prepareConnectionConfiguration(const karabo::util::Hash& outputChannelInfo) const;

            void onConnect(const karabo::net::ErrorCode& error,
                           karabo::net::Connection::Pointer connection,
                           const karabo::util::Hash& outputChannelInfo,
                           karabo::net::Channel::Pointer channel);

            // TODO Keep m_connectedOutputChannels in sync and adapt eos tokens on sudden death
            void startConnectionAsync(karabo::net::Connection::Pointer connection, const karabo::util::Hash& outputChannelInfo);

            void onTcpConnectionError(const karabo::net::ErrorCode&, const karabo::net::Connection::Pointer&);

            void onTcpChannelError(const karabo::net::ErrorCode&, const karabo::net::Channel::Pointer&);

            void onTcpChannelRead(const karabo::net::ErrorCode& ec, karabo::net::Channel::Pointer channel,
                                  const karabo::util::Hash& header, const std::vector<char>& data);

            void swapBuffers();

            bool canCompute() const;

            void update();

            void notifyOutputChannelsForPossibleRead();

            void notifyOutputChannelForPossibleRead(const karabo::net::Channel::Pointer& channel);

            bool respondsToEndOfStream();

            void parseOutputChannelConfiguration(const karabo::util::Hash& config);

            void updateOutputChannelConfiguration(const std::string& outputChannelString, const karabo::util::Hash& config);

        private: // functions

            void deferredNotificationsOfOutputChannelsForPossibleRead();

            void deferredNotificationOfOutputChannelForPossibleRead(const karabo::net::Channel::Pointer& channel);

            bool needsDeviceConnection() const;

            void closeChannelsAndStopConnections();
        };

        class InputChannelElement {

            karabo::util::NodeElement m_inputChannel;
            karabo::util::NodeElement m_dataSchema;

        public:

            InputChannelElement(karabo::util::Schema& s) : m_inputChannel(s), m_dataSchema(s) {
                m_inputChannel.appendParametersOf<InputChannel>();
            }

            InputChannelElement& key(const std::string& key) {
                m_inputChannel.key(key);
                m_dataSchema.key(key + ".schema");
                return *(static_cast<InputChannelElement*> (this));
            }

            InputChannelElement& displayedName(const std::string& name) {
                m_inputChannel.displayedName(name);
                return *(static_cast<InputChannelElement*> (this));
            }

            InputChannelElement& description(const std::string& description) {
                m_inputChannel.description(description);
                return *(static_cast<InputChannelElement*> (this));
            }

            InputChannelElement& dataSchema(const karabo::util::Schema& schema) {
                m_dataSchema.appendSchema(schema);
                return *(static_cast<InputChannelElement*> (this));
            }

            void commit() {
                m_inputChannel.commit();
                m_dataSchema.commit();
            }

        };

        typedef InputChannelElement INPUT_CHANNEL_ELEMENT;
        typedef InputChannelElement INPUT_CHANNEL;


    }
}

#endif
