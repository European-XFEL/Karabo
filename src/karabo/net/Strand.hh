/* 
 * File:   Strand.hh
 *
 * Author: gero.flucke@xfel.eu
 *
 * Created on November 10, 2017, 9:24 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 *
 */

#include "karabo/util/ClassInfo.hh"

#include <boost/asio/io_service.hpp>
#include <boost/function.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/thread/mutex.hpp>

#include <queue>

#ifndef KARABO_NET_STRAND_HH
#define	KARABO_NET_STRAND_HH

namespace karabo {
    namespace net {

        /**
         * A poor man's substitute for boost::asio::io_service::strand because that does not guarantee that
         * handlers posted on different strands can run in parallel ("strand collision").
         * Compared to boost::asio::io_service::strand, this
         * - lacks dispatch:               we usually do not want that in Karabo since it allows the handler to be
         *                                 called now in this scope
         * - lacks running_in_this_thread: probably not too important
         * - has a more restrictive wrap:  would be useful to support more, but a proper implementation would also
         *                                 need dispatch
         *
         * Every handler posted will be put into a FIFO queue and the FIFO will be emptied one-by-one in the background
         * by posting the handlers to the boost::asio::io_service given in the constructor.
         *
         * NOTE:
         * Do not create a Strand on the stack, but do it on the heap using a shared pointer:
         *
         * auto stack = boost::make_shared<Strand>(karabo::net::EventLoop::getIOService());
         *
         * Otherwise 'enable_shared_from_this' does not work which is needed to guarantee (via usage of
         * karab::util::bind_weak) that an internal Strand method is executed on the event loop when the Strand
         * is already destructed.
         *
         */
        class Strand : public boost::enable_shared_from_this<Strand> {


            public:

            KARABO_CLASSINFO(Strand, "Strand", "2.1")

            explicit Strand(boost::asio::io_service& ioService);

            Strand(const Strand& orig) = delete;

            virtual ~Strand();

            /**
             * Post a handler to the io_service given to the constructor with the guarantee that it is not executed
             * before any other handler posted before has finished.
             * Handlers posted on different Strands can always be run in parallel.
             * Note that, when a handler posted has not yet run when the Strand is destructed, it will never run.
             *
             * @param handler function without arguments and return value - will be copied
             */
            void post(const boost::function<void()>& handler);

            /**
             * Post a handler to the io_service given to the constructor with the guarantee that it is not executed
             * before any other handler posted before has finished.
             * Handlers posted on different Strands can always be run in parallel.
             * Note that, when a handler posted has not yet run when the Strand is destructed, it will never run.
             *
             * @param handler function without arguments and return value as r-value reference - will be moved to avoid a copy
             */
            void post(boost::function<void()>&& handler);

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
            boost::function<void() > wrap(boost::function<void()> handler);

            /**
             * This function may be used to obtain the io_service object that the strand uses to post handlers.
             *
             * @return A reference to the io_service of the Strand. Ownership is not transferred to the caller.
             */
            boost::asio::io_service& get_io_service() const {
                return m_ioService;
            }

        private:

            /// Helper for post - to be called under protection of m_tasksMutex!
            void startRunningIfNeeded();

            /// Helper to run one task after another until tasks queue is empty
            void run();

            void postWrapped(boost::function<void() > handler);

            boost::asio::io_service& m_ioService;

            boost::mutex m_tasksMutex; // to protect both, m_tasksRunning and m_tasks
            bool m_tasksRunning;
            std::queue<boost::function<void()> > m_tasks;
        };

    }
}
#endif	/* KARABO_NET_STRAND_HH */

