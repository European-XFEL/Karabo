#include <iostream>
#include <assert.h>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/util/MetaTools.hh>
#include <boost/pointer_cast.hpp>
#include <snappy.h>
#include "Channel.hh"
#include "TcpChannel.hh"
#include "EventLoop.hh"
#include "karabo/io/HashBinarySerializer.hh"

namespace karabo {
    namespace net {

        using namespace std;
        using namespace boost::asio;
        using namespace karabo::util;

        const size_t kDefaultQueueCapacity = 5000; //JW: Moved from Queue.h


        TcpChannel::TcpChannel(Connection::Pointer connection)
            : m_connectionPointer(boost::dynamic_pointer_cast<TcpConnection>(connection))
            , m_socket(EventLoop::getIOService())
            , m_activeHandler(TcpChannel::NONE)
            , m_readHeaderFirst(false)
            , m_inboundData(new std::vector<char>())
            , m_inboundHeader(new std::vector<char>())
            , m_outboundData(new std::vector<char>())
            , m_outboundHeader(new std::vector<char>())
            , m_queue(10)
            , m_queueWrittenBytes(m_queue.size(), 0)
            , m_readBytes(0)
            , m_writtenBytes(0)
            , m_writeInProgress(false)
            , m_quit(false)
            , m_syncCounter(0)
            , m_asyncCounter(0) {
            m_queue[4] = Queue::Pointer(new LosslessQueue);
            m_queue[2] = Queue::Pointer(new RemoveOldestQueue(kDefaultQueueCapacity));
            m_queue[0] = Queue::Pointer(new RejectNewestQueue(kDefaultQueueCapacity));

            if (m_connectionPointer->m_serializationType == "binary") {
                m_binarySerializer = karabo::io::BinarySerializer<Hash>::create("Bin");
            } else {
                m_textSerializer = karabo::io::TextSerializer<Hash>::create("Xml", Hash("indentation", -1));
            }
        }


