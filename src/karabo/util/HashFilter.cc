/* 
 * $ID$
 * 
 * File:   hashFilter.cc
 * Author: <krzysztof.wrona@xfel.eu>
 * 
 * Created on April 12, 2013, 11:50 AM
 */

#include "Schema.hh"
#include "FromLiteral.hh"
#include "HashFilter.hh"
#include "StringTools.hh"
#include <set>

namespace karabo {
    namespace util {


        void HashFilter::byTag(const Schema& schema, const Hash& input, Hash& result, const std::string& tags, const std::string& sep) {

            const Hash& master = schema.getParameterHash();
            std::set<std::string> tagSet = fromString<std::string, std::set>(tags, sep);
            for (Hash::const_iterator it = input.begin(); it != input.end(); ++it) {
                r_byTag(master, *it, result, it->getKey(), tagSet);
            }
        }


        void HashFilter::r_byTag(const Hash& master, const Hash::Node& inputNode, Hash& result, const std::string& path, const std::set<std::string>& tags) {

            if (!master.has(path)) {
                //std::clog << "Path: " << path << " does not exist in schema" << std::endl;
                return;
            }

            if (inputNode.is<Hash>()) {

                // if the tag was found in the HASH copy complete Hash and return
                // otherwise process the Hash further
                if (processNode(master, inputNode, result, path, tags) == true) return;

                const Hash& input = inputNode.getValue<Hash>();
                for (Hash::const_iterator it = input.begin(); it != input.end(); ++it) {
                    std::string newPath = path + "." + it->getKey();
                    r_byTag(master, *it, result, newPath, tags);
                }
            } else if (inputNode.is<std::vector<Hash> >()) {

                // if the tag was found in the vector<HASH> (defined for LIST_ELEMENT) copy complete vector<Hash> and return
                // otherwise process the Hash further
                if (processNode(master, inputNode, result, path, tags) == true) {
                    //std::clog << "Copy entire vector<Hash> " << path << std::endl;
                    return;
                }
                // Check if the path is pointing to "Table" element in the schema ...
                if (master.hasAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE) && master.hasAttribute(path, KARABO_SCHEMA_VALUE_TYPE)
                    && master.getAttribute<std::string>(path, KARABO_SCHEMA_DISPLAY_TYPE) == "Table"
                    && master.getAttribute<std::string>(path, KARABO_SCHEMA_VALUE_TYPE) == "VECTOR_HASH") {
                    //std::clog << "The path \"" << path << "\" is TableElement and considered atomic.  Stop recursion...\n";
                    return;
                }
                //std::clog << "Further processing vector<Hash>  " << path << std::endl;

                // For vector<Hash> the following policy is implemented
                // The size of the vector is preserved unless all Hashes in the vector are empty
                const std::vector<Hash>& inputVector = inputNode.getValue<std::vector<Hash> >();

                // At the moment we copy the vector<Hash> to the result in all cases except 
                // if every Hash is empty after running a filter.
                // One can optimize it

                //vector<Hash>& outputVector = result.bindReference<vector<Hash> > (path);
                //outputVector.resize(inputVector.size());                
                //vector<Hash>& outputVector(inputVector.size());
                std::vector<Hash> outputVector(inputVector.size());

                for (size_t i = 0; i < inputVector.size(); ++i) {
                    //std::clog << "index i=" << i << std::endl;
                    const Hash& input = inputVector[i];
                    Hash& output = outputVector[i];
                    for (Hash::const_iterator it = input.begin(); it != input.end(); ++it) {
                        //clog << "Hash in vector path=" << path << endl;
                        r_byTag(master.get<Hash>(path), *it, output, it->getKey(), tags);
                    }
                }
                for (size_t i = 0; i < outputVector.size(); ++i) {
                    const Hash& output = outputVector[i];
                    if (output.size() > 0) {
                        result.set(path, outputVector);
                        return;
                    }
                }


            } else {
                processNode(master, inputNode, result, path, tags);
            }
        }


        bool HashFilter::processNode(const Hash& master, const Hash::Node& inputNode, Hash& result,
                                     const std::string& path, const std::set<std::string>& tags) {

            if (master.hasAttribute(path, KARABO_SCHEMA_TAGS)) {
                const std::vector<std::string>& t = master.getAttribute< std::vector<std::string> >(path, KARABO_SCHEMA_TAGS);
                for (size_t i = 0; i < t.size(); ++i) {
                    std::set<std::string>::const_iterator its = tags.find(t[i]);
                    if (its != tags.end()) {
                        //std::clog << "take element " << inputNode.getKey() << ", tag found: " << t[i] << std::endl;
                        result.set(path, inputNode);
                        result.setAttributes(path, inputNode.getAttributes());
                        return true;
                    } else {
                        //std::clog << "tag " << t[i] << " not requested for: " << inputNode.getKey() << std::endl;
                    }
                }
            }
            return false;


        }


