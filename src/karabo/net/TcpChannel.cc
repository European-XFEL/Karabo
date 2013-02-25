#include <iostream>
#include <assert.h>
#include <vector>
#include <boost/asio.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/io/Reader.hh>
#include <karabo/io/Writer.hh>
#include "Channel.hh"
#include "TcpChannel.hh"
#include "AsioIOService.hh"

namespace karabo {
    namespace net {

        using namespace std;
        using namespace boost::asio;
        using namespace karabo::util;


        TcpChannel::TcpChannel(TcpConnection& c) :
        Channel(c),
        m_socket(*(c.m_boost_io_service)),
        m_timer(*(c.m_boost_io_service)),
        m_inboundMessagePrefix(0),
        m_inboundHeaderPrefix(0) {
        }


        TcpChannel::~TcpChannel() {
        }

#define VECTOR_TO_SIZE(x, v) {\
                size_t sizeofLength = getConnection()->getSizeofLength();\
                assert(x.size() == sizeofLength);\
                if (getConnection()->lengthIsText()) {\
                    try {\
                        v = boost::lexical_cast<size_t>(std::string(x.begin(), x.end()));\
                    } catch(const boost::bad_lexical_cast& e) {\
                        throw CAST_EXCEPTION(std::string(e.what()) + " ( the source is '" \
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


#define VECTOR_TO_SIZE_2(x, v, y, w) {\
                size_t sizeofLength = getConnection()->getSizeofLength();\
                assert(x.size() == sizeofLength);\
                assert(y.size() == sizeofLength);\
                if (getConnection()->lengthIsText()) {\
                    try {\
                        v = boost::lexical_cast<size_t>(std::string(x.begin(), x.end()));\
                    } catch(const boost::bad_lexical_cast& e) {\
                        throw CAST_EXCEPTION(std::string(e.what()) + " ( the source is '" \
                                + std::string(x.begin(), x.end()) + "', source_type=" + e.source_type().name()\
                                + " and target_type=" + e.target_type().name() + " )");\
                    }\
                    try {\
                        w = boost::lexical_cast<size_t>(std::string(y.begin(), y.end()));\
                    } catch(const boost::bad_lexical_cast& e) {\
                        throw CAST_EXCEPTION(std::string(e.what()) + " ( the source is '" \
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


#define SIZE_TO_VECTOR(x, v) {\
                size_t sizeofLength = getConnection()->getSizeofLength();\
                bool lengthIsText = getConnection()->lengthIsText();\
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


#define SIZE_TO_VECTOR_2(messagePrefix, msgSize, headerPrefix, headerSize) {\
                size_t sizeofLength = getConnection()->getSizeofLength();\
                bool lengthIsText = getConnection()->lengthIsText();\
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


        void TcpChannel::read(char*& data, size_t& size) {
            try {
                ErrorCode error; //in case of error
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength > 0) {
                    m_inboundMessagePrefix.resize(sizeofLength);
                    boost::asio::read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(), error);
                    if (error)
                        throw boost::system::system_error(error);

                    // prefix[0] - message length (body)
                    size = 0;
                    VECTOR_TO_SIZE(m_inboundMessagePrefix, size); // size = m_inboundMessagePrefix;
                }
                if (size > 0) {
                    data = new char[ size ];
                    vector<mutable_buffer> buf;
                    buf.push_back(buffer(data, size));

                    boost::asio::read(m_socket, buf, transfer_all(), error);
                    if (error)
                        throw boost::system::system_error(error);
                }
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::read(char*& data, size_t& size, Hash& header) {
            char* raw = 0;
            size_t rsize = 0;
            read(data, size, raw, rsize);
            if (rsize > 0) {
                string s(raw, raw + rsize);
                stringToHash(s, header);
            }
        }


        void TcpChannel::read(char*& data, size_t& dsize, char*& hdr, size_t& hsize) {
            try {
                ErrorCode error; //in case of error
                data = 0;
                hdr = 0;
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength == 0) {
                    throw PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use read(char*& data, size_t& size) instead.");
                }
                m_inboundMessagePrefix.resize(sizeofLength, 0);
                m_inboundHeaderPrefix.resize(sizeofLength, 0);

                vector<mutable_buffer> bufhdr;
                bufhdr.push_back(buffer(m_inboundMessagePrefix));
                bufhdr.push_back(buffer(m_inboundHeaderPrefix));
                boost::asio::read(m_socket, bufhdr, transfer_all(), error);
                if (error)
                    throw boost::system::system_error(error);

                // prefix[0] - message length (header + body)
                // prefix[1] - header length
                size_t fullsize;
                VECTOR_TO_SIZE_2(m_inboundMessagePrefix, fullsize, m_inboundHeaderPrefix, hsize);
                dsize = fullsize - hsize;

                vector<mutable_buffer> buf;
                if (hsize) {
                    hdr = new char[ hsize ];
                    buf.push_back(buffer(hdr, hsize));
                }
                if (dsize) {
                    data = new char[ dsize ];
                    buf.push_back(buffer(data, dsize));
                }

                boost::asio::read(m_socket, buf, transfer_all(), error);
                if (error)
                    throw boost::system::system_error(error);
            } catch (...) {
                RETHROW
            }
        }

        // asynchronous read


        void TcpChannel::readAsyncVector(const Channel::ReadVectorHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength == 0) {
                    throw PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use readAsyncRaw(...) instead.");
                }
                m_inboundMessagePrefix.resize(sizeofLength);
                boost::asio::async_read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(),
                        boost::bind(&TcpChannel::readVectorPrefixHandler, this, handler, boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                        ));
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readVectorPrefixHandler(const Channel::ReadVectorHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    try {
                        size_t size;
                        VECTOR_TO_SIZE(m_inboundMessagePrefix, size);
                        m_inboundData.resize(size);
                    } catch (const bad_alloc& e) {
                        string prefix(m_inboundMessagePrefix.begin(), m_inboundMessagePrefix.end());
                        throw PARAMETER_EXCEPTION("Fail to resize vector with size = " + prefix + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundData));
                    boost::asio::async_read(m_socket, buf, transfer_all(),
                            boost::bind(&TcpChannel::asyncReadVectorHandler, this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::asyncReadVectorHandler(const Channel::ReadVectorHandler& handler, const boost::system::error_code& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    handler(channel(), m_inboundData);
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readAsyncString(const Channel::ReadStringHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength == 0) {
                    throw PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use readAsyncRaw(...) instead.");
                }
                m_inboundMessagePrefix.resize(sizeofLength);
                boost::asio::async_read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(),
                        boost::bind(&TcpChannel::readStringPrefixHandler, this, handler, boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                        ));
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readStringPrefixHandler(const Channel::ReadStringHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    try {
                        size_t size;
                        VECTOR_TO_SIZE(m_inboundMessagePrefix, size);
                        m_inboundData.resize(size);
                    } catch (const bad_alloc& e) {
                        string prefix(m_inboundMessagePrefix.begin(), m_inboundMessagePrefix.end());
                        throw PARAMETER_EXCEPTION("Fail to resize vector with size = " + prefix + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundData));
                    boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::asyncReadStringHandler,
                            this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::asyncReadStringHandler(const Channel::ReadStringHandler& handler, const boost::system::error_code& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    string s(m_inboundData.begin(), m_inboundData.end());
                    handler(channel(), s);
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readAsyncHash(const Channel::ReadHashHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength == 0) {
                    throw PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use readAsyncRaw(...) instead.");
                }
                m_inboundMessagePrefix.resize(sizeofLength);
                boost::asio::async_read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(),
                        boost::bind(&TcpChannel::readHashPrefixHandler, this, handler, boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                        ));
            } catch (...) {
                RETHROW
            }

        }


        void TcpChannel::readHashPrefixHandler(const Channel::ReadHashHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    try {
                        size_t size;
                        VECTOR_TO_SIZE(m_inboundMessagePrefix, size);
                        m_inboundData.resize(size);
                    } catch (const bad_alloc& e) {
                        string prefix(m_inboundMessagePrefix.begin(), m_inboundMessagePrefix.end());
                        throw PARAMETER_EXCEPTION("Fail to resize vector with size = " + prefix + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundData));
                    boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::asyncReadHashHandler,
                            this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::asyncReadHashHandler(const Channel::ReadHashHandler& handler, const boost::system::error_code& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    Hash hash;
                    if (m_inboundData.size() > 0) {
                        using namespace karabo::io;
                        string s(m_inboundData.begin(), m_inboundData.end());
                        stringToHash(s, hash);
                    }
                    handler(channel(), hash);
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readAsyncRaw(char*& data, size_t& size, const ReadRawHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength > 0) {
                    // protocol should contain the message length at the beginning of message with size = sizeofLength 
                    m_inboundMessagePrefix.resize(sizeofLength);
                    boost::asio::async_read(m_socket, buffer(m_inboundMessagePrefix), transfer_all(), boost::bind(
                            &TcpChannel::readRawPrefixHandler, this, data, size, handler, boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
                    return;
                }
                if (size > 0) {
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(data, size));
                    boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::asyncReadRawHandler,
                            this, data, size, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                }
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readRawPrefixHandler(char*& data, size_t& size, const ReadRawHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    size_t len;
                    VECTOR_TO_SIZE(m_inboundMessagePrefix, len);
                    if (size < len) {
                        string slen(m_inboundMessagePrefix.begin(), m_inboundMessagePrefix.end());
                        string ssize = String::toString(size);
                        throw PARAMETER_EXCEPTION("Size of read buffer is less than data size: " + ssize + " < " + slen);
                    }
                    vector<mutable_buffer > buf;
                    if (len > 0) {
                        buf.push_back(buffer(data, len));
                        boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::asyncReadRawHandler,
                                this, data, size, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                    }
                }
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::asyncReadRawHandler(char*& data, size_t& size, const ReadRawHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    size_t len;
                    VECTOR_TO_SIZE(m_inboundMessagePrefix, len);
                    handler(channel(), data, len);
                } else if (m_errorHandler) {
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes");
                } else {
                    throw NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readAsyncVectorHash(const Channel::ReadVectorHashHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength == 0) {
                    throw PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use readAsyncRaw(...) instead.");
                }
                m_inboundMessagePrefix.resize(sizeofLength);
                m_inboundHeaderPrefix.resize(sizeofLength);
                vector<mutable_buffer> buf;
                buf.push_back(buffer(m_inboundMessagePrefix));
                buf.push_back(buffer(m_inboundHeaderPrefix));
                boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::readPrefixHandler,
                        this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readPrefixHandler(const Channel::ReadVectorHashHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    size_t fsize, hsize;
                    VECTOR_TO_SIZE_2(m_inboundMessagePrefix, fsize, m_inboundHeaderPrefix, hsize);
                    size_t dataPrefix = fsize - hsize;
                    try {
                        m_inboundData.resize(dataPrefix);
                        m_inboundHeader.resize(hsize);
                    } catch (const std::bad_alloc& e) {
                        string sdata = String::toString(dataPrefix);
                        string shdr = String::toString(hsize);
                        throw PARAMETER_EXCEPTION("Failed to resize vectors with data size = "
                                + sdata + " and header size = " + shdr + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundHeader, hsize));
                    buf.push_back(buffer(m_inboundData, dataPrefix));
                    boost::asio::async_read(m_socket, buf, transfer_all(),
                            boost::bind(&TcpChannel::asyncReadHandler, this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::asyncReadHandler(const Channel::ReadVectorHashHandler& handler, const boost::system::error_code& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    Hash hash;
                    if (m_inboundHeader.size() > 0) {
                        using namespace karabo::io;
                        string s(m_inboundHeader.begin(), m_inboundHeader.end());
                        stringToHash(s, hash);
                    }
                    handler(channel(), m_inboundData, hash);
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readAsyncStringHash(const Channel::ReadStringHashHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength == 0) {
                    throw PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use readAsyncRaw(...) instead.");
                }
                m_inboundMessagePrefix.resize(sizeofLength);
                m_inboundHeaderPrefix.resize(sizeofLength);
                vector<mutable_buffer> buf;
                buf.push_back(buffer(m_inboundMessagePrefix));
                buf.push_back(buffer(m_inboundHeaderPrefix));
                boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::readStringHashPrefixHandler,
                        this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::readStringHashPrefixHandler(const Channel::ReadStringHashHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    size_t fsize, hsize;
                    VECTOR_TO_SIZE_2(m_inboundMessagePrefix, fsize, m_inboundHeaderPrefix, hsize);
                    size_t dataPrefix = fsize - hsize;
                    try {
                        m_inboundData.resize(dataPrefix);
                        m_inboundHeader.resize(hsize);
                    } catch (const bad_alloc& e) {
                        string sdata = String::toString(dataPrefix);
                        string shead = String::toString(hsize);
                        throw PARAMETER_EXCEPTION("Fail to resize vectors with data size = " + sdata
                                + " and header size = " + shead + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundHeader));
                    buf.push_back(buffer(m_inboundData));
                    boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::asyncReadStringHashHandler,
                            this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::asyncReadStringHashHandler(const Channel::ReadStringHashHandler& handler, const boost::system::error_code& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    Hash hash;
                    if (m_inboundHeader.size() > 0) {
                        using namespace karabo::io;
                        string s(m_inboundHeader.begin(), m_inboundHeader.end());
                        stringToHash(s, hash);
                    }
                    string s(m_inboundData.begin(), m_inboundData.end());
                    handler(channel(), s, hash);
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes");
                else
                    throw NETWORK_EXCEPTION(e.message());
            } catch (...) {
                RETHROW
            }
        }

        // synchronous write


        void TcpChannel::write(const char* data, const size_t& size) {
            try {
                boost::system::error_code error; //in case of error
                vector<const_buffer> buf;
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength > 0) {
                    SIZE_TO_VECTOR(m_outboundMessagePrefix, size); // m_outboundMessagePrefix = size;
                    buf.push_back(buffer(m_outboundMessagePrefix)); // size of the prefix
                }
                buf.push_back(buffer(data, size)); // body
                boost::asio::write(m_socket, buf, transfer_all(), error);
                if (!error)
                    return;
                else if (m_errorHandler)
                    m_errorHandler(channel(), error.message());
                else
                    throw NETWORK_EXCEPTION(error.message());

            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::write(const char* data, const size_t& dsize, const Hash& header) {
            try {
                using namespace karabo::io;
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength == 0) {
                    throw PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const size_t& size) instead.");
                }
                string s;
                hashToString(header, s);
                write(data, dsize, s.c_str(), s.size());
                return;
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::write(const char* data, const size_t& dsize, const char* header, const string::size_type& hsize) {
            try {
                boost::system::error_code error; //in case of error

                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength == 0) {
                    throw PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use write(const char* data, const size_t& size) instead.");
                }
                size_t fsize = hsize + dsize;
                SIZE_TO_VECTOR_2(m_outboundMessagePrefix, fsize, m_outboundHeaderPrefix, hsize);
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(header, hsize));
                buf.push_back(buffer(data, dsize));
                boost::asio::write(m_socket, buf, transfer_all(), error);
                if (!error)
                    return;
                else if (m_errorHandler)
                    m_errorHandler(channel(), error.message());
                else
                    throw NETWORK_EXCEPTION(error.message());

            } catch (...) {
                RETHROW
            }
        }

        // asynchronous write


        void TcpChannel::writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                vector<const_buffer> buf;
                if (sizeofLength > 0) {
                    SIZE_TO_VECTOR(m_outboundMessagePrefix, size);
                    buf.push_back(buffer(m_outboundMessagePrefix));
                }
                buf.push_back(buffer(data, size));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::writeAsyncVector(const vector<char>& data, const Channel::WriteCompleteHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength == 0) {
                    throw PARAMETER_EXCEPTION("With sizeofLength=0 you cannot use this interface.  Use writeAsyncRaw instead.");
                }
                size_t size = data.size();
                SIZE_TO_VECTOR(m_outboundMessagePrefix, size); //m_outboundMessagePrefix = size;
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(data));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::writeAsyncString(const string& data, const Channel::WriteCompleteHandler& handler) {
            try {
                try {
                    m_outboundData.resize(data.size());
                } catch (const bad_alloc& e) {
                    throw PARAMETER_EXCEPTION("Fail to resize vector with size = " + String::toString(data.size()) + " => " + e.what());
                }
                m_outboundData.assign(data.begin(), data.end());
                writeAsyncVector(m_outboundData, handler);
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::writeAsyncHash(const Hash& data, const Channel::WriteCompleteHandler& handler) {
            try {
                using namespace karabo::io;
                string s;
                hashToString(data, s);
                writeAsyncString(s, handler);
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::writeAsyncRawHash(const char* data, const size_t& size, const karabo::util::Hash& header, const WriteCompleteHandler& handler) {
            try {
                using namespace karabo::io;
                string s;
                if (!header.empty()) {
                    hashToString(header, s);
                }
                size_t fsize = s.size() + size;
                size_t hsize = s.size();
                SIZE_TO_VECTOR_2(m_outboundMessagePrefix, fsize, m_outboundHeaderPrefix, hsize); //m_outboundMessagePrefix=fsize; m_outboundHeaderPrefix=hsize;
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(s));
                buf.push_back(buffer(data, size));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::writeAsyncStringHash(const std::string& data, const Hash& header, const Channel::WriteCompleteHandler& handler) {
            try {
                vector<char> v(data.begin(), data.end());
                writeAsyncVectorHash(v, header, handler);
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::writeAsyncVectorHash(const std::vector<char>& data, const Hash& header, const Channel::WriteCompleteHandler& handler) {
            try {
                using namespace karabo::io;
                string s;
                if (!header.empty()) {
                    hashToString(header, s);
                }
                m_outboundHeader.assign(s.begin(), s.end());
                m_outboundData.assign(data.begin(), data.end()); // TODO: is it an extra copy operation??
                writeAsyncVectorVector(m_outboundData, m_outboundHeader, handler);
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::writeAsyncVectorVector(const vector<char>& data, const vector<char>& header, const Channel::WriteCompleteHandler& handler) {
            try {
                size_t fsize = header.size() + data.size();
                size_t hsize = header.size();
                SIZE_TO_VECTOR_2(m_outboundMessagePrefix, fsize, m_outboundHeaderPrefix, hsize); // m_outboundMessagePrefix = fsize; m_outboundHeaderPrefix = hsize;
                vector<const_buffer> buf;
                buf.push_back(buffer(m_outboundMessagePrefix));
                buf.push_back(buffer(m_outboundHeaderPrefix));
                buf.push_back(buffer(header));
                buf.push_back(buffer(data));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e) {
            try {
                if (!e) {
                    try {
                        handler(channel());
                    } catch (...) {
                        RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(channel(), e.message());
                } else {
                    throw NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::waitAsync(int millisecs, const WaitHandler& handler) {
            try {
                m_timer.expires_from_now(boost::posix_time::milliseconds(millisecs));
                m_timer.async_wait(boost::bind(&TcpChannel::asyncWaitHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::asyncWaitHandler(const Channel::WaitHandler& handler, const ErrorCode& e) {
            try {
                if (!e) {
                    try {
                        handler(channel());
                    } catch (...) {
                        RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(channel(), e.message());
                } else {
                    throw NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                RETHROW
            }
        }


        void TcpChannel::close() {
            try {
                m_socket.close();
                unregisterChannel(shared_from_this());
            } catch (...) {
                RETHROW
            }
        }


        bool TcpChannel::isOpen() {
            return m_socket.is_open();
        }


#undef VECTOR_TO_SIZE
#undef VECTOR_TO_SIZE_2
#undef SIZE_TO_VECTOR
#undef SIZE_TO_VECTOR_2


    }
}
