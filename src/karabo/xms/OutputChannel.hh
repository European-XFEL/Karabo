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
#include "Data.hh"


/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xms {

        /**
         * The DeviceOutput class.
         */
        class OutputChannel : public boost::enable_shared_from_this<OutputChannel> {
            typedef boost::shared_ptr<karabo::net::Channel> TcpChannelPointer;

            /*
             * InputChannelInfo (karabo::util::Hash)
             * 
             *     instanceId (std::string)
             *     memoryLocation (std::string) [local/remote]
             *     tcpChannel (TcpChannelPointer)
             *     onSlowness (std::string) [queue/drop/wait/throw]
             *     queuedChunks (std::deque<int>)
             *
             */
            typedef karabo::util::Hash InputChannelInfo;

            typedef std::vector<InputChannelInfo> InputChannels;

            typedef std::deque< std::string > InputChannelQueue;

            typedef std::map<unsigned int, int> CurrentWritersCount;

            typedef std::map<TcpChannelPointer, unsigned int> TcpChannelPointer2ChunkId;

            typedef Memory<karabo::util::Hash> MemoryType;

            // Callback on available input
            boost::function<void (const boost::shared_ptr<OutputChannel>&) > m_ioEventHandler;

            std::string m_instanceId;

            // Server related
            std::string m_hostname;
            unsigned int m_ownPort;
            int m_compression;

            karabo::net::Connection::Pointer m_dataConnection;
            //TcpChannelPointer m_dataChannel;
            karabo::net::IOService::Pointer m_dataIOService;
            boost::thread m_dataThread;

            std::string m_onNoSharedInputChannelAvailable;
            std::string m_distributionMode;

            InputChannels m_registeredSharedInputs;
            InputChannels m_registeredCopyInputs;

            unsigned int m_sharedInputIndex;

            InputChannelQueue m_shareNext;
            InputChannelQueue m_copyNext;

            boost::mutex m_nextInputMutex;
            boost::mutex m_chunkIdsMutex;
            boost::mutex m_currentWritersCountMutex;
            boost::mutex m_onTcpReadMutex;
            //boost::mutex m_updateMutex;


            // Async out
            CurrentWritersCount m_currentWritersCount;
            CurrentWritersCount m_maxWritersCount;
            std::map<TcpChannelPointer, unsigned int> m_channel2ChunkId;

            unsigned int m_channelId;
            unsigned int m_chunkId;

            std::map<int, int> m_writersOnChunk;

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

            void setInstanceId(const std::string& instanceId);

            const std::string& getInstanceId() const;

            void registerIOEventHandler(const boost::function<void (const OutputChannel::Pointer&)>& ioEventHandler);


            karabo::util::Hash getInformation() const;

            /**
             * This interface will make an (possibly expensive) copy of your data
             * @param data
             */
            void write(const karabo::util::Hash& data) {
                MemoryType::write(boost::shared_ptr<karabo::util::Hash>(new karabo::util::Hash(data)), m_channelId, m_chunkId);
            }

            /**
             * This function will use the data as pointer and does not copy
             * @param data
             */
            void write(const boost::shared_ptr<karabo::util::Hash>& data) {
                MemoryType::write(data, m_channelId, m_chunkId);
            }

            void write(const Data& data) {
                MemoryType::write(data.hash(), m_channelId, m_chunkId);
            }

            void update();
                     
            void signalEndOfStream();

        private:

            void onTcpConnect(TcpChannelPointer channel);


            // TODO Implement this !!!!

            void onTcpConnectionError(karabo::net::Connection::Pointer conn, const karabo::net::ErrorCode& error);

            void onTcpChannelError(const TcpChannelPointer& channel, const karabo::net::ErrorCode& error);

            void onTcpChannelRead(TcpChannelPointer channel, const karabo::util::Hash& message);

            void onInputAvailable(const std::string& instanceId);

            void triggerIOEvent();

            void onInputGone(const TcpChannelPointer& channel);

            void distributeQueue(karabo::util::Hash& channelInfo);

            void copyQueue(karabo::util::Hash& channelInfo);

            void pushShareNext(const std::string& instanceId);

            std::string popShareNext();

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

            unsigned int getNextSharedInputIdx();

            void distributeLocal(unsigned int chunkId, const InputChannelInfo & channelInfo);

            void distributeRemote(const unsigned int& chunkId, const InputChannelInfo & channelInfo);

            void copy(unsigned int chunkId);

            void copyLocal(const unsigned int& chunkId, const InputChannelInfo & channelInfo);

            void copyRemote(const unsigned int& chunkId, const InputChannelInfo & channelInfo);
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
