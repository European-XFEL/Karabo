#include <iostream>
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

        void TcpChannel::read(char*& data, size_t& size) {
            try {
                ErrorCode error; //in case of error
                unsigned int sizeofLength = getConnection()->getSizeofLength();
                m_inboundMessagePrefix = 0;
                boost::asio::read(m_socket, buffer(&m_inboundMessagePrefix, sizeofLength), transfer_all(), error);
                if (error)
                    throw boost::system::system_error(error);

                // prefix[0] - message length (body)
                size = m_inboundMessagePrefix;
                data = new char[ size ];
                vector<mutable_buffer> buf;
                buf.push_back(buffer(data, size));

                boost::asio::read(m_socket, buf, transfer_all(), error);
                if (error)
                    throw boost::system::system_error(error);
            } catch (...) {
                KARABO_RETHROW
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
                vector<mutable_buffer> bufhdr;
                bufhdr.push_back(buffer(&m_inboundMessagePrefix, sizeofLength));
                bufhdr.push_back(buffer(&m_inboundHeaderPrefix, sizeofLength));
                m_inboundMessagePrefix = 0;
                m_inboundHeaderPrefix = 0;
                boost::asio::read(m_socket, bufhdr, transfer_all(), error);
                if (error)
                    throw boost::system::system_error(error);

                // prefix[0] - message length (header + body)
                // prefix[1] - header length
                hsize = m_inboundHeaderPrefix;
                dsize = m_inboundMessagePrefix - m_inboundHeaderPrefix;

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
                KARABO_RETHROW
            }
        }

        // asynchronous read

        void TcpChannel::readAsyncVector(const Channel::ReadVectorHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_inboundMessagePrefix = 0;
                boost::asio::async_read(m_socket, buffer(&m_inboundMessagePrefix, sizeofLength), transfer_all(),
                        boost::bind(&TcpChannel::readVectorPrefixHandler, this, handler, boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                        ));
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readVectorPrefixHandler(const Channel::ReadVectorHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    try {
                        m_inboundData.resize(m_inboundMessagePrefix);
                    } catch (const bad_alloc& e) {
                        throw KARABO_PARAMETER_EXCEPTION("Fail to resize vector with size = "
                                + String::toString(m_inboundMessagePrefix) + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundData));
                    boost::asio::async_read(m_socket, buf, transfer_all(),
                            boost::bind(&TcpChannel::asyncReadVectorHandler, this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::asyncReadVectorHandler(const Channel::ReadVectorHandler& handler, const boost::system::error_code& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    handler(channel(), m_inboundData);
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes");
                else
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readAsyncString(const Channel::ReadStringHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_inboundMessagePrefix = 0;
                boost::asio::async_read(m_socket, buffer(&m_inboundMessagePrefix, sizeofLength), transfer_all(),
                        boost::bind(&TcpChannel::readStringPrefixHandler, this, handler, boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                        ));
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readStringPrefixHandler(const Channel::ReadStringHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    try {
                        m_inboundData.resize(m_inboundMessagePrefix);
                    } catch (const bad_alloc& e) {
                        throw KARABO_PARAMETER_EXCEPTION("Fail to resize vector with size = "
                                + String::toString(m_inboundMessagePrefix) + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundData));
                    boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::asyncReadStringHandler,
                            this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
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
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readAsyncHash(const Channel::ReadHashHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_inboundMessagePrefix = 0;
                boost::asio::async_read(m_socket, buffer(&m_inboundMessagePrefix, sizeofLength), transfer_all(),
                        boost::bind(&TcpChannel::readHashPrefixHandler, this, handler, boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred
                        ));
            } catch (...) {
                KARABO_RETHROW
            }

        }

        void TcpChannel::readHashPrefixHandler(const Channel::ReadHashHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    try {
                        m_inboundData.resize(m_inboundMessagePrefix);
                    } catch (const bad_alloc& e) {
                        throw KARABO_PARAMETER_EXCEPTION("Fail to resize vector with size = "
                                + String::toString(m_inboundMessagePrefix) + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundData));
                    boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::asyncReadHashHandler,
                            this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
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
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readAsyncRaw(char*& data, size_t& size, const ReadRawHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                if (sizeofLength > 0) {
                    // protocol should contain the message length at the beginning of message with size = sizeofLength 
                    m_inboundMessagePrefix = 0;
                    boost::asio::async_read(m_socket, buffer(&m_inboundMessagePrefix, sizeofLength), transfer_all(), boost::bind(
                            &TcpChannel::readRawPrefixHandler, this, data, size, handler, boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
                    return;
                } else {
                    m_inboundMessagePrefix = size;
                }
                readRawPrefixHandler(data, size, handler, boost::system::error_code(), 0UL);
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readRawPrefixHandler(char*& data, size_t& size, const ReadRawHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    if (size < m_inboundMessagePrefix) {
                        throw KARABO_PARAMETER_EXCEPTION("Size of read buffer is less than data size: "
                                + String::toString(size) + " < " + String::toString(m_inboundMessagePrefix));
                    }
                    vector<mutable_buffer > buf;
                    if (m_inboundMessagePrefix > 0) {
                        buf.push_back(buffer(data, m_inboundMessagePrefix));
                        boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::asyncReadRawHandler,
                                this, data, size, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                    }
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::asyncReadRawHandler(char*& data, size_t& size, const ReadRawHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    handler(channel(), data, m_inboundMessagePrefix);
                } else if (m_errorHandler) {
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes");
                } else {
                    throw KARABO_NETWORK_EXCEPTION(e.message());
                }
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readAsyncVectorHash(const Channel::ReadVectorHashHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_inboundMessagePrefix = 0;
                m_inboundHeaderPrefix = 0;
                vector<mutable_buffer> buf;
                buf.push_back(buffer(&m_inboundMessagePrefix, sizeofLength));
                buf.push_back(buffer(&m_inboundHeaderPrefix, sizeofLength));
                boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::readPrefixHandler,
                        this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readPrefixHandler(const Channel::ReadVectorHashHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    size_t dataPrefix = m_inboundMessagePrefix - m_inboundHeaderPrefix;
                    try {
                        m_inboundData.resize(dataPrefix);
                        m_inboundHeader.resize(m_inboundHeaderPrefix);
                    } catch (const std::bad_alloc& e) {
                        throw KARABO_PARAMETER_EXCEPTION("Failed to resize vectors with data size = "
                                + String::toString(dataPrefix) + " and header size = "
                                + String::toString(m_inboundHeaderPrefix) + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundHeader, m_inboundHeaderPrefix));
                    buf.push_back(buffer(m_inboundData, dataPrefix));
                    boost::asio::async_read(m_socket, buf, transfer_all(),
                            boost::bind(&TcpChannel::asyncReadHandler, this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
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
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readAsyncStringHash(const Channel::ReadStringHashHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_inboundMessagePrefix = 0;
                m_inboundHeaderPrefix = 0;
                vector<mutable_buffer> buf;
                buf.push_back(buffer(&m_inboundMessagePrefix, sizeofLength));
                buf.push_back(buffer(&m_inboundHeaderPrefix, sizeofLength));
                boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::readStringHashPrefixHandler,
                        this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::readStringHashPrefixHandler(const Channel::ReadStringHashHandler& handler, const ErrorCode& e, size_t bytes_transferred) {
            try {
                if (!e) {
                    size_t dataPrefix = m_inboundMessagePrefix - m_inboundHeaderPrefix;
                    try {
                        m_inboundData.resize(dataPrefix);
                        m_inboundHeader.resize(m_inboundHeaderPrefix);
                    } catch (const bad_alloc& e) {
                        throw KARABO_PARAMETER_EXCEPTION("Fail to resize vectors with data size = " + String::toString(dataPrefix)
                                + " and header size = " + String::toString(m_inboundHeaderPrefix) + " => " + e.what());
                    }
                    vector<mutable_buffer > buf;
                    buf.push_back(buffer(m_inboundHeader));
                    buf.push_back(buffer(m_inboundData));
                    boost::asio::async_read(m_socket, buf, transfer_all(), boost::bind(&TcpChannel::asyncReadStringHashHandler,
                            this, handler, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
                } else if (m_errorHandler)
                    m_errorHandler(channel(), e.message() + " and transferred " + String::toString(bytes_transferred) + " bytes.");
                else
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
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
                    throw KARABO_NETWORK_EXCEPTION(e.message());
            } catch (...) {
                KARABO_RETHROW
            }
        }

        // synchronous write

        void TcpChannel::write(const char* data, const size_t& size) {
            try {
                boost::system::error_code error; //in case of error
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_outboundMessagePrefix = size;
                vector<const_buffer> buf;
                buf.push_back(buffer(&m_outboundMessagePrefix, sizeofLength)); // size of the prefix
                buf.push_back(buffer(data, size)); // body
                boost::asio::write(m_socket, buf, transfer_all(), error);
                if (!error)
                    return;
                else if (m_errorHandler)
                    m_errorHandler(channel(), error.message());
                else
                    throw KARABO_NETWORK_EXCEPTION(error.message());

            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::write(const char* data, const size_t& dsize, const Hash& header) {
            try {
                using namespace karabo::io;
                string s;
                hashToString(header, s);
                write(data, dsize, s.c_str(), s.size());
                return;
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::write(const char* data, const size_t& dsize, const char* header, const string::size_type& hsize) {
            try {
                boost::system::error_code error; //in case of error

                size_t sizeofLength = getConnection()->getSizeofLength();
                m_outboundMessagePrefix = hsize + dsize;
                m_outboundHeaderPrefix = hsize;
                vector<const_buffer> buf;
                buf.push_back(buffer(&m_outboundMessagePrefix, sizeofLength));
                buf.push_back(buffer(&m_outboundHeaderPrefix, sizeofLength));
                buf.push_back(buffer(header, hsize));
                buf.push_back(buffer(data, dsize));
                boost::asio::write(m_socket, buf, transfer_all(), error);
                if (!error)
                    return;
                else if (m_errorHandler)
                    m_errorHandler(channel(), error.message());
                else
                    throw KARABO_NETWORK_EXCEPTION(error.message());

            } catch (...) {
                KARABO_RETHROW
            }
        }

        // asynchronous write

        void TcpChannel::writeAsyncRaw(const char* data, const size_t& size, const WriteCompleteHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_outboundMessagePrefix = size;
                vector<const_buffer> buf;
                buf.push_back(buffer(&m_outboundMessagePrefix, sizeofLength));
                buf.push_back(buffer(data, size));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::writeAsyncVector(const vector<char>& data, const Channel::WriteCompleteHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_outboundMessagePrefix = data.size();
                vector<const_buffer> buf;
                buf.push_back(buffer(&m_outboundMessagePrefix, sizeofLength));
                buf.push_back(buffer(data));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::writeAsyncString(const string& data, const Channel::WriteCompleteHandler& handler) {
            try {
                try {
                    m_outboundData.resize(data.size());
                } catch (const bad_alloc& e) {
                    throw KARABO_PARAMETER_EXCEPTION("Fail to resize vector with size = " + String::toString(data.size()) + " => " + e.what());
                }
                m_outboundData.assign(data.begin(), data.end());
                writeAsyncVector(m_outboundData, handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::writeAsyncHash(const Hash& data, const Channel::WriteCompleteHandler& handler) {
            try {
                using namespace karabo::io;
                string s;
                hashToString(data, s);
                writeAsyncString(s, handler);
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::writeAsyncRawHash(const char* data, const size_t& size, const karabo::util::Hash& header, const WriteCompleteHandler& handler) {
            try {
                using namespace karabo::io;
                string s;
                if (!header.empty()) {
                    hashToString(header, s);
                }
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_outboundMessagePrefix = s.size() + size;
                m_outboundHeaderPrefix = s.size();
                vector<const_buffer> buf;
                buf.push_back(buffer(&m_outboundMessagePrefix, sizeofLength));
                buf.push_back(buffer(&m_outboundHeaderPrefix, sizeofLength));
                buf.push_back(buffer(s));
                buf.push_back(buffer(data, size));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::writeAsyncStringHash(const std::string& data, const Hash& header, const Channel::WriteCompleteHandler& handler) {
            try {
                vector<char> v(data.begin(), data.end());
                writeAsyncVectorHash(v, header, handler);
            } catch (...) {
                KARABO_RETHROW
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
                KARABO_RETHROW
            }
        }

        void TcpChannel::writeAsyncVectorVector(const vector<char>& data, const vector<char>& header, const Channel::WriteCompleteHandler& handler) {
            try {
                size_t sizeofLength = getConnection()->getSizeofLength();
                m_outboundMessagePrefix = header.size() + data.size();
                m_outboundHeaderPrefix = header.size();
                vector<const_buffer> buf;
                buf.push_back(buffer(&m_outboundMessagePrefix, sizeofLength));
                buf.push_back(buffer(&m_outboundHeaderPrefix, sizeofLength));
                buf.push_back(buffer(header));
                buf.push_back(buffer(data));
                boost::asio::async_write(m_socket, buf, boost::bind(&TcpChannel::asyncWriteHandler, this, handler, boost::asio::placeholders::error));
            } catch (...) {
                KARABO_RETHROW
            }
        }

        void TcpChannel::asyncWriteHandler(const Channel::WriteCompleteHandler& handler, const ErrorCode& e) {
            try {
                if (!e) {
                    try {
                        handler(channel());
                    } catch (...) {
                        KARABO_RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(channel(), e.message());
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
                        handler(channel());
                    } catch (...) {
                        KARABO_RETHROW
                    }
                } else if (m_errorHandler) {
                    m_errorHandler(channel(), e.message());
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
                unregisterChannel(shared_from_this());
            } catch (...) {
                KARABO_RETHROW
            }
        }

        bool TcpChannel::isOpen() {
            return m_socket.is_open();
        }
    }
}
