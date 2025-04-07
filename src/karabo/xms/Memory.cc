/*
 * $Id$
 *
 * File:   Memory.cc
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

#include "Memory.hh"

#include "karabo/data/io/HashBinarySerializer.hh"

namespace karabo {
    namespace xms {

        // Static initializations
        // std::vector< std::vector< std::vector<std::shared_ptr<std::vector<char> > > > Memory::m_cache =
        // std::vector< std::vector< std::vector< std::shared_ptr<std::vector<char> > > > >(MAX_N_CHANNELS,
        // std::vector< std::vector< std::shared_ptr<std::vector<char> > > >(MAX_N_CHUNKS));
        Memory::Channels Memory::m_cache = Memory::Channels(MAX_N_CHANNELS, Memory::Chunks(MAX_N_CHUNKS));
        Memory::ChannelMetaDataEntries Memory::m_metaData =
              Memory::ChannelMetaDataEntries(MAX_N_CHANNELS, Memory::ChunkMetaDataEntries(MAX_N_CHUNKS));
        std::vector<std::vector<bool>> Memory::m_isEndOfStream(MAX_N_CHANNELS, std::vector<bool>(MAX_N_CHUNKS, false));

        // std::vector<std::vector<bool> > Memory::m_chunkStatus = std::vector<std::vector<bool> >(MAX_N_CHANNELS,
        // std::vector<bool>(MAX_N_CHUNKS));
        Memory::ChunkStatus Memory::m_chunkStatus =
              Memory::ChunkStatus(MAX_N_CHANNELS, std::vector<int>(MAX_N_CHUNKS, 0));

        Memory::ChannelStatus Memory::m_channelStatus = Memory::ChannelStatus(MAX_N_CHANNELS, 0);

        std::mutex Memory::m_accessMutex;

        std::shared_ptr<Memory::SerializerType> Memory::m_serializer;


        void Memory::read(karabo::data::Hash& data, const size_t dataIdx, const size_t channelIdx,
                          const size_t chunkIdx) {
            Memory::_ensureSerializer();

            data.clear();

            const DataPointer& bufferPtr = m_cache[channelIdx][chunkIdx][dataIdx];
            m_serializer->load(data, *bufferPtr);
        }

        Memory::DataPointer Memory::read(const size_t dataIdx, const size_t channelIdx, const size_t chunkIdx) {
            return m_cache[channelIdx][chunkIdx][dataIdx];
        }

        const Memory::Data& Memory::readChunk(const size_t channelIdx, const size_t chunkIdx) {
            return m_cache[channelIdx][chunkIdx];
        }

        void Memory::write(const karabo::data::Hash& data, const size_t channelIdx, const size_t chunkIdx,
                           const MetaData& metaData, bool copyAllData) {
            Memory::_ensureSerializer();

            DataPointer buffer(new DataType(copyAllData));
            m_serializer->save(data, *buffer);
            m_cache[channelIdx][chunkIdx].push_back(buffer);
            m_metaData[channelIdx][chunkIdx].push_back(metaData);
        }

        void Memory::writeChunk(const Memory::Data& chunk, const size_t channelIdx, const size_t chunkIdx,
                                const std::vector<MetaData>& metaData) {
            if (chunk.size() != metaData.size()) {
                throw KARABO_LOGIC_EXCEPTION("Number of data tokens and number of meta data entries must be equal!");
            }
            Data& src = m_cache[channelIdx][chunkIdx];
            src.insert(src.end(), chunk.begin(), chunk.end());
            MetaDataEntries& srcInfo = m_metaData[channelIdx][chunkIdx];
            srcInfo.insert(srcInfo.end(), metaData.begin(), metaData.end());
        }


        void Memory::setEndOfStream(const size_t channelIdx, const size_t chunkIdx, bool isEos) {
            m_isEndOfStream[channelIdx][chunkIdx] = isEos;
        }


        bool Memory::isEndOfStream(const size_t channelIdx, const size_t chunkIdx) {
            return m_isEndOfStream[channelIdx][chunkIdx];
        }


        size_t Memory::registerChannel() {
            std::lock_guard<std::mutex> lock(m_accessMutex);
            for (size_t i = 0; i < m_cache.size(); ++i) { // Find free channel
                if (m_channelStatus[i] == 0) {            // Found a free channel
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
            std::lock_guard<std::mutex> lock(m_accessMutex);
            m_channelStatus[channelIdx]++;
        }

        void Memory::decrementChannelUsage(const size_t& channelIdx) {
            std::lock_guard<std::mutex> lock(m_accessMutex);
            if (--m_channelStatus[channelIdx] == 0) {
                for (size_t i = 0; i < m_chunkStatus[channelIdx].size(); ++i) {
                    m_chunkStatus[channelIdx][i] = 0;
                    m_cache[channelIdx][i].clear();
                    m_metaData[channelIdx][i].clear();
                }
            }
        }

        size_t Memory::registerChunk(const size_t channelIdx) {
            std::lock_guard<std::mutex> lock(m_accessMutex);
            for (size_t i = 0; i < m_cache[channelIdx].size(); ++i) { // Find free chunk
                if (m_chunkStatus[channelIdx][i] == 0) {              // Found a free chunk
                    m_cache[channelIdx][i] = Data();
                    m_metaData[channelIdx][i] = MetaDataEntries();
                    m_chunkStatus[channelIdx][i] = 1;
                    m_isEndOfStream[channelIdx][i] = false;
                    return i;
                }
            }
            throw KARABO_MEMORY_INIT_EXCEPTION("Total number chunks is exhausted");
        }

        void Memory::unregisterChunk(const size_t channelIdx, const size_t chunkIdx) {
            decrementChunkUsage(channelIdx, chunkIdx);
        }

        void Memory::incrementChunkUsage(const size_t& channelIdx, const size_t& chunkIdx) {
            std::lock_guard<std::mutex> lock(m_accessMutex);
            m_chunkStatus[channelIdx][chunkIdx]++;
        }

        void Memory::decrementChunkUsage(const size_t& channelIdx, const size_t& chunkIdx) {
            std::lock_guard<std::mutex> lock(m_accessMutex);
            if (--m_chunkStatus[channelIdx][chunkIdx] == 0) {
                KARABO_LOG_FRAMEWORK_TRACE << "Freeing memory for [" << channelIdx << "][" << chunkIdx << "]";
                clearChunkData(channelIdx, chunkIdx);
            }
        }

        void Memory::clearChunkData(const size_t& channelIdx, const size_t& chunkIdx) {
            m_cache[channelIdx][chunkIdx].clear();
            m_metaData[channelIdx][chunkIdx].clear();
            m_isEndOfStream[channelIdx][chunkIdx] = false;
        }

        int Memory::getChannelStatus(const size_t channelIdx) {
            std::lock_guard<std::mutex> lock(m_accessMutex);
            return m_channelStatus[channelIdx];
        }

        void Memory::setChannelStatus(const size_t channelIdx, const int status) {
            std::lock_guard<std::mutex> lock(m_accessMutex);
            m_channelStatus[channelIdx] = status;
        }

        int Memory::getChunkStatus(const size_t channelIdx, const size_t chunkIdx) {
            std::lock_guard<std::mutex> lock(m_accessMutex);
            return m_chunkStatus[channelIdx][chunkIdx];
        }


        void Memory::readIntoBuffers(std::vector<karabo::data::BufferSet::Pointer>& buffers, karabo::data::Hash& header,
                                     const size_t channelIdx, const size_t chunkIdx) {
            const Data& data = m_cache[channelIdx][chunkIdx];
            for (const auto& bp : data) {
                buffers.push_back(bp);
            }
            const MetaDataEntries& metaData = m_metaData[channelIdx][chunkIdx];
            header.clear();
            header.set("sourceInfo", *reinterpret_cast<const std::vector<karabo::data::Hash>*>(&metaData));
        }

        void Memory::assureAllDataIsCopied(const size_t channelIdx, const size_t chunkIdx) {
            const Data& data = m_cache[channelIdx][chunkIdx];

            bool containsNonCopies = false;
            for (Data::const_iterator it = data.begin(); it != data.end(); ++it) {
                containsNonCopies |= (*it)->containsNonCopies();
            }
            if (!containsNonCopies) {
                return; // all good, no need to copy
            }

            Data copiedData(data.size());
            for (size_t i = 0; i < data.size(); ++i) {
                copiedData[i] = DataPointer(new DataType(true));
                data[i]->appendTo(*(copiedData[i]), false);
                copiedData[i]->rewind();
            }

            m_cache[channelIdx][chunkIdx].swap(copiedData);
        }


        void Memory::writeFromBuffers(const std::vector<karabo::data::BufferSet::Pointer>& buffers,
                                      const karabo::data::Hash& header, const size_t channelIdx, const size_t chunkIdx,
                                      bool copyAllData) {
            Data& chunkData = m_cache[channelIdx][chunkIdx];

            boost::optional<const karabo::data::Hash::Node&> sourceInfo = header.find("sourceInfo");
            if (sourceInfo) {
                const MetaDataEntries& newMetaData = *reinterpret_cast<const MetaDataEntries*>(
                      &sourceInfo->getValue<std::vector<karabo::data::Hash>>());
                if (buffers.size() != newMetaData.size()) {
                    throw KARABO_LOGIC_EXCEPTION(
                          "Number of data tokens and number of meta data entries must be equal!");
                }
                MetaDataEntries& metaData = m_metaData[channelIdx][chunkIdx];
                metaData.insert(metaData.end(), newMetaData.begin(), newMetaData.end());
            } else if (!buffers.empty()) {
                throw KARABO_LOGIC_EXCEPTION("Data tokens given, but header lacks meta data info!");
            }
            chunkData.insert(chunkData.end(), buffers.begin(), buffers.end());
        }

        size_t Memory::size(const size_t channelIdx, const size_t chunkIdx) {
            return m_cache[channelIdx][chunkIdx].size();
        }

        void Memory::_ensureSerializer() {
            if (!m_serializer) {
                m_serializer = SerializerType::create("Bin");
            }
        }

        const std::vector<Memory::MetaData>& Memory::getMetaData(const size_t channelIdx, const size_t chunkIdx) {
            return m_metaData[channelIdx][chunkIdx];
        }


    } // namespace xms
} // namespace karabo
