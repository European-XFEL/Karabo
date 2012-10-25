/*
 * $Id$
 *
 * File:   Algorithm.cc
 * Author: <bukhard.heisen@xfel.eu>
 *
 * Created on May 15, 2012, 2:37 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Algorithm.hh"

using namespace std;
using namespace karabo::xms;

namespace karabo {
    namespace xip {

        void Algorithm::expectedParameters(karabo::util::Schema& expected) {

            SLOT_ELEMENT(expected).key("slotStartRun")
                    .displayedName("StartRun")
                    .description("Starts a new pipeline run")
                    .allowedStates("Ok.Idle")
                    .commit();

            SLOT_ELEMENT(expected).key("slotCompute")
                    .displayedName("Compute")
                    .description("Do a single computation")
                    .allowedStates("Ok.Ready")
                    .commit();

            SLOT_ELEMENT(expected).key("slotReset")
                    .displayedName("Reset")
                    .description("Completely reset this device")
                    .allowedStates("Error.WaitingIO")
                    .commit();
        }

        void Algorithm::configure(const karabo::util::Hash & input) {

            SLOT0(slotStartRun);
            SLOT0(slotCompute);
            SLOT0(slotReset);
            SLOT0(slotIOEvent);

            SLOT2(slotGetOutputChannelInformation, string, string)
            SLOT2(slotInputChannelCanRead, string, string)


        }

        void Algorithm::onStartRun() {

            connectDeviceInputs();
        }

        bool Algorithm::canCompute() {
            // Loop channels
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end(); ++it) {
                AbstractInput::Pointer channel = it->second;
                if (!channel->canCompute()) {
                    KARABO_LOG_INFO << "Can not compute yet";
                    //boost::this_thread::sleep(boost::posix_time::millisec(1000));
                    //notifyOutputChannelForPossibleRead(channel);
                    return false;
                }
            }
            for (OutputChannels::const_iterator it = m_outputChannels.begin(); it != m_outputChannels.end(); ++it) {
                if (!it->second->canCompute()) {
                    KARABO_LOG_INFO << "Can not compute yet";
                    //boost::this_thread::sleep(boost::posix_time::millisec(1000));
                    return false;
                }
            }
            KARABO_LOG_INFO << "Ready for computing";
            return true;
        }

        void Algorithm::computingStateOnEntry() {
            if (m_computeThread.joinable()) m_computeThread.join();
            notifyOutputChannelsForPossibleRead();
            m_computeThread = boost::thread(boost::bind(&karabo::xip::Algorithm::doCompute, this));
        }

        void Algorithm::computingStateOnExit() {
            for (OutputChannels::const_iterator it = m_outputChannels.begin(); it != m_outputChannels.end(); ++it) {
                it->second->onComputeFinished();
            }
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end(); ++it) {
                it->second->onComputeFinished();
            }
        }

        void Algorithm::doCompute() {
            this->compute();
            computeFinished();
        }

        void Algorithm::connectDeviceInputs() {
            // Loop channels
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end(); ++it) {
                AbstractInput::Pointer channel = it->second;
                if (channel->needsDeviceConnection()) {
                    // Loop connected outputs
                    std::vector<karabo::util::Hash> outputChannels = channel->getConnectedOutputChannels();
                    for (size_t j = 0; j < outputChannels.size(); ++j) {
                        const std::string& instanceId = outputChannels[j].get<string > ("instanceId");
                        const std::string& channelId = outputChannels[j].get<string > ("channelId");
                        bool channelExists;
                        karabo::util::Hash reply;
                        try {
                            request(instanceId, "slotGetOutputChannelInformation", channelId, getDeviceServerInstanceId()).timeout(1000).receive(channelExists, reply);
                        } catch (karabo::util::TimeoutException&) {
                            karabo::util::Exception::clearTrace();
                            throw IO_EXCEPTION("Could not find instanceId \"" + instanceId + "\" for IO connection");
                        }
                        if (channelExists) {
                            channel->connectNow(getInstanceId(), reply); // Synchronous
                        } else {
                            throw IO_EXCEPTION("Could not find outputChannel \"" + channelId + "\" on instanceId \"" + instanceId + "\"");
                        }
                    }

                }
            }
        }

        void Algorithm::notifyOutputChannelsForPossibleRead() {
            // Loop channels
            for (InputChannels::const_iterator it = m_inputChannels.begin(); it != m_inputChannels.end(); ++it) {
                AbstractInput::Pointer channel = it->second;
                if (channel->needsDeviceConnection()) {
                    notifyOutputChannelForPossibleRead(channel);
                }
            }
        }

        void Algorithm::notifyOutputChannelForPossibleRead(const AbstractInput::Pointer& channel) {
            std::vector<karabo::util::Hash> outputChannels = channel->getConnectedOutputChannels();
            for (size_t j = 0; j < outputChannels.size(); ++j) {
                const std::string& instanceId = outputChannels[j].get<string > ("instanceId");
                const std::string& channelId = outputChannels[j].get<string > ("channelId");
                bool channelExists;
                try {
                    request(instanceId, "slotInputChannelCanRead", channelId, m_instanceId).timeout(1000).receive(channelExists);
                } catch (karabo::util::TimeoutException&) {
                    karabo::util::Exception::clearTrace();
                    throw IO_EXCEPTION("Could not find instanceId \"" + instanceId + "\" for IO connection");
                }
                if (!channelExists) {
                    throw IO_EXCEPTION("Could not find outputChannel \"" + channelId + "\" on instanceId \"" + instanceId + "\"");
                }
            }
        }

        void Algorithm::slotGetOutputChannelInformation(const std::string& ioChannelId, const std::string& sendersDeviceServerInstanceId) {
            OutputChannels::const_iterator it = m_outputChannels.find(ioChannelId);
            if (it != m_outputChannels.end()) {
                karabo::util::Hash h(it->second->getInformation());
                if (!sendersDeviceServerInstanceId.empty() && sendersDeviceServerInstanceId == this->getDeviceServerInstanceId()) {
                    h.set("memoryLocation", "local");
                } else {
                    h.set("memoryLocation", "remote");
                }
                reply(true, h);
            } else {
                reply(false, karabo::util::Hash());
            }
        }

        void Algorithm::slotInputChannelCanRead(const std::string& ioChannelId, const std::string& inputChannelInstanceId) {
            OutputChannels::const_iterator it = m_outputChannels.find(ioChannelId);
            if (it != m_outputChannels.end()) {
                it->second->onInputAvailable(inputChannelInstanceId);
                reply(true);
            } else {
                reply(false);
            }
        }




    }
}
