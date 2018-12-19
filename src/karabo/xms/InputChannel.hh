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

#include <map>
#include <boost/asio.hpp>
#include <boost/function.hpp>

#include "karabo/net.hpp"
#include "karabo/io.hpp"
#include "karabo/log.hpp"
#include "karabo/util.hpp"
#include "Memory.hh"

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
         *      for(unsigned int i = 0; i != input->size(); ++i) {
         *           Hash h;
         *           const InputChannel::MetaData& meta = input->read(i);
         *           std::cout<<"Source: <<meta.getSource()<<std::endl;
         *           std::cout<<"TrainId: <<meta.getTimestamp().getTrainId()<<std::endl;
         *      }
         * }
         * @endcode
         */
        class InputChannel : public boost::enable_shared_from_this<InputChannel> {

        public:

            KARABO_CLASSINFO(InputChannel, "InputChannel", "1.0")

            typedef Memory::MetaData MetaData;
            typedef boost::function<void (const InputChannel::Pointer&) > InputHandler;
            typedef boost::function<void (const karabo::util::Hash&, const MetaData&) > DataHandler;


        private:
            // Maps outputChannelString to the Hash with connection parameters
            typedef std::map<std::string, karabo::util::Hash> ConnectedOutputChannels;
            // Maps outputChannelString to the TCP (connection, channel) pair
            typedef std::map<std::string, std::pair<karabo::net::Connection::Pointer, karabo::net::Channel::Pointer> > OpenConnections;

            karabo::net::Strand::Pointer m_strand;
            boost::asio::deadline_timer m_deadline;

            /// Callback on available data (per InputChannel)
            InputHandler m_inputHandler;

            /// Callback on available data (per item in InputChannel)
            DataHandler m_dataHandler;

            // Callback on end-of-stream
            InputHandler m_endOfStreamHandler;

            std::string m_instanceId;

            std::string m_dataDistribution;
            unsigned int m_minData;
            std::string m_onSlowness;

            unsigned int m_channelId;

            boost::mutex m_inactiveDataMutex;
            boost::mutex m_activeDataMutex;

            int m_activeChunk;
            int m_inactiveChunk;

            boost::mutex m_outputChannelsMutex;
            ConnectedOutputChannels m_connectedOutputChannels;
            OpenConnections m_openConnections;

            boost::mutex m_isEndOfStreamMutex;
            bool m_isEndOfStream;
            bool m_respondToEndOfStream;

            // Tracks channels that send EOS
            std::set<karabo::net::Channel::Pointer> m_eosChannels;

            int m_delayOnInput;

            std::vector<MetaData> m_metaDataList;
            std::multimap<std::string, unsigned int> m_sourceMap;
            std::multimap<unsigned long long, unsigned int> m_trainIdMap;
            std::map<unsigned int, MetaData> m_reverseMetaDataMap;
            
        public:

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

            void registerInputHandler(const InputHandler& ioInputHandler);

            void registerDataHandler(const DataHandler& ioDataHandler);

            void registerEndOfStreamEventHandler(const InputHandler& endOfStreamEventHandler);

            void triggerIOEvent();

            void triggerEndOfStreamEvent();

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
             * outputChannelString (STRING) represented like "instanceId@channelName" or "instanceId:channelName"
             * outputChannelInfo contains connection parameters or is empty, depending on connection state.
             * @return map.
             */
            std::map<std::string, karabo::util::Hash> getConnectedOutputChannels();

            /**
             * Read data from the InputChannel
             * @param data reference that will hold the data
             * @param idx of the data token to read from the available data tokens. Use InputChannel::size to request number
             *        of available tokens
             * @return meta data associated to the data token. Lifetime of the object corresponds to live time of the InputHandler callback.
             */
            const MetaData& read(karabo::util::Hash& data, size_t idx = 0);

            karabo::util::Hash::Pointer read(size_t idx = 0);

            karabo::util::Hash::Pointer read(size_t idx, MetaData& source);

            size_t size();

            unsigned int getMinimumNumberOfData() const;

            /**
             * Asynchronously connect this input channel to the output channel described by the first argument
             * @param outputChannelInfo  Hash with three keys
             *                   - "connectionType": a string - currently only "tcp" supported
             *                   - "hostname": a string
             *                   - "port": an unsigned int
             * @param handler  indicates asynchronously (like via EventLoop::post) the success of the connection request
             */
            void connect(const karabo::util::Hash& outputChannelInfo,
                         const boost::function<void (const karabo::net::ErrorCode&)>& handler
                         = boost::function<void (const karabo::net::ErrorCode&)>());

            void disconnect(const karabo::util::Hash& outputChannelInfo);

            void disconnect(const std::string& connectionString);

            karabo::util::Hash prepareConnectionConfiguration(const karabo::util::Hash& outputChannelInfo) const;

            void onConnect(const karabo::net::ErrorCode& error,
                           karabo::net::Connection::Pointer connection,
                           const karabo::util::Hash& outputChannelInfo,
                           karabo::net::Channel::Pointer channel,
                           const boost::function<void (const karabo::net::ErrorCode&)>& handler);

            void onTcpChannelError(const karabo::net::ErrorCode&, const karabo::net::Channel::Pointer&);

            void onTcpChannelRead(const karabo::net::ErrorCode& ec, karabo::net::Channel::Pointer channel,
                                  const karabo::util::Hash& header, const std::vector<karabo::io::BufferSet::Pointer>& data);
                                  
            bool canCompute() const;

            void notifyOutputChannelsForPossibleRead();

            void notifyOutputChannelForPossibleRead(const karabo::net::Channel::Pointer& channel);

            bool respondsToEndOfStream();

            void parseOutputChannelConfiguration(const karabo::util::Hash& config);

            void updateOutputChannelConfiguration(const std::string& outputChannelString, const karabo::util::Hash& config);

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

            // void update();

            void deferredNotificationsOfOutputChannelsForPossibleRead();

            void deferredNotificationOfOutputChannelForPossibleRead(const karabo::net::Channel::Pointer& channel);

            bool needsDeviceConnection() const;

            void closeChannelsAndStopConnections();

            void prepareMetaData();
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
