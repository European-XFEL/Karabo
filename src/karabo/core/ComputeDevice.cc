/* 
 * $Id$
 * 
 * Authors: <burkhard.heisen@xfel.eu>
 *          <steffen.hauf@xfel.eu>
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
                    .description("Starts computing if data is available.")
                    .allowedStates("Ok.Paused Ok.Finished Ok.Compute Ok.Ready")
                    .commit();

            SLOT_ELEMENT(expected).key("pause")
                    .displayedName("Pause")
                    .description("Will finish current computation and pause. End of stream events are not forwarded in the paused state if expert-mode parameter setEOSPauseAllowed is false!")
                    .allowedStates("Ok.Computing")
                    .commit();

            SLOT_ELEMENT(expected).key("abort")
                    .displayedName("Abort")
                    .description("Try to abort the current computation by interrupting the compute thread and emptying the current buffer. "
                    "The compute thread is afterwards restarted and resetting the device enables further computations."
                    "Usage Scenario: abort a long running computation which has gone wrong.")
                    .allowedStates("Ok.Computing Ok.Paused")
                    .commit();

            SLOT_ELEMENT(expected).key("endOfStream")
                    .displayedName("End-Of-Stream")
                    .description("Send an End-Of-Stream Token")
                    .allowedStates("Ok.Paused Ok.Computing Ok.Finished")
                    .commit();

            SLOT_ELEMENT(expected).key("reset")
                    .displayedName("Reset")
                    .description("Completely reset this device")
                    .allowedStates("Error.Ready Ok.Aborted Ok")
                    .commit();

            BOOL_ELEMENT(expected).key("autoCompute")
                    .displayedName("Auto Compute")
                    .description("Trigger computation automatically once data is available. The device will stay in the computing state until manually paused or aborted.")
                    .reconfigurable()
                    .assignmentOptional().defaultValue(true)
                    .commit();

            BOOL_ELEMENT(expected).key("autoEndOfStream")
                    .displayedName("Auto end-of-stream")
                    .description("If true, automatically forwards the end-of-stream signal to all connected (downstream) devices. Requires the device to not be paused if autoCompute is enabled.")
                    .reconfigurable()
                    .expertAccess()
                    .assignmentOptional().defaultValue(true)
                    .commit();

            BOOL_ELEMENT(expected).key("autoUpdate")
                    .displayedName("Auto update")
                    .description("If true, automatically updates all input and output channels after compute was called")
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
            
            BOOL_ELEMENT(expected).key("pauseEOSAllowed")
                    .displayedName("Forward end-of-stream when paused")
                    .description("If true, end-of-stream events are forwarded when the device is paused. If false, they are only allowed in the computing and finished states.")
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
            
            UINT32_ELEMENT(expected).key("abortTimeOut")
                    .displayedName("Abort timeout (ms)")
                    .description("Time out to wait for compute to finish before calling interrupt to choke it off. The interrupt call will have the same timeout.")
                    .assignmentOptional().defaultValue(5000)
                    .reconfigurable()
                    .expertAccess()
                    .commit();
            
        }


        ComputeDevice::ComputeDevice(const Hash& input) : Device<>(input), m_isAborted(false), m_isPaused(true), m_isEndOfStream(false), m_deviceIsDead(false), m_nEndOfStreams(0), m_iterationCount(0) {

            m_computeThread = boost::thread(boost::bind(&karabo::core::ComputeDevice::doCompute, this));

            SLOT0(start);
            SLOT0(pause);
            SLOT0(abort);
            SLOT0(endOfStream);
            SLOT0(reset);


        }


        ComputeDevice::~ComputeDevice() {
            setDeviceDead();
            
            //notify compute thread to acknowledge death of device
            m_computeCond.notify_one();

            if (m_computeThread.joinable()) m_computeThread.join();

            KARABO_LOG_DEBUG << "dead.";
        }


        void ComputeDevice::setDeviceDead() {
       
            m_deviceIsDead.store(true, boost::memory_order_relaxed);
        }


        void ComputeDevice::_onInputAvailable(const karabo::io::AbstractInput::Pointer&) {
            
            
            //If we don't want to auto compute, do nothing
            if(get<bool>("autoCompute")){
                if(!m_isPaused) {
                    m_computeCond.notify_one();
                }  
            } 
            
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
            return m_isAborted.load(boost::memory_order_consume);
        }


        void ComputeDevice::connectAction() {
            KARABO_LOG_FRAMEWORK_DEBUG << "Connecting "<<this->getInputChannels().size()<<" IO channels";
            this->connectInputChannels();
            this->set<bool>("autoCompute", this->checkAutoComputeValidity(this->get<bool>("autoCompute")));
           
        }
        
        void ComputeDevice::readyStateOnEntry(){
           
            if(this->get<bool>("autoCompute")){
                m_isPaused = false;
                this->start();
            } else {
                this->pause();
            }
        }
        
        void ComputeDevice::preReconfigure(karabo::util::Hash& incomingReconfiguration){
            
            incomingReconfiguration.set<bool>("autoCompute", this->checkAutoComputeValidity(incomingReconfiguration.get<bool>("autoCompute")));
            
        }

        bool ComputeDevice::checkAutoComputeValidity(const bool & requestedValue){
            //if now input channels are connected autoCompute does not make any sense
            if(this->getInputChannels().size() == 0 &&  requestedValue){
                KARABO_LOG_WARN<<"This device does not have any input channels connected. Setting autoCompute=false";
                return false;
            }
            return requestedValue; 
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
                
                //the condition variable here will block until
                // a) an input becomes available and triggers it
                // b) the device goes back into the compute state from one of the other states
                // c) the device is killed, i.e. from the destructor, so that the thread returns
                {
                   m_computeCond.wait(lock);
                }
                
                
                
                //unset promise: it will be set again when this iteration has finished.
                //an unset promise blocks any transitions to pause if a computation is still in process
                boost::promise<bool> p(m_workIsFinished.move());
        
                if (m_deviceIsDead.load(boost::memory_order_consume)) {
                    m_workIsFinished.set_value(true);
                    return;
                }

                if (!m_isAborted.load(boost::memory_order_consume) && this->canCompute()) {
                    
                    try {
                        
                      this->compute();
                      
                    } catch (const karabo::util::Exception& e) {
                        
                        KARABO_LOG_ERROR << "Caught exception in compute thread: " << e.userFriendlyMsg();
                        
                    } catch (const boost::thread_interrupted& ){
                        
                        KARABO_LOG_WARN << "Compute thread has been interrupted by call to abort.";
                        return;
                        
                    } catch (...) {
                        
                        KARABO_LOG_ERROR << "Caught unknown exception in compute thread";
                        
                    }
                    
                    //if we are not in autoCompute mode this compute call is now finished, and we change to finished state
                    //otherwise, the loop will return to the condition variable, which is triggered by the inputs
                    if(!this->get<bool>("autoCompute")){
                        this->computeFinished();
                    }

                }
                
                try {
                    
                    this->update();
                    
                } catch (const karabo::util::Exception& e) {
                    
                      KARABO_LOG_ERROR << "Caught exception while updating channels: " << e.userFriendlyMsg();
                      
                } catch (...) {
                    
                      KARABO_LOG_ERROR << "Caught unknown exception while updating channels";
                }
                
                m_workIsFinished.set_value(true);
               
            }
        } catch (const karabo::util::Exception& e) {
            KARABO_LOG_ERROR << e;
        }


        void ComputeDevice::computingStateOnExit() {
            
        }

        void ComputeDevice::setComputationAborted(){
            this->abort();
        }
       
        bool ComputeDevice::registerAbort() {
          
            m_isAborted.store(true, boost::memory_order_release);
          
            unsigned int timeout = this->get<unsigned int>("abortTimeOut");
           
            
            boost::unique_future<bool> f = m_workIsFinished.get_future();
            boost::future_status status = f.wait_for(boost::chrono::milliseconds(timeout));
            if(status == boost::future_status::timeout){
                //we kill and restart the computing thread if the wait timed out. In case it succeed we can keep the thread alive.
           
                //the cleanest way is if interruption points are set
                m_computeThread.interrupt();
                if(m_computeThread.try_join_for(boost::chrono::milliseconds(timeout))) {
                    m_computeThread = boost::thread(boost::bind(&karabo::core::ComputeDevice::doCompute, this));
                } else {
                    KARABO_LOG_WARN<<"Thread could not be properly interrupted.  Consider using boost::interruption_point() within your compute implementation.";
                    
                    //The following part would work, but is not safe, as terminating the thread has a high likelihood of tearing the complete device (and device server) down
                    
                    /* int killSucceeded = 0;
                    try{
#if defined(_MSC_VER)
                        killSucceeded = !(bool)(TerminateThread(m_computeThread.native_handle()));
#else
                        killSucceeded = pthread_kill(m_computeThread.native_handle(), 0);
#endif
                    } catch (...){
                        killSucceeded = 1;
                    }
                    if(killSucceeded != 0){
                        KARABO_LOG_WARN<<"Could not terminate thread. Detaching it instead and hoping that it dies gracefully...";
                        try{
                            m_computeThread.detach();
                        } catch (...){
                            KARABO_LOG_WARN<<"Could not detach thread!";
                        }
                    }*/
                }
                
            }
            
            //clear any data in the buffer which was being worked upon
            this->update();
   
            return true;
        }


        bool ComputeDevice::registerPause() {
            //wait for any work to finish
            boost::unique_future<bool> f = m_workIsFinished.get_future();
            f.wait();
            
            return true;
        }
        
         void ComputeDevice::pausedStateOnEntry(){
            m_isPaused = true;
        }
        
        void ComputeDevice::pausedStateOnExit(){
            m_isPaused = false;
            
        }

        bool ComputeDevice::checkPauseEOSAllowed(){
            return this->get<bool>("pauseEOSAllowed");
        }
        
        void ComputeDevice::finishedOnEntry() {
            m_iterationCount++;
            m_isEndOfStream = false;
            
        }


        void ComputeDevice::finishedOnExit() {
            set("iteration", m_iterationCount);
            
        }

        void ComputeDevice::abortedOnEntry() {
            
        }

        void ComputeDevice::abortedOnExit() {
           
            m_isAborted.store(false, boost::memory_order_release);
        }


        int ComputeDevice::getCurrentIteration() const {
            return get<int>("iteration");
        }
    }
}