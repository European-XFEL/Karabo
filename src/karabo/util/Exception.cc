/*
 * $Id: Exception.cc 5061 2012-02-08 08:15:37Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 15, 2010, 11:17 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

#include "Exception.hh"

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

using namespace std;

namespace karabo {
    namespace util {

        using namespace boost::posix_time;
        using namespace boost::filesystem;

        // Init static members
        boost::mutex Exception::m_mutex;
        // circular buffer to avoid leakage if trace is never cleared
        boost::circular_buffer<Exception::ExceptionInfo> Exception::m_trace(100);
        std::map<void*, Exception::ExceptionHandler> Exception::m_exceptionHandlers;
        bool Exception::m_hasUnhandled = false;


        Exception::Exception(const string& message, const string& type, const string& filename, const string& function, int lineNumber) {
            m_exceptionInfo.message = message;
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
            m_exceptionInfo.timestamp = to_simple_string(ptime(microsec_clock::local_time()));
        }


        void Exception::addToTrace(const ExceptionInfo& value) {
            boost::mutex::scoped_lock lock(Exception::m_mutex);
            m_hasUnhandled = true;
            m_trace.push_back(value);
        }


        void Exception::addToTrace(const Exception& e) {
            addToTrace(e.m_exceptionInfo);
        }


        void Exception::clearTrace() {
            boost::mutex::scoped_lock lock(Exception::m_mutex);
            Exception::m_hasUnhandled = false;
            Exception::m_trace.clear();
        }


        void Exception::memorize() {

#define KARABO_EXCEPTION(CLASS) Exception::ExceptionInfo myException; \
  myException.type = CLASS; \
  myException.message = string(e.what()); \
  myException.filename = ""; \
  myException.function = ""; \
  myException.lineNumber = ""; \
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
                // See https://stackoverflow.com/questions/561997/determining-exception-type-after-the-exception-is-caught
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
            boost::mutex::scoped_lock lock(Exception::m_mutex);
            if (m_trace.empty()) return;
            ostringstream oss;
            oss << endl << " Exception with trace (listed from inner to outer):" << endl;
            for (unsigned int i = 0; i < Exception::m_trace.size(); ++i) {
                string fill(i * 3, ' ');
                oss << fill << i + 1 << ". Exception " << string(5, '=') << ">  {" << endl;
                format(oss, Exception::m_trace[i], fill);
                oss << fill << "}" << endl << endl;
            }
            os << oss.str();
        }


        ostream & operator<<(ostream& os, const Exception& exception) {
            Exception::showTrace(os); // no-op if m_trace is empty

            {
                boost::mutex::scoped_lock lock(Exception::m_mutex);
                string fill(Exception::m_trace.size()*3, ' ');
                os << fill << Exception::m_trace.size() + 1 << ". Exception " << string(5, '=') << ">  {" << endl;
                Exception::format(os, exception.m_exceptionInfo, fill);
                os << fill << "}" << endl << endl;
            }
            Exception::clearTrace();
            return os;
        }


        void Exception::format(ostream& os, const ExceptionInfo& exceptionInfo, const string& spacing) {
            if (!exceptionInfo.type.empty()) os << spacing << "    Exception Type....:  " << exceptionInfo.type << endl;
            if (!exceptionInfo.message.empty()) os << spacing << "    Message...........:  " << exceptionInfo.message << endl;
            if (!exceptionInfo.filename.empty()) os << spacing << "    File..............:  " << exceptionInfo.filename << endl;
            if (!exceptionInfo.function.empty()) os << spacing << "    Function..........:  " << exceptionInfo.function << endl;
            if (!exceptionInfo.lineNumber.empty()) os << spacing << "    Line Number.......:  " << exceptionInfo.lineNumber << endl;
            if (!exceptionInfo.timestamp.empty()) os << spacing << "    Timestamp.........:  " << exceptionInfo.timestamp << endl;
        }


        void Exception::msg(std::ostream& os) const {
            os << *this;
        }


        const char* Exception::what() const throw () {
            m_detailedMsg = detailedMsg();
            return m_detailedMsg.c_str();
        }


        string Exception::userFriendlyMsg() const {
            string err = "An error has occured: ";
            if (!m_exceptionInfo.message.empty() && m_exceptionInfo.message != "Propagation") {
                string s = m_exceptionInfo.message + "\n";
                err += s;
            }
            {
                boost::mutex::scoped_lock lock(Exception::m_mutex);
                for (unsigned int i = 0; i < Exception::m_trace.size(); ++i) {
                    const Exception::ExceptionInfo& myException = Exception::m_trace[i];
                    if (!myException.message.empty() && myException.message != "Propagation") {
                        string s = myException.message + "\n";
                        err += s;
                    }
                }
            }
            Exception::clearTrace();

            return err;
        }


        string Exception::detailedMsg() const {
            std::ostringstream os;
            os << *this;
            return os.str();
        }
    } // namespace util
} // namespace karabo
