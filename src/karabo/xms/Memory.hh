/*
 * $Id$
 *
 * File:   Memory.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on June 05, 2012, 12:44 PM
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

#ifndef KARABO_XMS_MEMORY_HH
#define KARABO_XMS_MEMORY_HH

#include <karabo/io/BinarySerializer.hh>
#include <karabo/io/BufferSet.hh>
#include <karabo/log/Logger.hh>
#include <karabo/util/Factory.hh>

namespace karabo {
    namespace xms {

        /**
         * The Memory class is an internal utility for InputChannel and OutputChannel to provide static shared memory.
         */
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
            typedef std::shared_ptr<DataType> DataPointer;
            typedef std::vector<DataPointer> Data;
            typedef std::vector<Data> Chunks;
            typedef std::vector<Chunks> Channels;

            typedef std::vector<MetaData> MetaDataEntries;
            typedef std::vector<MetaDataEntries> ChunkMetaDataEntries;
            typedef std::vector<ChunkMetaDataEntries> ChannelMetaDataEntries;

            typedef std::vector<std::vector<int>> ChunkStatus;
            typedef std::vector<int> ChannelStatus;

            typedef karabo::io::BinarySerializer<karabo::util::Hash> SerializerType;

            static ChunkStatus m_chunkStatus;
            static ChannelStatus m_channelStatus;

            static Channels m_cache;
            static ChannelMetaDataEntries m_metaData;
            static std::vector<std::vector<bool>> m_isEndOfStream;

            static std::mutex m_accessMutex;

            static std::shared_ptr<SerializerType> m_serializer;

            static const int MAX_N_CHANNELS = 128;
            static const int MAX_N_CHUNKS = 2056;

            Memory() {}

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
            static void read(karabo::util::Hash& data, const size_t dataIdx, const size_t channelIdx,
                             const size_t chunkIdx);

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
             * be serialized before control is returned to the caller. Note that the data of an NDArray inside the Hash
             * will not be copied, i.e. the Memory internal buffer will point to the same memory as the NDArray,
             * except if copyAllData = true.
             * it is safe to mutate the Hash after writing it.
             * @param data input
             * @param channelIdx where to store the serialised data
             * @param chunkIdx where to store the serialised data
             * @param metaData of the data
             * @param copyAllData defines whether all data (incl. NDArray data) is copied into the internal buffer
             * (default: true)
             */
            static void write(const karabo::util::Hash& data, const size_t channelIdx, const size_t chunkIdx,
                              const MetaData& metaData, bool copyAllData = true);
            static void writeChunk(const Data& chunk, const size_t channelIdx, const size_t chunkIdx,
                                   const std::vector<MetaData>& metaData);

            static void setEndOfStream(const size_t channelIdx, const size_t chunkIdx, bool eos = true);
            static bool isEndOfStream(const size_t channelIdx, const size_t chunkIdx);

            static size_t registerChannel();
            static void unregisterChannel(const size_t channelIdx);

            static void incrementChannelUsage(const size_t& channelIdx);
            static void decrementChannelUsage(const size_t& channelIdx);

            static size_t registerChunk(const size_t channelIdx);
            static void unregisterChunk(const size_t channelIdx, const size_t chunkIdx);

            static void incrementChunkUsage(const size_t& channelIdx, const size_t& chunkIdx);
            static void decrementChunkUsage(const size_t& channelIdx, const size_t& chunkIdx);

            static void clearChunkData(const size_t& channelIdx, const size_t& chunkIdx);

            static int getChannelStatus(const size_t channelIdx);
            static void setChannelStatus(const size_t channelIdx, const int status);
            static int getChunkStatus(const size_t channelIdx, const size_t chunkIdx);

            /**
             * Ensure that the data of given chunk is not shared with anyone else, i.e. copy data if needed.
             *
             * @param channelIdx
             * @param chunkIdx
             */
            static void assureAllDataIsCopied(const size_t channelIdx, const size_t chunkIdx);

            static void readIntoBuffers(std::vector<karabo::io::BufferSet::Pointer>& buffers,
                                        karabo::util::Hash& header, const size_t channelIdx, const size_t chunkIdx);

            static void writeFromBuffers(const Data& /*std::vector<karabo::io::BufferSet::Pointer>&*/ buffers,
                                         const karabo::util::Hash& header, const size_t channelIdx,
                                         const size_t chunkIdx, bool copyAllData = false);

            static size_t size(const size_t channelIdx, const size_t chunkIdx);

            /**
             * Return a vector of MetaData objects for the data tokens in the bucket identified by channelIdx and
             * chunkIdx.
             * @param channelIdx
             * @param chunkIdx
             * @return
             */
            static const std::vector<Memory::MetaData>& getMetaData(const size_t channelIdx, const size_t chunkIdx);


           private:
            static void _ensureSerializer();
        };

    } // namespace xms
} // namespace karabo


#endif
