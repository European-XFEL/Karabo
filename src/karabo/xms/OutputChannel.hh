/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_NETWORKOUTPUT_HH
#define	KARABO_XMS_NETWORKOUTPUT_HH

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <karabo/util.hpp>
#include <karabo/log.hpp>
#include <karabo/io.hpp>
#include <karabo/net.hpp>

#include "Statics.hh"
#include "Memory.hh"
#include "InputChannel.hh"


/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xms {

#define KARABO_P2P_SOURCE_INFO "__sourceInfo"

        /**
         * @class OutputChannel
         * @brief An OutputChannel for passing data to pipelined processing
         * 
         * The OutputChannel class is used for writing data to pipelined processing
         * inputs. It supports tracking of meta data for each data token written to it.
         * Specifically, it e.g. allows for keeping track of data producers, here called
         * sources, and timing and train information.
         * Meta data information enables aggregation of multiple data source into one
         * output channel interaction with a remote host, as well as aggregation of
         * multiple train-related data of the same source. A mixture of both scenarios
         * is possible.
         * 
         * An example of these use cases
         * 
         * @code
         * 
         * OutputChannel::Pointer output = ... // 
         * 
         * Hash data1;
         * ....
         * OutputChannel::MetaData meta1("THIS/IS/SOURCE/A/channel1", karabo::util::Timestamp());
         * output->write(data1, meta1)
         * 
         * Hash data2_10;
         * ....
         * OutputChannel::MetaData meta2_10("THIS/IS/SOURCE/B/channel2", timestampForTrain10);
         * output->write(data2_10, meta2)
         * OutputChannel::MetaData meta2_11("THIS/IS/SOURCE/B/channel2", timestampForTrain11);
         * output->write(data2_11, meta2_11)
         *
         * Hash data_this_source;
         * ...
         * // not passing any meta data to write will default the source to [deviceId]/[channelName]
         * // and the timestamp to the current timestamp
         * output->write(data_this_source);
         * 
         * // now actually send over the network
         * output->update();
         * @endcode
         */
        class OutputChannel : public boost::enable_shared_from_this<OutputChannel> {


            /*
             * InputChannelInfo (karabo::util::Hash)
             *
             *     instanceId (std::string)
             *     memoryLocation (std::string) [local/remote]
             *     tcpChannel (karabo::net::Channel::Pointer)
             *     onSlowness (std::string) [queue/drop/wait/throw]
             *     queuedChunks (std::deque<int>)
             *
             */
            typedef karabo::util::Hash InputChannelInfo;

            typedef std::vector<InputChannelInfo> InputChannels;

            typedef std::deque< std::string > InputChannelQueue;

            // Callback on available input
            boost::function<void (const boost::shared_ptr<OutputChannel>&) > m_ioEventHandler;

            std::string m_instanceId;
            std::string m_channelName;

            // Server related
            std::string m_hostname;
            unsigned int m_port;
            int m_compression;

            karabo::net::Connection::Pointer m_dataConnection;

            std::string m_onNoSharedInputChannelAvailable;
            std::string m_distributionMode;

            mutable boost::mutex m_registeredInputsMutex;
            InputChannels m_registeredSharedInputs;
            InputChannels m_registeredCopyInputs;

            unsigned int m_sharedInputIndex;

            InputChannelQueue m_shareNext;
            InputChannelQueue m_copyNext;

            mutable boost::mutex m_nextInputMutex;

            unsigned int m_channelId;
            unsigned int m_chunkId;

        public:
            typedef Memory::MetaData MetaData;

        public:

            KARABO_CLASSINFO(OutputChannel, "OutputChannel", "1.0");

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected);

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            OutputChannel(const karabo::util::Hash& config);

            virtual ~OutputChannel();

            void setInstanceIdAndName(const std::string& instanceId, const std::string& name);

            const std::string& getInstanceId() const;

            /**
             * Get ids of all input channels currently connected
             *
             * @return vector of strings with the ids
             */
            std::vector<std::string> getRegisteredInputChannels() const;

            void registerIOEventHandler(const boost::function<void (const OutputChannel::Pointer&)>& ioEventHandler);


            karabo::util::Hash getInformation() const;

            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens asynchronously.
             * @param data input Hash object
             * @param metaData a MetaData object containing meta data for this data token.
             * @param copyAllData If false, serialization is optimized to avoid copies for big data.
             * 
             * Note: when using copyAllData==false, data must stay untouched and in scope until update() has been
             * called for the channel.
             */
            void write(const karabo::util::Hash& data, const Memory::MetaData& metaData, bool copyAllData=true);
            
            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens asynchronously.
             * Metadata is initialized to default values. Namely the sending devices device id and the output channel's
             * name are used as data source.
             * @param data input Hash object
             * @param copyAllData If false, serialization is optimized to avoid copies for big data.
             * 
             * Note: when using copyAllData==false, data must stay untouched and in scope until update() has been
             * called for the channel.
             */
            void write(const karabo::util::Hash& data, bool copyAllData=true);

            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens asynchronously.
             * @param data shared pointer to input Hash object
             * @param metaData a MetaData object containing meta data for this data token.
             */
            KARABO_DEPRECATED void write(const karabo::util::Hash::Pointer& data, const Memory::MetaData& metaData);
            
            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens asynchronously.
             * Metadata is initialized to default values. Namely the sending devices device id and the output channel's
             * name are used as data source.
             * @param data shared pointer to input Hash object
             */
            KARABO_DEPRECATED void write(const karabo::util::Hash::Pointer& data);

            /**
             * Update the output channel, i.e. send all data over the wire that was previously written
             * by calling write(...).
             */
            void update();

            void signalEndOfStream();

        private:

            void initializeServerConnection(int countdown);

            void onTcpConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

            // TODO Implement this !!!!

            //void onTcpConnectionError(const karabo::net::Connection::Pointer& conn, const karabo::net::ErrorCode& error);

            void onTcpChannelError(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

            void onTcpChannelRead(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, const karabo::util::Hash& message);

            void onInputAvailable(const std::string& instanceId);

            void triggerIOEvent();

            void onInputGone(const karabo::net::Channel::Pointer& channel);

            /// Requires protection of m_registeredInputsMutex
            void distributeQueue(karabo::util::Hash& channelInfo);

            /// Requires protection of m_registeredInputsMutex
            void copyQueue(karabo::util::Hash& channelInfo);

            void pushShareNext(const std::string& instanceId);

            std::string popShareNext();

            bool isShareNextEmpty() const;

            bool hasSharedInput(const std::string& instanceId);

            void eraseSharedInput(const std::string& instanceId);

            void pushCopyNext(const std::string& info);

            std::string popCopyNext();

            bool hasCopyInput(const std::string& instanceId);

            void eraseCopyInput(const std::string& instanceId);

            // TODO Check if needed
            bool canCompute() const;

            void registerWritersOnChunk(unsigned int chunkId);

            void unregisterWriterFromChunk(int chunkId);

            void distribute(unsigned int chunkId);

            /**
             * Get index of next one of the shared inputs.
             *
             * Requires protection of m_registeredInputsMutex
             */
            unsigned int getNextSharedInputIdx();

            /// Requires protection of m_registeredInputsMutex
            void distributeLocal(unsigned int chunkId, const InputChannelInfo & channelInfo);

            /// Requires protection of m_registeredInputsMutex
            void distributeRemote(const unsigned int& chunkId, const InputChannelInfo & channelInfo);

            void copy(unsigned int chunkId);

            /// Requires protection of m_registeredInputsMutex
            void copyLocal(const unsigned int& chunkId, const InputChannelInfo & channelInfo);

            /// Requires protection of m_registeredInputsMutex
            void copyRemote(const unsigned int& chunkId, const InputChannelInfo & channelInfo);

            /// Provide a string identifying this output channel (useful in DEBUG logging)
            std::string debugId() const;



        };

        class OutputChannelElement {

            karabo::util::NodeElement m_outputChannel;
            karabo::util::NodeElement m_dataSchema;

        public:

            OutputChannelElement(karabo::util::Schema& s) : m_outputChannel(s), m_dataSchema(s) {
                m_outputChannel.appendParametersOf<OutputChannel>();
            }

            OutputChannelElement& key(const std::string& key) {
                m_outputChannel.key(key);
                m_dataSchema.key(key + ".schema");
                return *(static_cast<OutputChannelElement*> (this));
            }

            OutputChannelElement& displayedName(const std::string& name) {
                m_outputChannel.displayedName(name);
                return *(static_cast<OutputChannelElement*> (this));
            }

            OutputChannelElement& description(const std::string& description) {
                m_outputChannel.description(description);
                return *(static_cast<OutputChannelElement*> (this));
            }

            OutputChannelElement& dataSchema(const karabo::util::Schema& schema) {
                m_dataSchema.appendSchema(schema);
                return *(static_cast<OutputChannelElement*> (this));
            }

            void commit() {
                m_outputChannel.commit();
                m_dataSchema.commit();
            }

        };

        typedef OutputChannelElement OUTPUT_CHANNEL_ELEMENT;
        typedef OutputChannelElement OUTPUT_CHANNEL;

    }
}

#endif
