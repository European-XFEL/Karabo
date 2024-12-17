/*
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
/*
 * File:   FileTools.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 1, 2013, 6:18 PM
 */

#include <filesystem>
#include <karabo/log/Logger.hh>
#include <ostream>

#include "BinaryFileInput.hh"
#include "Input.hh"
#include "Output.hh"

#ifndef KARABO_IO_FILETOOLS_HH
#define KARABO_IO_FILETOOLS_HH

#include <karabo/util/karaboDll.hh>

namespace karabo {

    namespace io {

        /**
         * Load an object of class T from a file. The configuration determines which
         * access and de-serialzation methods to use.
         * @param object to load data into
         * @param filename of the file to load from. The input chose depends on the file
         *        extension:
         *        - .h5: Hdf5File
         *        - .bin: BinaryFile
         *        - .others: TextFile
         * @param config for the Input class that will be used
         */
        template <class T>
        inline void loadFromFile(T& object, const std::string& filename,
                                 const karabo::util::Hash& config = karabo::util::Hash()) {
            std::filesystem::path filepath(filename);
            std::string extension = filepath.extension().string().substr(1);
            boost::to_lower(extension);
            karabo::util::Hash h("filename", filepath.lexically_normal().string());
            h.merge(config);
            if (extension == "h5") {
                typename Input<T>::Pointer p = Input<T>::create("Hdf5File", h);
                p->read(object);
            } else if (extension != "bin") {
                typename Input<T>::Pointer p = Input<T>::create("TextFile", h);
                p->read(object);
            } else {
                typename Input<T>::Pointer p = Input<T>::create("BinaryFile", h);
                p->read(object);
            }
        }

        /**
         * Save an object of class T to a file. The configuration determines which
         * access and serialzation methods to use.
         * @param object to save
         * @param filename of the file to load from. The input chose depends on the file
         *        extension:
         *        - .h5: Hdf5File
         *        - .bin: BinaryFile
         *        - .others: TextFile
         * @param config for the Output class that will be used
         */
        template <class T>
        inline void saveToFile(const T& object, const std::string& filename,
                               const karabo::util::Hash& config = karabo::util::Hash()) {
            std::filesystem::path filepath(filename);
            std::string extension = filepath.extension().string().substr(1);
            std::filesystem::path directory = filepath.parent_path();

            // Create the directory and any parents if not existing already
            if (!directory.empty() && !std::filesystem::exists(directory)) {
                boost::system::error_code ec;
                std::filesystem::create_directories(directory, ec);
                if (ec) {
                    KARABO_LOG_FRAMEWORK_ERROR_C("karabo::io::saveToFile")
                          << "Failed to create directories: " << directory << ". code = " << ec.value() << " -- "
                          << ec.message();
                }
            }

            boost::to_lower(extension);
            karabo::util::Hash h("filename", filepath.lexically_normal().string());
            h.merge(config);

            if (extension == "h5") {
                typename Output<T>::Pointer p = Output<T>::create("Hdf5File", h);
                p->write(object);
            } else if (extension != "bin") {
                typename Output<T>::Pointer p = Output<T>::create("TextFile", h);
                p->write(object);
            } else {
                typename Output<T>::Pointer p = Output<T>::create("BinaryFile", h);
                p->write(object);
            }
        }

        /**
         * Save a buffer into a file
         * @param buffer
         * @param filename
         */
        inline void saveToFile(const std::vector<char>& buffer, const std::string& filename) {
            std::ofstream file(filename.c_str(), std::ios::binary);
            file.write(const_cast<const char*>(&buffer[0]), buffer.size());
            file.close();
        }

        /**
         * Load a buffer from a file
         * @param buffer
         * @param filename
         */
        inline void loadFromFile(std::vector<char>& buffer, const std::string& filename) {
            std::ifstream file(filename.c_str(), std::ios::binary);
            // fileContents.reserve(10000);
            buffer.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        }

        /**
         * Get the I/O datatype for type T in terms of the Karabo class id
         * @return
         */
        template <class T>
        inline std::string getIODataType() {
            using namespace karabo::util;
            return T::classInfo().getClassId();
        }

        template <>
        inline std::string getIODataType<std::vector<char> >() {
            return "Raw";
        }

    } // namespace io
} // namespace karabo

#ifndef __SO__
extern template void karabo::io::loadFromFile<karabo::util::Hash>(
      karabo::util::Hash& object, const std::string& filename, const karabo::util::Hash& config = karabo::util::Hash());
extern template void karabo::io::saveToFile<karabo::util::Hash>(
      const karabo::util::Hash& object, const std::string& filename,
      const karabo::util::Hash& config = karabo::util::Hash());
extern template std::string karabo::io::getIODataType<karabo::util::Hash>();
extern template void karabo::io::loadFromFile<karabo::util::Schema>(
      karabo::util::Schema& object, const std::string& filename,
      const karabo::util::Hash& config = karabo::util::Hash());
extern template void karabo::io::saveToFile<karabo::util::Schema>(
      const karabo::util::Schema& object, const std::string& filename,
      const karabo::util::Hash& config = karabo::util::Hash());
extern template std::string karabo::io::getIODataType<karabo::util::Schema>();
#endif

#endif
