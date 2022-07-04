/*
 * File:   Strand.cc
 *
 * Author: gero.flucke@xfel.eu
 *
 * Created on November 10, 2017, 9:24 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 *
 */

#include "Strand.hh"

#include <utility> // for std::move

#include "karabo/log/Logger.hh" // for KARABO_LOG_FRAMEWORK_XXX
#include "karabo/util/Hash.hh"
#include "karabo/util/MetaTools.hh" // for bind_weak


namespace karabo {
    namespace net {


        Strand::Strand(boost::asio::io_service& ioService, const karabo::util::Hash& cfg)
            : m_ioService(ioService),
              m_tasksRunning(false),
              m_maxInARow(cfg.has("maxInARow") ? cfg.get<unsigned int>("maxInARow") : 1u),
              m_guaranteeToRun(cfg.has("guaranteeToRun") ? cfg.get<bool>("guaranteeToRun") : false) {
            if (m_maxInARow == 0u) { // silently convert to useful value
                m_maxInARow = 1u;
            }
            size_t nKeys = 0;
            if (cfg.has("maxInARow")) ++nKeys;
            if (cfg.has("guaranteeToRun")) ++nKeys;
            if (cfg.size() > nKeys) {
                // Exception in ctr. is bad - to be fixed later
                throw KARABO_PARAMETER_EXCEPTION("Unknown key in " + toString(cfg));
            }
        }

        Strand::~Strand() {
            if (m_guaranteeToRun) {
                // We are being destructed, so there is no shared pointer left pointing to us.
                // ==> No need for mutex protection.
                while (!m_tasks.empty()) {
                    try {
                        m_tasks.front()();
                    } catch (const std::exception& e) {
                        KARABO_LOG_FRAMEWORK_ERROR << "Caught exception in posted method during destruction: "
                                                   << e.what();
                    }
                    m_tasks.pop();
                }
            }
        }


        void Strand::post(const boost::function<void()>& handler) {
            boost::mutex::scoped_lock lock(m_tasksMutex);
            m_tasks.push(handler);

            startRunningIfNeeded(); // needs mutex to be locked!
        }


        void Strand::post(boost::function<void()>&& handler) {
            boost::mutex::scoped_lock lock(m_tasksMutex);
            m_tasks.push(std::move(handler)); // actually forward the rvalue-ness

            startRunningIfNeeded(); // needs mutex to be locked!
        }


        boost::function<void()> Strand::wrap(boost::function<void()> handler) {
            return karabo::util::bind_weak(&Strand::postWrapped, this, std::move(handler));
        }


        void Strand::startRunningIfNeeded() {
            // We rely on being called under protection of m_tasksMutex
            if (!m_tasksRunning) {
                m_tasksRunning = true;
                // Instead of bind_weak to 'this' we could boost::bind to 'shared_from_this()'.
                // The difference would only be that in the latter 'run' (and thus the tasks to be executed
                // sequentially) would be executed even if all other shared pointers to it are reset between
                // this post and when m_ioService actually invokes it.
                m_ioService.post(karabo::util::bind_weak(&Strand::run, this));
            }
        }


        void Strand::run() {
            boost::function<void()> nextTask;
            unsigned int counter = 0;
            while (++counter <= m_maxInARow) {
                {
                    boost::mutex::scoped_lock lock(m_tasksMutex);
                    if (m_tasks.empty()) {
                        m_tasksRunning = false;
                        // Nothing else to do, so stop running
                        return;
                    } else {
                        // Get oldest handler - move it to avoid a copy:
                        nextTask = std::move(m_tasks.front());
                        // Removes the object from queue (it is in undefined state after std::move):
                        m_tasks.pop();
                    }
                }
                // Actually run the task without lock
                // Catch exceptions, otherwise this Strand would completely stop functioning:
                // run not posted anymore, but m_tasksRunning still true.
                try {
                    nextTask();
                } catch (const std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Caught exception in posted method: " << e.what();
                }
            }
            // Repost to eventually run next task - see comment in startRunningIfNeeded about use of bind_weak.
            m_ioService.post(karabo::util::bind_weak(&Strand::run, this));
        }


        void Strand::postWrapped(boost::function<void()> handler) {
            // Hm - this will probably be the second copy of the handler...
            post(std::move(handler));
        }

    } // namespace net
} // namespace karabo
