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
         * Compared to boost::asio::io_service::strand, this lacks
         * - dispatch:               we usually do not want that in Karabo since it allows the handler to be called
         *                           now in this scope
         * - running_in_this_thread: probably not too important
         * - wrap:                   would be very useful
         *
         * Every handler posted will be put into a FIFO queue and the FIFO will be emptied one-by-one in the background
         * by posting the handlers to the boost::asio::io_service given in the constructor.
         */
        class Strand : public boost::enable_shared_from_this<Strand> {


            public:
            explicit Strand(boost::asio::io_service& ioService);

            Strand(const Strand& orig) = delete;

            virtual ~Strand();

            /**
             * Post a handler to the io_service given to the constructor with the guarantee that it is not executed
             * before any other handler posted before has finished.
             * Handlers posted on different Strands can always be run in parallel.
             *
             * @param handler function without arguments and return value - will be copied
             */
            void post(const boost::function<void()>& handler);

            /**
             * Post a handler to the io_service given to the constructor with the guarantee that it is not executed
             * before any other handler posted before has finished.
             * Handlers posted on different Strands can always be run in parallel.
             *
             * @param handler function without arguments and return value as r-value reference - will be moved to avoid a copy
             */
            void post(boost::function<void()>&& handler);

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

            /// Helper actually running one tasks after another until queue is empty
            void run();


            boost::asio::io_service& m_ioService;

            boost::mutex m_tasksMutex; // to protect both, m_tasksRunning and m_tasks
            bool m_tasksRunning;
            std::queue<boost::function<void()> > m_tasks;
        };

    }
}
#endif	/* KARABO_NET_STRAND_HH */

