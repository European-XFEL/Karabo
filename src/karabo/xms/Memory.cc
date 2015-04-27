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

//        // Static initializations of specialized members
//
//        std::vector< std::vector< std::vector<boost::shared_ptr<std::vector<char> > > > > Memory<std::vector<char> >::m_cache = std::vector< std::vector< std::vector< boost::shared_ptr<std::vector<char> > > > >(MAX_N_CHANNELS, std::vector< std::vector< boost::shared_ptr<std::vector<char> > > >(MAX_N_CHUNKS));
//
//        Memory<std::vector<char> >::ChunkStatus Memory<std::vector<char> >::m_chunkStatus =  Memory<std::vector<char> >::ChunkStatus(MAX_N_CHANNELS, std::vector<int>(MAX_N_CHUNKS, 0));
//
//        Memory<std::vector<char> >::ChannelStatus Memory<std::vector<char> >::m_channelStatus =  Memory<std::vector<char> >::ChannelStatus(MAX_N_CHANNELS, 0);
//
//        std::map<std::string, size_t> Memory<std::vector<char> >::m_name2Idx;
//
//        boost::mutex Memory<std::vector<char> >::m_accessMutex;
//
//        Memory<std::vector<char> >::SerializedChannels Memory<std::vector<char> >::m_serializedCache = Memory<std::vector<char> >::SerializedChannels(MAX_N_CHANNELS, Memory<std::vector<char> >::SerializedChunks(MAX_N_CHUNKS));
//    
//        Memory<std::vector<char> >::ChannelUsers Memory<std::vector<char> >::m_cacheUsers =  Memory<std::vector<char> >::ChannelUsers(MAX_N_CHANNELS,  Memory<std::vector<char> >::ChunkUsers(MAX_N_CHUNKS, 0));
    }
}
