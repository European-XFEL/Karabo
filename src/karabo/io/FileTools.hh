/* 
 * File:   FileTools.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 1, 2013, 6:18 PM
 */

#include <boost/filesystem.hpp>

#include "Input.hh"

#ifndef KARABO_IO_FILETOOLS_HH
#define	KARABO_IO_FILETOOLS_HH

#include <karabo/util/karaboDll.hh>

namespace karabo {
    
    namespace io {
        
        template <class T>
        inline void loadFromFile(T& object, const std::string& filename) {
            boost::filesystem::path filepath(filename);
            std::string extension = filepath.extension().string().substr(1);
            boost::to_lower(extension);
            if (extension != "bin") {
                typename Input<T>::Pointer p = Input<T>::create("TextFile", karabo::util::Hash("filename", filepath.normalize().string()));
                p->read(object);
            } else {
                //typename TextFileInput<T>::Pointer p = TextFileInput<T>::create("BinaryFile", karabo::util::Hash("filename", filepath.normalize().string()));
               // p->read(object);
            }
        }
    }
}
#endif
