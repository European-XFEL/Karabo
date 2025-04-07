/*
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
 */
/*
 * File:   Queues.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on September 21, 2015, 11:56 AM
 */

#ifndef KARABO_NET_QUEUES_HH
#define KARABO_NET_QUEUES_HH

#include <boost/circular_buffer.hpp>
#include <deque>
#include <memory>
#include <vector>

#include "karabo/log/Logger.hh"

namespace karabo {
    namespace net {

        typedef std::shared_ptr<std::vector<char> > VectorCharPointer;

        /**
         * @class Message
         * @brief This class represents a message in the distributed Karabo system
         */
        class Message {
           public:
            typedef std::shared_ptr<Message> Pointer;

            Message() : m_body(new karabo::data::BufferSet()), m_header() {}

            explicit Message(const karabo::data::BufferSet::Pointer& data) : m_body(data), m_header() {}

            Message(const karabo::data::BufferSet::Pointer& data, const VectorCharPointer& header)
                : m_body(data), m_header(header) {}

            Message(const Message& other) : m_body(other.body()), m_header(other.header()) {}

            virtual ~Message() {}

            /**
             * Return the body of the message
             * @return
             */
            const karabo::data::BufferSet::Pointer& body() const {
                return m_body;
            }

            /**
             * Return the header of the message
             * @return
             */
            const VectorCharPointer& header() const {
                return m_header;
            }


           private:
            karabo::data::BufferSet::Pointer m_body;
            VectorCharPointer m_header;
        };

        /**
         * @class Queue
         * @brief This class defines the interface for message queues in the Karabo
         *        distributed system
         */
        class Queue {
           public:
            KARABO_CLASSINFO(Queue, "Queue", "1.0")

            Queue() : m_counter(0) {}

            virtual ~Queue() {}

            /**
             * Return the size of the queue, i.e. the number of messages it holds
             * @return
             */
            virtual size_t size() = 0;

            /**
             * Return the maximum allowed size of this queue
             * @return
             */
            virtual size_t max_size() = 0;

            /**
             * Set the capacity in terms of messages this queue can hold
             * @param capacity
             */
            virtual void set_capacity(size_t capacity) = 0;

            /**
             * Return this queues message capacity
             * @return
             */
            virtual size_t capacity() = 0;

            /**
             * Clear this queue
             */
            virtual void clear() = 0;

            /**
             * Check if this queue is empty, i.e. size is 0
             * @return
             */
            virtual bool empty() = 0;

            /**
             * Check if this queue is full, i.e. if it has reached its maximum capacity
             * @return
             */
            virtual bool full() = 0;

            /**
             * Resize the queue to a new size
             * @param new_size
             */
            virtual void resize(size_t new_size) = 0;

            /**
             * Return the first element in the queue
             * @return
             */
            virtual const Message::Pointer& front() = 0;

            /**
             * Add an element to the end of the queue, increases the size by one
             * @param entry
             */
            virtual void push_back(const Message::Pointer& entry) = 0;

            /**
             * Pop the first element from the queue, decreases the size by one
             */
            virtual void pop_front() = 0;

           protected:
            unsigned long long m_counter;
        };

        /**
         * @class LosslessQueue
         * @brief The LosslessQueue implements a queue that guarantees to preserve messages.
         */
        class LosslessQueue : public Queue {
            std::deque<Message::Pointer> m_queue;

           public:
            KARABO_CLASSINFO(LosslessQueue, "LosslessQueue", "1.0")

            LosslessQueue() {}

            virtual ~LosslessQueue() {}

            size_t size() {
                return m_queue.size();
            }

            size_t max_size() {
                return m_queue.max_size();
            }

            void set_capacity(size_t capacity) {}

            size_t capacity() {
                return m_queue.max_size();
            }

            void clear() {
                m_queue.clear();
            }

            bool empty() {
                return m_queue.empty();
            }

            bool full() {
                return m_queue.size() == m_queue.max_size();
            }

            void resize(size_t new_size) {
                m_queue.resize(new_size);
            }

            const Message::Pointer& front() {
                return m_queue.front();
            }

            void push_back(const Message::Pointer& entry) {
                m_queue.push_back(entry);
            }

            void pop_front() {
                m_queue.pop_front();
            }
        };

        /**
         * @class RejectNewestQueue
         * @brief The RejectNewestQueue implements a queue that will reject
         *        new entries when it has reached its maximum capacity
         */
        class RejectNewestQueue : public LosslessQueue {
            size_t m_capacity;

           public:
            KARABO_CLASSINFO(RejectNewestQueue, "RejectNewestQueue", "1.0")

            RejectNewestQueue(const size_t capacity) : LosslessQueue(), m_capacity(capacity) {}

            virtual ~RejectNewestQueue() {
                clear();
            }

            void set_capacity(size_t capacity) {
                m_capacity = capacity;
            }

            size_t capacity() {
                return m_capacity;
            }

            size_t max_size() {
                return m_capacity;
            }

            void push_back(const Message::Pointer& entry) {
                if (size() < m_capacity) {
                    LosslessQueue::push_back(entry);
                } else {
                    if (m_counter++ % 1000 == 0) {
                        KARABO_LOG_FRAMEWORK_WARN << "Ignored message pointer upon pushing since capacity ("
                                                  << m_capacity << ") reached.";
                    }
                }
            }
        };

        /**
         * @class RemoveOldestQueue
         * @brief The RemoveOldestQueue implements a queue that removes the oldest
         *        element in the queue when it has reached is maximum capacity
         *        and a new element is pushed to it
         */
        class RemoveOldestQueue : public Queue {
            boost::circular_buffer<Message::Pointer> m_queue;

           public:
            KARABO_CLASSINFO(RemoveOldestQueue, "RemoveOldestQueue", "1.0")

            RemoveOldestQueue(const size_t capacity) : m_queue(capacity) {}

            virtual ~RemoveOldestQueue() {
                clear();
            }

            size_t size() {
                return m_queue.size();
            }

            size_t max_size() {
                return m_queue.max_size();
            }

            void set_capacity(size_t new_capacity) {
                m_queue.set_capacity(new_capacity);
            }

            size_t capacity() {
                return m_queue.capacity();
            }

            void clear() {
                m_queue.clear();
            }

            bool empty() {
                return m_queue.empty();
            }

            bool full() {
                return m_queue.full();
            }

            void resize(size_t new_size) {
                m_queue.resize(new_size);
            }

            const Message::Pointer& front() {
                return m_queue.front();
            }

            void push_back(const Message::Pointer& entry) {
                if (m_queue.full() && m_counter++ % 1000 == 0) {
                    KARABO_LOG_FRAMEWORK_WARN
                          << "Overwrite old message pointer upon pushing to buffer since it is full (size = "
                          << m_queue.size() << ").";
                }
                m_queue.push_back(entry);
            }

            void pop_front() {
                m_queue.pop_front();
            }
        };
    } // namespace net
} // namespace karabo

#endif /* KARABO_NET_QUEUES_HH */
