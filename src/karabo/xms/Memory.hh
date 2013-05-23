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

namespace karabo {
    namespace xms {

        enum MemoryStatus {

            EMPTY = 0,
            UNREAD,
            READ,
            CACHED,
            CAN_DELETE
        };

        template <class T>
        class Memory {

            typedef T DataType;
            typedef boost::shared_ptr<DataType> DataPointer;
            typedef std::vector< DataPointer > Data;
            typedef std::vector< Data > Chunks;
            typedef std::vector< Chunks > Channels;

            typedef std::pair<std::vector<char>, karabo::util::Hash> SerializedChunk;
            typedef boost::shared_ptr<SerializedChunk> SerializedChunkPointer;
            typedef std::vector< SerializedChunkPointer > SerializedChunks;
            typedef std::vector< SerializedChunks > SerializedChannels;

            typedef std::vector<std::vector<bool> > ChunkStatus;
            typedef std::vector<bool> ChannelStatus;

            static std::map<std::string, size_t> m_name2Idx;

            static std::vector<std::vector<bool> > m_chunkStatus;
            static std::vector<bool> m_channelStatus;

            static Channels m_cache;
            static SerializedChannels m_serializedCache;

            static boost::mutex m_accessMutex;

            static boost::shared_ptr<karabo::io::BinarySerializer<DataType> > m_serializer;

            static const int MAX_N_CHANNELS = 512;
            static const int MAX_N_CHUNKS = 512;

            Memory() {
            }

        public:

            KARABO_CLASSINFO(Memory, "Memory", "1.0")

            static void write(const DataType& data, const size_t channelIdx, const size_t chunkIdx) {
                m_cache[channelIdx][chunkIdx].push_back(DataPointer(new DataType(data)));
            }

            static void writeChunk(const Data& chunk, const size_t channelIdx, const size_t chunkIdx) {
                Data& src = m_cache[channelIdx][chunkIdx];
                src.insert(src.end(), chunk.begin(), chunk.end());
            }

            static size_t getChannelIdxFromName(const std::string& name) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                std::map<std::string, size_t>::const_iterator it = m_name2Idx.find(name);
                if (it == m_name2Idx.end()) throw KARABO_IMAGE_EXCEPTION("Requested channel \"" + name + "\" does not exist");
                return it->second;
            }

            static size_t registerChannel(const std::string& name = "") {
                for (size_t i = 0; i < m_cache.size(); ++i) { // Find free slot
                    if (m_channelStatus[i] == EMPTY) { // Found a free channel
                        boost::mutex::scoped_lock lock(m_accessMutex);
                        if (!name.empty()) m_name2Idx[name] = i;
                        m_channelStatus[i] = UNREAD;
                        return i;
                    }
                }
                throw KARABO_MEMORY_INIT_EXCEPTION("Total number channels is exhausted");
            }

            static size_t registerChunk(const size_t channelIdx) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                for (size_t i = 0; i < m_cache[channelIdx].size(); ++i) { // Find free chunk
                    if (m_chunkStatus[channelIdx][i] == EMPTY) { // Found a free channel
                        m_cache[channelIdx][i] = Data();
                        m_chunkStatus[channelIdx][i] = UNREAD;
                        return i;
                    }
                }
                throw KARABO_MEMORY_INIT_EXCEPTION("Total number chunks is exhausted");
            }

