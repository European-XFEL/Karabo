/* 
 * File:   FileTools.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 1, 2013, 6:18 PM
 */

#include <boost/filesystem.hpp>
#include <ostream>

#include "Input.hh"
#include "Output.hh"
#include "BinaryFileInput.hh"

#ifndef KARABO_IO_FILETOOLS_HH
#define	KARABO_IO_FILETOOLS_HH

#include <karabo/util/karaboDll.hh>

namespace karabo {

    namespace io {

        template <class T>
        inline void loadFromFile(T& object, const std::string& filename, const karabo::util::Hash& config = karabo::util::Hash()) {
            boost::filesystem::path filepath(filename);
            std::string extension = filepath.extension().string().substr(1);
            boost::to_lower(extension);
            karabo::util::Hash h("filename", filepath.normalize().string());
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

        template <class T>
        inline void saveToFile(const T& object, const std::string& filename, const karabo::util::Hash& config = karabo::util::Hash()) {
            boost::filesystem::path filepath(filename);
            std::string extension = filepath.extension().string().substr(1);
            boost::to_lower(extension);
            karabo::util::Hash h("filename", filepath.normalize().string());
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

        inline void saveToFile(const std::vector<char>& buffer, const std::string& filename) {
            std::ofstream file(filename.c_str(), std::ios::binary);
            file.write(const_cast<const char*> (&buffer[0]), buffer.size());
            file.close();
        }

        inline void loadFromFile(std::vector<char>& buffer, const std::string& filename) {
            std::ifstream file(filename.c_str(), std::ios::binary);
            //fileContents.reserve(10000);
            buffer.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        }

        template <class T>
        inline std::string getIODataType() {
            using namespace karabo::util;
            return T::classInfo().getClassId();
        }

        template<>
        inline std::string getIODataType<std::vector<char> >() {
            return "Raw";
        }

    }
}

#ifndef __SO__
extern template void karabo::io::loadFromFile<karabo::util::Hash>(karabo::util::Hash& object, const std::string& filename, const karabo::util::Hash& config = karabo::util::Hash());
extern template void karabo::io::saveToFile<karabo::util::Hash>(const karabo::util::Hash& object, const std::string& filename, const karabo::util::Hash& config = karabo::util::Hash());
extern template std::string karabo::io::getIODataType<karabo::util::Hash>();
extern template void karabo::io::loadFromFile<karabo::util::Schema>(karabo::util::Schema& object, const std::string& filename, const karabo::util::Hash& config = karabo::util::Hash());
extern template void karabo::io::saveToFile<karabo::util::Schema>(const karabo::util::Schema& object, const std::string& filename, const karabo::util::Hash& config = karabo::util::Hash());
extern template std::string karabo::io::getIODataType<karabo::util::Schema>();
#endif

#endif