        void HashFilter::byAccessMode(const Schema& schema, const Hash& input, Hash& result, const AccessType& value) {

            const Hash& master = schema.getParameterHash();
            for (Hash::const_iterator it = input.begin(); it != input.end(); ++it) {
                r_byAccessMode(master, *it, result, it->getKey(), value);
            }
        }


        void HashFilter::r_byAccessMode(const Hash& master, const Hash::Node& inputNode, Hash& result, const std::string& path, const AccessType& value) {

            if (!master.has(path)) {
                //std::clog << "Path: " << path << " does not exist in schema" << std::endl;
                return;
            }

            if (inputNode.is<Hash>()) {

                // if the tag was found in the HASH copy complete Hash and return
                // otherwise process the Hash further
                //if (processNodeForAccessMode(master, inputNode, result, path, value) == true) return;

                const Hash& input = inputNode.getValue<Hash>();
                for (Hash::const_iterator it = input.begin(); it != input.end(); ++it) {
                    std::string newPath = path + "." + it->getKey();
                    r_byAccessMode(master, *it, result, newPath, value);
                }
            } else if (inputNode.is<std::vector<Hash> >()) {

                // if the tag was found in the vector<HASH> (defined for LIST_ELEMENT) copy complete vector<Hash> and return
                // otherwise process the Hash further
                //if (processNodeForAccessMode(master, inputNode, result, path, value) == true) {
                //clog << "Copy entire vector<Hash> " << path << endl;
                //    return;
                //}

                // Check if the path is pointing to "Table" element in the schema ...
                if (master.hasAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE) && master.hasAttribute(path, KARABO_SCHEMA_VALUE_TYPE)
                    && master.getAttribute<std::string>(path, KARABO_SCHEMA_DISPLAY_TYPE) == "Table"
                    && master.getAttribute<std::string>(path, KARABO_SCHEMA_VALUE_TYPE) == "VECTOR_HASH") {
                    //std::clog << "The path \"" << path << "\" is TableElement and considered atomic.  Stop recursion...\n";
                    return;
                }

                //std::clog << "Further processing vector<Hash>  " << path << std::endl;

                // For vector<Hash> the following policy is implemented
                // The size of the vector is preserved unless all Hashes in the vector are empty
                const std::vector<Hash>& inputVector = inputNode.getValue<std::vector<Hash> >();

                // At the moment we copy the vector<Hash> to the result in all cases except 
                // if every Hash is empty after running a filter.
                // One can optimize it

                //vector<Hash>& outputVector = result.bindReference<vector<Hash> > (path);
                //outputVector.resize(inputVector.size());                
                //vector<Hash>& outputVector(inputVector.size());
                std::vector<Hash> outputVector(inputVector.size());

                for (size_t i = 0; i < inputVector.size(); ++i) {
                    //std::clog << "index i=" << i << std::endl;
                    const Hash& input = inputVector[i];
                    Hash& output = outputVector[i];
                    for (Hash::const_iterator it = input.begin(); it != input.end(); ++it) {
                        //std::clog << "Hash in vector path=" << path << std::endl;
                        r_byAccessMode(master.get<Hash>(path), *it, output, it->getKey(), value);
                    }
                }
                for (size_t i = 0; i < outputVector.size(); ++i) {
                    const Hash& output = outputVector[i];
                    if (output.size() > 0) {
                        result.set(path, outputVector);
                        return;
                    }
                }


            } else {
                processNodeForAccessMode(master, inputNode, result, path, value);
            }
        }


        bool HashFilter::processNodeForAccessMode(const Hash& master, const Hash::Node& inputNode, Hash& result,
                                                  const std::string& path, const AccessType& value) {

            //std::clog << "processNodeForAccessMode  " << path << std::endl;
            if (master.hasAttribute(path, KARABO_SCHEMA_ACCESS_MODE)) {
                int t = master.getAttribute<int>(path, KARABO_SCHEMA_ACCESS_MODE);
                bool rc = (t & int(value)) == int(value);
                //std::clog << "\t" << path << "   attribute value = 0x" << std::hex << t << ", input value = 0x" << value << " ==> result = " << std::boolalpha << rc << std::endl;
                if (rc) {
                    result.set(path, inputNode);
                    result.setAttributes(path, inputNode.getAttributes());
                    return true;
                }
            }
            return false;


        }


    }
}