            static int getChannelStatus(const size_t channelIdx) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                return m_channelStatus[channelIdx];
            }

            static void setChannelStatus(const size_t channelIdx, const int status) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                m_channelStatus[channelIdx] = status;
            }

            static int getChunkStatus(const size_t channelIdx, const size_t chunkIdx) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                return m_chunkStatus[channelIdx][chunkIdx];
            }

            static void setChunkStatus(const size_t channelIdx, const size_t chunkIdx, const int status) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                m_chunkStatus[channelIdx][chunkIdx] = status;
            }

            static bool setChunkStatusFromTo(const size_t channelIdx, const size_t chunkIdx, const int statusFrom, const int statusTo) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                if (m_chunkStatus[channelIdx][chunkIdx] == statusFrom) {
                    m_chunkStatus[channelIdx][chunkIdx] = statusTo;
                    return true;
                } else {
                    return false;
                }
            }

            static void read(DataType& data, const size_t dataIdx, const size_t channelIdx, const size_t chunkIdx) {
                data = *(m_cache[channelIdx][chunkIdx][dataIdx]);
            }

            static const Data& readChunk(const size_t channelIdx, const size_t chunkIdx) {
                return m_cache[channelIdx][chunkIdx];
            }

            static void readAsContiguosBlock(std::vector<char>& buffer, karabo::util::Hash& header, const size_t channelIdx, const size_t chunkIdx) {
                if (!m_serializer) m_serializer = karabo::io::BinarySerializer<DataType>::create("Default");
                const Data& data = m_cache[channelIdx][chunkIdx];
                std::vector<char> serializedDataElement;
                std::vector<unsigned int> byteSizes(data.size());
                size_t offset = 0;
                size_t idx = 0;
                for (typename Data::const_iterator it = data.begin(); it != data.end(); ++it) {
                    serializedDataElement.clear();
                    m_serializer->save(**it, serializedDataElement);
                    size_t byteSize = serializedDataElement.size();
                    if (it == data.begin()) buffer.reserve(byteSize * data.size());
                    buffer.resize(offset + byteSize);
                    std::memcpy(&buffer[offset], &serializedDataElement[0], byteSize);
                    offset += byteSize;
                    byteSizes[idx++] = byteSize;
                }
                header.clear();
                header.set<unsigned int>("nData", data.size());
                header.set<std::vector<unsigned int> >("byteSizes", byteSizes);
            }

            static void cacheAsContiguousBlock(const size_t channelIdx, const size_t chunkIdx) {
                SerializedChunkPointer scp = SerializedChunkPointer(new SerializedChunk);
                Memory<T>::readAsContiguosBlock(scp->first, scp->second, channelIdx, chunkIdx);
                m_serializedCache[channelIdx][chunkIdx] = scp;
            }

            static const std::pair<std::vector<char>, karabo::util::Hash>& readContiguousBlockCache(const size_t channelIdx, const size_t chunkIdx) {
                return *(m_serializedCache[channelIdx][chunkIdx]);
            }

            static void writeAsContiguosBlock(const std::vector<char>& buffer, const karabo::util::Hash& header, const size_t channelIdx, const size_t chunkIdx) {
                unsigned int nData = header.get<unsigned int>("nData");
                if (!m_serializer) m_serializer = karabo::io::BinarySerializer<DataType>::create("Default");
                const std::vector<unsigned int>& byteSizes = header.get<std::vector<unsigned int> >("byteSizes");
                Data& chunkData = m_cache[channelIdx][chunkIdx];
                size_t chunkDataIdx = chunkData.size();
                chunkData.resize(chunkData.size() + nData);
                size_t offset = 0;
                size_t idx = 0;
                for (size_t i = chunkDataIdx; i < chunkData.size(); ++i) {
                    chunkData[i] = DataPointer(new DataType());
                    size_t byteSize = byteSizes[idx++];
                    m_serializer->load(*(chunkData[i]), &buffer[offset], byteSize);
                    offset += byteSize;
                }
            }

            static void clearContiguousBlockCache(const size_t channelIdx, const size_t chunkIdx) {
                m_serializedCache[channelIdx][chunkIdx] = SerializedChunkPointer();
            }

            static void clearChannel(const size_t channelIdx) {
                m_cache[channelIdx].clear();
                m_channelStatus[channelIdx] = EMPTY;
                for (size_t i = 0; i < m_chunkStatus[channelIdx].size(); ++i) {
                    m_chunkStatus[channelIdx][i] = EMPTY;
                }
            }

            static void clearChunk(const size_t channelIdx, const size_t chunkIdx) {
                m_cache[channelIdx][chunkIdx].clear();
                m_chunkStatus[chunkIdx][chunkIdx] = EMPTY;
            }

            static size_t size(const size_t channelIdx, const size_t chunkIdx) {
                return m_cache[channelIdx][chunkIdx].size();
            }

        };

        template <>
        class Memory<std::vector<char> > {

            typedef std::vector<char> DataType;
            typedef boost::shared_ptr<DataType> DataPointer;
            typedef std::vector< DataPointer > Data;
            typedef std::vector< Data > Chunks;
            typedef std::vector< Chunks > Channels;

            typedef std::pair<std::vector<char>, karabo::util::Hash> SerializedChunk;
            typedef boost::shared_ptr<SerializedChunk> SerializedChunkPointer;
            typedef std::vector< SerializedChunkPointer > SerializedChunks;
            typedef std::vector< SerializedChunks > SerializedChannels;

            static std::map<std::string, size_t> m_name2Idx;

            static std::vector<std::vector<bool> > m_chunkStatus;
            static std::vector<bool> m_channelStatus;

            static Channels m_cache;
            static boost::mutex m_accessMutex;
            static SerializedChannels m_serializedCache;

            static const int MAX_N_CHANNELS = 2048;
            static const int MAX_N_CHUNKS = 2048;

            Memory() {
            }

        public:

            KARABO_CLASSINFO(Memory, "Memory", "1.0")

            static void write(const DataType& data, const size_t channelIdx, const size_t chunkIdx) {
                m_cache[channelIdx][chunkIdx].push_back(DataPointer(new DataType(data)));
            }

            static void writeChunk(const Data& chunk, const size_t channelIdx, const size_t chunkIdx) {
                Data& src = m_cache[channelIdx][chunkIdx];
                src.insert(src.end(), chunk.begin(), chunk.end());
            }

            static size_t getChannelIdxFromName(const std::string& name) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                std::map<std::string, size_t>::const_iterator it = m_name2Idx.find(name);
                if (it == m_name2Idx.end()) throw KARABO_IMAGE_EXCEPTION("Requested channel \"" + name + "\" does not exist");
                return it->second;
            }

            static size_t registerChannel(const std::string& name = "") {
                for (size_t i = 0; i < m_cache.size(); ++i) { // Find free slot
                    if (m_channelStatus[i] == EMPTY) { // Found a free channel
                        boost::mutex::scoped_lock lock(m_accessMutex);
                        if (!name.empty()) m_name2Idx[name] = i;
                        m_channelStatus[i] = UNREAD;
                        return i;
                    }
                }
                throw KARABO_MEMORY_INIT_EXCEPTION("Total number of channels is exhausted");
            }

            static size_t registerChunk(const size_t channelIdx) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                for (size_t i = 0; i < m_cache[channelIdx].size(); ++i) { // Find free chunk
                    if (m_chunkStatus[channelIdx][i] == EMPTY) { // Found a free channel
                        m_cache[channelIdx][i] = Data();
                        m_chunkStatus[channelIdx][i] = UNREAD;
                        return i;
                    }
                }
                throw KARABO_MEMORY_INIT_EXCEPTION("Total number of chunks is exhausted");
            }

            static int getChannelStatus(const size_t channelIdx) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                return m_channelStatus[channelIdx];
            }

            static void setChannelStatus(const size_t channelIdx, const int status) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                m_channelStatus[channelIdx] = status;
            }

            static int getChunkStatus(const size_t channelIdx, const size_t chunkIdx) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                return m_chunkStatus[channelIdx][chunkIdx];
            }

            static void setChunkStatus(const size_t channelIdx, const size_t chunkIdx, const int status) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                m_chunkStatus[channelIdx][chunkIdx] = status;
            }

            static bool setChunkStatusFromTo(const size_t channelIdx, const size_t chunkIdx, const int statusFrom, const int statusTo) {
                boost::mutex::scoped_lock lock(m_accessMutex);
                if (m_chunkStatus[channelIdx][chunkIdx] == statusFrom) {
                    m_chunkStatus[channelIdx][chunkIdx] = statusTo;
                    return true;
                } else {
                    return false;
                }
            }

            static void read(DataType& data, const size_t dataIdx, const size_t channelIdx, const size_t chunkIdx) {
                data = *(m_cache[channelIdx][chunkIdx][dataIdx]);
            }

            static const Data& readChunk(const size_t channelIdx, const size_t chunkIdx) {
                return m_cache[channelIdx][chunkIdx];
            }

            static void readAsContiguosBlock(std::vector<char>& buffer, karabo::util::Hash& header, const size_t channelIdx, const size_t chunkIdx) {
                const Data& data = m_cache[channelIdx][chunkIdx];
                std::vector<unsigned int> byteSizes(data.size());
                size_t offset = 0;
                size_t idx = 0;
                for (Data::const_iterator it = data.begin(); it != data.end(); ++it) {
                    size_t byteSize = (*it)->size();
                    if (it == data.begin()) buffer.reserve(byteSize * data.size());
                    buffer.resize(offset + byteSize);
                    std::memcpy(&buffer[offset], &(**it)[0], byteSize);
                    offset += byteSize;
                    byteSizes[idx++] = byteSize;
                }
                header.clear();
                header.set<unsigned int>("nData", data.size());
                header.set<std::vector<unsigned int> >("byteSizes", byteSizes);

            }

            static void cacheAsContiguousBlock(const size_t channelIdx, const size_t chunkIdx) {
                SerializedChunkPointer scp = SerializedChunkPointer(new SerializedChunk);
                Memory<std::vector<char> >::readAsContiguosBlock(scp->first, scp->second, channelIdx, chunkIdx);
                m_serializedCache[channelIdx][chunkIdx] = scp;
            }

            static const std::pair<std::vector<char>, karabo::util::Hash>& readContiguousBlockCache(const size_t channelIdx, const size_t chunkIdx) {
                return *(m_serializedCache[channelIdx][chunkIdx]);
            }

            static void clearContiguousBlockCache(const size_t channelIdx, const size_t chunkIdx) {
                m_serializedCache[channelIdx][chunkIdx] = SerializedChunkPointer();
            }

            static void writeAsContiguosBlock(const std::vector<char>& buffer, const karabo::util::Hash& header, const size_t channelIdx, const size_t chunkIdx) {
                unsigned int nData = header.get<unsigned int>("nData");
                const std::vector<unsigned int>& byteSizes = header.get<std::vector<unsigned int> >("byteSizes");
                m_cache[channelIdx][chunkIdx] = Data(nData);
                Data& chunkData = m_cache[channelIdx][chunkIdx];
                size_t offset = 0;
                size_t idx = 0;
                for (Data::iterator it = chunkData.begin(); it != chunkData.end(); ++it) {
                    size_t byteSize = byteSizes[idx++];
                    *it = DataPointer(new DataType(byteSize));
                    std::memcpy(&(**it)[0], &buffer[offset], byteSize);
                    offset += byteSize;
                }
            }

            static void clearChannel(const size_t channelIdx) {
                m_cache[channelIdx].clear();
                m_channelStatus[channelIdx] = EMPTY;
                for (size_t i = 0; i < m_chunkStatus[channelIdx].size(); ++i) {
                    m_chunkStatus[channelIdx][i] = EMPTY;
                }
            }

            static void clearChunk(const size_t channelIdx, const size_t chunkIdx) {
                m_cache[channelIdx][chunkIdx].clear();
                m_chunkStatus[chunkIdx][chunkIdx] = EMPTY;
            }

            static size_t size(const size_t channelIdx, const size_t chunkIdx) {
                return m_cache[channelIdx][chunkIdx].size();
            }

        };


        // Static initializations
        template <class T>
        //std::vector< std::vector< std::vector<boost::shared_ptr<T> > > > Memory<T>::m_cache = std::vector< std::vector< std::vector< boost::shared_ptr<T> > > >(MAX_N_CHANNELS, std::vector< std::vector< boost::shared_ptr<T> > >(MAX_N_CHUNKS));
        typename Memory<T>::Channels Memory<T>::m_cache = typename Memory<T>::Channels(MAX_N_CHANNELS, typename Memory<T>::Chunks(MAX_N_CHUNKS));

        template <class T>
        //std::vector<std::vector<bool> > Memory<T>::m_chunkStatus = std::vector<std::vector<bool> >(MAX_N_CHANNELS, std::vector<bool>(MAX_N_CHUNKS));
        typename Memory<T>::ChunkStatus Memory<T>::m_chunkStatus = typename Memory<T>::ChunkStatus(MAX_N_CHANNELS, std::vector<bool> (MAX_N_CHUNKS));

        template <class T>
        typename Memory<T>::ChannelStatus Memory<T>::m_channelStatus = typename Memory<T>::ChannelStatus(MAX_N_CHANNELS);

        template <class T>
        typename Memory<T>::SerializedChannels Memory<T>::m_serializedCache = typename Memory<T>::SerializedChannels(MAX_N_CHANNELS, typename Memory<T>::SerializedChunks(MAX_N_CHUNKS));

        template <class T>
        std::map<std::string, size_t> Memory<T>::m_name2Idx;

        template <class T>
        boost::mutex Memory<T>::m_accessMutex;

        template <class T>
        boost::shared_ptr<karabo::io::BinarySerializer<T> > Memory<T>::m_serializer;
    }
}



#endif	

