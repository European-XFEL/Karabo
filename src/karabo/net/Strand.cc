/*
 * File:   Strand.cc
 *
 * Author: gero.flucke@xfel.eu
 *
 * Created on November 10, 2017, 9:24 AM
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
 *
 */

#include "Strand.hh"

#include <utility> // for std::move

#include "EventLoop.hh"
#include "karabo/log/Logger.hh" // for KARABO_LOG_FRAMEWORK_XXX
#include "karabo/util/Hash.hh"
#include "karabo/util/MetaTools.hh" // for bind_weak
#include "karabo/util/SimpleElement.hh"

using karabo::util::BOOL_ELEMENT;
using karabo::util::UINT32_ELEMENT;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::net::Strand)

namespace karabo {
    namespace net {

        void Strand::expectedParameters(karabo::util::Schema& expected) {
            UINT32_ELEMENT(expected)
                  .key("maxInARow")
                  .description(
                        "Up to this number of handlers are run in a row before control is given back to the event loop")
                  .assignmentOptional()
                  .defaultValue(1u)
                  .minInc(1u)
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("guaranteeToRun")
                  .description(
                        "If true, all handlers posted are guaranteed to run, even those that are left when destruction "
                        "of the Strand starts.")
                  .assignmentOptional()
                  .defaultValue(false)
                  .commit();
        }


        Strand::Strand(boost::asio::io_context& ioContext)
            : Strand(karabo::util::Hash("maxInARow", 1u, "guaranteeToRun", false)) {
            setContext(ioContext);
        };

        Strand::Strand(const karabo::util::Hash& cfg)
            : m_ioContext(&karabo::net::EventLoop::getIOService()),
              m_tasksRunning(false),
              m_maxInARow(cfg.get<unsigned int>("maxInARow")),
              m_guaranteeToRun(cfg.get<bool>("guaranteeToRun")) {
            if (m_maxInARow == 0u) { // Cannot happen if created via Configurator<Strand>::create
                // nevertheless silently convert to useful value
                m_maxInARow = 1u;
            }
        }

        Strand::~Strand() {
            if (m_guaranteeToRun) {
                // We are being destructed, so there is no shared pointer left pointing to us.
                // ==> No need for mutex protection.
                if (!m_tasks.empty()) {
                    auto runTasks = [tasks{std::move(m_tasks)}]() mutable { // mutable to be able to modify 'tasks'
                        while (!tasks.empty()) {
                            try {
                                tasks.front()();
                            } catch (const std::exception& e) {
                                KARABO_LOG_FRAMEWORK_ERROR << "Caught exception in method posted from destructor: "
                                                           << e.what();
                            }
                            tasks.pop();
                        }
                    };
                    // Do not block the destructor and also ensure that tasks run in a thread of the given io_context
                    // (destructor might be called in a 'foreign' thread)
                    m_ioContext->post(runTasks);
                }
            }
        }

        void Strand::setContext(boost::asio::io_context& ioContext) {
            m_ioContext = &ioContext;
        }

        void Strand::post(const std::function<void()>& handler) {
            std::lock_guard<std::mutex> lock(m_tasksMutex);
            m_tasks.push(handler);

            startRunningIfNeeded(); // needs mutex to be locked!
        }


        void Strand::post(std::function<void()>&& handler) {
            std::lock_guard<std::mutex> lock(m_tasksMutex);
            m_tasks.push(std::move(handler)); // actually forward the rvalue-ness

            startRunningIfNeeded(); // needs mutex to be locked!
        }


        std::function<void()> Strand::wrap(std::function<void()> handler) {
            return karabo::util::bind_weak(&Strand::postWrapped, this, std::move(handler));
        }


        void Strand::startRunningIfNeeded() {
            // We rely on being called under protection of m_tasksMutex
            if (!m_tasksRunning) {
                m_tasksRunning = true;
                // Instead of bind_weak to 'this' we could std::bind to 'shared_from_this()'.
                // The difference would only be that in the latter 'run' (and thus the tasks to be executed
                // sequentially) would be executed even if all other shared pointers to it are reset between
                // this post and when m_ioContext actually invokes it.
                m_ioContext->post(karabo::util::bind_weak(&Strand::run, this));
            }
        }


        void Strand::run() {
            std::function<void()> nextTask;
            unsigned int counter = 0;
            while (++counter <= m_maxInARow) {
                {
                    std::lock_guard<std::mutex> lock(m_tasksMutex);
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
            m_ioContext->post(karabo::util::bind_weak(&Strand::run, this));
        }


        void Strand::postWrapped(std::function<void()> handler) {
            // Hm - this will probably be the second copy of the handler...
            post(std::move(handler));
        }

    } // namespace net
} // namespace karabo
