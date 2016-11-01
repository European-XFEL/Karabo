/*
 * $Id$
 *
 * File:   Memory.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on June 05, 2012, 12:44 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Memory.hh"

namespace karabo {
    namespace xms {

        // Static initializations
        //std::vector< std::vector< std::vector<boost::shared_ptr<std::vector<char> > > > Memory::m_cache = std::vector< std::vector< std::vector< boost::shared_ptr<std::vector<char> > > > >(MAX_N_CHANNELS, std::vector< std::vector< boost::shared_ptr<std::vector<char> > > >(MAX_N_CHUNKS));
        Memory::Channels Memory::m_cache = Memory::Channels(MAX_N_CHANNELS, Memory::Chunks(MAX_N_CHUNKS));
        Memory::ChannelMetaHash Memory::m_metaData = Memory::ChannelMetaHash(MAX_N_CHANNELS, Memory::ChunkMetaHash(MAX_N_CHUNKS));

        //std::vector<std::vector<bool> > Memory::m_chunkStatus = std::vector<std::vector<bool> >(MAX_N_CHANNELS, std::vector<bool>(MAX_N_CHUNKS));
        Memory::ChunkStatus Memory::m_chunkStatus = Memory::ChunkStatus(MAX_N_CHANNELS, std::vector<int> (MAX_N_CHUNKS, 0));

        Memory::ChannelStatus Memory::m_channelStatus = Memory::ChannelStatus(MAX_N_CHANNELS, 0);

        Memory::SerializedChannels Memory::m_serializedCache = Memory::SerializedChannels(MAX_N_CHANNELS, Memory::SerializedChunks(MAX_N_CHUNKS));

        Memory::ChannelUsers Memory::m_cacheUsers = Memory::ChannelUsers(MAX_N_CHANNELS, Memory::ChunkUsers(MAX_N_CHUNKS, 0));

        std::map<std::string, size_t> Memory::m_name2Idx;

        boost::mutex Memory::m_accessMutex;

        boost::shared_ptr<Memory::SerializerType > Memory::m_serializer;


        void Memory::read(karabo::util::Hash& data, const size_t dataIdx, const size_t channelIdx, const size_t chunkIdx) {
            Memory::_ensureSerializer();

            data.clear();

            const DataPointer& bufferPtr = m_cache[channelIdx][chunkIdx][dataIdx];
            m_serializer->load(data, &(*bufferPtr)[0], bufferPtr->size());
        }

        Memory::DataPointer Memory::read(const size_t dataIdx, const size_t channelIdx, const size_t chunkIdx) {
            return m_cache[channelIdx][chunkIdx][dataIdx];
        }

        const Memory::Data& Memory::readChunk(const size_t channelIdx, const size_t chunkIdx) {
            return m_cache[channelIdx][chunkIdx];
        }

        void Memory::write(const karabo::util::Hash& data, const size_t channelIdx, const size_t chunkIdx, const MetaData& metaData) {
            Memory::_ensureSerializer();

            DataPointer buffer(new DataType());
            m_serializer->save(data, *buffer);
            m_cache[channelIdx][chunkIdx].push_back(buffer);
            m_metaData[channelIdx][chunkIdx].push_back(metaData);
        }

        void Memory::writeChunk(const Memory::Data& chunk, const size_t channelIdx, const size_t chunkIdx, const std::vector<MetaData>& metaData) {
            Data& src = m_cache[channelIdx][chunkIdx];
            src.insert(src.end(), chunk.begin(), chunk.end());
            MetaHash& srcInfo =  m_metaData[channelIdx][chunkIdx];
            srcInfo.insert(srcInfo.end(), metaData.begin(), metaData.end());
        }

        size_t Memory::getChannelIdxFromName(const std::string& name) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            std::map<std::string, size_t>::const_iterator it = m_name2Idx.find(name);
            if (it == m_name2Idx.end()) throw KARABO_IMAGE_EXCEPTION("Requested channel \"" + name + "\" does not exist");
            return it->second;
        }

        size_t Memory::registerChannel(const std::string& name) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            for (size_t i = 0; i < m_cache.size(); ++i) { // Find free channel
                if (m_channelStatus[i] == 0) { // Found a free channel
                    if (!name.empty()) m_name2Idx[name] = i;
                    m_channelStatus[i] = 1;
                    return i;
                }
            }
            throw KARABO_MEMORY_INIT_EXCEPTION("Total number channels is exhausted");
        }

        void Memory::unregisterChannel(const size_t channelIdx) {
            decrementChannelUsage(channelIdx);
        }

        void Memory::incrementChannelUsage(const size_t& channelIdx) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            m_channelStatus[channelIdx]++;
        }

        void Memory::decrementChannelUsage(const size_t& channelIdx) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            if (--m_channelStatus[channelIdx] == 0) {
                for (size_t i = 0; i < m_chunkStatus[channelIdx].size(); ++i) {
                    m_chunkStatus[channelIdx][i] = 0;
                    m_cache[channelIdx][i].clear();
                    m_metaData[channelIdx][i].clear();
                }
            }
        }

        size_t Memory::registerChunk(const size_t channelIdx) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            for (size_t i = 0; i < m_cache[channelIdx].size(); ++i) { // Find free chunk
                if (m_chunkStatus[channelIdx][i] == 0) { // Found a free chunk
                    m_cache[channelIdx][i] = Data();
                    m_metaData[channelIdx][i] = MetaHash();
                    m_chunkStatus[channelIdx][i] = 1;
                    return i;
                }
            }
            throw KARABO_MEMORY_INIT_EXCEPTION("Total number chunks is exhausted");
        }

        void Memory::unregisterChunk(const size_t channelIdx, const size_t chunkIdx) {
            decrementChunkUsage(channelIdx, chunkIdx);
        }

        void Memory::incrementChunkUsage(const size_t& channelIdx, const size_t& chunkIdx) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            m_chunkStatus[channelIdx][chunkIdx]++;
        }

        void Memory::decrementChunkUsage(const size_t& channelIdx, const size_t& chunkIdx) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            if (--m_chunkStatus[channelIdx][chunkIdx] == 0) {
                KARABO_LOG_FRAMEWORK_TRACE << "Freeing memory for [" << channelIdx << "][" << chunkIdx << "]";
                m_cache[channelIdx][chunkIdx].clear();
                m_metaData[channelIdx][chunkIdx].clear();
            }
        }

        void Memory::clearChunkData(const size_t & channelIdx, const size_t & chunkIdx) {
            m_cache[channelIdx][chunkIdx].clear();
            m_metaData[channelIdx][chunkIdx].clear();
        }

        int Memory::getChannelStatus(const size_t channelIdx) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            return m_channelStatus[channelIdx];
        }

        void Memory::setChannelStatus(const size_t channelIdx, const int status) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            m_channelStatus[channelIdx] = status;
        }

        int Memory::getChunkStatus(const size_t channelIdx, const size_t chunkIdx) {
            boost::mutex::scoped_lock lock(m_accessMutex);
            return m_chunkStatus[channelIdx][chunkIdx];
        }

        void Memory::readAsContiguousBlock(std::vector<char>& buffer, karabo::util::Hash& header, const size_t channelIdx, const size_t chunkIdx) {
            Memory::_ensureSerializer();

            const Data& data = m_cache[channelIdx][chunkIdx];
            const MetaHash& metaData = m_metaData[channelIdx][chunkIdx];
            std::vector<unsigned int> byteSizes;
            size_t totalSize = 0;
            byteSizes.reserve(data.size());
            for (Data::const_iterator it = data.begin(); it != data.end(); ++it) {
                byteSizes.push_back((*it)->size());
                totalSize += byteSizes.back();
            }

            size_t offset = 0;
            size_t idx = 0;
            buffer.resize(totalSize);  // .resize() because we're using memcpy and not push_back
            for (Data::const_iterator it = data.begin(); it != data.end(); ++it) {
                const DataType& serializedDataElement = **it;
                const size_t byteSize = byteSizes[idx++];
                std::memcpy(&buffer[offset], &serializedDataElement[0], byteSize);
                offset += byteSize;
            }
            header.clear();
            header.set<unsigned int>("nData", data.size());
            header.set<std::vector<unsigned int> >("byteSizes", byteSizes);
            header.set("sourceInfo", metaData);
        }

        void Memory::cacheAsContiguousBlock(const size_t channelIdx, const size_t chunkIdx) {
            SerializedChunkPointer scp = SerializedChunkPointer(new SerializedChunk);
            Memory::readAsContiguousBlock(scp->first, scp->second, channelIdx, chunkIdx);
            //boost::shared_lock<boost::shared_mutex> lock(*m_serializedCacheMutexes[channelIdx][chunkIdx]);
            m_serializedCache[channelIdx][chunkIdx] = scp;
        }

        const Memory::SerializedChunk& Memory::readContiguousBlockCache(const size_t channelIdx, const size_t chunkIdx) {
            return *(m_serializedCache[channelIdx][chunkIdx]);
        }

        void Memory::writeAsContiguousBlock(const std::vector<char>& buffer, const karabo::util::Hash& header, const size_t channelIdx, const size_t chunkIdx) {
            Memory::_ensureSerializer();

            unsigned int nData = header.get<unsigned int>("nData");
            const std::vector<unsigned int>& byteSizes = header.get<std::vector<unsigned int> >("byteSizes");
            const MetaHash& metaData = header.get<MetaHash>("sourceInfo");
            m_metaData[channelIdx][chunkIdx] = metaData;

            Data& chunkData = m_cache[channelIdx][chunkIdx];
            size_t chunkDataIdx = chunkData.size();
            chunkData.resize(chunkData.size() + nData);
            size_t offset = 0;
            size_t idx = 0;
            for (size_t i = chunkDataIdx; i < chunkData.size(); ++i) {
                chunkData[i] = DataPointer(new DataType());
                const size_t byteSize = byteSizes[idx++];
                DataType& destBuffer = *chunkData[i];

                destBuffer.resize(byteSize);
                std::memcpy(&destBuffer[0], &buffer[offset], byteSize);
                offset += byteSize;
            }
        }

        void Memory::clearContiguousBlockCache(const size_t channelIdx, const size_t chunkIdx) {
            m_serializedCache[channelIdx][chunkIdx] = SerializedChunkPointer();
        }

        size_t Memory::size(const size_t channelIdx, const size_t chunkIdx) {
            return m_cache[channelIdx][chunkIdx].size();
        }

        void Memory::_ensureSerializer(){
            if (!m_serializer) {
                m_serializer = SerializerType::create("Bin");
            }
        }

        const std::vector<Memory::MetaData>& Memory::getMetaData(const size_t channelIdx, const size_t chunkIdx) {
            return m_metaData[channelIdx][chunkIdx];
        }


    }
}