/*
 * $Id: Exception.hh 6353 2012-06-06 11:06:01Z esenov $
 *
 * File:   Exception.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 15, 2010, 10:33 AM
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


#ifndef KARABO_UTIL_EXCEPTION_HH
#define KARABO_UTIL_EXCEPTION_HH

#include <boost/circular_buffer.hpp>
#include <boost/current_function.hpp>
#include <boost/thread.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "karaboDll.hh"

namespace karabo {

    namespace util {

        /**
         * @class Exception
         * @brief Karabo's main exception class. Inherits from std::exception
         */
        class KARABO_DECLSPEC Exception : public std::exception {
           public:
            typedef std::function<void(const Exception&)> ExceptionHandler; // unused - remove?


            /**
             * Constructor using message, exception type, filename, function name, line number and possible detailsMsg
             */
            Exception(const std::string& message, const std::string& type, const std::string& filename,
                      const std::string& function, int lineNumber, const std::string& detailsMsg = std::string());

            /**
             * Destructor
             */
            virtual ~Exception() throw(){};

            template <class T>
            static void rethrow(const T& exception) {
                Exception::memorize();
                throw exception;
            }

            /**
             * Use this function if you want to just memorize the exception but keep on running.
             * This function should be used for exceptions thrown in multi-threaded context, as
             * C++'s intrinsic exception handling is not thread safe.
             *
             * Example:
             * TODO
             */
            static void memorize();

            /**
             * Clears the trace of memorized messages for current thread
             */
            static void clearTrace();

            /**
             * Shows all memorized messages.
             */
            static void showTrace(std::ostream& os = std::cerr);

            static void addToTrace(const Exception& e);

            /**
             *  Automatic output using the << operator
             */
            KARABO_DECLSPEC friend std::ostream& operator<<(std::ostream& os, const Exception& Exception);

            /**
             * Explicit output function.
             * In contrast to the inherited what() function this function also lists
             * memorized/traced exceptions.
             */
            void msg(std::ostream& os = std::cerr) const;

            /**
             * Overrides std::exception.
             *
             * Same as detailedMsg() except returning 'const char*', not 'std::string'.
             * The returned pointer is valid as long as exception object is alive or until what() is called again.
             * Also clears the exception stack.
             */
            virtual const char* what() const throw();

            /**
             * This function is intended to be used for example in GUIs.
             *
             * @param clearTrace Whether to clear the exception stack, default is true.
             */
            std::string userFriendlyMsg(bool clearTrace = true) const;

            /**
             * This function returns the full error stack with all information.
             *
             * Clears the exception stack.
             *
             * @return The stack of exceptions as string, incl. source code line number.
             */
            std::string detailedMsg() const;

            /**
             * The type of the exception
             */
            const std::string& type() const;

            /**
             * The details of the exception - without trace.
             *
             * Some exceptions do not offer to provide details, then an empty string is returned.
             * Stack is not touched/cleared.
             */
            const std::string& details() const;

           protected:
            // Generic Exception information structure

            typedef struct {
                std::string type;
                std::string message;
                std::string details; // some exception types provide further details
                std::string filename;
                std::string function;
                std::string lineNumber;
                std::string timestamp;
            } ExceptionInfo;

            /**
             * Small helper function
             */
            template <class T>
            std::string toString(const T& value) {
                std::ostringstream s;
                s << value;
                return s.str();
            }

            /**
             * Formats the exception content
             * @param os Any output stream to that the exception content will be added
             * @param exceptionInfo Content of an exception, @see ExceptionInfo stuct
             */
            static void format(std::ostream& os, const ExceptionInfo& exceptionInfo, const std::string& spacing);

            /**
             * Adds exception information to trace for current thread
             */
            static void addToTrace(const ExceptionInfo& value);

            static std::string current_time_string();

            ExceptionInfo m_exceptionInfo;
            mutable std::string m_detailedMsg;
            static std::mutex m_mutex;
            static std::map<boost::thread::id, boost::circular_buffer<Exception::ExceptionInfo>> m_trace;
        };


        // ---------- Concrete exceptions derived from Exception.hh ----------

        // Please note: It is generally not encouraged to place more than one class
        // in a file. However, as the following classes are extremely compact an
        // exception :-) to the rule was accepted.

        /**
         * The PropagatedException handles exceptions that reflect anonymous nodes within a exception trace
         */
        class PropagatedException : public Exception {
           public:
            PropagatedException(const std::string& message, const std::string& filename, const std::string& function,
                                int lineNumber)
                : Exception(message, "Propagated Exception", filename, function, lineNumber) {}
        };
