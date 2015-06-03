#include <iostream>
#include <assert.h>
#include <vector>
#include <boost/asio.hpp>
#include <karabo/util/Hash.hh>
#include <boost/pointer_cast.hpp>
#include <snappy.h>
#include "Channel.hh"
#include "TcpChannel.hh"
#include "AsioIOService.hh"

namespace karabo {
    namespace net {

        using namespace std;
        using namespace boost::asio;
        using namespace karabo::util;


        TcpChannel::TcpChannel(Connection::Pointer connection)
        : m_connectionPointer(boost::dynamic_pointer_cast<TcpConnection>(connection))
        , m_socket(*(m_connectionPointer->m_boostIoServicePointer))
        , m_timer(*(m_connectionPointer->m_boostIoServicePointer))
        , m_activeHandler(TcpChannel::NONE)
        , m_readHeaderFirst(false)
        , m_inboundData(new std::vector<char>())
        , m_inboundHeader(new std::vector<char>())
        , m_outboundData(new std::vector<char>())
        , m_outboundHeader(new std::vector<char>()) {
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
            size_t sizeofLength = m_connectionPointer->getSizeofLength();
            if (sizeofLength == 0) throw KARABO_LOGIC_EXCEPTION("Message's sizeTag size was configured to be 0. Thus, registration of this function does not make sense!");
            m_inboundMessagePrefix.resize(sizeofLength);
            boost::asio::async_read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(),
                                    boost::bind(&karabo::net::TcpChannel::onSizeInBytesAvailable,
                                                shared_from_this(), handler, boost::asio::placeholders::bytes_transferred,
                                                boost::asio::placeholders::error));
        }


        void TcpChannel::onSizeInBytesAvailable(const ReadSizeInBytesHandler& handler, const size_t bytesTransferred, const ErrorCode& error) {
            if (error) {
                if (m_errorHandler) m_errorHandler(error);
                else throw KARABO_NETWORK_EXCEPTION(error.message());
            } else {
                size_t messageSize;
                _KARABO_VECTOR_TO_SIZE(m_inboundMessagePrefix, messageSize);
                handler(messageSize);
            }
        }


        void TcpChannel::byteSizeAvailableHandler(const Channel::Pointer& channel, const size_t byteSize) {
            m_inboundData->resize(byteSize);
            this->readAsyncRaw(&(*m_inboundData)[0], byteSize, boost::bind(&karabo::net::TcpChannel::bytesAvailableHandler, this, channel));
        }


        void TcpChannel::readAsyncRaw(char* data, const size_t& size, const ReadRawHandler& handler) {
            boost::asio::async_read(m_socket, buffer(data, size), transfer_all(),
                                    boost::bind(&karabo::net::TcpChannel::onBytesAvailable, shared_from_this(), handler,
                                                boost::asio::placeholders::error));
        }


        void TcpChannel::onBytesAvailable(const ReadRawHandler& handler, const ErrorCode& error) {
            if (error) {
                if (m_errorHandler) m_errorHandler(error);
                else throw KARABO_NETWORK_EXCEPTION(error.message());
            } else {
                handler();
            }
        }


