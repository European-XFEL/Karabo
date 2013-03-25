#include <iostream>
#include <assert.h>
#include <vector>
#include <boost/asio.hpp>
#include <karabo/util/Hash.hh>
#include "Channel.hh"
#include "TcpChannel.hh"
#include "AsioIOService.hh"

namespace karabo {
    namespace net {

        using namespace std;
        using namespace boost::asio;
        using namespace karabo::util;


        TcpChannel::TcpChannel(const TcpConnection::Pointer& connection) : m_connectionPointer(connection),
        m_socket(*(connection->m_boostIoServicePointer)), m_timer(*(connection->m_boostIoServicePointer)) {
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


        #define _KARABO_VECTOR_TO_SIZE_2(x, v, y, w) {\
                size_t sizeofLength = m_connectionPointer->getSizeofLength();\
                assert(x.size() == sizeofLength);\
                assert(y.size() == sizeofLength);\
                if (m_connectionPointer->lengthIsText()) {\
                    try {\
                        v = boost::lexical_cast<size_t>(std::string(x.begin(), x.end()));\
                    } catch(const boost::bad_lexical_cast& e) {\
                        throw KARABO_CAST_EXCEPTION(std::string(e.what()) + " ( the source is '" \
                                + std::string(x.begin(), x.end()) + "', source_type=" + e.source_type().name()\
                                + " and target_type=" + e.target_type().name() + " )");\
                    }\
                    try {\
                        w = boost::lexical_cast<size_t>(std::string(y.begin(), y.end()));\
                    } catch(const boost::bad_lexical_cast& e) {\
                        throw KARABO_CAST_EXCEPTION(std::string(e.what()) + " ( the source is '" \
                                + std::string(y.begin(), y.end()) + "', source_type=" + e.source_type().name()\
                                + " and target_type=" + e.target_type().name() + " )");\
                    }\
                } else if (sizeofLength == sizeof (uint8_t)) {\
                    w = *reinterpret_cast<uint8_t*> (&y[0]);\
                    v = *reinterpret_cast<uint8_t*> (&x[0]);\
                } else if (sizeofLength == sizeof (uint16_t)) {\
                    w = *reinterpret_cast<uint16_t*> (&y[0]);\
                    v = *reinterpret_cast<uint16_t*> (&x[0]);\
                } else if (sizeofLength == sizeof (uint64_t)) {\
                    w = *reinterpret_cast<uint64_t*> (&y[0]);\
                    v = *reinterpret_cast<uint64_t*> (&x[0]);\
                } else {\
                    w = *reinterpret_cast<uint32_t*> (&y[0]);\
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


        #define _KARABO_SIZE_TO_VECTOR_2(messagePrefix, msgSize, headerPrefix, headerSize) {\
                size_t sizeofLength = m_connectionPointer->getSizeofLength();\
                bool lengthIsText = m_connectionPointer->lengthIsText();\
                if (lengthIsText) {\
                    ostringstream oss;\
                    oss.fill('0');\
                    oss.width(sizeofLength);\
                    oss << msgSize;\
                    string sfull = oss.str();\
                    messagePrefix.assign(sfull.begin(), sfull.end());\
                    oss.str("");\
                    oss << headerSize;\
                    string shead = oss.str();\
                    headerPrefix.assign(shead.begin(), shead.end());\
                } else {\
                    const char* p = reinterpret_cast<const char*> (&msgSize);\
                    m_outboundMessagePrefix.assign(p, p + sizeofLength);\
                    p = reinterpret_cast<const char*> (&headerSize);\
                    m_outboundHeaderPrefix.assign(p, p + sizeofLength);\
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


        void TcpChannel::readAsyncSizeInBytes(const ReadSizeInBytesCompleted& handler) {
            size_t sizeofLength = m_connectionPointer->getSizeofLength();
            if (sizeofLength == 0) throw KARABO_LOGIC_EXCEPTION("Message's sizeTag size was configured to be 0. Thus, registration of this function does not make sense!");
            m_inboundMessagePrefix.resize(sizeofLength);
            boost::asio::async_read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(),
                                    boost::bind(&karabo::net::TcpChannel::onSizeInBytesAvailable, this, handler, boost::asio::placeholders::bytes_transferred, boost::asio::placeholders::error));
        }


        void TcpChannel::onSizeInBytesAvailable(const ReadSizeInBytesCompleted& handler, const size_t byteSize, const ErrorCode& error) {
            if (error) throw KARABO_NETWORK_EXCEPTION(error.message());
            handler(shared_from_this(), byteSize);
        }


        void TcpChannel::byteSizeAvailableHandler(Channel::Pointer channel, const size_t byteSize) {
            m_inboundData.resize(byteSize);
            this->readAsyncRaw(&m_inboundData[0], byteSize, boost::bind(&karabo::net::TcpChannel::bytesAvailableHandler, this, _1));
        }


        void TcpChannel::readAsyncRaw(char* data, const size_t& size, const ReadRawCompleted& handler) {
            boost::asio::async_read(m_socket, buffer(data, size), transfer_all(),
                                    boost::bind(&karabo::net::TcpChannel::onBytesAvailable, this, handler, boost::asio::placeholders::error));
        }


        void TcpChannel::onBytesAvailable(const ReadRawCompleted& handler, const ErrorCode& error) {
            if (error) throw KARABO_NETWORK_EXCEPTION(error.message());
            handler(shared_from_this());
        }


        void TcpChannel::bytesAvailableHandler(Channel::Pointer channel) {
            switch (m_activeHandler) {
                case RAW:
                    return m_rawHandler(shared_from_this(), &m_inboundData[0], m_inboundData.size());
                    break;
                case VECTOR:
                    return m_vectorHandler(shared_from_this(), m_inboundData);
                    break;
                case STRING:
                {
                    string tmp(m_inboundData.begin(), m_inboundData.end());
                    m_stringHandler(shared_from_this(), tmp);
                    break;
                }
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
                else if (m_errorHandler) m_errorHandler(shared_from_this(), error.message());
                else throw KARABO_NETWORK_EXCEPTION(error.message());

            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const char* data, const size_t& size, const Hash& header) {
            try {
                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const size_t& size) instead.");
                }
                if (m_textSerializer) {
                    string archive;
                    m_textSerializer->save(header, archive);
                    write(data, size, archive.c_str(), archive.size());
                } else {
                    std::vector<char> archive;
                    m_binarySerializer->save(header, archive);
                    write(data, size, &archive[0], archive.size());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::write(const char* data, const size_t& dsize, const char* header, const string::size_type& hsize) {
            try {
                boost::system::error_code error; //in case of error

                size_t sizeofLength = m_connectionPointer->getSizeofLength();
                if (sizeofLength == 0) {
                    throw KARABO_PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const size_t& size) instead.");
                }
                size_t fsize = hsize + dsize;
                _KARABO_SIZE_TO_VECTOR_2(m_outboundMessagePrefix, fsize, m_outboundHeaderPrefix, hsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(header, hsize));
                buf.push_back(buffer(data, dsize));
                boost::asio::write(m_socket, buf, transfer_all(), error);
                if (!error)
                    return;
                else if (m_errorHandler)
                    m_errorHandler(shared_from_this(), error.message());
                else
                    throw KARABO_NETWORK_EXCEPTION(error.message());

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
                size_t fsize = hsize + m_outboundData.size(); // Full size
                _KARABO_SIZE_TO_VECTOR_2(m_outboundMessagePrefix, fsize, m_outboundHeaderPrefix, hsize); //m_outboundMessagePrefix=fsize; m_outboundHeaderPrefix=hsize;
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(m_outboundHeader));
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
                size_t fsize = hsize + size; // Full size
                _KARABO_SIZE_TO_VECTOR_2(m_outboundMessagePrefix, fsize, m_outboundHeaderPrefix, hsize); //m_outboundMessagePrefix=fsize; m_outboundHeaderPrefix=hsize;
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(m_outboundHeader));
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


        void TcpChannel::prepareDataFromHash(const karabo::util::Hash& hash) {
            if (m_textSerializer) {
                string archive;
                m_textSerializer->save(hash, archive);
                m_outboundData.assign(archive.begin(), archive.end());
            } else {
                m_binarySerializer->save(hash, m_outboundData);
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


        void TcpChannel::writeAsyncRawHash(const char* data, const size_t& size, const karabo::util::Hash& header, const WriteCompleteHandler& handler) {
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


        void TcpChannel::writeAsyncVectorHash(const std::vector<char>& data, const Hash& header, const Channel::WriteCompleteHandler& handler) {
            try {
                this->writeAsyncRawHash(&data[0], data.size(), header, handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::writeAsyncHashHash(const karabo::util::Hash& data, const karabo::util::Hash& header, const WriteCompleteHandler& handler) {
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
                    m_errorHandler(shared_from_this(), e.message());
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
                    m_errorHandler(shared_from_this(), e.message());
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
        #undef _KARABO_VECTOR_TO_SIZE_2
        #undef _KARABO_SIZE_TO_VECTOR
        #undef _KARABO_SIZE_TO_VECTOR_2


    }
}