#define KARABO_PROPAGATED_EXCEPTION(msg) \
    karabo::util::PropagatedException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        // Convenience defines
#define KARABO_RETHROW karabo::util::Exception::rethrow(KARABO_PROPAGATED_EXCEPTION(""));
#define KARABO_RETHROW_AS(exception) karabo::util::Exception::rethrow(exception);

        // ---- Fundamental Exceptions

        /**
         * The ParameterException handles exceptions that result from missing or out-of-bounds parameter
         */
        class ParameterException : public Exception {
           public:
            ParameterException(const std::string& message, const std::string& filename, const std::string& function,
                               int lineNumber)
                : Exception(message, "Parameter Exception", filename, function, lineNumber) {}
        };
#define KARABO_PARAMETER_EXCEPTION(msg) \
    karabo::util::ParameterException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         *  The LogicException handles exceptions that are raised by any unexpected logical behaviour
         */
        class LogicException : public Exception {
           public:
            LogicException(const std::string& message, const std::string& filename, const std::string& function,
                           int lineNumber)
                : Exception(message, "Logic Exception", filename, function, lineNumber) {}
        };
#define KARABO_LOGIC_EXCEPTION(msg) karabo::util::LogicException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The NotImplementedException handles exceptions that are raised due to unimplemented functions/class calls
         */
        class NotImplementedException : public Exception {
           public:
            NotImplementedException(const std::string& message, const std::string& filename,
                                    const std::string& function, int lineNumber)
                : Exception(message, "Not implemented", filename, function, lineNumber) {}
        };
#define KARABO_NOT_IMPLEMENTED_EXCEPTION(msg) \
    karabo::util::NotImplementedException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The NotImplementedException handles exceptions that are raised by requesting unsupported features
         */
        class NotSupportedException : public Exception {
           public:
            NotSupportedException(const std::string& message, const std::string& filename, const std::string& function,
                                  int lineNumber)
                : Exception(message, "Call not supported", filename, function, lineNumber) {}
        };
#define KARABO_NOT_SUPPORTED_EXCEPTION(msg) \
    karabo::util::NotSupportedException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The SchemaException handles exceptions that are raised whilst constructing a Schema
         */
        class SchemaException : public Exception {
           public:
            SchemaException(const std::string& message, const std::string& filename, const std::string& function,
                            int lineNumber)
                : Exception(message, "Bad schema construction", filename, function, lineNumber) {}
        };
#define KARABO_SCHEMA_EXCEPTION(msg) karabo::util::SchemaException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The CastException handles exceptions that are raised due to illegal type castings
         */
        class CastException : public Exception {
           public:
            CastException(const std::string& message, const std::string& filename, const std::string& function,
                          int lineNumber)
                : Exception(message, "Cast Exception", filename, function, lineNumber) {}
        };
#define KARABO_CAST_EXCEPTION(msg) karabo::util::CastException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        // ---- Image Exceptions

        /**
         * The ImageException handles an generic image exception
         */
        class ImageException : public Exception {
           public:
            ImageException(const std::string& message, const std::string& filename, const std::string& function,
                           int lineNumber)
                : Exception(message, "Image Exception", filename, function, lineNumber) {}

            ImageException(const std::string& message, const std::string& type, const std::string& filename,
                           const std::string& function, int lineNumber)
                : Exception(message, type, filename, function, lineNumber) {}
#define KARABO_IMAGE_EXCEPTION(msg) karabo::util::ImageException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)
        };

        /**
         * The ImageDimensionException handles exceptions raised due to illegal image dimensions
         */
        class ImageDimensionException : public ImageException {
           public:
            ImageDimensionException(const std::string& message, const std::string& filename,
                                    const std::string& function, int lineNumber)
                : ImageException(message, "Image Dimension Exception", filename, function, lineNumber) {}
        };
#define KARABO_IMAGE_DIMENSION_EXCEPTION(msg) \
    karabo::util::ImageDimensionException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The ImageFormatException handles exceptions raised due to unsupported image formats
         */
        class ImageFormatException : public ImageException {
           public:
            ImageFormatException(const std::string& message, const std::string& filename, const std::string& function,
                                 int lineNumber)
                : ImageException(message, "Image Format Exception", filename, function, lineNumber) {}
        };