        void TcpChannel::readAsyncVector(const ReadVectorHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::VECTOR;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, shared_from_this(), _1));
        }


        void TcpChannel::readAsyncVectorPointer(const ReadVectorPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::VECTOR_POINTER;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, shared_from_this(), _1));
        }


        void TcpChannel::readAsyncString(const ReadStringHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::STRING;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, shared_from_this(), _1));
        }


        void TcpChannel::readAsyncHash(const ReadHashHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, shared_from_this(), _1));
        }


        void TcpChannel::readAsyncHashVector(const ReadHashVectorHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_VECTOR;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, shared_from_this(), _1));
        }


        void TcpChannel::readAsyncHashVectorPointer(const ReadHashVectorPointerHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_VECTOR_POINTER;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, shared_from_this(), _1));
        }


        void TcpChannel::readAsyncHashString(const ReadHashStringHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_STRING;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, shared_from_this(), _1));
        }


        void TcpChannel::readAsyncHashHash(const ReadHashHashHandler& handler) {
            if (m_activeHandler != TcpChannel::NONE) throw KARABO_NETWORK_EXCEPTION("Multiple async read: You are allowed to register only exactly one asynchronous read or write per channel.");
            m_activeHandler = TcpChannel::HASH_HASH;
            m_readHeaderFirst = true;
            m_readHandler = handler;
            this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, shared_from_this(), _1));
        }


        void TcpChannel::bytesAvailableHandler(const Channel::Pointer& channel) {

            // Only parse header information
            if (m_readHeaderFirst) {
                m_readHeaderFirst = false;
                m_inboundData.swap(m_inboundHeader);
                this->readAsyncSizeInBytes(boost::bind(&karabo::net::TcpChannel::byteSizeAvailableHandler, this, channel, _1));
                return;
            }

            HandlerType type = m_activeHandler;
            m_activeHandler = TcpChannel::NONE;
            switch (type) {
                case VECTOR:
                {
                    boost::any_cast<ReadVectorHandler>(m_readHandler) (*m_inboundData);
                    break;
                }
                case VECTOR_POINTER:
                {
                    boost::shared_ptr<std::vector<char> > vec(new std::vector<char>());
                    vec.swap(m_inboundData);
                    boost::any_cast<ReadVectorPointerHandler>(m_readHandler) (vec);
                    break;
                }
                case STRING:
                {
                    string tmp(m_inboundData->begin(), m_inboundData->end());
                    boost::any_cast<ReadStringHandler>(m_readHandler) (tmp);
                    break;
                }
                case HASH:
                {
                    Hash h;
                    this->prepareHashFromData(h);
                    boost::any_cast<ReadHashHandler>(m_readHandler) (h);
                    break;
                }

                case HASH_VECTOR:
                {
                    Hash header;
                    this->prepareHashFromHeader(header);
                    if (header.has("__compression__")) {
                        std::vector<char> inData;
                        decompress(header, *m_inboundData, inData);
                        boost::any_cast<ReadHashVectorHandler>(m_readHandler) (header, inData);
                    } else
                        boost::any_cast<ReadHashVectorHandler>(m_readHandler) (header, *m_inboundData);
                    break;
                }
                case HASH_VECTOR_POINTER:
                {
                    Hash header;
                    this->prepareHashFromHeader(header);
                    if (header.has("__compression__")) {
                        boost::shared_ptr<std::vector<char> > inData(new std::vector<char>());
                        decompress(header, *m_inboundData, *inData);
                        boost::any_cast<ReadHashVectorPointerHandler>(m_readHandler) (header, inData);
                    } else {
                        boost::shared_ptr<std::vector<char> > vec(new std::vector<char>());
                        vec.swap(m_inboundData);
                        boost::any_cast<ReadHashVectorPointerHandler>(m_readHandler) (header, vec);
                    }
                    break;
                }

                case HASH_STRING:
                {
                    Hash header;
                    this->prepareHashFromHeader(header);
                    string tmp;
                    if (header.has("__compression__"))
                        decompress(header, *m_inboundData, tmp);
                    else
                        tmp.assign(m_inboundData->begin(), m_inboundData->end());
                    boost::any_cast<ReadHashStringHandler>(m_readHandler) (header, tmp);
                    break;
                }

                case HASH_HASH:
                {
                    Hash header;
                    this->prepareHashFromHeader(header);
                    if (header.has("__compression__")) {
                        boost::shared_ptr<std::vector<char> > tmp(new std::vector<char>());
                        tmp.swap(m_inboundData);
                        decompress(header, *tmp, *m_inboundData);
                    }
                    Hash body;
                    this->prepareHashFromData(body);
                    boost::any_cast<ReadHashHashHandler>(m_readHandler) (header, body);
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

                boost::asio::write(m_socket, buf, transfer_all(), error);

                if (!error) return;
                else if (m_errorHandler) m_errorHandler(error);
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
                boost::asio::write(m_socket, buf, transfer_all(), error);
                if (error) {
                    if (m_errorHandler) m_errorHandler(error);
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
                    size_t s = m_outboundData->size();
                    _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, s);
                    buf.push_back(buffer(m_outboundMessagePrefix));
                }
                buf.push_back(buffer(*m_outboundData));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
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
                size_t hsize = m_outboundHeader->size(); // Header size
                size_t dsize = m_outboundData->size(); // Data size
                _KARABO_SIZE_TO_VECTOR(m_outboundHeaderPrefix, hsize);
                _KARABO_SIZE_TO_VECTOR(m_outboundMessagePrefix, dsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(*m_outboundHeader));
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
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler,
                                                                    this, handler, dataPtr,
                                                                    boost::asio::placeholders::error));
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
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler,
                                                                    this, handler, header, body,
                                                                    boost::asio::placeholders::error));
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


        void TcpChannel::asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e) {
            try {
                if (!e) {
                    try {
                        handler();
                    } catch (...) {
                        KARABO_RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(e);
                } else {
                    throw KARABO_NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::asyncWriteHandler(const Channel::WriteCompleteHandler& handler,
                                           const boost::shared_ptr<std::vector<char> >& body,
                                           const ErrorCode& e) {
            try {
                if (!e) {
                    try {
                        handler();
                    } catch (...) {
                        KARABO_RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(e);
                } else {
                    throw KARABO_NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }


        void TcpChannel::asyncWriteHandler(const Channel::WriteCompleteHandler& handler,
                                           const boost::shared_ptr<std::vector<char> >& header,
                                           const boost::shared_ptr<std::vector<char> >& body,
                                           const ErrorCode& e) {
            try {
                if (!e) {
                    try {
                        handler();
                    } catch (...) {
                        KARABO_RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(e);
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
                        handler();
                    } catch (...) {
                        KARABO_RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(e);
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
