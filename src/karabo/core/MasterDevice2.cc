/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/io/Input.hh>
#include "MasterDevice2.hh"
#include "DeviceServer.hh"

namespace karabo {
    namespace core {

        using namespace log4cpp;

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::io;


        KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device<OkErrorFsm>, MasterDevice2)

        void MasterDevice2::expectedParameters(Schema& expected) {
        }


        MasterDevice2::MasterDevice2(const Hash& input) : Device<OkErrorFsm>(input) {
            setupSlots();
            cacheAvailableInstances();
        }


        MasterDevice2::~MasterDevice2() {
        }
      


        void MasterDevice2::setupSlots() {
            //GLOBAL_SLOT2(slotInstanceUpdated, string /*instanceId*/, Hash /*instanceInfo*/);
            //GLOBAL_SLOT1(slotInstanceGone, string /*instanceId*/);
            //SLOT2(slotChanged, Hash /*changedConfig*/, string /*instanceId*/);
        }


        void MasterDevice2::cacheAvailableInstances() {
            boost::mutex::scoped_lock lock(m_runtimeSystemDescriptionMutex);
            vector<pair<string, Hash> > instances = getAvailableInstances();
            for (size_t i = 0; i < instances.size(); ++i) {
                const string& instanceId = instances[i].first;
                std::cout << "### ID " << instanceId << std::endl;
                const Hash& instanceInfo = instances[i].second;
                
                trackExistenceOfInstance(instanceId); // Start tracking
                                                
                boost::optional<const Hash::Node&> node = instanceInfo.find("type");
                string type = "unknown";
                if (node) type = node->getValue<string>();
                Hash entry;
                Hash::Node& entryNode = entry.set(type + "." + instanceId, Hash());
                for (Hash::const_iterator it = instanceInfo.begin(); it != instanceInfo.end(); ++it) {
                    entryNode.setAttribute(it->getKey(), it->getValueAsAny());
                }
                m_runtimeSystemDescription.merge(entry);
                
                if (type == "device") {
                    Hash hash;
                    try {
                        request(instanceId, "slotRefresh").timeout(2000).receive(hash);
                    } catch (const TimeoutException&) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Configuration request for instance \"" << instanceId << "\" timed out";
                        Exception::clearTrace();
                    }
                    Hash configuration;
                    for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                        Hash val("val", it->getValueAsAny());
                        val.setAttributes("val", it->getAttributes());
                        configuration.set<vector<Hash> >(it->getKey(), vector<Hash>(1, val));
                    }
                    Schema schema;
                    try {
                        request(instanceId, "slotGetSchema", true).timeout(2000).receive(schema); // Retrieves active schema
                    } catch (const TimeoutException&) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Schema request for instance \"" << instanceId << "\" timed out";
                        Exception::clearTrace();
                    }
                    Hash tmp("configuration", configuration, "description", vector<Hash>(1, Hash("val", schema)));
                    tmp.setAttribute("configuration", "t0", karabo::util::Timestamp().getMsSinceEpoch());
                    tmp.setAttribute("description", "t0", karabo::util::Timestamp().getMsSinceEpoch());
                    
                    string path(type + "." + instanceId);
                    if (!m_systemArchive.has(path)) m_systemArchive.set(path, vector<Hash>());
                    m_systemArchive.get<vector<Hash> >(type + "." + instanceId).push_back(tmp);
                } else if (type == "server") {
                    
                }
            }
            KARABO_LOG_FRAMEWORK_DEBUG << "cacheAvailableInstances() was called, runtimeSystemDescription looks like:";
            KARABO_LOG_FRAMEWORK_DEBUG << m_runtimeSystemDescription;
            KARABO_LOG_FRAMEWORK_DEBUG << "system archive looks like:";
            KARABO_LOG_FRAMEWORK_DEBUG << m_systemArchive;
        }
    }
}
