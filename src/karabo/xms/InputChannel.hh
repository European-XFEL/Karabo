/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Modified to new concepts: April 17, 2015
 */

#ifndef KARABO_XMS_INPUTCHANNEL_HH
#define KARABO_XMS_INPUTCHANNEL_HH

#include <boost/asio.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>

#include "Memory.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/Strand.hh"
#include "karabo/net/utils.hh"

/**
 * The main Karabo namespace
 */
namespace karabo {

    namespace xms {

        /**
         * @class InputChannel
         * @brief The InputChannel class is used to receive data from pipelined
         *        processing OutputChannels.
         *
         * The InputChannel class is used to receive data from pipelined
         * processing OutputChannels. It additionally supports receiving
         * meta data associated with each data token read. Specifically, the
         * meta information contains source information, i.e. what produced the
         * data token, and timing information, e.g. train ids.
         *
         * @code
         * void onInput(const InputChannel::Pointer& input) {
         *
         *      for (unsigned int i = 0; i != input->size(); ++i) {
         *           Hash h;
         *           const InputChannel::MetaData& meta = input->read(i);
         *           std::cout<<"Source: <<meta.getSource()<<std::endl;
         *           std::cout<<"TrainId: <<meta.getTimestamp().getTrainId()<<std::endl;
         *      }
         * }
         * @endcode
         */
        class InputChannel : public std::enable_shared_from_this<InputChannel> {
           public:
            KARABO_CLASSINFO(InputChannel, "InputChannel", "1.0")

            typedef Memory::MetaData MetaData;
            typedef std::function<void(const InputChannel::Pointer&)> InputHandler;
            typedef std::function<void(const karabo::data::Hash&, const MetaData&)> DataHandler;
            using ConnectionTracker = std::function<void(const std::string&, net::ConnectionStatus)>;

            /**
             * Container for InputChannel handlers that concern data handling.
             */
            struct Handlers {
                Handlers(){
                      // members are correctly initialised by their default empty constructors
                };

                /**
                 * Construct with data and end-of-stream handlers - input handler could be specified afterwards
                 */
                explicit Handlers(const DataHandler& data, const InputHandler& eos = InputHandler())
                    : dataHandler(data), inputHandler(), eosHandler(eos){};

                /**
                 * Construct with input and end-of-stream handlers - data handler could be specified afterwards
                 */
                explicit Handlers(const InputHandler& input, const InputHandler& eos = InputHandler())
                    : dataHandler(), inputHandler(input), eosHandler(eos){};

                DataHandler dataHandler;
                InputHandler inputHandler;
                InputHandler eosHandler;
            };

            // Default maximum queue length on a connected output channel before
            // it starts dropping data or waiting for sending data. The max
            // queue length only comes into play when the input channel is
            // is connected in "copy" mode with "queue" or "queueDrop" for its
            // "onSlowness" policy.
            static const unsigned int DEFAULT_MAX_QUEUE_LENGTH;

           private:
            // Maps outputChannelString to the Hash with connection parameters
            typedef std::map<std::string, karabo::data::Hash> ConfiguredOutputChannels;
            // Maps outputChannelString to the TCP (connection, channel) pair
            typedef std::map<std::string, std::pair<karabo::net::Connection::Pointer, karabo::net::Channel::Pointer>>
                  OpenConnections;

            karabo::net::Strand::Pointer m_strand;
            boost::asio::steady_timer m_deadline;

            /// Callback on available data (per InputChannel)
            InputHandler m_inputHandler;

            /// Callback on available data (per item in InputChannel)
            DataHandler m_dataHandler;

            // Callback on end-of-stream
            InputHandler m_endOfStreamHandler;

            ConnectionTracker m_connectionTracker;
            karabo::net::Strand::Pointer m_connectStrand; // for handlers concerning connection setup/status

            std::string m_instanceId;

            std::string m_dataDistribution;
            unsigned int m_minData;
            std::string m_onSlowness;
            unsigned int m_maxQueueLength;

            unsigned int m_channelId;

