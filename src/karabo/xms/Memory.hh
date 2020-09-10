/*
 * $Id$
 *
 * File:   Memory.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on June 05, 2012, 12:44 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XMS_MEMORY_HH
#define	KARABO_XMS_MEMORY_HH

#include <karabo/util/Factory.hh>
#include <karabo/io/BinarySerializer.hh>
#include <karabo/io/BufferSet.hh>

#include <karabo/log/Logger.hh>

namespace karabo {
    namespace xms {

        class Memory {

        public:

            /**
             * @class Memory::MetaData
             * @brief The MetaData class is s class for transporting
             *        meta data related to data tokens on pipelined processing
             *        interfaces. It derives from karabo::util::Hash for
             *        transparent serialization.
             */
            class MetaData : protected karabo::util::Hash {

                // Note that if you extend this class you need to use set/get internally
                // to assure Hash serializion.

            public:

                /**
                 * Constructor to directly set meta data entries
                 * @param source an identifier of the data producer
                 * @param timestamp a timestamp relevant for this data token.
                 */
                MetaData(const std::string& source, const karabo::util::Timestamp& timestamp) {
                    setSource(source);
                    setTimestamp(timestamp);
                }

                /**
                 * Set data source, i.e. identifier of the data producer
                 * @param source
                 */
                inline void setSource(const std::string& source) {
                    set("source", source);
                }

                /**
                 * Get data source, i.e. identifier of the data producer
                 * @return
                 */
                inline const std::string& getSource() const {
                    return get<std::string>("source");
                }

                /**
                 * Set the timestamp relevant to this data token
                 * @param timestamp
                 */
                inline void setTimestamp(const karabo::util::Timestamp& timestamp) {
                    karabo::util::Hash::Node& h = set("timestamp", true);
                    timestamp.toHashAttributes(h.getAttributes());
                }

                /**
                 * Get the timestamp relevant to this data token
                 * @return
                 */
                inline const karabo::util::Timestamp getTimestamp() const {
                    return karabo::util::Timestamp::fromHashAttributes(getAttributes("timestamp"));
                }

            };

            typedef karabo::io::BufferSet DataType;
            typedef boost::shared_ptr<DataType> DataPointer;
            typedef std::vector< DataPointer > Data;
            typedef std::vector< Data > Chunks;
            typedef std::vector< Chunks > Channels;

            typedef std::vector<MetaData> MetaDataEntries;
            typedef std::vector<MetaDataEntries> ChunkMetaDataEntries;
            typedef std::vector<ChunkMetaDataEntries> ChannelMetaDataEntries;

            typedef std::pair<std::vector<karabo::io::BufferSet::Pointer>, karabo::util::Hash> SerializedChunk;
            typedef boost::shared_ptr<SerializedChunk> SerializedChunkPointer;
            typedef std::vector< SerializedChunkPointer > SerializedChunks;
            typedef std::vector< SerializedChunks > SerializedChannels;

            typedef std::vector<std::vector<int> > ChunkStatus;
            typedef std::vector<int> ChannelStatus;

            typedef std::vector<size_t> ChunkUsers;
            typedef std::vector<ChunkUsers> ChannelUsers;

            typedef karabo::io::BinarySerializer<karabo::util::Hash> SerializerType;


            static ChannelUsers m_cacheUsers;

            static std::map<std::string, size_t> m_name2Idx;

            static ChunkStatus m_chunkStatus;
            static ChannelStatus m_channelStatus;

            static Channels m_cache;
            static SerializedChannels m_serializedCache;
            static ChannelMetaDataEntries m_metaData;
            static std::vector<std::vector<bool>> m_isEndOfStream;

            static boost::mutex m_accessMutex;

            static boost::shared_ptr<SerializerType > m_serializer;

            static const int MAX_N_CHANNELS = 128;
            static const int MAX_N_CHUNKS = 2056;

            Memory() {
            }

        public:

            KARABO_CLASSINFO(Memory, "Memory", "1.0")

            /**
             * Read the contents of a single Hash out of the cache. The passed in
             * Hash will be cleared first.
             * @param data
             * @param dataIdx
             * @param channelIdx
             * @param chunkIdx
             */
            static void read(karabo::util::Hash& data, const size_t dataIdx, const size_t channelIdx, const size_t chunkIdx);

            /**
             * Read the contents of a single Hash out of the cache. A pointer tag_of
             * a newly created Hash will be returned.
             * @param dataIdx
             * @param channelIdx
             * @param chunkIdx
             */
            static DataPointer read(const size_t dataIdx, const size_t channelIdx, const size_t chunkIdx);
            static const Data& readChunk(const size_t channelIdx, const size_t chunkIdx);

            /**
             * Write the contents of a single Hash into the cache. The Hash will
             * be serialized before control is returned to the caller. Therefore
             * it is safe to mutate the Hash after writing it.
             * @param data
             * @param channelIdx
             * @param chunkIdx
             */
            static void write(const karabo::util::Hash& data, const size_t channelIdx, const size_t chunkIdx, const MetaData& metaData, bool copyAllData=true);
            static void writeChunk(const Data& chunk, const size_t channelIdx, const size_t chunkIdx, const std::vector<MetaData>& metaData);

            static void setEndOfStream(const size_t channelIdx, const size_t chunkIdx, bool eos = true);
            static bool isEndOfStream(const size_t channelIdx, const size_t chunkIdx);

            static size_t getChannelIdxFromName(const std::string& name);

            static size_t registerChannel(const std::string& name = "");
            static void unregisterChannel(const size_t channelIdx);

            static void incrementChannelUsage(const size_t& channelIdx);
            static void decrementChannelUsage(const size_t& channelIdx);

            static size_t registerChunk(const size_t channelIdx);
            static void unregisterChunk(const size_t channelIdx, const size_t chunkIdx);

            static void incrementChunkUsage(const size_t& channelIdx, const size_t& chunkIdx);
            static void decrementChunkUsage(const size_t& channelIdx, const size_t& chunkIdx);

            static void clearChunkData(const size_t & channelIdx, const size_t & chunkIdx);

            static int getChannelStatus(const size_t channelIdx);
            static void setChannelStatus(const size_t channelIdx, const int status);
            static int getChunkStatus(const size_t channelIdx, const size_t chunkIdx);

            static void assureAllDataIsCopied(const size_t channelIdx, const size_t chunkIdx);
            static void cacheAsContiguousBlock(const size_t channelIdx, const size_t chunkIdx);
            static const SerializedChunk& readContiguousBlockCache(const size_t channelIdx, const size_t chunkIdx);

            static void readAsContiguousBlock(std::vector<karabo::io::BufferSet::Pointer>& buffers,
                                              karabo::util::Hash& header,
                                              const size_t channelIdx,
                                              const size_t chunkIdx);

            static void writeAsContiguousBlock(const std::vector<karabo::io::BufferSet::Pointer>& buffers,
                                               const karabo::util::Hash& header,
                                               const size_t channelIdx,
                                               const size_t chunkIdx,
                                               bool copyAllData = false);

            static void clearContiguousBlockCache(const size_t channelIdx, const size_t chunkIdx);

            static size_t size(const size_t channelIdx, const size_t chunkIdx);

            /**
             * Return a vector of MetaData objects for the data tokens in the bucket identified by channelIdx and chunkIdx.
             * @param channelIdx
             * @param chunkIdx
             * @return
             */
            static const std::vector<Memory::MetaData>& getMetaData(const size_t channelIdx, const size_t chunkIdx);


        private:

            static void _ensureSerializer();

        };

    }
}



#endif

