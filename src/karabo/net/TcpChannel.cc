#include <iostream>
#include <assert.h>
#include <vector>
#include <boost/asio.hpp>
#include <karabo/util/Hash.hh>
#include <boost/pointer_cast.hpp>
#include "Channel.hh"
#include "TcpChannel.hh"
#include "AsioIOService.hh"

namespace karabo {
    namespace net {

        using namespace std;
        using namespace boost::asio;
        using namespace karabo::util;


        TcpChannel::TcpChannel(const TcpConnection::Pointer& connection) : m_connectionPointer(connection),
        m_socket(*(connection->m_boostIoServicePointer)), m_timer(*(connection->m_boostIoServicePointer)), m_activeHandler(TcpChannel::NONE), m_readHeaderFirst(false) {
            if (m_connectionPointer->m_serializationType == "binary") {
                m_binarySerializer = karabo::io::BinarySerializer<Hash>::create("Bin");
            } else {
                m_textSerializer = karabo::io::TextSerializer<Hash>::create("Xml", Hash("indentation", -1));
            }
        }


        TcpChannel::~TcpChannel() {
        }

        #define _KARABO_VECTOR_TO_SIZE(x, v) {\
                size_t sizeofLength = m_connectionPointer->getSizeofLength();\
                assert(x.size() == sizeofLength);\
                if (m_connectionPointer->lengthIsText()) {\
                    try {\
                        v = boost::lexical_cast<size_t>(std::string(x.begin(), x.end()));\
                    } catch(const boost::bad_lexical_cast& e) {\
                        throw KARABO_CAST_EXCEPTION(std::string(e.what()) + " ( the source is '" \
                                + std::string(x.begin(), x.end()) + "', source_type=" + e.source_type().name()\
                                + " and target_type=" + e.target_type().name() + " )");\
                    }\
                } else if (sizeofLength == sizeof (uint8_t)) {\
                    v = *reinterpret_cast<uint8_t*> (&x[0]);\
                } else if (sizeofLength == sizeof (uint16_t)) {\
                    v = *reinterpret_cast<uint16_t*> (&x[0]);\
                } else if (sizeofLength == sizeof (uint64_t)) {\
                    v = *reinterpret_cast<uint64_t*> (&x[0]);\
                } else {\
                    v = *reinterpret_cast<uint32_t*> (&x[0]);\
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
                    x.assign(slen.begin(), slen.end());\
                } else {\
                    const char* p = reinterpret_cast<const char*> (&v);\
                    x.assign(p, p + sizeofLength);\
                }\
                }


        size_t TcpChannel::readSizeInBytes() {
            size_t sizeofLength = m_connectionPointer->getSizeofLength();
            if (sizeofLength > 0) {
                ErrorCode error; //in case of error
                m_inboundMessagePrefix.resize(sizeofLength);
                boost::asio::read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(), error);
                if (error) throw KARABO_NETWORK_EXCEPTION(error.message());
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
            boost::asio::read(m_socket, buffer(data, size), transfer_all(), error);
            if (error) {
                throw KARABO_NETWORK_EXCEPTION(error.message());
            }
        }


        void TcpChannel::read(std::vector<char>& data) {
            size_t nBytes = this->readSizeInBytes();
            data.resize(nBytes);
            this->read(&data[0], nBytes);
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
            this->read(data, size);
        }


        void TcpChannel::read(karabo::util::Hash& header, std::vector<char>& data) {
            this->read(header);
            this->read(data);
        }


        void TcpChannel::read(karabo::util::Hash& header, karabo::util::Hash& data) {
            this->read(header);
            this->read(data);
        }


