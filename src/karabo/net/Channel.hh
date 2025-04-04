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
 * File:   Channel.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 1, 2011, 5:02 PM
 */

#ifndef KARABO_NET_CHANNEL_HH
#define KARABO_NET_CHANNEL_HH

#include <functional>
#include <karabo/io/BufferSet.hh>
#include <memory>
#include <vector>

#include "Connection.hh"
#include "karabo/data/schema/Factory.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {


        /**
         * @class Channel
         * @brief Represents a communication channel used for p2p messaging on
         *        a connection to a remote instance. This is only an interface,
         *        see TcpChannel for a concrete implementation using the tcp protocol
         *
         */
        class Channel : public std::enable_shared_from_this<Channel> {
           public:
            KARABO_CLASSINFO(Channel, "Channel", "1.0")

            typedef std::function<void(const size_t&)> ReadSizeInBytesHandler;
            typedef std::function<void(const boost::system::error_code&)> ReadRawHandler;
            typedef std::function<void(const boost::system::error_code&, karabo::data::Hash&)> ReadHashRawHandler;

            typedef std::function<void(const boost::system::error_code&,
                                       const std::vector<karabo::io::BufferSet::Pointer>&)>
                  ReadVectorBufferSetPointerHandler;
            typedef std::function<void(const boost::system::error_code&, std::vector<char>&)> ReadVectorHandler;
            typedef std::function<void(const boost::system::error_code&, std::string&)> ReadStringHandler;
            typedef std::function<void(const boost::system::error_code&, karabo::data::Hash&)> ReadHashHandler;
            typedef std::function<void(const boost::system::error_code&, karabo::data::Hash::Pointer&)>
                  ReadHashPointerHandler;
            typedef std::function<void(const boost::system::error_code&, std::shared_ptr<std::vector<char> >&)>
                  ReadVectorPointerHandler;

            typedef std::function<void(const boost::system::error_code&, karabo::data::Hash&, std::vector<char>&)>
                  ReadHashVectorHandler;
            typedef std::function<void(const boost::system::error_code&, karabo::data::Hash&, std::string&)>
                  ReadHashStringHandler;
            typedef std::function<void(const boost::system::error_code&, karabo::data::Hash&, karabo::data::Hash&)>
                  ReadHashHashHandler;
            typedef std::function<void(const boost::system::error_code&, karabo::data::Hash::Pointer&,
                                       karabo::data::Hash::Pointer&)>
                  ReadHashPointerHashPointerHandler;
            typedef std::function<void(const boost::system::error_code&, karabo::data::Hash&,
                                       std::shared_ptr<std::vector<char> >&)>
                  ReadHashVectorPointerHandler;
            typedef std::function<void(const boost::system::error_code&, const karabo::data::Hash&,
                                       const karabo::io::BufferSet&)>
                  ReadHashBufferSetHandler;
            typedef std::function<void(const boost::system::error_code&, const karabo::data::Hash&,
                                       const std::vector<karabo::io::BufferSet::Pointer>&)>
                  ReadHashVectorBufferSetPointerHandler;

            typedef std::function<void(const boost::system::error_code&)> WriteCompleteHandler;

            virtual ~Channel() {}


            /**
             * Return a pointer to the connection this channels belongs to
             * @return
             */
            virtual Connection::Pointer getConnection() const = 0;

            /**
             * Synchronously reads the message's size.
             * Will block until a message arrives on the socket.
             * @return Size in bytes of incoming TCP message
             */
            virtual size_t readSizeInBytes() {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }


            /**************************************************************/
            /*              Synchronous Read - No Header                  */
            /**************************************************************/

            /**
             * Synchronously reads size bytes and return them as a string.
             * The reading will block until the bytes are read.
             * @param size This number of bytes will be copied into data
             *
             * @note reads up nBytes expecting no header. To be used ONLY
             * after a readAsyncStringUntil operation in case some bytes must
             * be read after readAsyncStringUntil has been used.
             */
            virtual std::string consumeBytesAfterReadUntil(const size_t nBytes) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Synchronously reads size bytes into data.
             * The reading will block until the data record is read.
             * @param data Pre-allocated contiguous block of memory
             * @param size This number of bytes will be copied into data
             */
            virtual void read(char* data, const size_t& size) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * This function reads into a vector of chars
             * The reading will block until the data record is read.
             * @param data A vector which will be updated accordingly
             */
            virtual void read(std::vector<char>& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void read(std::shared_ptr<std::vector<char> >& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * This function reads into a string
             * The reading will block until the data record is read.
             * CAVEAT: As string is not guaranteed to be represented by a contiguous block of memory this function
             * will always introduce a copy under the hood.
             * @param data A string which will be updated accordingly
             */
            virtual void read(std::string& data) {
                std::vector<char> tmp;
                this->read(tmp);
                data.assign(tmp.begin(), tmp.end());
            }

            /**
             * This function reads into a hash.
             * The reading will block until the data record is read.
             * The reading will block until the data record is read.
             * @param data Hash object which will be updated
             */
            virtual void read(karabo::data::Hash& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**************************************************************/
            /*              Synchronous Read - With Header                */
            /**************************************************************/

            /**
             * Synchronously reads size bytes from socket into data and provides a header.
             * The reading will block until the data record is read.
             * @param header Hash object which will be updated to contain header information
             * @param data Pre-allocated contiguous block of memory
             * @param size This number of bytes will be copied into data
             */
            virtual void read(karabo::data::Hash& header, char* data, const size_t& size) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * This function reads into a header and a vector of chars.
             * The reading will block until the data record is read.
             * @param header Hash object which will be updated to contain header information
             * @param data A vector which will be updated accordingly
             */
            virtual void read(karabo::data::Hash& header, std::vector<char>& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void read(karabo::data::Hash& header, std::shared_ptr<std::vector<char> >& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * This function reads into header and a string
             * The reading will block until the data record is read.
             * CAVEAT: As string is not guaranteed to be represented by a contiguous block of memory this function
             * will always introduce a copy under the hood.
             * @param data A string which will be updated accordingly
             */
            virtual void read(karabo::data::Hash& header, std::string& data) {
                std::vector<char> tmp;
                this->read(header, tmp);
                data.assign(tmp.begin(), tmp.end());
            }

            /**
             * This function reads into a header hash and a data hash.
             * The reading will block until the data record is read.
             * The reading will block until the data record is read.
             * @param header Hash object which will be updated to contain header information
             * @param data Hash object which will be updated to contain data information
             */
            virtual void read(karabo::data::Hash& header, karabo::data::Hash& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Asynchronous Read - No Header                 */
            //**************************************************************/

            /**
             * In case a message arrived, handler will be called back
             * The handler will inform about the number of bytes going to come in
             * @param handler Call-back function of signature: void (Channel::Pointer, const size_t&)
             */
            virtual void readAsyncSizeInBytes(const ReadSizeInBytesHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads size number of bytes into pre-allocated data buffer
             * A handler can be registered to inform about completion of writing
             * NOTE: This function only makes sense calling after having used "readAsyncSizeInBytes", which
             * gives a chance to correctly pre-allocated memory in user-space.
             * @param data Pre-allocated contiguous block of memory
             * @param size This number of bytes will be copied into data
             * @param handler Call-back function of signature: void (Channel::Pointer)
             */
            virtual void readAsyncRaw(char* data, const size_t& size, const ReadRawHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Read a string until terminator string is found. (No header is expected).
             *
             * @param terminator when this string found, read is done
             * @param handler handler with signature ReadStringHandler,
             *        i.e. void (const boost::system::error_code&, std::string&) is called.
             *        second handler parameter is the read string with terminator stripped away
             */
            virtual void readAsyncStringUntil(const std::string& terminator, const ReadStringHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a vector<char>. All memory management is done by the API.
             * @param handler Call-function of signature: void (Channel::Pointer, const std::vector<char>&)
             */
            virtual void readAsyncVector(const ReadVectorHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a string. All memory management is done by the API.
             * NOTE: A string in general is not storing data contiguously. Thus, an additional copy under the hood is
             * needed which makes this interface slightly slower.
             * @param handler Call-function of signature: void (Channel::Pointer, const std::string&)
             */
            virtual void readAsyncString(const ReadStringHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash. All memory management is done by the API.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::data::Hash&)
             */
            virtual void readAsyncHash(const ReadHashHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash. All memory management is done by the API.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::data::Hash&)
             */
            virtual void readAsyncHashPointer(const ReadHashPointerHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a vector<char>. All memory management is done by the API.
             * @param handler Call-function of signature: void (Channel::Pointer, const std::vector<char>&)
             */
            virtual void readAsyncVectorPointer(const ReadVectorPointerHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }


            //**************************************************************/
            //*              Asynchronous Read - With Header               */
            //**************************************************************/

            /**
             * Asynchronously reads data into a hash header and a vector<char>. All memory management is done by the
             * API.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::data::Hash&, const
             * std::vector<char>&)
             */
            virtual void readAsyncHashVector(const ReadHashVectorHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }


            /**
             * Asynchronously reads data into a hash header and a string. All memory management is done by the API.
             * NOTE: A string in general is not storing data contiguously. Thus, an additional copy under the hood is
             * needed which makes this interface slightly slower.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::data::Hash&, const
             * std::string&)
             */
            virtual void readAsyncHashString(const ReadHashStringHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash header and a hash body. All memory management is done by the API.
             * @param handler Call-function of signature: void (const ErrorCode&, const karabo::data::Hash&, const
             * karabo::data::Hash&)
             */
            virtual void readAsyncHashHash(const ReadHashHashHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash header and a hash body. All memory management is done by the API.
             * @param handler Call-function of signature: void (const ErrorCode&, const karabo::data::Hash&, const
             * karabo::data::Hash&)
             */
            virtual void readAsyncHashPointerHashPointer(const ReadHashPointerHashPointerHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash header and a vector<char>. All memory management is done by the
             * API.
             * @param handler Call-function of signature: void (const ErrorCode&, const karabo::data::Hash&, const
             * std::vector<char>&)
             */
            virtual void readAsyncHashVectorPointer(const ReadHashVectorPointerHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash header and into a vector of BufferSet pointers.
             * All memory management is done by the API.
             * @param handler Call-function of signature:
             * void (const ErrorCode&, const karabo::data::Hash&, const std::vector<karabo::io::BufferSet::Pointer>&)
             */
            virtual void readAsyncHashVectorBufferSetPointer(const ReadHashVectorBufferSetPointerHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for " + getClassInfo().getClassName());
            }


            //**************************************************************/
            //*              Synchronous Write - No Header                 */
            //**************************************************************/

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param data Pointer to a contiguous block of memory that should be written
             * @param size This number of bytes will be written
             */
            virtual void write(const char* data, const size_t& size) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param data vector of chars containing the data to be written
             */
            virtual void write(const std::vector<char>& data) {
                this->write(&data[0], data.size());
            }

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param data string, where each character represents on byte of data to be written
             */
            virtual void write(const std::string& data) {
                this->write(data.c_str(), data.size());
            }

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param data is contained in a Hash with no particular structure, but serializable, i.e. containing no
             *       non-karabo data types or Hash derived types
             */
            virtual void write(const karabo::data::Hash& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Synchronous Write - With Header               */
            //**************************************************************/

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param header containing metadata for the data being written
             * @param data Pointer to a contiguous block of memory that should be written
             * @param size This number of bytes will be written
             */
            virtual void write(const karabo::data::Hash& header, const char* data, const size_t& size) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param header containing metadata for the data being written
             * @param data vector of chars containing the data to be written
             */
            virtual void write(const karabo::data::Hash& header, const std::vector<char>& data) {
                this->write(header, &data[0], data.size());
            }

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param header containing metadata for the data being written and BufferSet's layout
             * @param body vector of BufferSet pointers
             */
            virtual void write(const karabo::data::Hash& header,
                               const std::vector<karabo::io::BufferSet::Pointer>& body) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for " + getClassInfo().getClassName());
            }

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param header containing metadata for the data being written
             * @param data vector of chars containing the data to be written, passed as a shared pointer
             */
            virtual void write(const karabo::data::Hash& header, std::shared_ptr<const std::vector<char> >& data) {
                this->write(header, &(*data)[0], data->size());
            }

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param header containing metadata for the data being written
             * @param data string, where each character represents on byte of data to be written
             */
            virtual void write(const karabo::data::Hash& header, const std::string& data) {
                this->write(header, data.c_str(), data.size());
            }

            /**
             * Synchronous write. The function blocks until all bytes are written.
             * @param header containing metadata for the data being written
             * @param data is contained in a Hash with no particular structure, but serializable, i.e. containing no
             *       non-karabo data types or Hash derived types
             */
            virtual void write(const karabo::data::Hash& header, const karabo::data::Hash& body) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Asynchronous Write - No Header                */
            //**************************************************************/

            /**
             * Write data asynchronously, i.e. do not block upon call. Upon write completion a handler function is
             * called
             * @param data Pointer to a contiguous block of memory that should be written
             * @param size This number of bytes will be written
             * @param handler to be called upon write completion handler. Needs to be a function wrapped into a
             * std::function which takes const boost::system::error_code& as its only argument.
             */
            virtual void writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Upon write completion a handler function is
             * called
             * @param data vector of chars containing the data to be written
             * @param handler to be called upon write completion handler. Needs to be a function wrapped into a
             * std::function which takes const boost::system::error_code& as its only argument.
             */
            virtual void writeAsyncVector(const std::vector<char>& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Upon write completion a handler function is
             * called
             * @param data vector of chars containing the data to be written, passed as a shared pointer
             * @param handler to be called upon write completion handler. Needs to be a function wrapped into a
             * std::function which takes const boost::system::error_code& as its only argument.
             */
            virtual void writeAsyncVectorPointer(const std::shared_ptr<std::vector<char> >& data,
                                                 const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Upon write completion a handler function is
             * called
             * @param data is contained in a Hash with no particular structure, but serializable, i.e. containing no
             *       non-karabo data types or Hash derived types
             * @param handler to be called upon write completion handler. Needs to be a function wrapped into a
             * std::function which takes const boost::system::error_code& as its only argument.
             */
            virtual void writeAsyncHash(const karabo::data::Hash& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Asynchronous Write - With Header              */
            //**************************************************************/

            /**
             * Write data asynchronously, i.e. do not block upon call. Upon write completion a handler function is
             * called
             * @param header containing metadata for the data being written
             * @param data Pointer to a contiguous block of memory that should be written
             * @param size This number of bytes will be written
             * @param handler to be called upon write completion handler. Needs to be a function wrapped into a
             * std::function which takes const boost::system::error_code& as its only argument.
             */
            virtual void writeAsyncHashRaw(const karabo::data::Hash& header, const char* data, const size_t& size,
                                           const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Upon write completion a handler function is
             * called
             * @param header containing metadata for the data being written
             * @param data vector of chars containing the data to be written
             * @param handler to be called upon write completion. Needs to be a function wrapped into a
             * std::function which takes const boost::system::error_code& as its only argument.
             */
            virtual void writeAsyncHashVector(const karabo::data::Hash& header, const std::vector<char>& data,
                                              const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Upon write completion a handler function is
             * called
             * @param header containing metadata for the data being written
             * @param data vector of chars containing the data to be written, passed as a shared pointer
             * @param handler to be called upon write completion handler. Needs to be a function wrapped into a
             * std::function which takes const boost::system::error_code& as its only argument.
             */
            virtual void writeAsyncHashVectorPointer(const karabo::data::Hash& header,
                                                     const std::shared_ptr<std::vector<char> >& data,
                                                     const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Upon write completion a handler function is
             * called
             * @param header containing metadata for the data being written
             * @param body data contained in a Hash with no particular structure, but serializable, i.e. containing no
             *       non-karabo data types or Hash derived types
             * @param handler to be called upon write completion. Needs to be a function wrapped into a
             * std::function which takes const boost::system::error_code& as its only argument.
             */
            virtual void writeAsyncHashHash(const karabo::data::Hash& header, const karabo::data::Hash& data,
                                            const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write header and vector<BufferSet::Pointer> asynchronously.
             *
             * Upon write completion a handler function is called. Data inside the buffers must not be changed or
             * deleted before this handler is called. Special care is needed if any Hash that had been serialised into
             * the buffers contained an NDArray: The raw data of the array will be shared between the BufferSet and the
             * Hash. Deletion of the Hash is safe, though.
             *
             * @param header containing metadata for the data being written
             * @param body data as a vector of BufferSet::Pointer
             * @param handler to be called upon write completion. Needs to be a function wrapped into a
             *        std::function which takes const boost::system::error_code& as its only argument.
             */
            virtual void writeAsyncHashVectorBufferSetPointer(const karabo::data::Hash& header,
                                                              const std::vector<karabo::io::BufferSet::Pointer>& body,
                                                              const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*                     Misc                                   */
            //**************************************************************/

            /**
             * Returns the number of bytes read since the last call of this method
             */
            virtual size_t dataQuantityRead() {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            /**
             * Returns the number of bytes written since the last call of this method
             */
            virtual size_t dataQuantityWritten() {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            /**
             * Set a timeout in when synchronous reads timeout if the haven't been handled
             * @param milliseconds
             */
            virtual void setTimeoutSyncRead(int milliseconds) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            /**
             * Close this channel
             */
            virtual void close() = 0;

            /**
             * Check if this channel is open
             * @return
             */
            virtual bool isOpen() = 0;

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param data Pointer to a contiguous block of memory that should be written
             * @param size This number of bytes will be written
             * @param prio the priority of this write operation
             */
            virtual void writeAsync(const char* data, const size_t& size, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param data vector of chars containing the data to be written
             * @param prio the priority of this write operation
             */
            virtual void writeAsync(const std::vector<char>& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param data vector of chars containing the data to be written, passed as a shared pointer
             * @param prio the priority of this write operation
             */
            virtual void writeAsync(const std::shared_ptr<std::vector<char> >& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param data passed as a string were each character represents one byte of the message
             * @param prio the priority of this write operation
             */
            virtual void writeAsync(const std::string& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param data is contained in a Hash with no particular structure, but serializable, i.e. containing no
             *        non-karabo data types or Hash derived types
             * @param prio the priority of this write operation
             * @param copyAllData When false, raw data (ByteArray) inside an NDArray won't be copied. For other kind of
             * data, the value of this flag is ignored and a copy will take place.
             */
            virtual void writeAsync(const karabo::data::Hash& data, int prio = 4, bool copyAllData = true) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param data Pointer to a contiguous block of memory that should be written
             * @param header containing metadata for the data being written
             * @param size This number of bytes will be written
             * @param prio the priority of this write operation
             */
            virtual void writeAsync(const karabo::data::Hash& header, const char* data, const size_t& size,
                                    int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param header containing metadata for the data being written
             * @param data vector of chars containing the data to be written
             * @param prio the priority of this write operation
             */
            virtual void writeAsync(const karabo::data::Hash& header, const std::vector<char>& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param header containing metadata for the data being written
             * @param data vector of chars containing the data to be written, passed as a shared pointer
             * @param prio the priority of this write operation
             */
            virtual void writeAsync(const karabo::data::Hash& header, const std::shared_ptr<std::vector<char> >& data,
                                    int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param header containing metadata for the data being written
             * @param data passed as a string were each character represents one byte of the message
             * @param prio the priority of this write operation
             */
            virtual void writeAsync(const karabo::data::Hash& header, const std::string& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Write data asynchronously, i.e. do not block upon call. Fire and forget, no callback called upon
             * completion
             * @param header containing metadata for the data being written
             * @param data is contained in a Hash with no particular structure, but serializable, i.e. containing no
             *        non-karabo data types or Hash derived types
             * @param prio the priority of this write operation.
             * @param copyAllData When false, raw data (ByteArray) inside an NDArray won't be copied. For other kind of
             * data, the value of this flag is ignored and a copy will take place.
             */
            virtual void writeAsync(const karabo::data::Hash& header, const karabo::data::Hash& data, int prio = 4,
                                    bool copyAllData = true) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Set the policy of how data is queue on this channel for the queue of the given priority.
             * Policies are:
             *
             * - "LOSSLESS": all data is queue and the queue increases in size within incoming data
             * - "REJECT_NEWEST": if the queue's fixed capacity is reached new data is rejected
             * - "REMOVE_OLDEST": if the queue's fixed capacity is the oldest data is rejected
             *
             * NOTE: This method can potentially modify the capacity of a queue which is in use! This
             * is undefined behavior. Users are encouraged to only call this method when intializing a
             * Channel object instance.
             *
             * @param priority of the queue to set the policy for
             * @param policy to set for this queue
             * @param capacity is an optional capacity for the queue
             */
            virtual void setAsyncChannelPolicy(int priority, const std::string& new_policy, const size_t capacity = 0) {
            }
        };
    } // namespace net
} // namespace karabo

#endif
