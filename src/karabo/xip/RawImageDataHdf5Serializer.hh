/* 
 * File:   RawImageDataHdf5SerializerOutput.hh
 * Author: haufs
 *
 * Created on August 6, 2014, 2:16 PM
 */

/*
    * For RawImageData objects information is stored in the Hdf5 files in such a way,
    * that it remains readable by external tools, but can also be de-serialized back.
    * 
    * For this images are stored in n-dimensional datasets, with the first dimension referring
    * to each subsequent image. Note: the ordering in RawImageData objects the ordering is exactly 
    * reversed. Here the last dimension should be used for subsequent images in order for slicing
    * by flat memory boundaries to properly work.
    * 
    * Serialization/Deserialization functionality is maintained by two helper tables: one records
    * insertion order of images within one call to save. The second relates headers to the insertion
    * call they originate from.
    * 
    * Headers are saved as XML serialized strings. For deserialization these headers are read in and
    * attached to the corresponding image data. Any tables in dataset format are provided for convenient
    * access using external tools, without the need of transferring a header to a karabo Hash first.
    * 
    * The tile information is stored in a tileId table in the identifiers section if provided. In such cases
    * a group geometry contains information of the geometry in a hierarchical mapping
    * 
    * On each hierarchical level alignment information can be provided. It consists of offset, which are
    * relative coordinate system of the parent of the corresponding node, as well as rotations, also relative 
    * to a nodes parent.
    * 
    * At the root level additional identifiers such as history, instrument and detector conditions and operator
    * can be provided. These are serialized from appropriate Karabo hash structures which should be located in
    * the image header under the respective keys defined by the RawImageData (or subformat's) standard headers.
    * 
    * Note that size() does not return the total number of images stored in this file, but the number of insertions.
    * If multiple images are inserted at the same time (number of images) >> (number of insertions). This also
    * implies that a single given image can not be selected by deserialization alone. Instead the block of images
    * which correspond to a the insertion operation containing this images is return and from this block the image
    * can be accessed.
    * 
    * Known limitations:
    * 
    * - changing the image data type when appending images to the same dataset is not supported!
    * - changing image dimensions (other than number of images per insertion) is not supported!
    * - a header may not contain any image type objects (to be on the safe side use only primitive and vector of
    *   primitive types.
*/

#ifndef RAWIMAGEDATAHDF5FILEOUTPUT_HH
#define	RAWIMAGEDATAHDF5FILEOUTPUT_HH

#include "RawImageData.hh"
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <karabo/io/h5/ErrorHandler.hh>


#include <iostream>
#include <hdf5/hdf5.h>
#include <karabo/io/Hdf5Serializer.hh>
#include "FromChannelSpace.hh"
#include <karabo/io/h5/ErrorHandler.hh>
#include <karabo/io/h5/TypeTraits.hh>
#include <karabo/io/h5/VLArray.hh>
#include <karabo/io/h5/Format.hh>
#include <karabo/io/h5/File.hh>
#include <karabo/io/h5/Table.hh>
#include <karabo/io/HashHdf5Serializer.hh>
#include <karabo/io/HashXmlSerializer.hh>
#include <karabo/xip/FromChannelSpace.hh>

#include <map>

/**
 * The main European XFEL namespace
 */
namespace karabo {

    namespace xip {

        class RawImageDataHdf5Serializer : public karabo::io::Hdf5Serializer<karabo::xip::RawImageData> {

            /**
             * This struct is filled by readPaths function
             */
            struct opData {
                unsigned        recurs;         /* Recursion level.  0=root */
                struct opData   *prev;          /* Pointer to previous opdata */
                haddr_t         addr;           /* Group address */
                std::string path;
                std::map<long long, std::string>* tileId2path;
            };
            
        public:

            KARABO_CLASSINFO(RawImageDataHdf5Serializer, "h5", "1.0")

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this image (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
            }

            RawImageDataHdf5Serializer(const karabo::util::Hash& input);
            virtual ~RawImageDataHdf5Serializer();

            void save(const karabo::xip::RawImageData& image, hid_t h5fileId, const std::string& groupName);
   
            void load(karabo::xip::RawImageData& image, hid_t h5file, const std::string& groupName);

            unsigned long long size(hid_t h5fileId, const std::string & groupName);
            
            void onCloseFile();
            
        private:
            
            
          
            /**
             * Returns the number of entries in a data set located at a given path
             */
            unsigned long long getTableSize(hid_t fileId, const std::string & fullPath);
            
            
            /**
             * The following three functions create formats required by the karabo::h5 API
             */
            karabo::io::h5::Format::Pointer headerVectorFormat();
            
            //karabo::io::h5::Format::Pointer idVectorFormat(std::string  key);
            
            karabo::io::h5::Format::Pointer imageFormat(const std::vector<unsigned long long> & dims, const int & cs);
            
            karabo::io::h5::Format::Pointer generateFormatFromHash(const karabo::util::Hash & h, const std::string & topKey);
            
            /**
             * If no tileIds to read in are given in the header this function creates a list of existing tileIds in the file.
             */
            std::vector<long long> createTileIds();
           
            karabo::util::Hash vectorizeEntries(const karabo::util::Hash & h, const size_t  & nImages);
          
            
        private:
            
            //writing (+reading))
            std::string m_basePath;
            bool m_structureWrote;
            unsigned long long m_datasetId;
            karabo::io::HashHdf5Serializer m_hashSerializer;
            karabo::io::HashXmlSerializer m_hashXmlSerializer;
          
            
            karabo::io::h5::Table::Pointer m_imageTable;
            karabo::io::h5::Table::Pointer m_identifiersTable;
            karabo::io::h5::Table::Pointer m_conditionsTable;
            karabo::io::h5::Table::Pointer m_headersTable;
            
            std::set<long long> m_tileIdHeaderWritten;
            
            long long m_lastIndex;
            
            //reading
            bool m_structureRead;
            bool m_h5structureRead;
           
            hid_t  m_h5fileId;
            karabo::io::h5::File::Pointer m_h5file;
            bool m_writeAccess;
            std::string m_fileName;
            
        };
        
        
    }
}

#endif	/* RAWIMAGEDATAHDF5FILEOUTPUT_HH */

