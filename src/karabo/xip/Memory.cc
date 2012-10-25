/*
 * $Id$
 *
 * File:   Memory.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 25, 2011, 12:44 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Memory.hh"

namespace exfel {
    namespace xip {

       // Static initializations of specialized members
      
        std::vector< std::vector< std::vector<boost::shared_ptr<std::vector<char> > > > > Memory<std::vector<char> >::m_cache = std::vector< std::vector< std::vector< boost::shared_ptr<std::vector<char> > > > >(MAX_N_CHANNELS, std::vector< std::vector< boost::shared_ptr<std::vector<char> > > >(MAX_N_CHUNKS));

        std::vector<std::vector<bool> > Memory<std::vector<char> >::m_chunkStatus = std::vector<std::vector<bool> >(MAX_N_CHANNELS, std::vector<bool>(MAX_N_CHUNKS));
       
        std::vector<bool> Memory<std::vector<char> >::m_channelStatus = std::vector<bool>(MAX_N_CHANNELS);
        
        std::map<std::string, size_t> Memory<std::vector<char> >::m_name2Idx;
       
        boost::mutex Memory<std::vector<char> >::m_accessMutex;
        
       
       

    }
}
