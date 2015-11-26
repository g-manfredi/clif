#ifndef _CLIF_DATASET_H
#define _CLIF_DATASET_H

#include "attribute.hpp"
#include "datastore.hpp"
#include "clif.hpp"

namespace clif {
  
class Datastore;

class Intrinsics {
  public:
    Intrinsics() {};
    Intrinsics(Attributes *attrs, boost::filesystem::path &path) { load(attrs, path); };
    void load(Attributes *attrs, boost::filesystem::path path);
    cv::Mat* getUndistMap(double depth, int w, int h);
    
    double f[2], c[2];
    DistModel model = DistModel::INVALID;
    std::vector<double> cv_dist;
    cv::Mat cv_cam;

  private:
    cv::Mat _undist_map;
};

/** Class representing a single light field dataset.
 * This class inherits from Attributes for Metadata handling and from Datastore for actual image/light field IO.
 * The class mostly wraps functionality from the inherited classes, while higher level light field handling like EPI generation is available in the respective library bindings (TODO ref and document opencv/vigra interfaces)
 * Not that functions taking a path (either as std::string or boost::filesystem::path) all reference the path relative to the dataset root. An attribute adressed as \a /blub/someattribute is actually stored under \a /clif/datasetname/blub/someattribute.
 */
class Dataset : public Attributes {
  public:
    Dataset() {};
    ~Dataset();
    void reset();
    //FIXME use only open/create methods?
    //Dataset(H5::H5File &f_, std::string path);
    
    /** Open the dataset \a name from file \a f_ */
    void open(ClifFile &f, const cpath &name);
    
    //link other into this file, attributes are copied, "main" datastore is linked read-only
    //TODO link other existing datastores!
    void link(const Dataset *other);
    void link(ClifFile &file, const Dataset *other);
    
    //create memory backed file and link other into this file, attributes are copied, "main" datastore is linked read-only
    //TODO link other existing datastores!
    void memory_link(const Dataset *other);
    
    /** Create or open (if existing) the dataset \a name in file \a f_ */
    void create(ClifFile &file, cpath name = cpath());
      
    //writes only Attributes! FIXME hide Attributes::Write
    //TODO automatically call this on file close (destructor)
    /** Sync attributes back to the underlying HDF5 file 
     */
    void writeAttributes() { Attributes::write(f(), _path); }
    void flush();
    
    Datastore *getStore(const boost::filesystem::path &path, bool create = true, int create_dims = 4);
    Datastore *addStore(const boost::filesystem::path &path, int dims = 4);
    void addStore(Datastore *store);
    
    bool memoryFile();
    
    //FIXME fix or remove this method
    bool valid();
    
    StringTree<Attribute*,Datastore*> getTree();
    
    /** load the intrinsics group with name \a intrset.
     * default (empty string) will load the first intrinsics group from the file, 
     * which is also the group that is loaded on opening the dataset. The specified
     * intrinsics group will be used for automatic processing like undistortion.
     */
    void load_intrinsics(std::string intrset = std::string());
    
    /** return the actual full path of the dataset root in the HDF5 file
     */
    boost::filesystem::path path();
    
    H5::H5File& f();
    ClifFile& file();

    //TODO if attributes get changed automatically refresh intrinsics on getIntrinsics?
    //TODO hide and create accessor
    Intrinsics intrinsics;
private:
    /** The internal HDF5 reference
     */
    ClifFile _file;
    Datastore *calib_images = NULL;
    cpath _path;
    
    //hide copy assignment operator
    Dataset& operator=(const Dataset& other) = delete;
    
    //FIXME delete on destruct!
    std::unordered_map<std::string,Datastore*> _stores;
    bool _memory_file = false;
    
    void datastores_append_group(Dataset *set, std::unordered_map<std::string,Datastore*> &stores, H5::Group &g, cpath basename, cpath group_path);
};
  
}

#endif