#define KARABO_IMAGE_FORMAT_EXCEPTION(msg) \
    karabo::util::ImageFormatException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The ImageTypeException handles exceptions raised due to imcompatible image types
         */
        class ImageTypeException : public ImageException {
           public:
            ImageTypeException(const std::string& message, const std::string& filename, const std::string& function,
                               int lineNumber)
                : ImageException(message, "Image Type Exception", filename, function, lineNumber) {}
        };
#define KARABO_IMAGE_TYPE_EXCEPTION(msg) \
    karabo::util::ImageTypeException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The InitException handles exceptions raised during any initialization procedures
         */
        class InitException : public Exception {
           public:
            InitException(const std::string& message, const std::string& filename, const std::string& function,
                          int lineNumber)
                : Exception(message, "Initialization failure", filename, function, lineNumber) {}

            InitException(const std::string& message, const std::string type, const std::string& filename,
                          const std::string& function, int lineNumber)
                : Exception(message, type, filename, function, lineNumber) {}
        };
#define KARABO_INIT_EXCEPTION(msg) karabo::util::InitException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The MemoryInitException handles exceptions raised during any memory initializations
         */
        class MemoryInitException : public InitException {
           public:
            MemoryInitException(const std::string& message, const std::string& filename, const std::string& function,
                                int lineNumber)
                : InitException(message, "Memory init Exception", filename, function, lineNumber) {}
        };
#define KARABO_MEMORY_INIT_EXCEPTION(msg) \
    karabo::util::MemoryInitException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        // ---- IO Exceptions

        /**
         * The IOException handles exceptions raised related to Input/Output routines
         */
        class IOException : public Exception {
           public:
            IOException(const std::string& message, const std::string& filename, const std::string& function,
                        int lineNumber)
                : Exception(message, "IO Exception", filename, function, lineNumber) {}

            IOException(const std::string& message, std::string type, const std::string& filename,
                        const std::string& function, int lineNumber)
                : Exception(message, type, filename, function, lineNumber) {}
        };
#define KARABO_IO_EXCEPTION(msg) karabo::util::IOException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The SystemException handles exceptions raised related to Input/Output routines
         */
        class SystemException : public Exception {
           public:
            SystemException(const std::string& message, const std::string& filename, const std::string& function,
                            int lineNumber)
                : Exception(message, "System Exception", filename, function, lineNumber) {}

            SystemException(const std::string& message, std::string type, const std::string& filename,
                            const std::string& function, int lineNumber)
                : Exception(message, type, filename, function, lineNumber) {}
        };
#define KARABO_SYSTEM_EXCEPTION(msg) karabo::util::SystemException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        class HdfIOException : public IOException {
           public:
            HdfIOException(const std::string& message, const std::string& filename, const std::string& function,
                           int lineNumber)
                : IOException(message, "HdfIOException", filename, function, lineNumber) {}

            void set(const std::string& message, const std::string& filename, const std::string& function,
                     int lineNumber) {
                m_exceptionInfo.message = message;
                m_exceptionInfo.filename = filename;
                m_exceptionInfo.function = function;
                m_exceptionInfo.lineNumber = toString(lineNumber);
            }
        };
#define KARABO_HDF_IO_EXCEPTION(msg) karabo::util::HdfIOException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * FileNotFoundIOException
         */
        class FileNotFoundIOException : public IOException {
           public:
            FileNotFoundIOException(const std::string& message, const std::string& filename,
                                    const std::string& function, int lineNumber)
                : IOException(message, "FileNotFound IOException", filename, function, lineNumber) {}
        };
#define KARABO_FILENOTFOUND_IO_EXCEPTION(msg) \
    karabo::util::FileNotFoundIOException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The MessageException handles exceptions that are caused during messaging
         */
        class MessageException : public Exception {
           public:
            MessageException(const std::string& message, const std::string& filename, const std::string& function,
                             int lineNumber)
                : Exception(message, "Message Exception", filename, function, lineNumber) {}
        };
