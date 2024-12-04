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

        typedef boost::function<std::string(const std::vector<std::string>&)> SharedInputSelector;

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
             *     tcpChannel (karabo::net::Channel::WeakPointer)
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

            boost::mutex m_inputNetChannelsMutex;
            std::set<karabo::net::Channel::Pointer> m_inputNetChannels;

            mutable boost::mutex m_registeredInputsMutex;
            InputChannels m_registeredSharedInputs;
            // Used for storing chunks for shared input channels when
            // distribution mode is "load-balanced" and the strategy for
            // NoSharedInputChannel available is "queueDrop".
            // Relies on lock protection using m_registeredInputsMutex.
            std::deque<int> m_sharedLoadBalancedQueuedChunks;
            // Handlers to unblock synchronous update actions - one for shared and map of individual ones
            boost::function<void(InputChannelInfo*)> m_unblockSharedHandler;
            std::map<std::string, boost::function<void(InputChannelInfo*)>> m_unblockHandlers;

            InputChannels m_registeredCopyInputs;

            // m_shareNext could also be an unordered_set, but the deque allows a more uniform
            // distribution of work in the load-balanced case, even in the presence of a fast
            // enough input channel that could, potentially, handle all the load by itself.
            // Both need protection of m_registeredInputsMutex.
            std::deque<std::string> m_shareNext;
            std::unordered_set<std::string> m_copyNext;

            unsigned int m_channelId;
            unsigned int m_chunkId;
            // Mark m_chunkId as invalid with this number
            static const unsigned int m_invalidChunkId = std::numeric_limits<unsigned int>::max();

            mutable boost::mutex m_showConnectionsHandlerMutex;
            ShowConnectionsHandler m_showConnectionsHandler;
            ShowStatisticsHandler m_showStatisticsHandler;
            SharedInputSelector m_sharedInputSelector; // protected by m_registeredInputsMutex
            std::vector<karabo::util::Hash> m_connections;
            boost::asio::deadline_timer m_updateDeadline;
            int m_period;
            int m_addedThreads;

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
             * Writes a Hash containing data to the output channel. Sending to the network happens when update() is
             * called.
             *
             * @param data input Hash object
             * @param metaData a MetaData object containing meta data for this data token.
             *
             * Note: Any NDArray/ImageData inside data must stay untouched at least until update() or the callback of
             *       asyncUpdate(cb) has been called. See also the documentation of the safeNDArray flag of the
             *       update()/asyncUpdate() methods.
             *
             * Thread safety:
             * All the 'write(..)' methods, '[async]Update[NoWait](..)' and '[async]SignalEndOfStream(..)' must not be
             * called concurrently.
             */
            void write(const karabo::util::Hash& data, const Memory::MetaData& metaData, bool /*unused*/ = false);

            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens when update() is
             * called.
             * Metadata is initialized to default values. Namely the sending devices device id and the output channel's
             * name are used as data source.
             *
             * @param data input Hash object
             *
             * Note: Any NDArray/ImageData inside data must stay untouched at least until update() or the callback of
             *       asyncUpdate(cb) has been called. See also the documentation of the safeNDArray flag of the
             *       update()/asyncUpdate() methods.
             *
             * Thread safety:
             * All the 'write(..)' methods, '[async]Update[NoWait](..)' and '[async]SignalEndOfStream(..)' must not be
             * called concurrently.
             */
            void write(const karabo::util::Hash& data, bool /*unused*/ = false);

            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens when update() is
             * called.
             * @param data shared pointer to input Hash object
             * @param metaData a MetaData object containing meta data for this data token.
             *
             * Note: Any NDArray/ImageData inside data must stay untouched at least until update() or the callback of
             *       asyncUpdate(cb) has been called. See also the documentation of the safeNDArray flag of the
             *       update()/asyncUpdate() methods.
             *
             * Thread safety:
             * All the 'write(..)' methods, '[async]Update[NoWait](..)' and '[async]SignalEndOfStream(..)' must not be
             * called concurrently.
             */
            KARABO_DEPRECATED void write(const karabo::util::Hash::Pointer& data, const Memory::MetaData& metaData);

            /**
             * Writes a Hash containing data to the output channel. Sending to the network happens asynchronously.
             * Metadata is initialized to default values. Namely the sending devices device id and the output channel's
             * name are used as data source.
             * @param data shared pointer to input Hash object
             *
             * Note: Any NDArray/ImageData inside data must stay untouched at least until update() or the callback of
             *       asyncUpdate(cb) has been called. See also the documentation of the safeNDArray flag of the
             *       update()/asyncUpdate() methods.
             *
             * Thread safety:
             * All the 'write(..)' methods, '[async]Update[NoWait](..)' and '[async]SignalEndOfStream(..)' must not be
             * called concurrently.
             */
            KARABO_DEPRECATED void write(const karabo::util::Hash::Pointer& data);

            /**
             * Update the output channel, i.e. send all data over the wire that was previously written
             * by calling write(...).
             * This is a synchronous method, i.e. blocks until all data is actually sent (or dropped or queued).
             *
             * @param safeNDArray boolean to indicate whether all NDArrays inside the Hash passed to write(..) before
             *                    are 'safe', i.e. their memory will not be referred to elsewhere after update is
             *                    finished. Default is 'false', 'true' can avoid safety copies of NDArray content when
             *                    data is queued or sent locally.
             *
             * Thread safety:
             * All the 'write(..)' methods, '[async]Update[NoWait](..)' and '[async]SignalEndOfStream(..)' must not be
             * called concurrently.
             */
            void update(bool safeNDArray = false);

            /**
             * Semi-asynchronously update the output channel, i.e. start asynchronous sending of data over the wire
             * that was previously written to the output channel's' buffer by calling write(...), but block as long as
             * required to really start sending.
             * The start of sending data is delayed
             * - for any connected input channel that is currently not ready to receive more data, but is configured
             *   with "dataDistribution" as "copy" and with "onSlowness" as "wait",
             * - or if none of the connected input channels that are configured with "dataDistribution" as "shared" are
             *   currently ready to receive data and if this output channel is configured with "noInputShared" as
             *   "wait".
             *
             * @param safeNDArray boolean to indicate whether all NDArrays inside the Hash passed to write(..) before
             *                    are 'safe', i.e. their memory will not be referred to elsewhere before 'readyHandler'
             *                    is called. Default is 'false', 'true' can avoid safety copies of NDArray content when
             *                    data is queued or sent locally.
             * @param writeDoneHandler callback when data (that is not queued) has been sent and thus even NDArray data
             *                         inside it can be re-used again (except if safeNDArray was set to 'true' in which
             *                         case its memory may still be used in a queue).
             *
             * Thread safety:
             * All the 'write(..)' methods, '[async]Update[NoWait](..)' and '[async]SignalEndOfStream(..)' must not be
             * called concurrently.
             */
            void asyncUpdate(bool safeNDArray = false, boost::function<void()>&& writeDoneHandler = []() {});

            /**
             * Expert method
             *
             * Asynchronously update the output channel, i.e. asynchronously send all data over the wire that was
             * previously written by calling write(...) without any blocking.
             *
             * This method must not be called again before either 'readyForNextHandler' or 'writeDoneHandler' have
             * been called. If next data should be sent, but neither handler has been called yet, one has to block or
             * skip the data. In the latter case, the wish of a connected input channel that is configured to make the
             * output "wait" if not ready, is ignored (policy violation).
             *
             * Both handlers have to be valid function pointers.
             *
             * @param readyForNextHandler callback when asyncUpdateNoWait may be called again (this can only be
             *                            delayed if any blocking input channel ["wait"] is connected)
             * @param writeDoneHandler callback when sending is finished (as confirmed by Tcp) or stopped due to
             *                         disconnection, or data is internally queued. So now all NDArray inside the Hash
             *                         passed to write(..) before can be re-used again (except if safeNDArray was set
             *                         to 'true' in which case its memory may still be used in a queue)
             * @param safeNDArray boolean to indicate whether all NDArrays inside the Hash passed to write(..) before
             *                    are 'safe', i.e. their memory will not be referred to elsewhere after update is
             *                    finished. False triggers a data copy if data needs to be queued.
             *
             * TODO: Provide a handler called when sending data is completed, including any queued data, and thus
             *       NDArray data can be re-used again even if safeNDArray=false (i.e. buffers could be re-used).
             *
             * Thread safety:
             * All the 'write(..)' methods, '[async]Update[NoWait](..)' and '[async]SignalEndOfStream(..)' must not be
             * called concurrently.
             */
            void asyncUpdateNoWait(boost::function<void()>&& readyForNextHandler,
                                   boost::function<void()>&& writeDoneHandler, bool safeNDArray);

            /**
             * Synchronously send end-of-stream (EOS) notification to all connected input channels to indicate a logical
             * break in the data stream.
             *
             * Thread safety:
             * All the 'write(..)' methods, '[async]Update[NoWait](..)' and '[async]SignalEndOfStream(..)' must not be
             * called concurrently.
             */
            void signalEndOfStream();

            /**
             * Asynchonously send end-of-stream (EOS) notification to all connected input channels to indicate a
             * logical break in the data stream.
             *
             * @param readyHandler callback when notification has been sent or queued
             *
             * Thread safety:
             * All the 'write(..)' methods, 'update()'/'asyncUpdate(cb)' and
             * 'signalEndOfStream()'/'asyncSignalEndOfStream(cb)' must not be called concurrently.
             */
            void asyncSignalEndOfStream(boost::function<void()>&& readyHandler);

            void registerShowConnectionsHandler(const ShowConnectionsHandler& handler);

            void registerShowStatisticsHandler(const ShowStatisticsHandler& handler);

            /**
             * Register handler that selects which of the connected input channels that have dataDistribution =
             * "shared" is to be served.
             *
             * The handler will be called during update(..)/asyncUpdate[NoWait](..) with the ids of the connected
             * "shared" input channels (e.g. "deviceId:input") as argument. The returned channel id will receive the
             * data. If an empty string or an unknown id is returned, the data will be dropped.
             *
             * @param selector takes vector<string> as argument and returns string
             */
            void registerSharedInputSelector(SharedInputSelector&& selector);

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

            /// Erase instance with 'instanceId' from 'channelContainer' if existing - if same as 'newChannel', do
            /// not close
            void eraseOldChannel(InputChannels& channelContainer, const std::string& instanceId,
                                 const karabo::net::Channel::Pointer& newChannel) const;

            void updateConnectionTable();

            void updateNetworkStatistics(const boost::system::error_code& e);

            void onInputAvailable(const std::string& instanceId);

            void triggerIOEvent();

            void onInputGone(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& error);

            /**
             * Helper to indicate that given shared input is ready to receive more data
             *
             * Requires m_registeredInputsMutex to be locked
             */
            void pushShareNext(const std::string& instanceId);

            /**
             * Helper to provide id of shared input that is ready to receive more data
             *
             * Requires m_registeredInputsMutex to be locked
             */
            std::string popShareNext();

            /**
             * Helper to tell whether none of the shared inputs is ready to receive more data
             *
             * Requires m_registeredInputsMutex to be locked
             */
            bool isShareNextEmpty() const;

            /**
             * Helper to query whether given shared input is ready to receive more data
             *
             * Requires m_registeredInputsMutex to be locked
             */
            bool hasSharedInput(const std::string& instanceId);

            /**
             * Helper to indicate that given shared input is currently not ready to receive more data
             *
             * Requires m_registeredInputsMutex to be locked
             */
            void eraseSharedInput(const std::string& instanceId);

            /**
             * Helper to indicate that given copy input is ready to receive more data
             *
             * Requires m_registeredInputsMutex to be locked
             */
            void pushCopyNext(const std::string& instanceId);

            /**
             * Erase instance from container of copy channels that are ready to receive data
             *
             * Requires m_registeredInputsMutex to be locked
             *
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
             * helper for asyncUpdate() to ensure that at the end m_chunkId is valid - may block a while
             *
             * @param lockOfRegisteredInputsMutex a scoped_lock locking m_registeredInputsMutex
             *                                    (may be unlocked and locked again during execution)
             */
            void ensureValidChunkId(boost::mutex::scoped_lock& lockOfRegisteredInputsMutex);

            void unregisterWriterFromChunk(int chunkId);

            /**
             * Figure out how to treat copy inputs, return via appending to reference arguments
             *
             * Requires m_registeredInputsMutex to be locked
             */
            void asyncPrepareCopy(unsigned int chunkId, std::vector<karabo::util::Hash*>& toSendImmediately,
                                  std::vector<karabo::util::Hash*>& toQueue, std::vector<karabo::util::Hash*>& toBlock);
            /**
             * Figure out how to treat shared inputs, return via (appending to) reference arguments
             *
             * Requires m_registeredInputsMutex to be locked
             *
             */
            void asyncPrepareDistribute(unsigned int chunkId, std::vector<karabo::util::Hash*>& toSendImmediately,
                                        std::vector<karabo::util::Hash*>& toQueue,
                                        std::vector<karabo::util::Hash*>& toBlock, bool& queue, bool& block);
            /**
             * Figure out how to send EndOfStream for shared outputs, return via reference arguments
             *
             * Requires m_registeredInputsMutex to be locked
             *
             * @returns whether to queue for shared queue
             */
            bool asyncPrepareDistributeEos(unsigned int chunkId, std::vector<karabo::util::Hash*>& toSendImmediately,
                                           std::vector<karabo::util::Hash*>& toQueue,
                                           std::vector<karabo::util::Hash*>& toBlock);

            /**
             * Figure out how to treat shared inputs if sharedInputSelector is registered
             *
             * Requires m_registeredInputsMutex to be locked
             *
             */
            void asyncPrepareDistributeSelected(unsigned int chunkId,
                                                std::vector<karabo::util::Hash*>& toSendImmediately,
                                                std::vector<karabo::util::Hash*>& toQueue,
                                                std::vector<karabo::util::Hash*>& toBlock);
            /**
             * Figure out how to treat shared inputs when load-balancing
             *
             * Requires m_registeredInputsMutex to be locked
             *
             */
            void asyncPrepareDistributeLoadBal(unsigned int chunkId,
                                               std::vector<karabo::util::Hash*>& toSendImmediately,
                                               std::vector<karabo::util::Hash*>& toQueue,
                                               std::vector<karabo::util::Hash*>& toBlock, bool& queue, bool& block);

            /**
             * Helper that sets the sendOngoing flag to false for given instanceId
             */
            void resetSendOngoing(const std::string& instanceId);


            /**
             * Helper to asynchronously send chunk data to channel in given channelInfo
             *
             * @param chunkId The chunk to send
             * @param channelInfo Container with info about channel to send to
             * @param doneHandler Callback when sending done or failed
             */
            void asyncSendOne(unsigned int chunkId, InputChannelInfo& channelInfo,
                              boost::function<void()>&& doneHandler);

            /**
             * Helper for waiting for future that in case of long delay adds a thread to unblock
             *
             * Throws TimeoutException if not unblocked after two minutes.
             */
            void awaitUpdateFuture(std::future<void>& fut, const char* which);

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
                m_dataSchema.key(key + ".schema").setSpecialDisplayType("OutputSchema");
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
