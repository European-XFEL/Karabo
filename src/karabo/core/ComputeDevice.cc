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

            BOOL_ELEMENT(expected).key("autoEndOfStream")
                    .displayedName("Auto end-of-stream")
                    .description("If true, automatically forwards the end-of-stream signal to all connected (downstream) devices")
                    .reconfigurable()
                    .expertAccess()
                    .assignmentOptional().defaultValue(true)
                    .commit();

            BOOL_ELEMENT(expected).key("autoUpdate")
                    .displayedName("Auto update")
                    .description("If true, automatically updates all input and output channels")
                    .reconfigurable()
                    .expertAccess()
                    .assignmentOptional().defaultValue(true)
                    .commit();

            BOOL_ELEMENT(expected).key("autoIterate")
                    .displayedName("Auto iterate")
                    .description("If true, automatically iterates cyclic workflows")
                    .reconfigurable()
                    .expertAccess()
                    .assignmentOptional().defaultValue(true)
                    .commit();

            INT32_ELEMENT(expected).key("iteration")
                    .displayedName("Iteration")
                    .description("The current iteration")
                    .readOnly()
                    .initialValue(0)
                    .commit();
        }


        ComputeDevice::ComputeDevice(const Hash& input) : Device<>(input), m_isAborted(false), m_isEndOfStream(false), m_deviceIsDead(false), m_nEndOfStreams(0), m_iterationCount(0) {

            m_computeThread = boost::thread(boost::bind(&karabo::core::ComputeDevice::doCompute, this));

            m_waitingIOThread = boost::thread(boost::bind(&karabo::core::ComputeDevice::doWait, this));

            SLOT0(start);
            SLOT0(pause);
            SLOT0(abort);
            SLOT0(endOfStream);
            SLOT0(reset);


        }


        ComputeDevice::~ComputeDevice() {
            setDeviceDead();
            m_computeCond.notify_one();
            m_waitingIOCond.notify_one();

            if (m_computeThread.joinable()) m_computeThread.join();
            if (m_waitingIOThread.joinable()) m_waitingIOThread.join();
            KARABO_LOG_DEBUG << "dead.";
        }


        void ComputeDevice::setDeviceDead() {
            boost::lock_guard<boost::mutex> lock(m_computeMutex);
            boost::lock_guard<boost::mutex> lockIO(m_waitingIOMutex);
            m_deviceIsDead = true;
        }


        void ComputeDevice::_onInputAvailable(const karabo::io::AbstractInput::Pointer&) {
            if (get<string>("state") == "Ok.Finished" && !get<bool>("autoIterate")) {
                // Do nothing
            } else this->start();
        }


        void ComputeDevice::_onEndOfStream() {

            size_t m_expectedEndOfStreams = 0;

            // Count all channels that should respond to eos
            const InputChannels& inputChannels = this->getInputChannels();
            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                if (it->second->respondsToEndOfStream()) m_expectedEndOfStreams++;
            }

            m_nEndOfStreams++; // Counts this function call
            if (m_nEndOfStreams >= m_expectedEndOfStreams) {
                m_nEndOfStreams = 0;
                m_isEndOfStream = true;
                this->endOfStream();
            }
        }


        void ComputeDevice::endOfStreamAction() {

            this->onEndOfStream();
            if (get<bool>("autoEndOfStream")) {
                const OutputChannels& outputChannels = this->getOutputChannels();
                for (OutputChannels::const_iterator it = outputChannels.begin(); it != outputChannels.end(); ++it)
                    it->second->signalEndOfStream();
            }
        }

        // User hook


        void ComputeDevice::onEndOfStream() {

        }


        void ComputeDevice::update() {
            if (get<bool>("autoUpdate")) {
                const InputChannels& inputChannels = this->getInputChannels();
                const OutputChannels& outputChannels = this->getOutputChannels();
                for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it)
                    it->second->update();
                for (OutputChannels::const_iterator it = outputChannels.begin(); it != outputChannels.end(); ++it)
                    it->second->update();
            }
        }


        bool ComputeDevice::isAborted() const {
            return m_isAborted;
        }


        void ComputeDevice::connectAction() {
            KARABO_LOG_FRAMEWORK_DEBUG << "Connecting IO channels";
            this->connectInputChannels();

        }


        void ComputeDevice::readyStateOnEntry() {
            //KARABO_LOG_FRAMEWORK_DEBUG << "isEndOfStream: " << m_isEndOfStream;
            //KARABO_LOG_FRAMEWORK_DEBUG << "canCompute: " << canCompute();
            if (m_isEndOfStream && get<bool>("autoEndOfStream") && !canCompute()) this->endOfStream();
            else if (m_isAborted) this->abort();
            else if (this->getInputChannels().size() > 0 && this->get<bool>("autoCompute")) this->start();
        }


        bool ComputeDevice::canCompute() {

            //            size_t m_expectedEndOfStreams = 0;
            //
            //            const InputChannels& inputChannels = this->getInputChannels();
            //            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
            //                if (it->second->respondsToEndOfStream()) m_expectedEndOfStreams++;
            //            }


            //            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
            //                if (!it->second->canCompute() && (it->second->respondsToEndOfStream() || m_expectedEndOfStreams == 0)) return false;
            //            }

            // Moved logic of the upper code into the canCompute function of the input channel, thus:
            const InputChannels& inputChannels = this->getInputChannels();
            for (InputChannels::const_iterator it = inputChannels.begin(); it != inputChannels.end(); ++it) {
                if (!it->second->canCompute()) {
                    return false;
                }
            }
            return true;
        }


        void ComputeDevice::computingStateOnEntry() {           
            m_computeCond.notify_one();
        }


        void ComputeDevice::doCompute() try {

            boost::unique_lock<boost::mutex> lock(m_computeMutex);

            while (true) {

                m_computeCond.wait(lock);

                if (m_deviceIsDead) {
                    return;
                }

                if (!m_isAborted) {

                    try {

                        this->compute();
                        m_abortCond.notify_one();

                    } catch (const karabo::util::Exception& e) {
                        KARABO_LOG_ERROR << "Caught exception in compute thread: " << e.userFriendlyMsg();
                    } catch (...) {
                        KARABO_LOG_ERROR << "Caught unknown exception in compute thread";
                    }
                    this->computeFinished();

                }
            }
        } catch (const karabo::util::Exception& e) {
            KARABO_LOG_ERROR << e;
        }


        void ComputeDevice::computingStateOnExit() {
        }


        bool ComputeDevice::registerAbort() {
            {
                boost::lock_guard<boost::mutex> lock(m_computeMutex);
                m_isAborted = true;
            }
            m_computeCond.notify_one();

            boost::unique_lock<boost::mutex> lock(m_abortMutex);
            m_abortCond.wait(lock);

            return true;
        }


        bool ComputeDevice::registerPause() {
            return true;
        }


        void ComputeDevice::waitingIOOnEntry() {
            //if(!m_waitingIOMutex.try_lock()){
            //    m_waitingIOMutex.unlock();
            //}
            m_waitingIOCond.notify_one();
        }


        void ComputeDevice::doWait() try {
            while (true) {
                //m_waitingIOMutex.lock();
                boost::unique_lock<boost::mutex> lock(m_waitingIOMutex);
                m_waitingIOCond.wait(lock);
                if (m_deviceIsDead) return;
                try {
                    update();
                } catch (const Exception& e) {
                    //m_waitingIOMutex.unlock();
                    this->errorFound(e.userFriendlyMsg(), e.detailedMsg());
                    return;
                }
                //m_waitingIOMutex.unlock();
                this->updatedIO();
            }
        } catch (const karabo::util::Exception& e) {
            KARABO_LOG_FRAMEWORK_ERROR << e.what();
            KARABO_LOG_FRAMEWORK_ERROR << "Restarting IO thread...";
            if (m_waitingIOThread.joinable()) m_waitingIOThread.join();
            //m_waitingIOMutex.lock();
            m_waitingIOThread = boost::thread(boost::bind(&karabo::core::ComputeDevice::doWait, this));
        } 


        void ComputeDevice::waitingIOOnExit() {
            //m_waitingIOMutex.lock();
        }


        void ComputeDevice::finishedOnEntry() {
            m_iterationCount++;
            m_isEndOfStream = false;
        }


        void ComputeDevice::finishedOnExit() {
            set("iteration", m_iterationCount);
        }


        void ComputeDevice::abortedOnEntry() {
            {
                boost::lock_guard<boost::mutex> lock(m_computeMutex);
                m_isAborted = false;
            }
        }


        int ComputeDevice::getCurrentIteration() const {
            return get<int>("iteration");
        }
    }
}