#define KARABO_MESSAGE_EXCEPTION(msg) karabo::util::MessageException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * PythonException
         */
        class PythonException : public Exception {
           public:
            PythonException(const std::string& message, const std::string& detailsMsg, const std::string& filename,
                            const std::string& function, int lineNumber)
                : Exception(message, "Python Exception", filename, function, lineNumber, detailsMsg) {}
        };
#define KARABO_PYTHON_EXCEPTION(msg) karabo::util::PythonException(msg, "", __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)
#define KARABO_PYTHON_EXCEPTION2(msg, details) \
    karabo::util::PythonException(msg, details, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The CudaException handles exceptions that are caused by the NVIDIA CUDA runtime
         */
        class CudaException : public Exception {
           public:
            CudaException(const std::string& message, const std::string& filename, const std::string& function,
                          int lineNumber)
                : Exception(message, "GPU (NVIDIA CUDA) Exception", filename, function, lineNumber) {}
        };
#define KARABO_CUDA_EXCEPTION(msg) karabo::util::CudaException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The HardwareException handles exceptions that are caused by any connected hardware
         */
        class HardwareException : public Exception {
           public:
            HardwareException(const std::string& message, const std::string& filename, const std::string& function,
                              int lineNumber)
                : Exception(message, "Hardware Exception", filename, function, lineNumber) {}
        };
#define KARABO_HARDWARE_EXCEPTION(msg) karabo::util::HardwareException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The HardwareException handles exceptions that are caused by any connected hardware
         */
        class ReconfigureException : public Exception {
           public:
            ReconfigureException(const std::string& message, const std::string& filename, const std::string& function,
                                 int lineNumber)
                : Exception(message, "Reconfigure Exception", filename, function, lineNumber) {}
        };
#define KARABO_RECONFIGURE_EXCEPTION(msg) \
    karabo::util::ReconfigureException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The HardwareException handles exceptions that are caused by any connected hardware
         */
        class SignalSlotException : public Exception {
           public:
            SignalSlotException(const std::string& message, const std::string& filename, const std::string& function,
                                int lineNumber)
                : Exception(message, "SignalSlot Exception", filename, function, lineNumber) {}
        };
#define KARABO_SIGNALSLOT_EXCEPTION(msg) \
    karabo::util::SignalSlotException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The RemoteException represents an exception originating in a different device.
         * The parameters are refering to the exception on the other end.
         */
        class RemoteException : public Exception {
           public:
            RemoteException(const std::string& message, const std::string& device, const std::string& detailsMsg = "",
                            const std::string& filename = "", const std::string& function = "", int lineNumber = -1)
                : Exception(message, "Remote Exception from " + device, filename, function, lineNumber, detailsMsg) {}
        };

        /**
         * The NetworkException handles exceptions that are caused by network protocol related libraries (BoostAsio,
         * SNMP, ...)
         */
        class NetworkException : public Exception {
           public:
            NetworkException(const std::string& message, const std::string& filename, const std::string& function,
                             int lineNumber)
                : Exception(message, "Network Exception", filename, function, lineNumber) {}
        };
#define KARABO_NETWORK_EXCEPTION(msg) karabo::util::NetworkException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The TimeoutException handles exceptions that are caused by time out events
         */
        class TimeoutException : public Exception {
           public:
            TimeoutException(const std::string& message, const std::string& filename, const std::string& function,
                             int lineNumber)
                : Exception(message, "Timeout Exception", filename, function, lineNumber) {}
        };
#define KARABO_TIMEOUT_EXCEPTION(msg) karabo::util::TimeoutException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * DoocsException
         */
        class DoocsException : public Exception {
           public:
            DoocsException(const std::string& message, const std::string& filename, const std::string& function,
                           int lineNumber)
                : Exception(message, "DOOCS Exception", filename, function, lineNumber) {}
        };
#define KARABO_DOOCS_EXCEPTION(msg) karabo::util::DoocsException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

        /**
         * The LockException handles exceptions that result from not being able to acquire a lock
         */
        class LockException : public Exception {
           public:
            LockException(const std::string& message, const std::string& filename, const std::string& function,
                          int lineNumber)
                : Exception(message, "Lock Exception", filename, function, lineNumber) {}
        };
#define KARABO_LOCK_EXCEPTION(msg) karabo::util::LockException(msg, __FILE__, BOOST_CURRENT_FUNCTION, __LINE__)

    } // namespace util

} // namespace karabo

#endif /* KARABO_UTIL_EXCEPTION_HH */