            // Prevents simultaneous access to the inactive data and active data pots.
            std::mutex m_twoPotsMutex;

            int m_activeChunk;
            int m_inactiveChunk;

            std::mutex m_outputChannelsMutex;
            ConfiguredOutputChannels m_configuredOutputChannels;
            OpenConnections m_openConnections;
            /// All 'outputChannelString' for that a connection attempt is currently ongoing, with their handlers and
            /// ids
            std::unordered_map<std::string, std::pair<unsigned int, std::function<void(const karabo::net::ErrorCode&)>>>
                  m_connectionsBeingSetup;
            boost::random::mt19937 m_random; // any random generator would suite...

            bool m_respondToEndOfStream;

            // Tracks channels that sent EOS - to be protected by m_outputChannelsMutex
            std::set<karabo::net::Channel::WeakPointer, std::owner_less<karabo::net::Channel::WeakPointer>>
                  m_eosChannels;

            int m_delayOnInput;

            std::vector<MetaData> m_metaDataList;
            std::vector<karabo::data::Hash::Pointer> m_dataList;
            std::multimap<std::string, unsigned int> m_sourceMap;
            std::multimap<unsigned long long, unsigned int> m_trainIdMap;
            std::map<unsigned int, MetaData> m_reverseMetaDataMap;

           public:
            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::data::Schema& expected);

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            InputChannel(const karabo::data::Hash& config);

            virtual ~InputChannel();

            /**
             * Reconfigure InputChannel
             * Disconnects any previous "connectedOutputChannels" if not in config["connectedOutputChannels"].
             *
             * @param config as needed by the constructor
             * @param allowMissing if true, lack of keys "dataDistribution", "minData", "onSlowness", "delayOnInput"
             *                     and "respondToEndOfStream" is OK and their respective previous configuration is kept,
             *                     if false, an exception is thrown when these keys are missing in config
             */
            void reconfigure(const karabo::data::Hash& config, bool allowMissing = true);

            void setInstanceId(const std::string& instanceId);

            const std::string& getInstanceId() const;

            /**
             * Register handler to be called when new data has arrived.
             * For each index i from 0 to < size(), data and meta data can be received via read(i, dataHash)
             * and readMetaData(i, metaData), respectively.
             *
             * Note: The internal variable that stores the handler is neither protected against concurrent calls
             *       to getRegisteredHandlers() nor to concurrent usage of the handler when data arrives, i.e.
             *       this registration must not be called if (being) connected to any output channel.
             */
            void registerInputHandler(const InputHandler& ioInputHandler);

            /**
             * Register handler to be called for each data item that arrives.
             *
             * Note: The internal variable that stores the handler is neither protected against concurrent calls
             *       to getRegisteredHandlers() nor to concurrent usage of the handler when data arrives, i.e.
             *       this registration must not be called if (being) connected to any output channel.
             */
            void registerDataHandler(const DataHandler& ioDataHandler);

            /**
             * Register handler to be called when connected output channels inform about end-of-stream.
             * If connected to more than one output channel, the handler is called if the last of them sends the
             * end-of-stream signal.
             *
             * Note: The internal variable that stores the handler is neither protected against concurrent calls
             *       to getRegisteredHandlers() nor to concurrent usage of the handler when the end-of-stream signal
             * arrives, i.e. this registration must not be called if (being) connected to any outpu channel.
             */
            void registerEndOfStreamEventHandler(const InputHandler& endOfStreamEventHandler);

            void registerConnectionTracker(const ConnectionTracker& tracker);

            /**
             * Get handlers registered for data, input and end-of-stream handling.
             *
             * Do not call concurrrently with the corresponding register[Data|Input|EndOfStreamEvent]Handler() methods.
             */
            Handlers getRegisteredHandlers() const;

           private:
            void triggerIOEvent();

           public:
            /**
             * Returns the number of bytes read since the last call of this method
             *
             * See karabo::util::TcpChannel::dataQuantityRead()
             */
            size_t dataQuantityRead();

            /**
             * Returns the number of bytes written since the last call of this method
             *
             * See karabo::util::TcpChannel::dataQuantityWritten()
             */
            size_t dataQuantityWritten();

