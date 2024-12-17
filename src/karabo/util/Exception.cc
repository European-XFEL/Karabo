/*
 * $Id: Exception.cc 5061 2012-02-08 08:15:37Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 15, 2010, 11:17 AM
 *
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

#include "Exception.hh"

#include <chrono>
#include <filesystem>

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

using namespace std;

namespace karabo {
    namespace util {

        using namespace std::filesystem;

        // Init static members
        std::mutex Exception::m_mutex;
        std::map<boost::thread::id, boost::circular_buffer<Exception::ExceptionInfo>> Exception::m_trace;


        Exception::Exception(const string& message, const string& type, const string& filename, const string& function,
                             int lineNumber, const std::string& detailsMsg) {
            m_exceptionInfo.message = message;
            m_exceptionInfo.details = detailsMsg;
            m_exceptionInfo.type = type;
            m_exceptionInfo.function = function;
            path pf(filename);
            bool collect = false;
            for (path::iterator it = pf.begin(); it != pf.end(); ++it) {
                if (it->string() == "karabo" || it->string() == "src" || collect) {
                    if (collect) {
                        m_exceptionInfo.filename += "/" + it->generic_string();
                    } else {
                        m_exceptionInfo.filename += "/[...]/" + it->generic_string();
                    }
                    collect = true;
                }
            }
            if (m_exceptionInfo.filename == "") {
                m_exceptionInfo.filename = pf.generic_string();
            }
            if (lineNumber >= 0) {
                m_exceptionInfo.lineNumber = toString(lineNumber);
            } else {
                m_exceptionInfo.lineNumber = "";
            }
            m_exceptionInfo.timestamp = current_time_string();
        }


        void Exception::addToTrace(const ExceptionInfo& value) {
            std::lock_guard<std::mutex> lock(Exception::m_mutex);
            boost::circular_buffer<ExceptionInfo>& buffer = Exception::m_trace[boost::this_thread::get_id()];
            if (buffer.empty()) buffer.set_capacity(100u); // limit size in case never cleared
            buffer.push_back(value);
        }


        void Exception::addToTrace(const Exception& e) {
            addToTrace(e.m_exceptionInfo);
        }


        void Exception::clearTrace() {
            std::lock_guard<std::mutex> lock(Exception::m_mutex);
            Exception::m_trace.erase(boost::this_thread::get_id());
        }


        void Exception::memorize() {
#define KARABO_EXCEPTION(CLASS)             \
    Exception::ExceptionInfo myException;   \
    myException.type = CLASS;               \
    myException.message = string(e.what()); \
    myException.filename = "";              \
    myException.function = "";              \
    myException.lineNumber = "";            \
    Exception::addToTrace(myException);

            try {
                // This forwards any expession
                throw;
            } catch (Exception& e) { // ---- Forwarded exception is of our own type ----
                Exception::addToTrace(e.m_exceptionInfo);
            } catch (std::invalid_argument& e) { // ---- Forward exception is a standard exception ----
                KARABO_EXCEPTION("std::invalid_argument");
            } catch (std::out_of_range& e) {
                KARABO_EXCEPTION("std::out_of_range");
            } catch (std::logic_error& e) {
                KARABO_EXCEPTION("std::logic_error");
            } catch (std::bad_alloc& e) {
                KARABO_EXCEPTION("std::bad_alloc");
            } catch (std::bad_cast& e) {
                KARABO_EXCEPTION("std::bad_cast");
            } catch (std::bad_typeid& e) {
                KARABO_EXCEPTION("std::bad_typeid");
            } catch (std::ios_base::failure& e) {
                KARABO_EXCEPTION("std::ios_base::failure");
            } catch (std::bad_exception& e) {
                KARABO_EXCEPTION("std::bad_exception");
            } catch (std::exception& e) {
                KARABO_EXCEPTION("std::exception");
            } catch (boost::exception& e) { // ---- Forwarded exception is a boost exception ----
                Exception::ExceptionInfo myException;
                myException.type = "boost:exception";
                myException.message = string(boost::diagnostic_information(e));
                myException.filename = "";
                myException.function = "";
                myException.lineNumber = "";
                Exception::addToTrace(myException);
            } catch (...) { // ---- Forwarded exception is of unknown type ----
                Exception::ExceptionInfo myException;
                myException.type = "Unknown and unhandled exception";
                myException.message = "An unknown error happened";
                myException.filename = "";
                myException.function = "";
                myException.lineNumber = "";
#if defined(__GNUC__) || defined(__clang__)
                // See
                // https://stackoverflow.com/questions/561997/determining-exception-type-after-the-exception-is-caught
                int status = 42; // Better init with a non-zero value...
                char* txt = abi::__cxa_demangle(abi::__cxa_current_exception_type()->name(), 0, 0, &status);
                if (status == 0 && txt) {
                    (myException.type += " - type is: ") += txt;
                    free(txt);
                }
#endif
                Exception::addToTrace(myException);
            }
        }


        void Exception::showTrace(ostream& os) {
            std::lock_guard<std::mutex> lock(Exception::m_mutex);
            auto& currentBuffer = Exception::m_trace[boost::this_thread::get_id()];
            if (currentBuffer.empty()) return;
            ostringstream oss;
            oss << "Exception with trace (listed from inner to outer):" << endl;
            for (unsigned int i = 0; i < currentBuffer.size(); ++i) {
                string fill(i * 3, ' ');
                oss << fill << i + 1 << ". Exception " << string(5, '=') << ">  {" << endl;
                format(oss, currentBuffer[i], fill);
                oss << fill << "}" << endl << endl;
            }
            os << oss.str();
        }


        ostream& operator<<(ostream& os, const Exception& exception) {
            Exception::showTrace(os); // no-op if m_trace[boost::this_thread::get_id()] is empty

            {
                std::lock_guard<std::mutex> lock(Exception::m_mutex);
                auto& currentBuffer = Exception::m_trace[boost::this_thread::get_id()];
                string fill(currentBuffer.size() * 3, ' ');
                os << fill << currentBuffer.size() + 1 << ". Exception " << string(5, '=') << ">  {" << endl;
                Exception::format(os, exception.m_exceptionInfo, fill);
                os << fill << "}" << endl << endl;
            }
            Exception::clearTrace();
            return os;
        }


        void Exception::format(ostream& os, const ExceptionInfo& exceptionInfo, const string& spacing) {
            if (!exceptionInfo.type.empty()) os << spacing << "    Exception Type....:  " << exceptionInfo.type << endl;
            if (!exceptionInfo.message.empty())
                os << spacing << "    Message...........:  " << exceptionInfo.message << endl;
            if (!exceptionInfo.details.empty())
                os << spacing << "    Details...........:  " << exceptionInfo.details << endl; // typically multiline...
            if (!exceptionInfo.filename.empty())
                os << spacing << "    File..............:  " << exceptionInfo.filename << endl;
            if (!exceptionInfo.function.empty())
                os << spacing << "    Function..........:  " << exceptionInfo.function << endl;
            if (!exceptionInfo.lineNumber.empty())
                os << spacing << "    Line Number.......:  " << exceptionInfo.lineNumber << endl;
            if (!exceptionInfo.timestamp.empty())
                os << spacing << "    Timestamp.........:  " << exceptionInfo.timestamp << endl;
        }


        void Exception::msg(std::ostream& os) const {
            os << *this;
        }


        const char* Exception::what() const throw() {
            m_detailedMsg = detailedMsg();
            return m_detailedMsg.c_str();
        }


        string Exception::userFriendlyMsg(bool clearTrace) const {
            stringstream err;
            err << m_exceptionInfo.message;
            bool hasMsg = !m_exceptionInfo.message.empty();
            {
                std::lock_guard<std::mutex> lock(Exception::m_mutex);
                auto& currentBuffer = Exception::m_trace[boost::this_thread::get_id()];

                unsigned int j = 0;
                for (size_t i = currentBuffer.size(); i > 0; --i) {
                    // Caveat: Do not loop "...i = ...size() - 1; i >= 0;" - that's end endless due to unsigend-ness
                    const Exception::ExceptionInfo& myException = currentBuffer[i - 1]; // ...so we need -1 here
                    if (!myException.message.empty()) {
                        if (hasMsg) {
                            err << "\n";
                            const unsigned int indentLevel = ++j;
                            for (unsigned int k = 0; k < indentLevel; ++k) err << "  "; // 2 spaces per level
                            err << "because: ";
                        } else {
                            hasMsg = true;
                        }
                        err << myException.message;
                    }
                }
            }
            if (!hasMsg) {
                // Happens e.g. for KARABO_RETHROW (i.e. PropagatedException with empty message) after clearing of trace
                // or for other exceptions with empty message - then at least give type info:
                err << type();
            }
            if (clearTrace) Exception::clearTrace();

            return err.str();
        }


        string Exception::detailedMsg() const {
            std::ostringstream os;
            os << *this;
            return os.str();
        }


        const std::string& Exception::type() const {
            return m_exceptionInfo.type;
        }


        const std::string& Exception::details() const {
            return m_exceptionInfo.details;
        }


        std::string Exception::current_time_string() {
            std::ostringstream oss;
#if __GNUC__ >= 13 && __cplusplus >= 202002
            oss << std::chrono::system_clock::now();
#else
            auto now = std::chrono::system_clock::now();
            auto timet = std::chrono::system_clock::to_time_t(now);
            oss << std::put_time(std::localtime(&timet), "%F %T");
#endif
            return oss.str();
        }
    } // namespace util
} // namespace karabo
