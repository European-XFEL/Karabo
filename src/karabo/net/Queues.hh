/* 
 * File:   Queues.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on September 21, 2015, 11:56 AM
 */

#ifndef KARABO_NET_QUEUES_HH
#define	KARABO_NET_QUEUES_HH

#include <deque>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/circular_buffer.hpp>


namespace karabo {
    namespace net {

        typedef boost::shared_ptr<std::vector<char> > VectorCharPointer;

        class Message {
        public:

            typedef boost::shared_ptr<Message> Pointer;

            Message() : m_body(new std::vector<char>), m_header() {
            }

            Message(const VectorCharPointer& data) : m_body(data), m_header() {
            }

            Message(const VectorCharPointer& data, const VectorCharPointer& header) : m_body(data), m_header(header) {
            }
            
            Message(const Message& other) : m_body(other.body()), m_header(other.header()) {
            }
            
            virtual ~Message() {
            }

            const VectorCharPointer& body() const {
                return m_body;
            }

            const VectorCharPointer& header() const {
                return m_header;
            }


        private:
            
            VectorCharPointer m_body;
            VectorCharPointer m_header;
        };

        class Queue {
        public:

            typedef boost::shared_ptr<Queue> Pointer;

            Queue() {
            }

            virtual ~Queue() {
            }

            virtual size_t size() = 0;

            virtual size_t max_size() = 0;

            virtual void set_capacity(size_t capacity) = 0;

            virtual size_t capacity() = 0;

            virtual void clear() = 0;

            virtual bool empty() = 0;

            virtual bool full() = 0;

            virtual void resize(size_t new_size) = 0;

            virtual const Message::Pointer& front() = 0;

            virtual void push_back(const Message::Pointer& entry) = 0;

            virtual void pop_front() = 0;
        };

        class LosslessQueue : public Queue {
            
            std::deque<Message::Pointer> m_queue;

        public:

            LosslessQueue() {
            }

            virtual ~LosslessQueue() {
            }

            size_t size() {
                return m_queue.size();
            }

            size_t max_size() {
                return m_queue.max_size();
            }

            void set_capacity(size_t capacity) {
            }

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

        class RejectNewestQueue : public LosslessQueue {
            size_t m_capacity;

        public:

            RejectNewestQueue() : LosslessQueue(), m_capacity(1000) {
            }

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
                if (size() < m_capacity) LosslessQueue::push_back(entry);
            }

        };

        class RemoveOldestQueue : public Queue {
            boost::circular_buffer<Message::Pointer> m_queue;

        public:

            RemoveOldestQueue() : m_queue(1000) {
            }

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
                m_queue.push_back(entry);
            }

            void pop_front() {
                m_queue.pop_front();
            }

        };
    }
}

#endif	/* KARABO_NET_QUEUES_HH */