            /**
             * Returns a map of between  "output channel string" and "output channel info" Hash
             * outputChannelString (STRING) represented like "instanceId:channelName"
             * outputChannelInfo contains connection parameters or is empty, depending on connection state.
             * This contains all output channels that the InputChannel is configured for, irrespective whether
             * currently connected or not.
             * @return map.
             */
            std::map<std::string, karabo::data::Hash> getConnectedOutputChannels();

            /**
             * Provide a map between the output channels that are configured  and their connection status.
             * @return map
             */
            std::unordered_map<std::string, karabo::net::ConnectionStatus> getConnectionStatus();

            /**
             * Read data from the InputChannel - to be called inside an InputHandler callback
             *
             * Kept for backward compatibility only since internally the data is copied!
             * Use one of the other read methods instead.
             *
             * @param data reference that will hold the data
             * @param idx of the data token to read from the available data tokens. Use InputChannel::size to request
             *            number of available tokens
             * @return meta data associated to the data token. Lifetime of the object corresponds to live time of the
             *         InputHandler callback.
             */
            const MetaData& read(karabo::data::Hash& data, size_t idx = 0);

            /**
             * Read data from the InputChannel - to be called inside an InputHandler callback
             *
             * @param idx of the data token to read from the available data tokens. Use InputChannel::size to request
             *            number of available tokens
             * @return the data as a pointer
             */
            karabo::data::Hash::Pointer read(size_t idx = 0);

            /**
             * Read data and meta data from the InputChannel - to be called inside an InputHandler callback
             *
             * @param idx of the data token to read from the available data tokens. Use InputChannel::size to request
             *            number of available tokens
             * @param source reference that will hold the meta data
             * @return the data as a pointer
             */
            karabo::data::Hash::Pointer read(size_t idx, MetaData& source);

            /**
             * Number of data tokens - to be called inside an InputHandler callback
             */
            size_t size();

            unsigned int getMinimumNumberOfData() const;

            /**
             * Asynchronously connect this input channel to the output channel described by the first argument
             *
             * @param outputChannelInfo  Hash with three keys
             *                   - "outputChannelString": a string matching one of the configured
             * "connectedOutputChannels"
             *                   - "connectionType": a string - currently only "tcp" supported
             *                   - "hostname": a string telling which host/ip address to connect to
             *                   - "port": an unsigned int telling the port
             *                   - "memoryLocation: string "remote" or "local" to tell whether other end is in another
             *                                      process or can share memory
             * @param handler  indicates asynchronously (like via EventLoop::post) the success of the connection request
             */
            void connect(const karabo::data::Hash& outputChannelInfo,
                         const std::function<void(const karabo::net::ErrorCode&)>& handler =
                               std::function<void(const karabo::net::ErrorCode&)>());

            void disconnect(const karabo::data::Hash& outputChannelInfo);

            /**
             * Disconnect and clean internals
             *
             * @param connectionString One of the "connectedOutputChannels" given at construction
             */
            void disconnect(const std::string& connectionString);

           private:
            karabo::data::Hash prepareConnectionConfiguration(const karabo::data::Hash& outputChannelInfo) const;

            static void onConnectWrap(WeakPointer self, karabo::net::ErrorCode error,
                                      karabo::net::Connection::Pointer connection,
                                      const karabo::data::Hash& outputChannelInfo,
                                      karabo::net::Channel::Pointer channel, unsigned int connectId,
                                      const std::function<void(const karabo::net::ErrorCode&)>& handler);

            void onConnect(karabo::net::ErrorCode error, karabo::net::Connection::Pointer& connection,
                           const karabo::data::Hash& outputChannelInfo, karabo::net::Channel::Pointer& channel,
                           unsigned int connectId, const std::function<void(const karabo::net::ErrorCode&)>& handler);

            void onTcpChannelError(const karabo::net::ErrorCode&, const karabo::net::Channel::Pointer&);

