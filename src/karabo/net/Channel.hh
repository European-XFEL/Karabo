/* 
 * File:   Channel.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 1, 2011, 5:02 PM
 */

#ifndef KARABO_NET_CHANNEL_HH
#define	KARABO_NET_CHANNEL_HH

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

#include <karabo/util/Factory.hh>

#include "Connection.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package net
     */
    namespace net {

        class Channel {

            public:

            KARABO_CLASSINFO(Channel, "Channel", "1.0")

            typedef boost::function<void (const size_t&) > ReadSizeInBytesHandler;
            typedef boost::function<void () > ReadRawHandler;
            typedef boost::function<void (karabo::util::Hash&) > ReadHashRawHandler;

            typedef boost::function<void (std::vector<char>&) > ReadVectorHandler;
            typedef boost::function<void (std::string&) > ReadStringHandler;
            typedef boost::function<void (karabo::util::Hash&) > ReadHashHandler;
            typedef boost::function<void (karabo::util::Hash::Pointer&) > ReadHashPointerHandler;
            typedef boost::function<void (boost::shared_ptr<std::vector<char> >&) > ReadVectorPointerHandler;

            typedef boost::function<void (karabo::util::Hash&, std::vector<char>&) > ReadHashVectorHandler;
            typedef boost::function<void (karabo::util::Hash&, std::string&) > ReadHashStringHandler;
            typedef boost::function<void (karabo::util::Hash&, karabo::util::Hash&) > ReadHashHashHandler;
            typedef boost::function<void (karabo::util::Hash::Pointer&, karabo::util::Hash::Pointer&) > ReadHashPointerHashPointerHandler;
            typedef boost::function<void (karabo::util::Hash&, boost::shared_ptr<std::vector<char> >&) > ReadHashVectorPointerHandler;

            typedef boost::function<void () > WriteCompleteHandler;
            typedef boost::function<void () > WaitHandler;

            virtual ~Channel() {
            }

            // TODO Check, whether is needed
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

            virtual void read(boost::shared_ptr<std::vector<char> >& data) {
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
            virtual void read(karabo::util::Hash& data) {
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
            virtual void read(karabo::util::Hash& header, char* data, const size_t& size) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * This function reads into a header and a vector of chars. 
             * The reading will block until the data record is read.
             * @param header Hash object which will be updated to contain header information
             * @param data A vector which will be updated accordingly
             */
            virtual void read(karabo::util::Hash& header, std::vector<char>& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void read(karabo::util::Hash& header, boost::shared_ptr<std::vector<char> >& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * This function reads into header and a string
             * The reading will block until the data record is read.
             * CAVEAT: As string is not guaranteed to be represented by a contiguous block of memory this function
             * will always introduce a copy under the hood.
             * @param data A string which will be updated accordingly
             */
            virtual void read(karabo::util::Hash& header, std::string& data) {
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
            virtual void read(karabo::util::Hash& header, karabo::util::Hash& data) {
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
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::util::Hash&)
             */
            virtual void readAsyncHash(const ReadHashHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash. All memory management is done by the API.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::util::Hash&)
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
             * Asynchronously reads data into a hash header and a vector<char>. All memory management is done by the API.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::util::Hash&, const std::vector<char>&) 
             */
            virtual void readAsyncHashVector(const ReadHashVectorHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash header and a string. All memory management is done by the API.
             * NOTE: A string in general is not storing data contiguously. Thus, an additional copy under the hood is
             * needed which makes this interface slightly slower.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::util::Hash&, const std::string&)
             */
            virtual void readAsyncHashString(const ReadHashStringHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash header and a hash body. All memory management is done by the API.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::util::Hash&, const karabo::util::Hash&)
             */
            virtual void readAsyncHashHash(const ReadHashHashHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash header and a hash body. All memory management is done by the API.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::util::Hash&, const karabo::util::Hash&)
             */
            virtual void readAsyncHashPointerHashPointer(const ReadHashPointerHashPointerHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            /**
             * Asynchronously reads data into a hash header and a vector<char>. All memory management is done by the API.
             * @param handler Call-function of signature: void (Channel::Pointer, const karabo::util::Hash&, const std::vector<char>&) 
             */
            virtual void readAsyncHashVectorPointer(const ReadHashVectorPointerHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
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

            virtual void write(const std::vector<char>& data) {
                this->write(&data[0], data.size());
            }

            virtual void write(const std::string& data) {
                this->write(data.c_str(), data.size());
            }

            virtual void write(const karabo::util::Hash& data) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Synchronous Write - With Header               */
            //**************************************************************/

            virtual void write(const karabo::util::Hash& header, const char* data, const size_t& size) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void write(const karabo::util::Hash& header, const std::vector<char>& data) {
                this->write(header, &data[0], data.size());
            }

            virtual void write(const karabo::util::Hash& header, boost::shared_ptr<const std::vector<char> >& data) {
                this->write(header, &(*data)[0], data->size());
            }

            virtual void write(const karabo::util::Hash& header, const std::string& data) {
                this->write(header, data.c_str(), data.size());
            }

            virtual void write(const karabo::util::Hash& header, const karabo::util::Hash& body) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Asynchronous Write - No Header                */
            //**************************************************************/

            virtual void writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncVector(const std::vector<char>& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncVectorPointer(const boost::shared_ptr<std::vector<char> >& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncHash(const karabo::util::Hash& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*              Asynchronous Write - With Header              */
            //**************************************************************/

            virtual void writeAsyncHashRaw(const karabo::util::Hash& header, const char* data, const size_t& size, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncHashVector(const karabo::util::Hash& header, const std::vector<char>& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncHashVectorPointer(const karabo::util::Hash& header, const boost::shared_ptr<std::vector<char> >& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsyncHashHash(const karabo::util::Hash& header, const karabo::util::Hash& data, const WriteCompleteHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            //**************************************************************/
            //*                     Misc                                   */ 
            //**************************************************************/

            virtual void setErrorHandler(const ErrorHandler& handler) = 0;

            virtual void waitAsync(int milliseconds, const WaitHandler& handler) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void setTimeoutSyncRead(int milliseconds) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not implemented!");
            }

            virtual void close() = 0;

            virtual bool isOpen() = 0;

            virtual void writeAsync(const char* data, const size_t& size, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsync(const std::vector<char>& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsync(const boost::shared_ptr<std::vector<char> >& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsync(const std::string& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsync(const karabo::util::Hash& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsync(const karabo::util::Hash& header, const char* data, const size_t& size, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsync(const karabo::util::Hash& header, const std::vector<char>& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsync(const karabo::util::Hash& header,
                                    const boost::shared_ptr<std::vector<char> >& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsync(const karabo::util::Hash& header, const std::string& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void writeAsync(const karabo::util::Hash& header, const karabo::util::Hash& data, int prio = 4) {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Not supported for this transport layer");
            }

            virtual void setAsyncChannelPolicy(int priority, const std::string& policy) {
            }
        };
    }
}

#endif

