/* 
 * $Id$
 * 
 * Author: <burkhard.heisen@xfel.eu>
 * 
 * Created on July 22, 2013, 9:10 AM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "ComputeDevice.hh"
#include <karabo/xms/SlotElement.hh>

using namespace std;
using namespace karabo::util;
using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::io;
using namespace karabo::xms;
using namespace karabo::xip;

namespace karabo {
    namespace core {


        void ComputeDevice::expectedParameters(Schema& expected) {

            SLOT_ELEMENT(expected).key("start")
                    .displayedName("Compute")
                    .description("Starts computing if data is available")
                    .allowedStates("Ok.Ready")
                    .commit();

            SLOT_ELEMENT(expected).key("pause")
                    .displayedName("Pause")
                    .description("Will finish current computation and pause")
                    .allowedStates("Ok.Ready")
                    .commit();

            SLOT_ELEMENT(expected).key("abort")
                    .displayedName("Abort")
                    .description("Abort contribution to this run, fully disconnect")
                    .allowedStates("Ok.Ready Ok.Computing Ok.WaitingIO Ok.Paused")
                    .commit();

            SLOT_ELEMENT(expected).key("endOfStream")
                    .displayedName("End-Of-Stream")
                    .description("Completely reset this device")
                    .allowedStates("Ok.Ready")
                    .commit();

            SLOT_ELEMENT(expected).key("reset")
                    .displayedName("Reset")
                    .description("Completely reset this device")
                    .allowedStates("Error.Ready Error.Computing Error.WaitingIO Ok.Finished Ok.Aborted")
                    .commit();

            BOOL_ELEMENT(expected).key("autoCompute")
                    .displayedName("Auto Compute")
                    .description("Trigger computation automatically once data is available")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(true)
                    .commit();
        }


        ComputeDevice::ComputeDevice(const Hash& input) : Device<>(input), m_isAborted(false), m_isEndOfStream(false), m_deviceIsDead(false), m_nEndOfStreams(0) {

            m_computeMutex.lock();
            m_computeThread = boost::thread(boost::bind(&karabo::core::ComputeDevice::doCompute, this));

            m_waitingIOMutex.lock();
            m_waitingIOThread = boost::thread(boost::bind(&karabo::core::ComputeDevice::doWait, this));

            SLOT0(start);
            SLOT0(pause);
            SLOT0(abort);
            SLOT0(endOfStream);
            SLOT0(reset);
        }


        ComputeDevice::~ComputeDevice() {
            m_deviceIsDead = true;
            m_computeMutex.unlock();
            m_computeThread.join();
            m_waitingIOMutex.unlock();
            m_waitingIOThread.join();
            KARABO_LOG_DEBUG << "dead.";
        }


        void ComputeDevice::onInputAvailable(const karabo::io::AbstractInput::Pointer&) {
            this->start();
        }


        void ComputeDevice::onEndOfStream() {
            m_nEndOfStreams++;
            if (m_nEndOfStreams >= this->getInputChannels().size()) {
                m_nEndOfStreams = 0;
                m_isEndOfStream = true;
                this->endOfStream();
            }
        }


        void ComputeDevice::endOfStreamAction() {
            try {
                const OutputChannels& outputChannels = this->getOutputChannels();
                for (OutputChannels::const_iterator it = outputChannels.begin(); it != outputChannels.end(); ++it)
                    it->second->update();
            } catch (const Exception& e) {
                this->errorFound(e.userFriendlyMsg(), e.detailedMsg());
            }
        }


        void ComputeDevice::updateChannels() {
            const InputChannels& inputChannels = this->getInputChannels();
            const OutputChannels& outputChannels = this->getOutputChannels();
            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it)
                it->second->update();
            for (OutputChannels::const_iterator it = outputChannels.begin(); it != outputChannels.end(); ++it)
                it->second->update();
        }


        bool ComputeDevice::isAborted() const {
            return m_isAborted;
        }


        void ComputeDevice::connectAction() {
            this->connectInputChannels();
        }


        void ComputeDevice::readyStateOnEntry() {
            if (m_isEndOfStream) this->endOfStream();
            else if (m_isAborted) this->abort();
            else if (this->getInputChannels().size() > 0 && this->get<bool>("autoCompute")) this->start();
        }


        bool ComputeDevice::canCompute() {
            const InputChannels& inputChannels = this->getInputChannels();
            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                if (!it->second->canCompute()) return false;
            }
            return true;
        }


        void ComputeDevice::computingStateOnEntry() {
            m_computeMutex.unlock();
        }


        void ComputeDevice::doCompute() {
            while (true) {
                m_computeMutex.lock();
                if (m_deviceIsDead) return;
                this->compute();
                m_computeMutex.unlock();
                if (!m_isAborted) this->computeFinished();
            }
        }


        void ComputeDevice::computingStateOnExit() {
            m_computeMutex.lock();
        }


        bool ComputeDevice::registerAbort() {
            m_isAborted = true;
            return true;
        }


        bool ComputeDevice::registerPause() {
            return true;
        }


        void ComputeDevice::waitingIOOnEntry() {
            m_waitingIOMutex.unlock();
        }


        void ComputeDevice::doWait() {
            while (true) {
                m_waitingIOMutex.lock();
                if (m_deviceIsDead) return;
                try {
                    updateChannels();
                } catch (const Exception& e) {
                    m_waitingIOMutex.unlock();
                    this->errorFound(e.userFriendlyMsg(), e.detailedMsg());
                    return;
                }
                m_waitingIOMutex.unlock();
                this->updatedIO();
            }
        }


        void ComputeDevice::waitingIOOnExit() {
            m_waitingIOMutex.lock();
        }
        
        void ComputeDevice::finishedOnEntry() {
            m_isEndOfStream = false;
        }
        
        void ComputeDevice::abortedOnEntry() {
            m_isAborted = false;
        }
    }
}