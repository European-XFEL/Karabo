
/*
 * $Id$
 *
 * Author: <steffen.hauf@xfel.eu>
 * 
 * Created on August 6, 2014, 5:53 PM
 */



#include "RawImageDataHdf5Serializer.hh"
#include "karabo/io/Hdf5FileOutput.hh"
#include "karabo/io/Hdf5FileInput.hh"
#include <karabo/io/CppInputHandler.hh>

#include <boost/serialization/shared_ptr.hpp>

#include <memory>

namespace karabo {
    
    
    
    namespace xip {

        
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Hdf5Serializer<RawImageData>, RawImageDataHdf5Serializer)
                
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<RawImageData >, karabo::io::Hdf5FileOutput<RawImageData>)
        KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<RawImageData >, karabo::io::Hdf5FileInput<RawImageData>)
                
        
        //KARABO_REGISTER_IN_FACTORY_1(karabo::io::InputHandler, karabo::io::CppInputHandler<karabo::io::Input<RawImageData> > , karabo::io::AbstractInput::Pointer);
                
   
                
        RawImageDataHdf5Serializer::RawImageDataHdf5Serializer(const karabo::util::Hash& input) : 
            karabo::io::Hdf5Serializer<karabo::xip::RawImageData>(input), m_basePath(""), m_datasetId(0), m_hashSerializer(input),
            m_hashXmlSerializer(karabo::util::Hash("indentation", -1, "writeDataTypes", true, "readDataTypes", true, "insertXmlNamespace", false, "xmlns", "http://xfel.eu/config", "prefix", "KRB_")),
            m_h5structureRead(false), m_structureRead(false){
               
                
        }
        
        RawImageDataHdf5Serializer::~RawImageDataHdf5Serializer() {
        }
        
        
        void RawImageDataHdf5Serializer::save(const karabo::xip::RawImageData& image, hid_t h5fileId, const std::string& groupName){
                
                //get index and data set id from groupName
                unsigned long long splitterPos = groupName.find_last_of("/");
                std::string basePath = "/";
                if(splitterPos > 0){
                   basePath = groupName.substr(0, splitterPos);
                }
                 
                
                //we do not need to know that dataset id when writing images
                //unsigned long long datasetId = boost::lexical_cast<unsigned long long>(groupName.substr(splitterPos+1, groupName.size()-splitterPos-1));
            
                //map to high level interface
                bool openTables = false;
                if(m_h5fileId != h5fileId){
                    m_h5file.reset(new karabo::io::h5::File(h5fileId), boost::serialization::null_deleter());
                    m_h5fileId = h5fileId;
                    m_writeAccess = true;
                    if(m_fileName != m_h5file->getName()){
                        m_basePath = "";
                        
                    } else {
                        openTables = true;
                        
                    }
                    m_fileName = m_h5file->getName();
                }
                
               
                
                karabo::util::Hash header = image.getHeader();
              
                if(m_basePath != basePath){ //image header can trigger creation of new dataset
                    //create groups
                   
                    //check if a path structure already exists:
                    openTables = true;
                    int status = H5Eset_auto(NULL, NULL, NULL);
                    status = H5Gget_objinfo(h5fileId, basePath.c_str(), 0, NULL);
                    if(status != 0) openTables = false;
                  
                    
                    if(!openTables){
                        //these are optional
                        if(header.has("geometry")){
                            karabo::util::Hash geometry = header.get<karabo::util::Hash>("geometry");
                            m_hashSerializer.save(geometry, m_h5fileId, basePath+"/geometry");
                        }
                        
                        if(header.has("passport")){
                            karabo::util::Hash identifiers = header.get<karabo::util::Hash>("passport");
                            m_hashSerializer.save(identifiers, m_h5fileId, basePath+"/passport");
                        }

                        if(header.has("initialConditions")){
                            karabo::util::Hash initialConditions = header.get<karabo::util::Hash>("initialConditions");
                            m_hashSerializer.save(initialConditions, m_h5fileId, basePath+"/initialConditions");
                        }

                        if(header.has("history")){
                            karabo::util::Hash history = header.get<karabo::util::Hash>("history");
                            m_hashSerializer.save(history, m_h5fileId, basePath+"/history");
                        }
                      
                        //clear table map
                        m_imageTable.reset();
                        m_conditionsTable.reset();
                        m_identifiersTable.reset();
                        m_lastIndex = -1;
                    } 
                    m_structureWrote = true;
                    m_basePath = basePath;
                    
                    
                }
               
                std::vector<unsigned long long> dims =  image.getDimensions().toVector();
                unsigned long long tileDimensionIs = header.get<unsigned long long>("tileDimensionIs");
                
               
                unsigned long long numImages = 1;
                
               
                if(image.getDimensions().rank() >= (tileDimensionIs+1)) {
                    numImages = dims[tileDimensionIs];
                    std::swap(dims[0], dims[tileDimensionIs]);
                    dims[0] = 1;
                };
                
              
            
                karabo::util::Hash identifiers = vectorizeEntries(header.get<karabo::util::Hash>("identifiers"), numImages);
                karabo::util::Hash conditions = vectorizeEntries(header.get<karabo::util::Hash>("conditions"), numImages);
             
                std::cout<<"Vectorized entries"<<std::endl;
                
                //data of this image stack
                const char * dataPtr = image.getDataPointer();
                      
                //convert header to vector char
                std::string headerBuffer;
                m_hashXmlSerializer.save(header, headerBuffer);
                

                
            
                std::string path = basePath;

                //check if format exists, if not create table with this format

                if(!m_imageTable && !openTables){

                    m_imageTable =  m_h5file->createTable(path+"/images", imageFormat(dims, image.getChannelSpace()));
                    m_identifiersTable = m_h5file->createTable(path+"/identifiers", generateFormatFromHash(identifiers, "identifiers"));
                    m_conditionsTable = m_h5file->createTable(path+"/conditions", generateFormatFromHash(conditions, "conditions"));
                    m_headersTable = m_h5file->createTable(path+"/headers", headerVectorFormat());
                  
                  

                } else if(openTables){

                    unsigned long long currentImages = getTableSize(h5fileId, path+"/images/image");

                    m_imageTable =  m_h5file->getTable(path+"/images", imageFormat(dims, image.getChannelSpace()), 0);
                    m_identifiersTable = m_h5file->getTable(path+"/identifiers");//, generateFormatFromHash(identifiers[i], "identifiers"));
                    m_conditionsTable = m_h5file->getTable(path+"/conditions");//, generateFormatFromHash(conditions[i], "conditions"));
                    m_headersTable = m_h5file->getTable(path+"/headers");//, headerVectorFormat());
                }

                //now write out image
                karabo::util::Hash imageHash;
                karabo::util::Hash& images = imageHash.bindReference<karabo::util::Hash>("images");


                #define _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(inType, castType) case karabo::util::Types::inType: \
                            images.set("image", const_cast<castType*>(reinterpret_cast<const castType*>(dataPtr))); break;

                karabo::util::Types::ReferenceType imType = karabo::xip::FromChannelSpace::from(image.getChannelSpace());

                switch(imType){
                    //_KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(BOOL, bool);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(CHAR, char);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(INT8, signed char);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(UINT8, unsigned char);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(INT16, signed short);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(UINT16, unsigned short);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(INT32, signed int);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(UINT32, unsigned int);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(INT64, signed long);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(UINT64, unsigned long);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(FLOAT, float);
                    _KARABO_CAST_AND_SET_IMAGE_DATA_POINTER(DOUBLE, double);
                }

            
                images.setAttribute("image", "dims", dims);
                m_imageTable->write(imageHash, m_imageTable->size(), numImages);
                
                std::cout<<"Wrote image"<<std::endl;

                m_conditionsTable->write(conditions, m_conditionsTable->size(), numImages);

                std::cout<<"Wrote conditions"<<std::endl;
                
                m_identifiersTable->write(identifiers, m_identifiersTable->size(), numImages);
                
                std::cout<<"Wrote identifiers"<<std::endl; 
                
                karabo::util::Hash headerHash;
                
                headerHash.set("tileDimensionIs", tileDimensionIs);
                
                m_lastIndex += numImages;
                
                headerHash.set("ids", m_lastIndex); //set this to the index of the last image entry this header applies to
                headerHash.set("headers", headerBuffer);
         
                m_headersTable->append(headerHash);
                
                std::cout<<"Wrote headers"<<std::endl;
            
                
                
            }
        
        karabo::util::Hash RawImageDataHdf5Serializer::vectorizeEntries(const karabo::util::Hash & h, const size_t  & nImages){
            
            karabo::util::Hash vH;
            for(karabo::util::Hash::const_iterator it = h.begin(); it != h.end(); ++it){
                karabo::util::Types::ReferenceType type = it->getType();
                
                if(karabo::util::Types::isVector(type)){
                    //check if the vector has the right length
                    bool lengthIsOkay = true;
                    #define _KARABO_HDF5SER_VECTORIZE_VECTOR(type, kType)\
                           case karabo::util::Types::kType:\
                                if(it->getValue<type >().size() == nImages){\
                                    vH.set(it->getKey(), it->getValue<type >()); \
                                } else if (it->getValue<type >().size() == 1){\
                                    vH.set(it->getKey(), type(nImages, it->getValue<type >()[0])); \
                                } else {\
                                    lengthIsOkay = false;\
                                }\
                                break;
                                
                    switch(type){
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<bool>, VECTOR_BOOL)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<char>, VECTOR_CHAR)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<unsigned char>, VECTOR_UINT8)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<char>, VECTOR_INT8)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<unsigned short>, VECTOR_UINT16)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<short>, VECTOR_INT16)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<unsigned int>, VECTOR_UINT32)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<int>, VECTOR_INT32)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<unsigned long long>, VECTOR_UINT64)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<long long>, VECTOR_INT64)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<float>, VECTOR_FLOAT)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<double>, VECTOR_DOUBLE)
                        _KARABO_HDF5SER_VECTORIZE_VECTOR(std::vector<std::string>, VECTOR_STRING)
                        default:
                            //clarify what to do. Exception or warning?
                            KARABO_LOG_FRAMEWORK_WARN<<"Can't handle type (VECTOR) of "<<it->getKey()<<" for Hdf5 serialization. Skipping this entry.";
                    }
                    if(! lengthIsOkay){
                        //clarify what to do. Exception or warning?
                    }
                } else if (karabo::util::Types::isSimple(type)){
                    //extend to vector size
                    #define _KARABO_HDF5SER_VECTORIZE_SIMPLE(type, kType)\
                           case karabo::util::Types::kType:\
                                vH.set(it->getKey(), std::vector<type>(nImages, it->getValue<type >()));\
                                break;

                    switch(type){
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(bool, BOOL)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(char, CHAR)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(char, INT8)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(unsigned char, UINT8)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(short, INT16)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(unsigned short, UINT16)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(int, INT32)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(unsigned int, UINT32)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(long long, INT64)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(unsigned long long, UINT64)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(float, FLOAT)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(double, DOUBLE)
                        _KARABO_HDF5SER_VECTORIZE_SIMPLE(std::string, STRING)
                        default:
                            //clarify what to do. Exception or warning?
                            KARABO_LOG_FRAMEWORK_WARN<<"Can't handle type (SIMPLE) of "<<it->getKey()<<" for Hdf5 serialization. Skipping this entry.";
                            break;
                    }
                } else {
                    
                            //clarify what to do. Exception or warning?
                            KARABO_LOG_FRAMEWORK_WARN<<"Can't handle type (NOT SIMPLE or VECTOR) of "<<it->getKey()<<" for Hdf5 serialization. Skipping this entry.";
                }
            }
            
            return vH;
        }
        
        
        
        void RawImageDataHdf5Serializer::load(karabo::xip::RawImageData& image, hid_t h5file, const std::string& groupName){
                
             //get index and data set id from groupName
             unsigned long long splitterPos = groupName.find_last_of("/");
                std::string basePath = "/";
                if(splitterPos > 0){
                   basePath = groupName.substr(0, splitterPos);
             }
                
            //idx is NOT the index of the image slab, but the index in the header table, i.e. of the originally written chunk!
            unsigned long long idx = boost::lexical_cast<unsigned long long>(groupName.substr(splitterPos+1, groupName.size()-splitterPos));

            //map to high level interface
            if(m_h5fileId != h5file){
              
                m_imageTable.reset();
                m_headersTable.reset();
                m_h5file.reset(new karabo::io::h5::File(h5file), boost::serialization::null_deleter());
                m_writeAccess = false;
                m_structureRead = false;
                m_h5structureRead = false;
                m_h5fileId = h5file;
            }

            if(m_basePath != basePath){
                
                m_imageTable.reset();
                m_headersTable.reset();
                m_h5structureRead = false;
                m_structureRead = false;
                m_basePath = basePath;
            }

            karabo::util::Hash header = image.getHeader();

            //read in structure in case it isn't current
            //basically we need to map tileIds to the H5 paths here and recreate the geometry
            if(!m_structureRead){
                
                m_imageTable = m_h5file->getTable(m_basePath+"/images");
                m_headersTable = m_h5file->getTable(m_basePath+"/headers");
                m_identifiersTable = m_h5file->getTable(m_basePath+"/identifiers");
                m_structureRead = true;
            }

            //now check which tiles to read
            //if no tileIds are given we assume to read all tiles
            std::set<long long> tileIds;
            std::vector<long long> tileIdsVec;
            if(header.has("identifiers.tileIds")){
                header.get("identifiers.tileIds", tileIdsVec);
                if(tileIdsVec[0] != -1){
                    for(size_t i = 0; i != tileIdsVec.size(); ++i){
                        tileIds.insert(tileIdsVec[i]);
                    }
                } 
            }
            
           

            //read the data
            unsigned int byteWidth = 1;
            
            
            std::vector<unsigned long long> retreivedDims;
            std::string newHeaderSerialized;



            std::string path = m_basePath; //m_tileId2H5Path[*tid];


            //check which images are to be read for this index

            //hash to bind to
            karabo::util::Hash h;
            m_headersTable->bind(h);
            m_headersTable->read(idx);

            long long upperImageIndex;
            long long lowerImageIndex(-1);

            h.get("ids", upperImageIndex);


            newHeaderSerialized = h.get<std::string>("headers");
            unsigned long long tileDimensionIs = h.get<unsigned long long>("tileDimensionIs");

            if(idx > 0) {
                m_headersTable->read(idx-1);
                h.get("ids", lowerImageIndex);
            }


            //in case no images were recorded the upperImage index is set to 0 and thus a subtraction of previous index will yield negative lenght
            //0 length has to be allowed for the case of a single image written at the beginning
            if(upperImageIndex-lowerImageIndex < 0 ) return;

            karabo::util::Hash hIm("dims", std::vector<unsigned long long>(upperImageIndex- lowerImageIndex,4));
         
            m_imageTable->bind(hIm, upperImageIndex - lowerImageIndex);
            m_imageTable->read(lowerImageIndex+1, upperImageIndex- lowerImageIndex);

            //get tile Ids
            karabo::util::Hash identH;
            m_identifiersTable->bind(identH, upperImageIndex- lowerImageIndex);
            m_identifiersTable->read(lowerImageIndex+1, upperImageIndex- lowerImageIndex);
            std::vector<long long> tilesInData = identH.get<std::vector<long long> >("tileIds");

            unsigned long long nInserts = 0;



            retreivedDims = hIm.getAttribute<std::vector<unsigned long long> >("image", "dims");


            std::vector<char> byteVector;

            #define _KARABO_ACCUMULATE_IMAGE_DATA(cType, kType, dType)\
                if(cType == karabo::util::Types::VECTOR_##kType){ \
                    const std::vector<dType> & app = hIm.get<std::vector<dType> >("image");\
                    byteWidth = sizeof(dType);\
                    size_t imSize = app.size()/tilesInData.size();\
                    if(tileIds.size() != 0){\
                        for(size_t i = 0; i != tilesInData.size(); ++i){\
                            if(tileIds.find(tilesInData[i]) != tileIds.end()){\
                                size_t oldSize = byteVector.size();\
                                byteVector.resize(oldSize+imSize*byteWidth);\
                                std::memcpy(&byteVector[oldSize], reinterpret_cast<const char*>(&app[i*imSize]), imSize*byteWidth);\
                            }\
                        }\
                    } else {\
                        byteVector.resize(app.size()*byteWidth);\
                        std::memcpy(&byteVector[0], reinterpret_cast<const char*>(&app[0]), app.size()*byteWidth);\
                    }\
                };

            karabo::util::Types::ReferenceType imageType = hIm.getType("image");

            //_KARABO_ACCUMULATE_IMAGE_DATA(imageType, BOOL, bool);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, CHAR, char);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, INT8, signed char);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, UINT8, unsigned char);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, INT16, signed short);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, UINT16, unsigned short);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, INT32, signed int);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, UINT32, unsigned int);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, INT64, signed long);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, UINT64, unsigned long);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, FLOAT, float);
            _KARABO_ACCUMULATE_IMAGE_DATA(imageType, DOUBLE, double);
                
   
            karabo::util::Hash newHeader;
            m_hashXmlSerializer.load(newHeader, newHeaderSerialized);
            
            newHeader.set("identifiers.tileIds", tilesInData);
            header.merge(newHeader);
            image.setHeader(header);
            
            image.setData(byteVector, true);
            
            //calculate resulting dimensions
            retreivedDims[tileDimensionIs] = upperImageIndex- lowerImageIndex;
            karabo::util::Dims dims(retreivedDims);
            image.setDimensions(dims);

        }
        
        //we need to return the size maximum size of the header indexes across all tiles stored in this file
        unsigned long long RawImageDataHdf5Serializer::size(hid_t h5fileId, const std::string & groupName){
            
            //get index and data set id from groupName
             unsigned long long splitterPos = groupName.find_last_of("/");
             std::string basePath = "/";
             if(splitterPos > 0){
                   basePath = groupName.substr(0, splitterPos);
             }
                
            
            if(!m_h5structureRead) {
    
                int status = H5Eset_auto(NULL, NULL, NULL);
                status = H5Gget_objinfo(h5fileId, basePath.c_str(), 0, NULL);
                if(status < 0) return 0;
                m_h5structureRead = true;
            }
            
            
            return getTableSize(h5fileId, basePath+"/headers/ids");
        }
        
        void RawImageDataHdf5Serializer::onCloseFile(){
            if(m_writeAccess) KARABO_CHECK_HDF5_STATUS(H5Fflush(m_h5fileId, H5F_SCOPE_LOCAL));
            m_h5file->close();
            m_h5file.reset();
            m_h5fileId = -1;
            m_structureRead = false;
            m_fileName = "";
        }
       
        
        karabo::io::h5::Format::Pointer RawImageDataHdf5Serializer::headerVectorFormat(){
                
                karabo::io::h5::Format::Pointer format = karabo::io::h5::Format::createEmptyFormat();
                
                
                karabo::util::Hash c(
                       "h5path", "",
                       "h5name", "headers",
                       /*"dims", std::vector<unsigned long long>(1, 1),
                       "type", karabo::util::ToLiteral::to<karabo::util::Types::VECTOR_STRING>(),*/
                       "key", "headers"
                       );
           

                karabo::io::h5::Element::Pointer e = karabo::io::h5::Element::create("STRING", c);
                format->addElement(e);
                
                karabo::util::Hash c2(
                       "h5path", "",
                       "h5name", "ids",
                       "key", "ids"
                       );
            
                
                karabo::io::h5::Element::Pointer e2 = karabo::io::h5::Element::create(karabo::util::ToLiteral::to<karabo::util::Types::INT64>(), c2);
                format->addElement(e2);
                
                 karabo::util::Hash c3(
                       "h5path", "",
                       "h5name", "tileDimensionIs",
                       "key", "tileDimensionIs"
                       );
                               
                karabo::io::h5::Element::Pointer e3 = karabo::io::h5::Element::create(karabo::util::ToLiteral::to<karabo::util::Types::UINT64>(), c3);
                format->addElement(e3);
                
                return format;

            }
       
        
        karabo::io::h5::Format::Pointer RawImageDataHdf5Serializer::generateFormatFromHash(const karabo::util::Hash & h, const std::string & topKey){
                
                //TODO: Add support for vectors of vectors using VLARRAYs
            
                karabo::io::h5::Format::Pointer format = karabo::io::h5::Format::createEmptyFormat();
                for(karabo::util::Hash::const_iterator it = h.begin(); it != h.end(); ++it){
                    karabo::util::Types::ReferenceType type = it->getType();
                    if(karabo::util::Types::isSimple(type)){
                        karabo::util::Hash c(
                               "h5path", "",
                               "h5name", it->getKey(),
                               "key", it->getKey()
                               );
                  
                        karabo::io::h5::Element::Pointer e = karabo::io::h5::Element::create(karabo::util::Types::to<karabo::util::ToLiteral>(type), c);
                        format->addElement(e);
                    } else if( karabo::util::Types::isVector(type)){
                        karabo::util::Hash c(
                               "h5path", "",
                               "h5name", it->getKey(),
                               "key", it->getKey()
                               );
                     

                        karabo::io::h5::Element::Pointer e = karabo::io::h5::Element::create(karabo::util::Types::to<karabo::util::ToLiteral>(type).replace(0,7,""), c);
                        format->addElement(e);
                    }
                }
                return format;

        }
        
        karabo::io::h5::Format::Pointer RawImageDataHdf5Serializer::imageFormat(const std::vector<unsigned long long> & dims, const int & cs){
                
               
                
                karabo::io::h5::Format::Pointer format = karabo::io::h5::Format::createEmptyFormat();

                karabo::util::Types::ReferenceType imType = karabo::xip::FromChannelSpace::from(cs);
                std::string imPtrType;
                std::string imVtrType;
                
                #define _KARABO_REFERENCE_TYPE_TO_PTR(inType, resultVar1, resultVar2) case karabo::util::Types::inType: resultVar1 = karabo::util::ToLiteral::to<karabo::util::Types::PTR_##inType>(); resultVar2 = karabo::util::ToLiteral::to<karabo::util::Types::VECTOR_##inType>(); break;
     
                switch(imType){
                    
                    _KARABO_REFERENCE_TYPE_TO_PTR(BOOL, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(CHAR, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(INT8, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(UINT8, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(INT16, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(UINT16, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(INT32, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(UINT32, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(INT64, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(UINT64, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(FLOAT, imPtrType, imVtrType)
                    _KARABO_REFERENCE_TYPE_TO_PTR(DOUBLE, imPtrType, imVtrType)
                    default:
                        throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
                }
                
                
                karabo::util::Hash c(
                       "h5path", "",
                       "h5name", "image",
                       "dims", dims,
                       "type", imPtrType,
                       "key", "images.image"
                       );
                
                
                // define "dims" attribute for the image as a vector of 3 elements: "module width", "module height", "number of modules"
                c.set("attributes[0].VECTOR_UINT64.h5name", "dims");
                c.set("attributes[0].VECTOR_UINT64.dims", boost::lexical_cast<std::string>(dims.size()));

                karabo::io::h5::Element::Pointer e = karabo::io::h5::Element::create(imVtrType, c);
                format->addElement(e);
                
                return format;

            }
        
        
        unsigned long long RawImageDataHdf5Serializer::getTableSize(hid_t fileId, const std::string & fullPath){
            hid_t dset = H5Dopen(fileId, fullPath.c_str(), H5P_DEFAULT);
            hid_t dspace = H5Dget_space(dset);
            const int ndims = H5Sget_simple_extent_ndims(dspace);
            hsize_t dims[ndims];
            KARABO_CHECK_HDF5_STATUS(H5Sget_simple_extent_dims(dspace, dims, NULL));
            KARABO_CHECK_HDF5_STATUS(H5Sclose(dspace));
            KARABO_CHECK_HDF5_STATUS(H5Dclose(dset));
            return dims[0];
        }
      
        

    }
}


