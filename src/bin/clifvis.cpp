#include "cliini.h"

#include <cstdlib>
#include <vector>

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE //FIXME need portable extension matcher...
#endif
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef CLIF_COMPILER_MSVC
#include <unistd.h>
#endif

#include "H5Cpp.h"
#include "H5File.h"

#include "dataset.hpp"
#include "clif.hpp"

#ifdef CLIF_WITH_LIBIGL
  #include "mesh.hpp"
#endif

#ifdef CLIF_WITH_LIBIGL_VIEWER
  #include <igl/viewer/Viewer.h>
#endif

using namespace clif;
using namespace std;

cliini_opt opts[] = {
  {
    "help",
    0, //argcount
    0, //argcount
    CLIINI_NONE, //type
    0, //flags
    'h'
  },
  {
    "input",
    1, //argcount min
    CLIINI_ARGCOUNT_ANY, //argcount max
    CLIINI_STRING, //type
    0, //flags
    'i'
  },
  {
    "output",
    1, //argcount
    CLIINI_ARGCOUNT_ANY, //argcount
    CLIINI_STRING, //type
    0, //flags
    'o'
  },
  {
    "types", //FIXME remove this
    1, //argcount
    1, //argcount
    CLIINI_STRING, //type
    0, //flags
    't'
  }
};

typedef unsigned int uint;

cliini_optgroup group = {
  opts,
  NULL,
  sizeof(opts)/sizeof(*opts),
  0,
  0
};


void errorexit(const char *msg)
{
  printf("ERROR: %s\n",msg);
  exit(EXIT_FAILURE);
}


int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  
  if (!input || cliarg_sum(input) != 1)
    errorexit("need exactly on input file");
  
  string name(cliarg_str(input));
  
  ClifFile f(name, H5F_ACC_RDONLY);
  
  Dataset *set = f.openDataset(0);

  Mat_<double> extrinsics;
  Mat_<double> extrinsics_rel;
  set->get(set->getSubGroup("calibration/intrinsics")/"extrinsics", extrinsics);
  set->get(set->getSubGroup("calibration/intrinsics")/"extrinsics_cams", extrinsics_rel);
  extrinsics.names({"extrinsics","views"});
  extrinsics_rel.names({"extrinsics","channels","cams"});
  
#ifdef CLIF_WITH_LIBIGL
    
  Mesh cams;
  
  Mesh cam = mesh_cam().scale(20);
  cams.merge(cam);
  
  for(auto pos : Idx_It_Dims(extrinsics_rel,"channels","cams")) {
      //cv::Vec3b col(0,0,0);
      //col[ch%3]=255;
      Eigen::Vector3d trans(extrinsics_rel(3,pos.r("channels","cams")),extrinsics_rel(4,pos.r("channels","cams")),extrinsics_rel(5,pos.r("channels","cams")));
      Eigen::Vector3d rot(extrinsics_rel(0,pos.r("channels","cams")),extrinsics_rel(1,pos.r("channels","cams")),extrinsics_rel(2,pos.r("channels","cams")));
      
      //FIXME must exactly invert those operations?
      Mesh cam = mesh_cam().scale(20);
      cam -= trans;
      cam.rotate(-rot);
      std::cout << trans << "\n";
      cams.merge(cam);
      //cam_writer.add(cam_left,col);
    }
  
  for(auto pos : Idx_It_Dim(extrinsics, "views")) {
      Eigen::Vector3d trans(extrinsics(3,pos["views"]),extrinsics(4,pos["views"]),extrinsics(5,pos["views"]));
      Eigen::Vector3d rot(extrinsics(0,pos["views"]),extrinsics(1,pos["views"]),extrinsics(2,pos["views"]));
      
      Mesh plane = mesh_plane().scale(1000);
      /*plane.rotate(ref_rot);
      plane += ref_trans;*/
      
      plane -= trans;
      plane.rotate(-rot);
      
      cams.merge(plane);
  }
    
  cams.writeOBJ("cams.obj");  
     
#endif
  
#ifdef CLIF_WITH_LIBIGL_VIEWER    
  igl::viewer::Viewer viewer;
  viewer.core.set_rotation_type(igl::viewer::ViewerCore::RotationType::ROTATION_TYPE_TRACKBALL);
  viewer.data.set_mesh(cams.V, cams.F);
  viewer.launch();
#endif

  return EXIT_SUCCESS;
}