            void onTcpChannelRead(const karabo::net::ErrorCode& ec, karabo::net::Channel::WeakPointer channel,
                                  const karabo::data::Hash& header,
                                  const std::vector<karabo::data::BufferSet::Pointer>& data);

            void notifyOutputChannelsForPossibleRead();

            void notifyOutputChannelForPossibleRead(const karabo::net::Channel::WeakPointer& channel);

            bool respondsToEndOfStream();

            void parseOutputChannelConfiguration(const karabo::data::Hash& config);

            void postConnectionTracker(const std::string& outputChannel, karabo::net::ConnectionStatus status);

           public:
            /**
             * Update list of output channels that can be connected
             *
             * @param outputChannelString string that can later be used as key "outputChannelString" of Hash argument to
             * connect
             * @param config kept for backward compatibility
             */
            void updateOutputChannelConfiguration(const std::string& outputChannelString,
                                                  const karabo::data::Hash& config = karabo::data::Hash());

            /**
             * Get the current meta data for input data available on this input channel. Validity time of the object
             * corresponds to lifetime of the InputHandler callback. Also the InputHandler this is called in needs
             * to have been registered using registerInputHandler.
             * @return
             */
            const std::vector<MetaData>& getMetaData() const;

            /**
             * Return the list of indices of the data tokens (for read(index) ) for a given source identifier.
             * Multiple indices may be returned if the same source was appended more than once in one batch write.
             * Indices increase monotonically in insertion order of the write operations. Validity time of the indices
             * corresponds to lifetime of the InputHandler callback. Also the InputHandler this is called in needs
             * to have been registered using registerInputHandler.
             * @param source
             * @return
             */
            std::vector<unsigned int> sourceToIndices(const std::string& source) const;

            /**
             * Return the list of indices of the data tokens (for read(index) ) for a given train id.
             * Multiple indices may be returned if the same source was appended more than once in one batch write.
             * Indices increase monotonically in insertion order of the write operations. Validity time of the indices
             * corresponds to lifetime of the InputHandler callback. Also the InputHandler this is called in needs
             * to have been registered using registerInputHandler.
             * @param source
             * @return
             */
            std::vector<unsigned int> trainIdToIndices(unsigned long long trainId) const;

            /**
             * Return the data source identifier pertinent to a data token at a given index. Validity time of the object
             * corresponds to lifetime of the InputHandler callback. Also the InputHandler this is called in needs
             * to have been registered using registerInputHandler.
             * @param index
             * @return
             * @throw A KARABO_LOGIC_EXCEPTION of there is no meta data available for the given index.
             */
            const MetaData& indexToMetaData(unsigned int index) const;


           private: // functions
            /**
             * Disconnect internals - needs protection by m_outputChannelsMutex
             *
             * @param outputChannelString One of the "connectedOutputChannels" given at construction
             */
            void disconnectImpl(const std::string& outputChannelString);

            void deferredNotificationsOfOutputChannelsForPossibleRead();

            void deferredNotificationOfOutputChannelForPossibleRead(const karabo::net::Channel::WeakPointer& channel);

            void disconnectAll();

            void prepareData();
        };

        class InputChannelElement {
            karabo::data::NodeElement m_inputChannel;

           public:
            InputChannelElement(karabo::data::Schema& s) : m_inputChannel(s) {
                m_inputChannel.appendParametersOf<InputChannel>();
            }

            InputChannelElement& key(const std::string& key) {
                m_inputChannel.key(key);
                return *this;
            }

            InputChannelElement& displayedName(const std::string& name) {
                m_inputChannel.displayedName(name);
                return *this;
            }

            InputChannelElement& description(const std::string& description) {
                m_inputChannel.description(description);
                return *this;
            }

            [[deprecated(
                  "dataSchema does nothing since Karabo 2.19 and will be removed in the future")]] InputChannelElement&
            dataSchema(const karabo::data::Schema& schema) {
                return *this;
            }

            void commit();
        };

        typedef InputChannelElement INPUT_CHANNEL_ELEMENT;
        typedef InputChannelElement INPUT_CHANNEL;


    } // namespace xms
} // namespace karabo

#endif
