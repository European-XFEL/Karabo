/*
 * File:   Strand.hh
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

#include <boost/asio/io_context.hpp>
#include <functional>
#include <mutex>
#include <queue>

#include "karabo/util/ClassInfo.hh"
#include "karabo/util/Hash.hh"

#ifndef KARABO_NET_STRAND_HH
#define KARABO_NET_STRAND_HH

namespace karabo {
    namespace util {
        class Schema;
    }
    namespace net {

        /**
         * A poor man's substitute for boost::asio::strand because that does not guarantee that
         * handlers posted on different strands can run in parallel ("strand collision").
         * Compared to boost::asio::strand, this
         * - lacks dispatch:               we usually do not want that in Karabo since it allows the handler to be
         *                                 called now in this scope
         * - lacks running_in_this_thread: probably not too important
         * - has a more restrictive wrap:  would be useful to support more, but a proper implementation would also
         *                                 need dispatch
         *
         * Every handler posted will be put into a FIFO queue and the FIFO will be emptied in the background
         * by posting the handlers to the given boost::asio::io_context (either from net::EventLoop, passed in
         * constructor, or defined by setContext(..)).
         *
         * NOTE:
         * Do not create a Strand on the stack, but do it on the heap using the Configurator:
         *
         * auto stack = karabo::util::Configurator<Strand>::create(Hash(...));
         *
         * Otherwise 'enable_shared_from_this' does not work which is needed to guarantee (via usage of
         * karab::util::bind_weak) that an internal Strand method is executed on the event loop when the Strand
         * is already destructed.
         *
         */
        class Strand : public std::enable_shared_from_this<Strand> {
           public:
            KARABO_CLASSINFO(Strand, "Strand", "2.1")

            static void expectedParameters(karabo::util::Schema& expected);

            /**
             * Contructor only kept for backward compatibility.
             *
             * Better use karabo::util::Configurator<Strand>::create("Strand", Hash())
             */
            explicit Strand(boost::asio::io_context& ioContext);

            /**
             * Construct the Strand.
             *
             * The boost::asio::io_context of the karabo::net::EventLoop will be used.
             *
             * Best use this constructor indirectly via karabo::util::Configurator<Strand>::create("Strand", cfg) which
             * will validate cfg and create the Strand properly on the heap.
             *
             * Keys of cfg are "maxInARow" (unsigned int) and "guaranteeToRun" (bool), see expectedParameters.
             */
            explicit Strand(const karabo::util::Hash& config);

            Strand(const Strand& orig) = delete;

            virtual ~Strand();

            /**
             * Set the context to which the handlers are to be posted.
             *
             * No concurrency protection:
             * Must be called directly after Strand creation, before it is used.
             */
            void setContext(boost::asio::io_context& ioContext);

            /**
             * Post a handler to the io_context with the guarantee that it is not executed before any other handler
             * posted before has finished.
             * Handlers posted on different Strands can always be run in parallel.
             *
             * Note that "guaranteeToRun" flag of the constructor determines what happens with yet unhandled handlers
             * when the Strand is destructed.
             *
             * @param handler function without arguments and return value - will be copied
             */
            void post(const std::function<void()>& handler);

            /**
             * Post a handler to the io_context with the guarantee that it is not executed before any other handler
             * posted before has finished.
             * Handlers posted on different Strands can always be run in parallel.
             *
             * Note that "guaranteeToRun" flag of the constructor determines what happens with yet unhandled handlers
             * when the Strand is destructed.
             *
             * @param handler function without arguments and return value as r-value reference - will be moved to avoid
             * a copy
             */
            void post(std::function<void()>&& handler);

            /**
             * This function is used to create a new handler function object that, when invoked,
             * will pass the wrapped handler to the Strand's post function (instead of using dispatch
             * as boost::io_service::strand::wrap does).
             *
             * @param handler The handler to be wrapped. The strand will make a copy of
             * the handler object. Compared to boost::io_service::strand::wrap, the handler signature is
             * much more restricted, i.e. must be void().
             *
             * @return A function object that, when invoked, passes the wrapped handler to
             * the Strand's post function.
             */
            std::function<void()> wrap(std::function<void()> handler);

            /**
             * This function may be used to obtain the io_context object that the strand uses to post handlers.
             *
             * @return A reference to the io_context of the Strand. Ownership is not transferred to the caller.
             */
            boost::asio::io_context& getContext() const {
                return *m_ioContext;
            }

            /**
             * Deprecated.
             *
             * Use getContext() instead.
             */
            boost::asio::io_context& get_io_service() const {
                return getContext();
            }

           private:
            /// Helper for post - to be called under protection of m_tasksMutex!
            void startRunningIfNeeded();

            /// Helper to run one task after another until tasks queue is empty
            void run();

            void postWrapped(std::function<void()> handler);

            boost::asio::io_context* m_ioContext; // pointer to be able to re-assign it

            std::mutex m_tasksMutex; // to protect both, m_tasksRunning and m_tasks
            bool m_tasksRunning;
            std::queue<std::function<void()> > m_tasks;

            unsigned int m_maxInARow;
            const bool m_guaranteeToRun;
        };

    } // namespace net
} // namespace karabo
#endif /* KARABO_NET_STRAND_HH */
