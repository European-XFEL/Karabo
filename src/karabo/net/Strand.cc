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

#include "karabo/util/MetaTools.hh"  // for bind_weak

#include <utility>  // for std::move


namespace karabo {
    namespace net {


        Strand::Strand(boost::asio::io_service& ioService) : m_ioService(ioService) {
        }


        Strand::~Strand() {
        }


        void Strand::post(const boost::function<void()>& handler) {

            boost::mutex::scoped_lock lock(m_tasksMutex);
            m_tasks.push(handler);

            startRunningIfNeeded(); // needs mutex to be locked!
        }


        void Strand::post(boost::function<void()>&& handler) {

            boost::mutex::scoped_lock lock(m_tasksMutex);
            m_tasks.push(handler);

            startRunningIfNeeded(); // needs mutex to be locked!
        }


        boost::function<void() > Strand::wrap(boost::function<void()> handler) {
            return karabo::util::bind_weak(&Strand::postWrapped, this, std::move(handler));
        }


        void Strand::startRunningIfNeeded() {
            // We rely on being called under protection of m_tasksMutex
            if (!m_tasksRunning) {
                m_tasksRunning = true;
                // Instead of bind_weak to 'this' we could boost::bind to 'shared_from_this()'.
                // The difference would only be that in the latter 'run' would be executed even if all other
                // shared pointers to it are reset between this post and when m_ioService actually invokes it.
                m_ioService.post(karabo::util::bind_weak(&Strand::run, this));
            }
        }

        void Strand::run() {
            while (true) {
                boost::function<void() > nextTask;
                {
                    boost::mutex::scoped_lock lock(m_tasksMutex);
                    if (m_tasks.empty()) {
                        m_tasksRunning = false;
                        // Nothing else to do, so stop running
                        break;
                    } else {
                        // Get oldest handler - move it to avoid a copy:
                        nextTask = std::move(m_tasks.front());
                        // Removes the object from queue (note that it is in undefined state after std::move):
                        m_tasks.pop();
                    }
                }
                // Actually run the task without lock
                nextTask();
            }
        }


        void Strand::postWrapped(boost::function<void() > handler) {
            // Hm - this will probably be the second copy of the handler...
            post(std::move(handler));
        }

    }
}