        TcpChannel::~TcpChannel() {
            close();
        }

#define _KARABO_VECTOR_TO_SIZE(x, v) {\
                size_t sizeofLength = m_connectionPointer->getSizeofLength();\
                assert((x).size() == sizeofLength);\
                if (m_connectionPointer->lengthIsText()) {\
                    try {\
                        v = boost::lexical_cast<size_t>(std::string((x).begin(), (x).end()));\
                    } catch(const boost::bad_lexical_cast& e) {\
                        throw KARABO_CAST_EXCEPTION(std::string(e.what()) + " ( the source is '" \
                                + std::string((x).begin(), (x).end()) + "', source_type=" + e.source_type().name()\
                                + " and target_type=" + e.target_type().name() + " )");\
                    }\
                } else if (sizeofLength == sizeof (uint8_t)) {\
                    v = *reinterpret_cast<uint8_t*> (&(x)[0]);\
                } else if (sizeofLength == sizeof (uint16_t)) {\
                    v = *reinterpret_cast<uint16_t*> (&(x)[0]);\
                } else if (sizeofLength == sizeof (uint64_t)) {\
                    v = *reinterpret_cast<uint64_t*> (&(x)[0]);\
                } else {\
                    v = *reinterpret_cast<uint32_t*> (&(x)[0]);\
                }\
        }

#define _KARABO_SIZE_TO_VECTOR(x, v) {\
                size_t sizeofLength = m_connectionPointer->getSizeofLength();\
                bool lengthIsText = m_connectionPointer->lengthIsText();\
                if (lengthIsText) {\
                    ostringstream oss;\
                    oss.fill('0');\
                    oss.width(sizeofLength);\
                    oss << v;\
                    string slen = oss.str();\
                    (x).assign(slen.begin(), slen.end());\
                } else {\
                    const char* p = reinterpret_cast<const char*> (&v);\
                    (x).assign(p, p + sizeofLength);\
                }\
                }


        size_t TcpChannel::readSizeInBytes() {
            size_t sizeofLength = m_connectionPointer->getSizeofLength();
            if (sizeofLength > 0) {
                ErrorCode error; //in case of error
                m_inboundMessagePrefix.resize(sizeofLength);
                m_readBytes += boost::asio::read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(), error);
                if (error) throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message());
                // prefix[0] - message length (body)
                size_t byteSize = 0;
                _KARABO_VECTOR_TO_SIZE(m_inboundMessagePrefix, byteSize); // reads size of message and assigns
                return byteSize;
            } else {
                return 0;
            }
        }


        void TcpChannel::read(char* data, const size_t& size) {
            ErrorCode error;
            m_readBytes += boost::asio::read(m_socket, buffer(data, size), transfer_all(), error);
            if (error) {
                throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message());
            }
        }


        void TcpChannel::read(std::vector<char>& data) {
            size_t nBytes = this->readSizeInBytes();
            data.resize(nBytes);
            this->read(&data[0], nBytes);
        }


        void TcpChannel::read(boost::shared_ptr<std::vector<char> >& data) {
            size_t nBytes = this->readSizeInBytes();
            data->resize(nBytes);
            this->read(&(*data)[0], nBytes);
        }


        void TcpChannel::read(karabo::util::Hash& data) {
            std::vector<char> tmp;
            this->read(tmp);
            if (m_textSerializer) {
                m_textSerializer->load(data, reinterpret_cast<const char*> (&tmp[0])); // Fasted method (no copy via string)
            } else {
                m_binarySerializer->load(data, tmp);
            }
        }


        void TcpChannel::read(karabo::util::Hash& header, char* data, const size_t& size) {
            this->read(header);
            if (header.has("__compression__")) {
                std::vector<char> compressed;
                this->read(compressed);
                decompress(header, compressed, data, size);
            } else {
                this->read(data, size);
            }
        }


        void TcpChannel::read(karabo::util::Hash& header, std::vector<char>& data) {
            this->read(header);
            if (header.has("__compression__")) {
                std::vector<char> compressed;
                this->read(compressed);
                decompress(header, compressed, data);
            } else {
                this->read(data);
            }
        }


        void TcpChannel::read(karabo::util::Hash& header, boost::shared_ptr<std::vector<char> >& data) {
            this->read(header);
            if (header.has("__compression__")) {
                std::vector<char> compressed;
                this->read(compressed);
                decompress(header, compressed, *data);
            } else {
                this->read(*data);
            }
        }


        void TcpChannel::read(karabo::util::Hash& header, karabo::util::Hash& data) {
            this->read(header);
            if (header.has("__compression__")) {
                std::vector<char> compressed, tmp;
                this->read(compressed);
                decompress(header, compressed, tmp);
                if (m_textSerializer) {
                    m_textSerializer->load(data, reinterpret_cast<const char*> (&tmp[0])); // Fasted method (no copy via string)
                } else {
                    m_binarySerializer->load(data, tmp);
                }
            } else {
                this->read(data);
            }
        }


        void TcpChannel::readAsyncSizeInBytes(const ReadSizeInBytesHandler& handler) {
            // This public interface has to ensure that we go at least once via EventLoop, i.e. we are asynchronous.
            readAsyncSizeInBytesImpl(handler, false);
        }


        void TcpChannel::readAsyncSizeInBytesImpl(const ReadSizeInBytesHandler& handler, bool allowNonAsync) {
            size_t sizeofLength = m_connectionPointer->getSizeofLength();
            if (sizeofLength == 0) throw KARABO_LOGIC_EXCEPTION("Message's sizeTag size was configured to be 0. Thus, registration of this function does not make sense!");
            m_inboundMessagePrefix.resize(sizeofLength);
            if (allowNonAsync && m_socket.available() >= sizeofLength) {
                m_syncCounter++;
                boost::system::error_code ec;
                size_t rsize = m_socket.read_some(buffer(m_inboundMessagePrefix), ec);
                assert(rsize == sizeofLength);
                onSizeInBytesAvailable(ec, handler);
            } else {
                m_asyncCounter++;
                boost::asio::async_read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(),
                                        util::bind_weak(&karabo::net::TcpChannel::onSizeInBytesAvailable,
                                                        this, boost::asio::placeholders::error, handler));
            }
        }


        void TcpChannel::onSizeInBytesAvailable(const ErrorCode& e, const ReadSizeInBytesHandler& handler) {
            if (e) {
                bytesAvailableHandler(e);
                return;
            }

            size_t messageSize;
            _KARABO_VECTOR_TO_SIZE(m_inboundMessagePrefix, messageSize);
            handler(messageSize);
        }


        void TcpChannel::byteSizeAvailableHandler(const size_t byteSize) {
            // byteSizeAvailableHandler is only used as handler argument of readAsyncSizeInBytes
            // and readAsyncSizeInBytes binds it to another handler which is protected by a bind_weak
            // so we do not have to bind_weak here again.
            m_inboundData->resize(byteSize);
            if (m_socket.available() >= byteSize) {
                m_syncCounter++;
                boost::system::error_code ec;
                size_t rsize = m_socket.read_some(buffer(m_inboundData->data(), byteSize), ec);
                assert(rsize == byteSize);
                bytesAvailableHandler(ec);
            } else {
                m_asyncCounter++;
                this->readAsyncRawImpl(m_inboundData->data(), byteSize,
                                       boost::bind(&karabo::net::TcpChannel::bytesAvailableHandler, this, _1), true);
            }
        }


        void TcpChannel::readAsyncRaw(char* data, const size_t& size, const ReadRawHandler& handler) {
            // This public interface has to ensure that we go at least once via EventLoop, i.e. we are asynchronous.
            readAsyncRawImpl(data, size, handler, false);
        }


        void TcpChannel::readAsyncRawImpl(char* data, const size_t& size, const ReadRawHandler& handler, bool allowNonAsync) {
            if (allowNonAsync && m_socket.available() >= size) {
                m_syncCounter++;
                boost::system::error_code ec;
                size_t rsize = m_socket.read_some(buffer(data, size), ec);
                assert(rsize == size);
                onBytesAvailable(ec, rsize, handler);
            } else {
                m_asyncCounter++;
                boost::asio::async_read(m_socket, buffer(data, size), transfer_all(),
                                        util::bind_weak(&karabo::net::TcpChannel::onBytesAvailable, this,
                                                        boost::asio::placeholders::error,
                                                        boost::asio::placeholders::bytes_transferred(),
                                                        handler));
            }
        }


        void TcpChannel::onBytesAvailable(const ErrorCode& error, const size_t length, const ReadRawHandler& handler) {
            // Update the total read bytes count
            m_readBytes += length;
            handler(error);
        }


        void TcpChannel::readAsyncVector(const ReadVectorHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::VECTOR;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncVectorPointer(const ReadVectorPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            this->readAsyncVectorPointerImpl(handler);
        }


        void TcpChannel::readAsyncVectorPointerImpl(const ReadVectorPointerHandler& handler) {
            m_activeHandler = TcpChannel::VECTOR_POINTER;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncString(const ReadStringHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::STRING;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHash(const ReadHashHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashPointer(const ReadHashPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_POINTER;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        /////////////////////////////////   BufferSet handling  //////////////////////////////////////////////////////


        void TcpChannel::onVectorBufferSetPointerAvailable(const ErrorCode& e, size_t length,
                                                           const std::vector<karabo::io::BufferSet::Pointer>& buffers,
                                                           const ReadVectorBufferSetPointerHandler& handler) {
            size_t messageSize;
            _KARABO_VECTOR_TO_SIZE(m_inboundMessagePrefix, messageSize);
            if (!e) {
                assert(messageSize == length - 4);
                handler(e, buffers);
            } else {
                handler(e, std::vector<karabo::io::BufferSet::Pointer>());
            }
        }


        void TcpChannel::readAsyncVectorBufferSetPointerImpl(const std::vector<karabo::io::BufferSet::Pointer>& buffers,
                                                             const ReadVectorBufferSetPointerHandler& handler) {
            m_inboundMessagePrefix.clear();
            m_inboundMessagePrefix.resize(sizeof(unsigned int));
            std::vector<boost::asio::mutable_buffer> boostBuffers;
            boostBuffers.push_back(buffer(m_inboundMessagePrefix));
            karabo::io::BufferSet::appendTo(boostBuffers, buffers);
            boost::asio::async_read(m_socket, boostBuffers, transfer_all(),
                                    util::bind_weak(&TcpChannel::onVectorBufferSetPointerAvailable, this,
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred(),
                                                    buffers, handler));
        }


        void TcpChannel::onHashVectorBufferSetPointerRead(const boost::system::error_code& e,
                                                          const std::vector<karabo::io::BufferSet::Pointer>& buffers) {
            Hash::Pointer header = m_inHashHeader;
            m_inHashHeader.reset();
            m_activeHandler = TcpChannel::NONE;
            boost::any_cast<ReadHashVectorBufferSetPointerHandler>(m_readHandler) (e, *header, buffers);
        }


        void TcpChannel::onHashVectorBufferSetPointerVectorPointerRead(const boost::system::error_code& e,
                                                                       const boost::shared_ptr<std::vector<char>>& data,
                                                                       const ReadHashVectorBufferSetPointerHandler& handler) {
            Hash::Pointer header = m_inHashHeader;
            m_inHashHeader.reset();
            m_activeHandler = TcpChannel::NONE;
            // If OutputChannel is of old karabo version ...
            std::vector<karabo::io::BufferSet::Pointer> buffers;
            buffers.push_back(karabo::io::BufferSet::Pointer(new karabo::io::BufferSet(false)));
            if (!e) buffers[0]->emplaceBack(data);
            handler(e, *header, buffers);
        }


        void TcpChannel::readAsyncHashVectorBufferSetPointer(const ReadHashVectorBufferSetPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_VECTOR_BUFFER_SET_POINTER;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashVector(const ReadHashVectorHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_VECTOR;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashVectorPointer(const ReadHashVectorPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_VECTOR_POINTER;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashString(const ReadHashStringHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_STRING;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashHash(const ReadHashHashHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_HASH;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::readAsyncHashPointerHashPointer(const ReadHashPointerHashPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_POINTER_HASH_POINTER;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            // 'false' ensures that we leave the context and go via the event loop:
            this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), false);
        }


        void TcpChannel::bytesAvailableHandler(const boost::system::error_code& e) {
            if (!e) {
                // Only parse header information
                if (m_readHeaderFirst) {
                    m_readHeaderFirst = false;
                    m_inboundData.swap(m_inboundHeader);
                    if (m_activeHandler == HASH_VECTOR_BUFFER_SET_POINTER) {
                        m_inHashHeader = boost::shared_ptr<Hash>(new Hash());
                        this->prepareHashFromHeader(*m_inHashHeader);
                        if (m_inHashHeader->has("_bufferSetLayout_")) {
                            std::vector<karabo::io::BufferSet::Pointer> buffers;
                        
                            for (const karabo::util::Hash& layout : m_inHashHeader->get<std::vector<karabo::util::Hash>>("_bufferSetLayout_")) {
                                if (!layout.has("sizes") || !layout.has("types")) throw KARABO_LOGIC_EXCEPTION("Pipeline Protocol violation!");
                                const auto& sizes = layout.get<vector<unsigned int>>("sizes");
                                const auto& types = layout.get<vector<int>>("types");
                                if (sizes.size() != types.size()) throw KARABO_LOGIC_EXCEPTION("Pipeline Protocol violation!");
                                karabo::io::BufferSet::Pointer buffer(new karabo::io::BufferSet(false));
                                for (size_t ii = 0; ii < sizes.size(); ii++) buffer->add(sizes[ii], types[ii]);
                                buffers.push_back(buffer);
                            }
                            this->readAsyncVectorBufferSetPointerImpl(buffers, util::bind_weak(&TcpChannel::onHashVectorBufferSetPointerRead, this, _1, _2));
                        } else {
                            // OutputChannel from "old" karabo version? Then read the rest as vector of char
                            ReadHashVectorBufferSetPointerHandler handler = boost::any_cast<ReadHashVectorBufferSetPointerHandler>(m_readHandler);
                            this->readAsyncVectorPointerImpl(util::bind_weak(&TcpChannel::onHashVectorBufferSetPointerVectorPointerRead, this, _1, _2, handler));
                        }
                        return;
                    }
                    // 'true' allows non-async shortcut to avoid event loop posts if data has already arrived
                    this->readAsyncSizeInBytesImpl(util::bind_weak(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1), true);
                    return;
                }
            }
            HandlerType type = m_activeHandler;
            m_activeHandler = TcpChannel::NONE;
            switch (type) {
                case VECTOR:
                {
                    if (!e)
                        boost::any_cast<ReadVectorHandler>(m_readHandler) (e, *m_inboundData);
                    else {
                        std::vector<char> vec;
                        boost::any_cast<ReadVectorHandler>(m_readHandler) (e, vec);
                    }
                    break;
                }
                case VECTOR_POINTER:
                {
                    boost::shared_ptr<std::vector<char> > vec(new std::vector<char>());
                    if (!e) vec.swap(m_inboundData);
                    boost::any_cast<ReadVectorPointerHandler>(m_readHandler) (e, vec);
                    break;
                }
                case STRING:
                {
                    string tmp;
                    if (!e) tmp.assign(m_inboundData->begin(), m_inboundData->end());
                    boost::any_cast<ReadStringHandler>(m_readHandler) (e, tmp);
                    break;
                }
                case HASH:
                {
                    Hash h;
                    if (!e) this->prepareHashFromData(h);
                    boost::any_cast<ReadHashHandler>(m_readHandler) (e, h);
                    break;
                }
                case HASH_POINTER:
                {
                    Hash::Pointer h(new Hash);
                    if (!e) this->prepareHashFromData(*h);
                    boost::any_cast<ReadHashPointerHandler>(m_readHandler) (e, h);
                    break;
                }

                case HASH_VECTOR:
                {
                    Hash header;
                    std::vector<char> inData;
                    if (!e) {
                        this->prepareHashFromHeader(header);
                        if (header.has("__compression__"))
                            decompress(header, *m_inboundData, inData);
                        else {
                            boost::any_cast<ReadHashVectorHandler>(m_readHandler) (e, header, *m_inboundData);
                            break;
                        }
                    }
                    boost::any_cast<ReadHashVectorHandler>(m_readHandler) (e, header, inData);
                    break;
                }
                case HASH_VECTOR_POINTER:
                {
                    Hash header;
                    boost::shared_ptr<std::vector<char> > inData(new std::vector<char>());
                    if (!e) {
                        this->prepareHashFromHeader(header);
                        if (header.has("__compression__"))
                            decompress(header, *m_inboundData, *inData);
                        else
                            inData.swap(m_inboundData);
                    }
                    boost::any_cast<ReadHashVectorPointerHandler>(m_readHandler) (e, header, inData);
                    break;
                }

                case HASH_STRING:
                {
                    Hash header;
                    string tmp;
                    if (!e) {
                        this->prepareHashFromHeader(header);
                        if (header.has("__compression__"))
                            decompress(header, *m_inboundData, tmp);
                        else
                            tmp.assign(m_inboundData->begin(), m_inboundData->end());
                    }
                    boost::any_cast<ReadHashStringHandler>(m_readHandler) (e, header, tmp);
                    break;
                }

                case HASH_HASH:
                {
                    Hash header;
                    Hash body;
                    if (!e) {
                        this->prepareHashFromHeader(header);
                        if (header.has("__compression__")) {
                            boost::shared_ptr<std::vector<char> > tmp(new std::vector<char>());
                            tmp.swap(m_inboundData);
                            decompress(header, *tmp, *m_inboundData);
                        }
                        this->prepareHashFromData(body);
                    }
                    boost::any_cast<ReadHashHashHandler>(m_readHandler) (e, header, body);
                    break;
                }

                case HASH_POINTER_HASH_POINTER:
                {
                    Hash::Pointer header(new Hash);
                    Hash::Pointer body(new Hash);
                    if (!e) {
                        this->prepareHashFromHeader(*header);
                        if (header->has("__compression__")) {
                            boost::shared_ptr<std::vector<char> > tmp(new std::vector<char>());
                            tmp.swap(m_inboundData);
                            decompress(*header, *tmp, *m_inboundData);
                        }
                        this->prepareHashFromData(*body);
                    }
                    boost::any_cast<ReadHashPointerHashPointerHandler>(m_readHandler) (e, header, body);
                    break;
                }
                
                case HASH_VECTOR_BUFFER_SET_POINTER:
                {
                    // we will be here only if error code is not "success": "Operation canceled", "End of file"
                    boost::any_cast<ReadHashVectorBufferSetPointerHandler>(m_readHandler) (e, Hash(), std::vector<karabo::io::BufferSet::Pointer>());
                    break;
                }

                default:
                    throw KARABO_LOGIC_EXCEPTION("Bad internal error regarding asynchronous read handlers, contact BH or SE");
            }
        }


        void TcpChannel::decompress(karabo::util::Hash& header, const std::vector<char>& source, char* data, const size_t& size) {
            if (header.get<string>("__compression__") == "snappy") {
                decompressSnappy(&source[0], source.size(), data, size);
            } else
                throw KARABO_MESSAGE_EXCEPTION("Unsupported compression algorithm: \"" + header.get<string>("__compression__") + "\".");
            header.erase("__compression__");
        }


        void TcpChannel::decompress(karabo::util::Hash& header, const std::vector<char>& source, std::vector<char>& target) {
            if (header.get<string>("__compression__") == "snappy") {
                decompressSnappy(&source[0], source.size(), target);
            } else
                throw KARABO_MESSAGE_EXCEPTION("Unsupported compression algorithm: \"" + header.get<string>("__compression__") + "\".");
            header.erase("__compression__");
        }


        void TcpChannel::decompress(karabo::util::Hash& header, const std::vector<char>& source, std::string& target) {
            if (header.get<string>("__compression__") == "snappy") {
                std::vector<char> uncompressed;
                decompressSnappy(&source[0], source.size(), uncompressed);
                target.assign(uncompressed.begin(), uncompressed.end());
            } else
                throw KARABO_MESSAGE_EXCEPTION("Unsupported compression algorithm: \"" + header.get<string>("__compression__") + "\".");
            header.erase("__compression__");
        }


        void TcpChannel::decompressSnappy(const char* compressed, size_t compressed_length, char* data, const size_t& size) {
            // Get uncompressed length
            size_t uncompressedLength;
            bool res = snappy::GetUncompressedLength(compressed, compressed_length, &uncompressedLength);
            if (!res) {
                throw KARABO_NETWORK_EXCEPTION("Failed to call to GetUncompressedLength() for \"snappy\" compressed data.");
            }
            if (size < uncompressedLength) {
                throw KARABO_PARAMETER_EXCEPTION("No enough room for uncompressed data array: " + toString(uncompressedLength) + " bytes are required.");
            }
            // Decompress to output array
            if (!snappy::RawUncompress(compressed, compressed_length, data)) {
                throw KARABO_NETWORK_EXCEPTION("Failed to uncompress \"snappy\" compressed data.");
            }
        }


        void TcpChannel::decompressSnappy(const char* compressed, size_t compressed_length, std::vector<char>& target) {
            // Get uncompressed length
            size_t uncompressedLength;
            bool res = snappy::GetUncompressedLength(compressed, compressed_length, &uncompressedLength);
            if (!res) {
                throw KARABO_NETWORK_EXCEPTION("Failed to call to GetUncompressedLength() for \"snappy\" compressed data.");
            }
            // Decompress to array
            target.resize(uncompressedLength);
            if (!snappy::RawUncompress(compressed, compressed_length, &target[0])) {
                throw KARABO_NETWORK_EXCEPTION("Failed to uncompress \"snappy\" compressed data.");
            }
        }


        void TcpChannel::compress(karabo::util::Hash& header, const std::string& cmprs, const char* source, const size_t& source_length, std::vector<char>& target) {
            if (cmprs == "snappy")
                compressSnappy(source, source_length, target);
            else
                throw KARABO_MESSAGE_EXCEPTION("Unsupported compression algorithm: \"" + cmprs + "\".");
            header.set("__compression__", cmprs);
        }


        void TcpChannel::compress(karabo::util::Hash& header, const std::string& cmprs, const std::string& source, std::string& target) {
            std::vector<char> compressed;
            compress(header, cmprs, source.c_str(), source.size(), compressed);
            target.assign(compressed.begin(), compressed.end());
        }


        void TcpChannel::compress(karabo::util::Hash& header, const std::string& cmprs, const std::vector<char>& source, std::vector<char>& target) {
            compress(header, cmprs, &source[0], source.size(), target);
        }


        void TcpChannel::compressSnappy(const char* source, const size_t& source_length, std::vector<char>& target) {
            size_t maxlen = snappy::MaxCompressedLength(source_length);
            target.resize(maxlen);
            size_t compressed_length;
            snappy::RawCompress(source, source_length, &target[0], &compressed_length);
            target.resize(compressed_length);
        }


        void TcpChannel::write(const char* data, const size_t& size) {
            try {
                boost::system::error_code error; //in case of error
                vector<const_buffer> buf;
                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength > 0) {
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, size); // m_outboundMessagePrefix = size;
                    buf.push_back(buffer(m_outboundMessagePrefix)); // size of the prefix
                }
                buf.push_back(buffer(data, size)); // body

                m_writtenBytes += boost::asio::write(m_socket, buf, transfer_all(), error);

                if (!error) {
                    return;
                } else {
                    try {
                        m_socket.close();
                    } catch (...) {
                    } // close the socket because of error on the network
                    throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message() + ". Channel is closed!");
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const karabo::util::Hash& hdr, const std::vector<karabo::io::BufferSet::Pointer>& body) {
            using namespace karabo::io;
            Hash header = hdr; // copy header
            // Add the BufferSet layout structure into header just before serialization ...
            std::vector<Hash>& buffersVector = header.bindReference<std::vector<Hash>>("_bufferSetLayout_");
            for (std::vector<BufferSet::Pointer>::const_iterator it = body.begin(); it != body.end(); ++it) {
                buffersVector.push_back(Hash("sizes", (*it)->sizes(), "types", (*it)->types()));
            }
            try {
                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const size_t& size) instead.");
                }
                if (m_textSerializer) {
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Text serialization is not implemented for vectors of BufferSets");
                } else {
                    std::vector<char> headerBuf;
                    m_binarySerializer->save(header, headerBuf);
                    write(&headerBuf[0], headerBuf.size(), body);

                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const karabo::util::Hash& data) {
            try {
                if (m_textSerializer) {
                    string archive;
                    m_textSerializer->save(data, archive);
                    this->write(archive.c_str(), archive.size());
                } else {
                    std::vector<char> archive;
                    m_binarySerializer->save(data, archive);
                    this->write(&archive[0], archive.size());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const karabo::util::Hash& header, const karabo::util::Hash& body) {
            try {
                if (m_textSerializer) {
                    string headerBuf;
                    string bodyBuf;
                    m_textSerializer->save(body, bodyBuf);
                    if (m_connectionPointer->m_compressionUsageThreshold >= 0 && m_connectionPointer->m_compressionUsageThreshold < int(bodyBuf.size())) {
                        Hash hdr = header;
                        string compressed;
                        compress(hdr, m_connectionPointer->m_compression, bodyBuf, compressed);
                        m_textSerializer->save(hdr, headerBuf);
                        this->write(headerBuf.c_str(), headerBuf.size(), compressed.c_str(), compressed.size());
                    } else {
                        m_textSerializer->save(header, headerBuf);
                        this->write(headerBuf.c_str(), headerBuf.size(), bodyBuf.c_str(), bodyBuf.size());
                    }
                } else {
                    std::vector<char> headerBuf;
                    std::vector<char> bodyBuf;
                    m_binarySerializer->save(body, bodyBuf);
                    if (m_connectionPointer->m_compressionUsageThreshold >= 0 && m_connectionPointer->m_compressionUsageThreshold < int(bodyBuf.size())) {
                        Hash hdr = header;
                        std::vector<char> compressed;
                        compress(hdr, m_connectionPointer->m_compression, bodyBuf, compressed);
                        m_binarySerializer->save(hdr, headerBuf);
                        this->write(&headerBuf[0], headerBuf.size(), &compressed[0], compressed.size());
                    } else {
                        m_binarySerializer->save(header, headerBuf);
                        this->write(&headerBuf[0], headerBuf.size(), &bodyBuf[0], bodyBuf.size());
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const karabo::util::Hash& header, const char* data, const size_t& size) {
            try {
                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const size_t& size) instead.");
                }
                if (m_textSerializer) {
                    string headerBuf;
                    if (m_connectionPointer->m_compressionUsageThreshold >= 0 && m_connectionPointer->m_compressionUsageThreshold < int(size)) {
                        Hash hdr = header;
                        std::vector<char> compressed;
                        compress(hdr, m_connectionPointer->m_compression, data, size, compressed);
                        m_textSerializer->save(hdr, headerBuf);
                        write(headerBuf.c_str(), headerBuf.size(), &compressed[0], compressed.size());
                    } else {
                        m_textSerializer->save(header, headerBuf);
                        write(headerBuf.c_str(), headerBuf.size(), data, size);
                    }
                } else {
                    std::vector<char> headerBuf;
                    if (m_connectionPointer->m_compressionUsageThreshold >= 0 && m_connectionPointer->m_compressionUsageThreshold < int(size)) {
                        Hash hdr = header;
                        std::vector<char> compressed;
                        compress(hdr, m_connectionPointer->m_compression, data, size, compressed);
                        m_binarySerializer->save(hdr, headerBuf);
                        write(&headerBuf[0], headerBuf.size(), &compressed[0], compressed.size());
                    } else {
                        m_binarySerializer->save(header, headerBuf);
                        write(&headerBuf[0], headerBuf.size(), data, size);
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const char* header, const size_t& headerSize, const char* body, const size_t& bodySize) {
            try {
                boost::system::error_code error; //in case of error

                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const size_t& size) instead.");
                }
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, headerSize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, bodySize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(header, headerSize));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(body, bodySize));
                m_writtenBytes += boost::asio::write(m_socket, buf, transfer_all(), error);
                if (error) {
                    try {
                        m_socket.close();
                    } catch (...) {
                    }
                    throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message() + ". Channel is closed!");
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const char* header, const size_t& headerSize, const karabo::io::BufferSet& body) {
            try {
                boost::system::error_code error; //in case of error

                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const size_t& size) instead.");
                }
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, headerSize);
                size_t total_size = body.totalSize();
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, total_size);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(header, headerSize));
                buf.push_back(buffer(m_outboundMessagePrefix));
                body.appendTo(buf);
                m_writtenBytes += boost::asio::write(m_socket, buf, transfer_all(), error);
                if (error) {
                    const std::string local_ep_ip = m_socket.local_endpoint().address().to_string();
                    const std::string remote_ep_ip = m_socket.remote_endpoint().address().to_string();
                    try {
                        m_socket.close();
                    } catch (...) {
                    }
                    throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message() + ". Channel " + local_ep_ip + "->" + remote_ep_ip + " is closed!");
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const char* header, const size_t& headerSize, const std::vector<karabo::io::BufferSet::Pointer>& body) {
            try {
                boost::system::error_code error; //in case of error

                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const size_t& size) instead.");
                }
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, headerSize);
                size_t total_size = 0;
                for (const auto& b : body) total_size += b->totalSize(); 
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, total_size);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(header, headerSize));
                buf.push_back(buffer(m_outboundMessagePrefix));
                karabo::io::BufferSet::appendTo(buf, body);
                m_writtenBytes += boost::asio::write(m_socket, buf, transfer_all(), error);
                if (error) {
                    const std::string local_ep_ip = m_socket.local_endpoint().address().to_string();
                    const std::string remote_ep_ip = m_socket.remote_endpoint().address().to_string();
                    try {
                        m_socket.close();
                    } catch (...) {
                    }
                    throw KARABO_NETWORK_EXCEPTION("code #" + toString(error.value()) + " -- " + error.message() + ". Channel " + local_ep_ip + "->" + remote_ep_ip + " is closed!");
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }

        // asynchronous write


        void TcpChannel::managedWriteAsync(const WriteCompleteHandler& handler) {
            try {
                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                vector<const_buffer> buf;
                if (sizeofLength > 0) {
                    size_t s = m_outboundData->size();
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, s);
                    buf.push_back(buffer(m_outboundMessagePrefix));
                }
                buf.push_back(buffer(*m_outboundData));
                boost::asio::async_write(m_socket, buf,
                                         util::bind_weak(&TcpChannel::asyncWriteHandler, this,
                                                         boost::asio::placeholders::error,
                                                         boost::asio::placeholders::bytes_transferred(),
                                                         handler));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::managedWriteAsyncWithHeader(const WriteCompleteHandler& handler) {
            try {
                size_t hsize = m_outboundHeader->size(); // Header size
                size_t dsize = m_outboundData->size(); // Data size
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, hsize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, dsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(*m_outboundHeader));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(*m_outboundData));
                boost::asio::async_write(m_socket, buf,
                                         util::bind_weak(&TcpChannel::asyncWriteHandler, this,
                                                         boost::asio::placeholders::error,
                                                         boost::asio::placeholders::bytes_transferred(),
                                                         handler));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::unmanagedWriteAsync(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
            try {
                vector<const_buffer> buf;
                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength > 0) {
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, size);
                    buf.push_back(buffer(m_outboundMessagePrefix));
                }
                buf.push_back(buffer(data, size));
                boost::asio::async_write(m_socket, buf,
                                         util::bind_weak(&TcpChannel::asyncWriteHandler, this,
                                                         boost::asio::placeholders::error,
                                                         boost::asio::placeholders::bytes_transferred(),
                                                         handler));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::unmanagedWriteAsyncWithHeader(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
            try {
                size_t hsize = m_outboundHeader->size(); // Header size
                size_t dsize = m_outboundData->size(); // Data size
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, hsize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, dsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(*m_outboundHeader));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(data, size));
                boost::asio::async_write(m_socket, buf,
                                         util::bind_weak(&TcpChannel::asyncWriteHandler, this,
                                                         boost::asio::placeholders::error,
                                                         boost::asio::placeholders::bytes_transferred(),
                                                         handler));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::prepareHeaderFromHash(const karabo::util::Hash& hash) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                m_outboundHeader->assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, *m_outboundHeader);
            }
        }


        void TcpChannel::prepareHashFromHeader(karabo::util::Hash& hash) const {
            if (m_textSerializer) {
                m_textSerializer->load(hash, &(*m_inboundHeader)[0], m_inboundHeader->size());
            } else {
                m_binarySerializer->load(hash, *m_inboundHeader);
            }
        }


        void TcpChannel::prepareDataFromHash(const karabo::util::Hash& hash) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                m_outboundData->assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, *m_outboundData);
            }
        }


        void TcpChannel::prepareDataFromHash(const karabo::util::Hash& hash, boost::shared_ptr<std::vector<char> >& dataPtr) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                dataPtr->assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, *dataPtr);
            }
        }


        void TcpChannel::prepareHashFromData(karabo::util::Hash& hash) const {
            if (m_textSerializer) {
                m_textSerializer->load(hash, &(*m_inboundData)[0], m_inboundData->size());
            } else {
                m_binarySerializer->load(hash, *m_inboundData);
            }
        }


        void TcpChannel::writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
            try {
                if (m_connectionPointer->m_manageAsyncData) {
                    m_outboundData->resize(size);
                    std::memcpy(&(*m_outboundData)[0], data, size);
                    this->managedWriteAsync(handler);
                } else {
                    this->unmanagedWriteAsync(data, size, handler);
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncVector(const vector<char>& data, const Channel::WriteCompleteHandler& handler) {
            try {
                this->writeAsyncRaw(&data[0], data.size(), handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncVectorPointer(const boost::shared_ptr<vector<char> >& dataPtr,
                                                 const Channel::WriteCompleteHandler& handler) {
            try {
                if (!dataPtr)
                    throw KARABO_PARAMETER_EXCEPTION("Input parameter dataPtr is uninitialized shared pointer.");
                vector<const_buffer> buf;
                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength > 0) {
                    const size_t size = dataPtr->size();
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, size);
                    buf.push_back(buffer(m_outboundMessagePrefix));
                }
                buf.push_back(buffer(*dataPtr));
                boost::asio::async_write(m_socket, buf,
                                         util::bind_weak(&TcpChannel::asyncWriteHandlerBody, this,
                                                         boost::asio::placeholders::error,
                                                         boost::asio::placeholders::bytes_transferred(),
                                                         handler, dataPtr));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHash(const Hash& hash, const Channel::WriteCompleteHandler& handler) {
            try {
                boost::shared_ptr<std::vector<char> > dataPtr(new std::vector<char>());
                this->prepareDataFromHash(hash, dataPtr);
                this->writeAsyncVectorPointer(dataPtr, handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashRaw(const karabo::util::Hash& header, const char* data,
                                           const size_t& size, const WriteCompleteHandler& handler) {
            try {
                if (m_connectionPointer->m_compressionUsageThreshold >= 0 && m_connectionPointer->m_compressionUsageThreshold < int(size)) {
                    Hash hdr = header;
                    if (m_connectionPointer->m_manageAsyncData) {
                        m_outboundData->clear();
                        compress(hdr, m_connectionPointer->m_compression, data, size, *m_outboundData); // from "data,size" to "m_outboundData" (compressed data)
                        this->prepareHeaderFromHash(hdr);
                        this->managedWriteAsyncWithHeader(handler);
                    } else {
                        this->prepareHeaderFromHash(hdr);
                        this->unmanagedWriteAsyncWithHeader(data, size, handler);
                    }

                } else {
                    this->prepareHeaderFromHash(header);
                    if (m_connectionPointer->m_manageAsyncData) {
                        m_outboundData->resize(size);
                        std::memcpy(&(*m_outboundData)[0], data, size);
                        this->managedWriteAsyncWithHeader(handler);
                    } else {
                        this->unmanagedWriteAsyncWithHeader(data, size, handler);
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashVector(const Hash& header, const std::vector<char>& data,
                                              const Channel::WriteCompleteHandler& handler) {
            try {
                this->writeAsyncHashRaw(header, &data[0], data.size(), handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHeaderBodyImpl(const boost::shared_ptr<std::vector<char> >& header,
                                                  const boost::shared_ptr<std::vector<char> >& body,
                                                  const Channel::WriteCompleteHandler& handler) {
            try {
                size_t hsize = header->size(); // Header size
                size_t dsize = body->size(); // Data size
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, hsize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, dsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(*header));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(*body));
                boost::asio::async_write(m_socket, buf,
                                         util::bind_weak(&TcpChannel::asyncWriteHandlerHeaderBody,
                                                         this, boost::asio::placeholders::error,
                                                         boost::asio::placeholders::bytes_transferred(),
                                                         handler, header, body));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashVectorPointer(const karabo::util::Hash& header,
                                                     const boost::shared_ptr<std::vector<char> >& data,
                                                     const Channel::WriteCompleteHandler& handler) {
            try {
                if (!data)
                    throw KARABO_PARAMETER_EXCEPTION("Input parameter dataPtr is uninitialized shared pointer.");
                boost::shared_ptr<std::vector<char> > headerPtr(new std::vector<char>());
                if (m_connectionPointer->m_compressionUsageThreshold >= 0 && m_connectionPointer->m_compressionUsageThreshold < int(data->size())) {
                    Hash hdr = header;
                    m_outboundData->clear();
                    compress(hdr, m_connectionPointer->m_compression, *data, *m_outboundData);
                    this->prepareDataFromHash(hdr, headerPtr);
                    writeAsyncHeaderBodyImpl(headerPtr, m_outboundData, handler);
                } else {
                    this->prepareDataFromHash(header, headerPtr);
                    writeAsyncHeaderBodyImpl(headerPtr, data, handler);
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashHash(const karabo::util::Hash& header, const karabo::util::Hash& hash, const WriteCompleteHandler& handler) {
            try {
                boost::shared_ptr<std::vector<char> > dataPtr(new std::vector<char>());
                this->prepareDataFromHash(hash, dataPtr);
                this->writeAsyncHashVectorPointer(header, dataPtr, handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::asyncWriteHandler(const ErrorCode& e, const size_t length, const Channel::WriteCompleteHandler& handler) {
            try {
                m_writtenBytes += length;
                EventLoop::getIOService().post(boost::bind(handler, e));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::asyncWriteHandlerBody(const ErrorCode& e, const size_t length,
                                               const Channel::WriteCompleteHandler& handler,
                                               const boost::shared_ptr<std::vector<char> >& body) {
            try {
                m_writtenBytes += length;
                EventLoop::getIOService().post(boost::bind(handler, e));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::asyncWriteHandlerHeaderBody(const ErrorCode& e, const size_t length,
                                                     const Channel::WriteCompleteHandler& handler,
                                                     const boost::shared_ptr<std::vector<char> >& header,
                                                     const boost::shared_ptr<std::vector<char> >& body) {
            try {
                m_writtenBytes += length;
                EventLoop::getIOService().post(boost::bind(handler, e));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        // NOTE: There is the potential for a race condition here. However,
        // we are not worried about it. This method and `dataQuantityWritten`
        // should only be used for rough statistics gathering to be used
        // in understanding aggregate network usage. If a message here and there
        // gets overlooked, it's not a dealbreaker. The complexity needed to
        // protect access here is not worth the risk.


        size_t TcpChannel::dataQuantityRead() {
            const size_t ret = m_readBytes;
            m_readBytes = 0;
            return ret;
        }


        size_t TcpChannel::dataQuantityWritten() {
            const size_t ret = m_writtenBytes;
            m_writtenBytes = 0;
            return ret;
        }


        void TcpChannel::close() {
            if (m_socket.is_open())
                m_socket.cancel();
            m_socket.close();
        }


        bool TcpChannel::isOpen() {
            return m_socket.is_open();
        }


        karabo::util::Hash TcpChannel::queueInfo() {
            Hash info;
            boost::mutex::scoped_lock lock(m_queueMutex);
            for (size_t i = 0; i < m_queue.size(); ++i) {
                if (!m_queue[i]) continue;

                Hash queueStats;
                queueStats.set<unsigned long long>("pendingCount", m_queue[i]->size());
                queueStats.set<unsigned long long>("writtenBytes", m_queueWrittenBytes[i]);
                info.set<Hash>(karabo::util::toString(i), queueStats);
            }
            return info;
        }


#undef _KARABO_VECTOR_TO_SIZE
#undef _KARABO_SIZE_TO_VECTOR


        void TcpChannel::setAsyncChannelPolicy(int priority, const std::string& new_policy, const size_t capacity) {
            std::string candidate = boost::to_upper_copy<std::string>(new_policy);
            boost::mutex::scoped_lock lock(m_queueMutex);

            if (candidate == "LOSSLESS") {
                m_queue[priority] = Queue::Pointer(new LosslessQueue);

                if (capacity > 0) {
                    KARABO_LOG_FRAMEWORK_WARN << "Setting the max capacity of a LosslessQueue is not allowed!";
                }
            } else if (candidate == "REJECT_NEWEST") {
                m_queue[priority] = Queue::Pointer(new RejectNewestQueue(capacity > 0 ? capacity : kDefaultQueueCapacity));
            } else if (candidate == "REMOVE_OLDEST") {
                m_queue[priority] = Queue::Pointer(new RemoveOldestQueue(capacity > 0 ? capacity : kDefaultQueueCapacity));
            } else {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Trying to assign not supported channel policy : \"" + new_policy
                                                     + "\".  Supported policies are \"LOSSLESS\", \"REJECT_NEWEST\", \"REMOVE_OLDEST\"");
            }
        }


        void TcpChannel::prepareVectorFromHash(const karabo::util::Hash& hash, std::vector<char>& vec) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                vec.assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, vec);
            }
        }


        void TcpChannel::prepareHashFromVector(const std::vector<char>& vec, karabo::util::Hash& hash) const {
            if (m_textSerializer) {
                m_textSerializer->load(hash, &vec[0], vec.size());
            } else {
                m_binarySerializer->load(hash, vec);
            }
        }


        void TcpChannel::writeAsync(const Message::Pointer& mp, int prio) {

            boost::mutex::scoped_lock lock(m_queueMutex);
            m_queue[prio]->push_back(mp);

            if (!m_writeInProgress) {
                m_writeInProgress = true;
                EventLoop::getIOService().post(bind_weak(&TcpChannel::doWrite, this));
            }
        }


        void TcpChannel::doWrite() {

            try {
                Message::Pointer mp;
                int queueIndex = 0;
                {
                    boost::mutex::scoped_lock lock(m_queueMutex);
                    for (int i = 9; i >= 0; --i) {
                        if (!m_queue[i] || m_queue[i]->empty()) continue;
                        mp = m_queue[i]->front();
                        m_queue[i]->pop_front();
                        queueIndex = i;
                        break;
                    }
                    // if all queues are empty
                    if (!mp) {
                        m_writeInProgress = false;
                        return;
                    }
                }

                if (m_socket.is_open()) {
                    vector<const_buffer> buf;

                    if (mp->header()) {
                        VectorCharPointer hdr = mp->header();
                        m_headerSize = hdr->size();
                        buf.push_back(buffer(&m_headerSize, sizeof (unsigned int)));
                        buf.push_back(buffer(*hdr));
                    }

                    const VectorCharPointer& data = mp->body();
                    m_bodySize = data->size();

                    buf.push_back(buffer(&m_bodySize, sizeof (unsigned int)));
                    buf.push_back(buffer(*data));
                    boost::asio::async_write(m_socket, buf,
                                             util::bind_weak(&TcpChannel::doWriteHandler, this,
                                                             mp, boost::asio::placeholders::error,
                                                             boost::asio::placeholders::bytes_transferred(),
                                                             queueIndex));
                }
            } catch (const std::exception& e) {
                KARABO_LOG_FRAMEWORK_ERROR << "TcpChannel::doWrite exception : " << e.what();
                m_writeInProgress = false;
            }
        }


        void TcpChannel::doWriteHandler(Message::Pointer& mp, boost::system::error_code ec, const size_t length, const int queueIndex) {
            // Update the total written bytes counts
            m_queueWrittenBytes[queueIndex] += length;
            m_writtenBytes += length;

            if (!ec) {
                doWrite();
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "TcpChannel::doWriteHandler error : " << ec.value() << " -- " << ec.message()
                        << "  --  Channel is closed now!";
                m_writeInProgress = false;
                try {
                    m_socket.close();
                } catch (...) {
                }
            }
        }


        void TcpChannel::writeAsync(const char* data, const size_t& dsize, int prio) {
            VectorCharPointer datap = VectorCharPointer(new vector<char>(data, data + dsize));
            Message::Pointer mp(new Message(datap));
            writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const vector<char>& data, int prio) {
            VectorCharPointer datap = VectorCharPointer(new vector<char>(data.begin(), data.end()));
            Message::Pointer mp(new Message(datap));
            writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const VectorCharPointer& datap, int prio) {
            Message::Pointer mp(new Message(datap));
            writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const std::string& data, int prio) {
            VectorCharPointer datap(new vector<char>(data.begin(), data.end()));
            Message::Pointer mp(new Message(datap));
            writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::util::Hash& data, int prio) {
            VectorCharPointer datap(new std::vector<char>());
            prepareVectorFromHash(data, *datap);
            Message::Pointer mp(new Message(datap));
            this->writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const char* hdr, const size_t& hsize, const char* data, const size_t& dsize, int prio) {
            VectorCharPointer datap(new vector<char>(data, data + dsize));
            VectorCharPointer headerp(new vector<char>(hdr, hdr + hsize));
            Message::Pointer mp(new Message(datap, headerp));
            writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::util::Hash& header, const char* data, const size_t& dsize, int prio) {
            VectorCharPointer datap(new std::vector<char>(data, data + dsize));
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            Message::Pointer mp(new Message(datap, headerp));
            writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::util::Hash& header, const vector<char>& data, int prio) {
            VectorCharPointer datap(new std::vector<char>(data.begin(), data.end()));
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            Message::Pointer mp(new Message(datap, headerp));
            writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::util::Hash& header, const VectorCharPointer& datap, int prio) {
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            Message::Pointer mp(new Message(datap, headerp));
            writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::util::Hash& header, const std::string& data, int prio) {
            VectorCharPointer datap(new std::vector<char>(data.begin(), data.end()));
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            Message::Pointer mp(new Message(datap, headerp));
            writeAsync(mp, prio);
        }


        void TcpChannel::writeAsync(const karabo::util::Hash& header, const karabo::util::Hash& data, int prio) {
            VectorCharPointer headerp(new std::vector<char>());
            prepareVectorFromHash(header, *headerp);
            VectorCharPointer datap(new std::vector<char>());
            prepareVectorFromHash(data, *datap);
            Message::Pointer mp(new Message(datap, headerp));
            this->writeAsync(mp, prio);
        }
    }
}
