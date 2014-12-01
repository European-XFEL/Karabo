/* 
 * File:   DetectorGeometry.hh
 * Author: haufs
 *
 * Created on July 29, 2014, 5:47 PM
 */

#ifndef DETECTORGEOMETRY_HH
#define	DETECTORGEOMETRY_HH

#include <vector>
#include <boost/smart_ptr.hpp>
#include <karabo/util.hpp>


namespace karabo {
    namespace util {
        
        
        class DetectorGeometry : public boost::enable_shared_from_this<DetectorGeometry>{
        public:
            DetectorGeometry():m_offsets(std::vector<double>(3,0.)), m_rotations(std::vector<double>(3,0.)), m_tileId(-2), m_managedTiles(0){
                
            }
            
            DetectorGeometry(DetectorGeometry* parent):m_offsets(std::vector<double>(3,0.)),
                m_rotations(std::vector<double>(3,0.)), m_managedTiles(0){
                m_tileId = parent->assignTileId();;
                m_parent = parent;
            }
            
            DetectorGeometry(const karabo::util::Hash & h){
                m_offsets = h.get<std::vector<double> >("aligment.offsets");
                m_rotations = h.get<std::vector<double> >("aligment.rotations");
                m_tileId = h.get<int>("tileId");
                m_managedTiles = 0;
                
                
                std::vector<karabo::util::Hash> s = h.get<std::vector<karabo::util::Hash> >("subAssemblies");
                
                if(s.size() == 0 && m_tileId == -1) m_tileId = -2;
                
                for(std::vector<karabo::util::Hash>::iterator it = s.begin(); it != s.end(); ++it){
                    m_subAssemblies.push_back(DetectorGeometry(*it));
                }
            }
            
            virtual ~DetectorGeometry(){
                
            }
            
            DetectorGeometry& setOffsets(const double & ox, const double & oy, const double & oz){
                m_offsets[0] = ox;
                m_offsets[1] = oy;
                m_offsets[2] = oz;
                return *this;
            }
            
            
            std::vector<double> getOffsets(){
                return m_offsets;
            }
            
            DetectorGeometry& setRotations(const double & rx, const double & ry, const double & rz){
                m_rotations[0] = rx;
                m_rotations[1] = ry;
                m_rotations[2] = rz;
                return *this;
            }
            
            std::vector<double> getRotations(){
                return m_rotations;
            }
            
            DetectorGeometry& startSubAssembly(){
                m_subAssemblies.push_back(DetectorGeometry(this));
                //m_managedTiles++;
                return m_subAssemblies.back();
            }
            
            DetectorGeometry& endSubAssembly(){
                if(m_tileId != -2){
                    return *m_parent;
                } else {
                    return *this;
                }
            }
            
            int assignTileId(){
                
                if(m_tileId != -2){
                    return m_parent->assignTileId();
                } else {
                    return m_managedTiles++;
                }
            }
            
            int getTileId(){
                return m_tileId;
            }
            
            karabo::util::Hash toHash() const{
                karabo::util::Hash h;
                
                int tileId = m_tileId;
                if(m_subAssemblies.size() == 0 && tileId == -2) tileId = -1;
                
                karabo::util::Hash alignment;
                alignment.set("offsets", m_offsets);
                alignment.set("rotations", m_rotations);
                h.set("alignment", alignment);
                h.set("tileId", tileId);
                //h.set("managedTiles", m_managedTiles);
                
                std::vector<karabo::util::Hash> subAssemblies;
                for(std::vector<DetectorGeometry>::const_iterator it = m_subAssemblies.begin(); it != m_subAssemblies.end(); ++it){
                    subAssemblies.push_back(it->toHash());
                }
                if(subAssemblies.size() != 0) h.set("subAssemblies", subAssemblies);
                return h;
            }
            
           
            
            karabo::util::Schema toSchema(std::string topNode, karabo::util::Schema & schema){
                
                 int tileId = m_tileId;
                if(m_subAssemblies.size() == 0 && tileId == -2) tileId = -1;
                
                //std::string topNode = key+".detectorGeometry";
                karabo::util::NODE_ELEMENT(schema).key(topNode+".alignment")
                            .displayedName("Alignment")
                            .commit();
                
                    karabo::util::VECTOR_DOUBLE_ELEMENT(schema).key(topNode+".alignment.offsets")
                        .displayedName("Offsets")
                        .description("The x,y,z offsets of this element")
                        .readOnly()
                        .initialValue(m_offsets)
                        .commit();

                    karabo::util::VECTOR_DOUBLE_ELEMENT(schema).key(topNode+".alignment.rotations")
                        .displayedName("Rotations")
                        .description("The theta, phi, omega rotation of this element")
                        .readOnly()
                        .initialValue(m_rotations)
                        .commit();
                
                karabo::util::INT32_ELEMENT(schema).key(topNode+".tileId")
                    .displayedName("TileId")
                    .description("The tile id of this element")
                    .readOnly()
                    .initialValue(tileId)
                    .commit();
                
                karabo::util::UINT64_ELEMENT(schema).key(topNode+".managedTiles")
                    .displayedName("Managed tiles")
                    .description("The number of tiles managed on this hierarchy level")
                    .readOnly()
                    .initialValue(m_managedTiles)
                    .commit();
                
                int idOnHierarchy = 0;
                for(std::vector<DetectorGeometry>::iterator it = m_subAssemblies.begin(); it != m_subAssemblies.end(); ++it){
                    karabo::util::NODE_ELEMENT(schema).key(topNode+".t"+boost::lexical_cast<std::string>(it->getTileId()))
                            .displayedName(boost::lexical_cast<std::string>(idOnHierarchy))
                            .commit();
                    schema.merge(it->toSchema(topNode+".t"+boost::lexical_cast<std::string>(it->getTileId()), schema));
                    idOnHierarchy++;
                }
                
                return schema;
                
            }
            
            
        private:
            std::vector<double> m_offsets;
            std::vector<double> m_rotations;
            std::vector<DetectorGeometry> m_subAssemblies;
            int m_tileId;
            DetectorGeometry* m_parent;
            unsigned long long m_managedTiles;
        };
        
    }
    
}

#endif	/* DETECTORGEOMETRY_HH */