        void TcpChannel::readAsyncSizeInBytes(const ReadSizeInBytesHandler& handler) {
            size_t sizeofLength = m_connectionPointer->getSizeofLength();
            if (sizeofLength == 0) throw KARABO_LOGIC_EXCEPTION("Message's sizeTag size was configured to be 0. Thus, registration of this function does not make sense!");
            m_inboundMessagePrefix.resize(sizeofLength);
            boost::asio::async_read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(),
                                    boost::bind(&karabo::net::TcpChannel::onSizeInBytesAvailable, this, handler, boost::asio::placeholders::bytes_transferred, boost::asio::placeholders::error));
        }


        void TcpChannel::onSizeInBytesAvailable(const ReadSizeInBytesHandler& handler, const size_t bytesTransferred, const ErrorCode& error) {
            if (error) {
                if (m_errorHandler) m_errorHandler(shared_from_this(), error);
                else throw KARABO_NETWORK_EXCEPTION(error.message());
            } else {
                size_t messageSize;
                _KARABO_VECTOR_TO_SIZE(m_inboundMessagePrefix, messageSize);
                handler(shared_from_this(), messageSize);
            }
        }


        void TcpChannel::byteSizeAvailableHandler(Channel::Pointer channel, const size_t byteSize) {
            m_inboundData.resize(byteSize);
            this->readAsyncRaw(&m_inboundData[0], byteSize, boost::bind(&karabo::net::TcpChannel::bytesAvailableHandler, this, _1));
        }


        void TcpChannel::readAsyncRaw(char* data, const size_t& size, const ReadRawHandler& handler) {
            boost::asio::async_read(m_socket, buffer(data, size), transfer_all(),
                                    boost::bind(&karabo::net::TcpChannel::onBytesAvailable, this, handler, boost::asio::placeholders::error));
        }


        void TcpChannel::onBytesAvailable(const ReadRawHandler& handler, const ErrorCode& error) {
            if (error) {
                if (m_errorHandler) m_errorHandler(shared_from_this(), error);
                else throw KARABO_NETWORK_EXCEPTION(error.message());
            } else {
                handler(shared_from_this());
            }
        }


        void TcpChannel::readAsyncVector(const ReadVectorHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::VECTOR;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1, _2));
        }


        void TcpChannel::readAsyncString(const ReadStringHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::STRING;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1, _2));
        }


        void TcpChannel::readAsyncHash(const ReadHashHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1, _2));
        }


        void TcpChannel::readAsyncHashVector(const ReadHashVectorHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_VECTOR;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1, _2));
        }


        void TcpChannel::readAsyncHashString(const ReadHashStringHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_STRING;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1, _2));
        }


        void TcpChannel::readAsyncHashHash(const ReadHashHashHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_HASH;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1, _2));
        }


        void TcpChannel::bytesAvailableHandler(Channel::Pointer channel) {

            // Only parse header information
            if (m_readHeaderFirst) {
                m_readHeaderFirst = false;
                m_inboundData.swap(m_inboundHeader);
                this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, _1, _2));
                return;
            }

            HandlerType type = m_activeHandler;
            m_activeHandler = TcpChannel::NONE;
            switch (type) {
                case VECTOR:
                {
                    boost::any_cast<ReadVectorHandler>(m_readHandler) (shared_from_this(), m_inboundData);
                    break;
                }
                case STRING:
                {
                    string tmp(m_inboundData.begin(), m_inboundData.end());
                    boost::any_cast<ReadStringHandler>(m_readHandler) (shared_from_this(), tmp);
                    break;
                }
                case HASH:
                {
                    Hash h;
                    this->prepareHashFromData(h);
                    boost::any_cast<ReadHashHandler>(m_readHandler) (shared_from_this(), h);
                    break;
                }
                case HASH_VECTOR:
                {
                    Hash header;
                    this->prepareHashFromHeader(header);
                    boost::any_cast<ReadHashVectorHandler>(m_readHandler) (shared_from_this(), header, m_inboundData);
                    break;
                }

                case HASH_STRING:
                {
                    Hash header;
                    this->prepareHashFromHeader(header);
                    string tmp(m_inboundData.begin(), m_inboundData.end());
                    boost::any_cast<ReadHashStringHandler>(m_readHandler) (shared_from_this(), header, tmp);
                    break;
                }

                case HASH_HASH:
                {
                    Hash header;
                    this->prepareHashFromHeader(header);
                    Hash body;
                    this->prepareHashFromData(body);
                    boost::any_cast<ReadHashHashHandler>(m_readHandler) (shared_from_this(), header, body);
                    break;
                }
                default:
                    throw KARABO_LOGIC_EXCEPTION("Bad internal error regarding asynchronous read handlers, contact BH or SE");
            }
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

                boost::asio::write(m_socket, buf, transfer_all(), error);

                if (!error) return;
                else if (m_errorHandler) m_errorHandler(shared_from_this(), error);
                else throw KARABO_NETWORK_EXCEPTION(error.message());

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
                    m_textSerializer->save(header, headerBuf);
                    m_textSerializer->save(body, bodyBuf);
                    this->write(headerBuf.c_str(), headerBuf.size(), bodyBuf.c_str(), bodyBuf.size());
                } else {
                    std::vector<char> headerBuf;
                    std::vector<char> bodyBuf;
                    m_binarySerializer->save(header, headerBuf);
                    m_binarySerializer->save(body, bodyBuf);
                    this->write(&headerBuf[0], headerBuf.size(), &bodyBuf[0], bodyBuf.size());
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
                    m_textSerializer->save(header, headerBuf);
                    write(headerBuf.c_str(), headerBuf.size(), data, size);
                } else {
                    std::vector<char> headerBuf;
                    m_binarySerializer->save(header, headerBuf);
                    write(&headerBuf[0], headerBuf.size(), data, size);
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
                boost::asio::write(m_socket, buf, transfer_all(), error);
                if (error) {
                    if (m_errorHandler) m_errorHandler(shared_from_this(), error);
                    else throw KARABO_NETWORK_EXCEPTION(error.message());
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
                    size_t s = m_outboundData.size();
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, s);
                    buf.push_back(buffer(m_outboundMessagePrefix));
                }
                buf.push_back(buffer(m_outboundData));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::managedWriteAsyncWithHeader(const WriteCompleteHandler& handler) {
            try {
                size_t hsize = m_outboundHeader.size(); // Header size
                size_t dsize = m_outboundData.size(); // Data size
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, hsize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, dsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(m_outboundHeader));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(m_outboundData));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
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
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::unmanagedWriteAsyncWithHeader(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
            try {
                size_t hsize = m_outboundHeader.size(); // Header size
                size_t dsize = m_outboundData.size(); // Data size
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, hsize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, dsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(m_outboundHeader));
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(data, size));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::prepareHeaderFromHash(const karabo::util::Hash& hash) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                m_outboundHeader.assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, m_outboundHeader);
            }
        }


        void TcpChannel::prepareHashFromHeader(karabo::util::Hash& hash) const {
            if (m_textSerializer) {
                m_textSerializer->load(hash, &m_inboundHeader[0]);
            } else {
                m_binarySerializer->load(hash, m_inboundHeader);
            }
        }


        void TcpChannel::prepareDataFromHash(const karabo::util::Hash& hash) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                m_outboundData.assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, m_outboundData);
            }
        }


        void TcpChannel::prepareHashFromData(karabo::util::Hash& hash) const {
            if (m_textSerializer) {
                m_textSerializer->load(hash, &m_inboundData[0]);
            } else {
                m_binarySerializer->load(hash, m_inboundData);
            }
        }


        void TcpChannel::writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
            try {
                if (m_connectionPointer->m_manageAsyncData) {
                    m_outboundData.resize(size);
                    std::memcpy(&m_outboundData[0], data, size);
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


        void TcpChannel::writeAsyncHash(const Hash& data, const Channel::WriteCompleteHandler& handler) {
            try {
                this->prepareDataFromHash(data);
                this->managedWriteAsync(handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashRaw(const karabo::util::Hash& header, const char* data, const size_t& size, const WriteCompleteHandler& handler) {
            try {
                this->prepareHeaderFromHash(header);
                if (m_connectionPointer->m_manageAsyncData) {
                    m_outboundData.resize(size);
                    std::memcpy(&m_outboundData[0], data, size);
                    this->managedWriteAsyncWithHeader(handler);
                } else {
                    this->unmanagedWriteAsyncWithHeader(data, size, handler);
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashVector(const Hash& header, const std::vector<char>& data, const Channel::WriteCompleteHandler& handler) {
            try {
                this->writeAsyncHashRaw(header, &data[0], data.size(), handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashHash(const karabo::util::Hash& header, const karabo::util::Hash& data, const WriteCompleteHandler& handler) {
            try {
                this->prepareHeaderFromHash(header);
                this->prepareDataFromHash(data);
                this->managedWriteAsyncWithHeader(handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e) {
            try {
                if (!e) {
                    try {
                        handler(shared_from_this());
                    } catch (...) {
                        KARABO_RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(shared_from_this(), e);
                } else {
                    throw KARABO_NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::waitAsync(int millisecs, const WaitHandler& handler) {
            try {
                m_timer.expires_from_now(boost::posix_time::milliseconds(millisecs));
                m_timer.async_wait(boost::bind(&TcpChannel::asyncWaitHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::asyncWaitHandler(const Channel::WaitHandler& handler, const ErrorCode& e) {
            try {
                if (!e) {
                    try {
                        handler(shared_from_this());
                    } catch (...) {
                        KARABO_RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(shared_from_this(), e);
                } else {
                    throw KARABO_NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::close() {
            try {
                m_socket.close();
                m_connectionPointer->unregisterChannel(shared_from_this());
            } catch (...) {
                KARABO_RETHROW
            }
        }


        bool TcpChannel::isOpen() {
            return m_socket.is_open();
        }


        #undef _KARABO_VECTOR_TO_SIZE
        #undef _KARABO_SIZE_TO_VECTOR

    }
}
