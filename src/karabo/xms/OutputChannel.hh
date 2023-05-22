/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:57 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_XMS_NETWORKOUTPUT_HH
#define KARABO_XMS_NETWORKOUTPUT_HH

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <unordered_set>
#include <vector>

#include "Memory.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/NodeElement.hh"


/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xms {

        typedef boost::function<void(const std::vector<karabo::util::Hash>&)> ShowConnectionsHandler;
        typedef boost::function<void(const std::vector<unsigned long long>&, const std::vector<unsigned long long>&)>
              ShowStatisticsHandler;

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

            // With C++14, can use unordered map (since then standard allows to erase items while looping on
            // unordered_map)
            typedef std::map<std::string, InputChannelInfo> InputChannels; // input channel id is key

            // Callback on available input
            boost::function<void(const boost::shared_ptr<OutputChannel>&)> m_ioEventHandler;

            std::string m_instanceId;
            std::string m_channelName;

            // Server related
            std::string m_hostname;
            unsigned int m_port;

            karabo::net::Connection::Pointer m_dataConnection;

            std::string m_onNoSharedInputChannelAvailable;
            std::string m_distributionMode;

            boost::mutex m_inputNetChannelsMutex;
            std::set<karabo::net::Channel::Pointer> m_inputNetChannels;

            mutable boost::mutex m_registeredSharedInputsMutex;
            InputChannels m_registeredSharedInputs;
            // Used for storing chunks for shared input channels when
            // distribution mode is "load-balanced" and the strategy for
            // NoSharedInputChannel available is "queue".
            // Relies on lock protection using m_registeredSharedInputsMutex.
            std::deque<int> m_sharedLoadBalancedQueuedChunks;

            mutable boost::mutex m_registeredCopyInputsMutex;
            InputChannels m_registeredCopyInputs;

            unsigned int m_sharedInputIndex;

            // m_shareNext could also be an unordered_set, but the deque allows a more uniform
            // distribution of work in the load-balanced case, even in the presence of a fast
            // enough input channel that could, potentially, handle all the load by itself.
            std::deque<std::string> m_shareNext;

            std::unordered_set<std::string> m_copyNext;

            // The following two variables are only used in update() and methods called from that, i.e.
            // registerWritersOnChunk(..), copy(.. ), and distribute(..), so they do not need any mutex protection.
            bool m_toUnregisterSharedInput;
            std::unordered_set<std::string> m_toUnregisterCopyInputs;

            mutable boost::mutex m_nextInputMutex;

            unsigned int m_channelId;
            unsigned int m_chunkId;
            // Mark m_chunkId as invalid with this number
            static const unsigned int m_invalidChunkId = std::numeric_limits<unsigned int>::max();

            mutable boost::mutex m_showConnectionsHandlerMutex;
            ShowConnectionsHandler m_showConnectionsHandler;
            ShowStatisticsHandler m_showStatisticsHandler;
            std::vector<karabo::util::Hash> m_connections;
            boost::asio::deadline_timer m_updateDeadline;
            int m_period;

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
             * If this object is constructed using the factory/configuration system this method is called.
             *
             * The initialize() method must not be called if constructed this way.
             *
             * Deprecated:
             * Tcp server initialization is triggered, but there is no control when and whether it succeeded.
             * So better use the constructor with additional int argument (and set it to zero).
             *
             * @param config Validated (@see expectedParameters) and default-filled configuration
             */
            explicit OutputChannel(const karabo::util::Hash& config);

            /**
             * Recommended constructor, allowing guaranteed-to-work initialization.
             *
             * The recommended way to call it is via the Configurator and with autoInit == 0,
             * followed by calling initialize():
             *
             * Hash config(<here state non-default config parameters>);
             * OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", cfg, 0);
             * output->initialize();
             *
             * Caveat: Make sure you do not pass a 'bool' instead of an 'int' as argument to create(..) since then the
             *         other constructor is chosen and the value of the 'bool' determines whether to validate cfg or
             * not.
             *
             * @param config Validated (@see expectedParameters) and default-filled configuration
             * @param autoInit If set to 0 (strongly recommended), the constructor does not yet try to initiate the
             *                 TCP server initialization and the initialize() method has to be called as
             *                 "second constructor". The advantage is that the initialization cannot fail on busy
             *                 systems and one has control when the server is available for remote connections.
             *                 If autoInit != 0, this constructor behaves as the other constructor and initialize()
             *                 must not be called.
             */
            OutputChannel(const karabo::util::Hash& config, int autoInit);

            virtual ~OutputChannel();

            /**
             * "Second constructor", to be called after construction with second argument autoInit == 0.
             *
             * Initializes the underlying Tcp server connection and makes it available for others.
             *
             * May throw a karabo::util::NetworkException, e.g. if a non-zero port was defined in the input
             * configuration and that is not available since used by something else.
             */
            void initialize();

            void setInstanceIdAndName(const std::string& instanceId, const std::string& name);

            const std::string& getInstanceId() const;

            /**
             *  returns the initial readonly configuration parameters
             *
             * Returns a Hash containing the initial information that should
             * not be updated via `ShowConnectionHandler` and `ShowStatisticsHandler`.
             * Currently only the `address` key is included.
             */
            karabo::util::Hash getInitialConfiguration() const;

            /**
             *  Concatenation of instance id and name
             */
            std::string getInstanceIdName() const;
            /**
             * Check whether an InputChannel with given id is registered to receive all data
             *
             * i.e. an InputChannel with "dataDistribution == copy"
             *
             * @param instanceId of InputChannel
             * @return bool whether InputChannel of specified type is connected
             */
            bool hasRegisteredCopyInputChannel(const std::string& instanceId) const;

            /**
             * Check whether an InputChannel with given id is registered to receive a share of the data
             *
             * i.e. an InputChannel with "dataDistribution == shared"
             *
             * @param instanceId of InputChannel
             * @return bool whether InputChannel of specified type is connected
             */
            bool hasRegisteredSharedInputChannel(const std::string& instanceId) const;

            void registerIOEventHandler(const boost::function<void(const OutputChannel::Pointer&)>& ioEventHandler);


            karabo::util::Hash getInformation() const;

            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens asynchronously.
             * @param data input Hash object
             * @param metaData a MetaData object containing meta data for this data token.
             * @param copyAllData If false, serialization is optimized to avoid copies for big (i.e. NDArray) data.
             *
             * Note: When using copyAllData==false (default!), any NDArray/ImageData inside data must stay untouched
             *       at least until update() has been called for the channel.
             *
             * Thread safety:
             * All the 'write(..)' methods, 'update()' and 'signalEndOfStream()' must not be called concurrently.
             */
            void write(const karabo::util::Hash& data, const Memory::MetaData& metaData, bool copyAllData = false);

            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens asynchronously.
             * Metadata is initialized to default values. Namely the sending devices device id and the output channel's
             * name are used as data source.
             * @param data input Hash object
             * @param copyAllData If false, serialization is optimized to avoid copies for big (i.e. NDArray) data.
             *
             * Note: When using copyAllData==false (default!), any NDArray/ImageData inside data must stay untouched
             *       at least until update() has been called for the channel.
             *
             * Thread safety:
             * All the 'write(..)' methods, 'update()' and 'signalEndOfStream()' must not be called concurrently.
             */
            void write(const karabo::util::Hash& data, bool copyAllData = false);

            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens asynchronously.
             * @param data shared pointer to input Hash object
             * @param metaData a MetaData object containing meta data for this data token.
             *
             * Note: Any NDArray/ImageData inside data must stay untouched at least until update() has been called for
             *       the channel.
             *
             * Thread safety:
             * All the 'write(..)' methods, 'update()' and 'signalEndOfStream()' must not be called concurrently.
             */
            KARABO_DEPRECATED void write(const karabo::util::Hash::Pointer& data, const Memory::MetaData& metaData);

            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens asynchronously.
             * Metadata is initialized to default values. Namely the sending devices device id and the output channel's
             * name are used as data source.
             * @param data shared pointer to input Hash object
             *
             * Note: Any NDArray/ImageData inside data must stay untouched at least until update() has been called for
             *       the channel.
             *
             * Thread safety:
             * All the 'write(..)' methods, 'update()' and 'signalEndOfStream()' must not be called concurrently.
             */
            KARABO_DEPRECATED void write(const karabo::util::Hash::Pointer& data);

            /**
             * Update the output channel, i.e. send all data over the wire that was previously written
             * by calling write(...).
             *
             * @param safeNDArray boolean to indicate whether all NDArrays inside the Hash passed to write(..) before
             *                    are 'safe', i.e. their memory will not be referred to elsewhere after update is
             *                    finished. Default is 'true'. If false, safety copies are done when needed, i.e. when
             *                    data is queued or sent locally.
             *
             * Thread safety:
             * All the 'write(..)' methods, 'update()' and 'signalEndOfStream()' must not be called concurrently.
             */
            void update(bool safeNDArray = true);

            /**
             * Send end-of-stream (EOS) notification to all connected input channels to indicate a logical break
             * in the data stream.
             *
             * Thread safety:
             * All the 'write(..)' methods, 'update()' and 'signalEndOfStream()' must not be called concurrently.
             */
            void signalEndOfStream();

            void registerShowConnectionsHandler(const ShowConnectionsHandler& handler);

            void registerShowStatisticsHandler(const ShowStatisticsHandler& handler);

            /**
             * Shut down all underlying connections, object will not be usable afterwards.
             *
             * Needed if stray shared pointers may be kept somewhere.
             */
            void disable();

           private:
            void initializeServerConnection(int countdown);

            void onTcpConnect(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

            void onTcpChannelError(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

            void onTcpChannelRead(const karabo::net::ErrorCode& ec, const karabo::net::Channel::WeakPointer& channel,
                                  const karabo::util::Hash& message);

            /// Erase instance with 'instanceId' from 'channelContainer' if existing - if same as 'newChannel', do not
            /// close
            void eraseOldChannel(InputChannels& channelContainer, const std::string& instanceId,
                                 const karabo::net::Channel::Pointer& newChannel) const;

            void updateConnectionTable();

            void updateNetworkStatistics(const boost::system::error_code& e);

            void onInputAvailable(const std::string& instanceId);

            void triggerIOEvent();

            void onInputGone(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& error);

            /**
             * Send data to channel from queue
             * @param channelInfo is all information about channel to sent data to
             * @param chunkIds id queue to operate on (take from front)
             * @param lock will be unlocked after operating on chunkIds, but before using sendOne(chunkId, channelInfo)
             */
            void sendFromQueue(karabo::util::Hash& channelInfo, std::deque<int>& chunkIds,
                               boost::mutex::scoped_lock& lock);

            void pushShareNext(const std::string& instanceId);

            std::string popShareNext();

            bool isShareNextEmpty() const;

            bool hasSharedInput(const std::string& instanceId);

            void eraseSharedInput(const std::string& instanceId);

            void pushCopyNext(const std::string& info);

            /**
             * Erase instance from container of copy channels that are ready to receive data
             * @param instanceId
             * @return whether instanceId could be removed (i.e. was actually ready to receive)
             */
            bool eraseCopyInput(const std::string& instanceId);

            /**
             *  helper to set new m_chunkId
             * @return true if new m_chunkId is valid (i.e. not equal m_invalidChunkId)
             */
            bool updateChunkId();

            /**
             * helper for update() to ensure that at the end m_chunkId is valid - may block a while
             */
            void ensureValidChunkId();

            void registerWritersOnChunk(unsigned int chunkId);

            void unregisterWriterFromChunk(int chunkId);

            void distribute(unsigned int chunkId, bool safeNDArray);

            /**
             * Distribute endOfStream notification to all shared inputs
             *
             * Requires that m_registeredSharedInputsMutex is locked.
             */
            void distributeEndOfStream(unsigned int chunkId);

            /**
             * Distribute in round round-robin mode, i.e. one shared input after another
             *
             * requires that m_registeredSharedInputs not empty
             *
             * @param chunkId which chunk to distribute
             * @param lock of mutex m_registeredSharedInputsMutex which must be active/locked,
             *             and might get unlocked within function call (or not)
             * @param safeNDArray if true, no need to copy NDArray buffer if chunk is queued or sent locally
             *
             */
            void distributeRoundRobin(unsigned int chunkId, boost::mutex::scoped_lock& lock, bool safeNDArray);

            /**
             * Distribute in load balanced mode, i.e. pick one of the shared inputs that is ready
             *
             * @param chunkId which chunk to distribute
             * @param lock of mutex m_registeredSharedInputsMutex which must be active/locked,
             *             and might get unlocked within function call (or not)
             * @param safeNDArray if true, no need to copy NDArray buffer if chunk is queued or sent locally
             *
             */
            void distributeLoadBalanced(unsigned int chunkId, boost::mutex::scoped_lock& lock, bool safeNDArray);

            /**
             * Get iterator to next one of the shared inputs - round-robin case.
             *
             * Requires protection of m_registeredSharedInputsMutex and m_registeredSharedInputs.size() > 0
             */
            InputChannels::iterator getNextRoundRobinChannel();

            /**
             * Undo a previous getNextRoundRobinChannel()
             *
             * Requires protection of m_registeredSharedInputsMutex.
             * Even more, that mutex must not have been unlocked after the getNextSharedInputIdx() it should undo.
             */
            void undoGetNextRoundRobinChannel();

            /**
             * Serve all 'copy' input channels
             *
             * @param chunkId which chunk to send to them
             * @param safeNDArray see documentation of update(bool safeNDArray)
             */
            void copy(unsigned int chunkId, bool safeNDArray);

            /**
             * Helper to send chunkId to input channel identified by channelInfo
             *
             * Either uses sendLocal or sendRemote
             */
            void sendOne(const unsigned int& chunkId, const InputChannelInfo& channelInfo, bool safeNDArray);

            void sendLocal(const unsigned int& chunkId, const InputChannelInfo& channelInfo, bool eos,
                           bool safeNDArray);

            void sendRemote(const unsigned int& chunkId, const InputChannelInfo& channelInfo, bool eos);

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
                if (key.find_first_of(":@") != std::string::npos) {
                    // Input channel connections are configured with "<deviceId>:<outputChannelKey>" (or '@' instead of
                    // ':')
                    throw KARABO_PARAMETER_EXCEPTION(
                          "Bad output channel key with device/channel id delimiter (':' '@') : " + key);
                }
                m_outputChannel.key(key);
                m_dataSchema.key(key + ".schema");
                return *this;
            }

            OutputChannelElement& displayedName(const std::string& name) {
                m_outputChannel.displayedName(name);
                return *this;
            }

            OutputChannelElement& description(const std::string& description) {
                m_outputChannel.description(description);
                return *this;
            }

            OutputChannelElement& dataSchema(const karabo::util::Schema& schema) {
                m_dataSchema.appendSchema(schema);
                return *this;
            }

            void commit() {
                m_outputChannel.commit();
                m_dataSchema.commit();
            }
        };

        typedef OutputChannelElement OUTPUT_CHANNEL_ELEMENT;
        typedef OutputChannelElement OUTPUT_CHANNEL;

    } // namespace xms
} // namespace karabo

#endif
