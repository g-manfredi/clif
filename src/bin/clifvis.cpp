#include "cliini.h"

#include <cstdlib>
#include <vector>

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE //FIXME need portable extension matcher...
#endif
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <clif/dataset.hpp>
#include <clif/clif.hpp>

#ifdef CLIF_WITH_LIBIGL
  #include <clif/mesh.hpp>
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
    "cam",
    1, //argcount
    2, //argcount
    CLIINI_INT, //type
    0, //flags
    'c'
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

#ifdef CLIF_WITH_LIBIGL
void update_cams_mesh(Mesh &cams, Mat_<double> extrinsics, Mat_<double> extrinsics_rel, Mat_<double> lines)
{
  cams = mesh_cam().scale(20);
  
  for(auto pos : Idx_It_Dims(extrinsics_rel,"channels","cams")) {
      //cv::Vec3b col(0,0,0);
      //col[ch%3]=255;
      Eigen::Vector3d trans(extrinsics_rel(3,pos.r("channels","cams")),extrinsics_rel(4,pos.r("channels","cams")),extrinsics_rel(5,pos.r("channels","cams")));
      Eigen::Vector3d rot(extrinsics_rel(0,pos.r("channels","cams")),extrinsics_rel(1,pos.r("channels","cams")),extrinsics_rel(2,pos.r("channels","cams")));
      
      //FIXME must exactly invert those operations?
      Mesh cam = mesh_cam().scale(20);
      cam -= trans;
      cam.rotate(-rot);
      cams.merge(cam);
      //cam_writer.add(cam_left,col);
      //printf("extr:\n");
      //std::cout << -rot << "\n trans:\n" << -trans << std::endl;
      //printf("%.3f ", trans.norm());
      
      Mesh line_mesh;
      
      for(auto line_pos : Idx_It_Dims(lines,"x","y")) {
        if (line_pos["x"] % 4 != 0)
          continue;
        if (line_pos["y"] % 4 != 0)
          continue;
        
        Mat_<double> l = lines.bindAll(-1,line_pos["x"],line_pos["y"],pos["channels"],pos["cams"]);
        Eigen::Vector3d origin(l(0),l(1),0.0);
        Eigen::Vector3d dir(l(2),l(3),-1.0);
      
        Mesh line = mesh_line(origin,origin+dir*1000.0);
        
        line_mesh.merge(line);
      }
      
      line_mesh -= trans;
      line_mesh.rotate(-rot);
      
      cams.merge(line_mesh);
      //viewer.data.add_edges(P1,P2,Eigen::RowVector3d(r,g,b);
    }
  //printf("\n");
  
  for(auto pos : Idx_It_Dim(extrinsics, "views")) {
      Eigen::Vector3d trans(extrinsics(3,pos["views"]),extrinsics(4,pos["views"]),extrinsics(5,pos["views"]));
      Eigen::Vector3d rot(extrinsics(0,pos["views"]),extrinsics(1,pos["views"]),extrinsics(2,pos["views"]));
      
      Mesh plane = mesh_plane().scale(1000);
      
      plane -= trans;
      plane.rotate(-rot);
      
      cams.merge(plane);
  }
  
}

void single_cam_lines(Mesh &mesh, Mat_<double> lines, int channel, int cam)
{
  Mesh line_mesh;
  
  for(auto line_pos : Idx_It_Dims(lines,"x","y")) {
    if (line_pos["x"] % 4 != 0)
      continue;
    if (line_pos["y"] % 4 != 0)
      continue;
    Eigen::Vector3d origin(lines({0,line_pos.r("x","y"),channel,cam}),lines({1,line_pos.r("x","y"),channel,cam}),0.0);
    Eigen::Vector3d dir(lines({2,line_pos.r("x","y"),channel,cam}),lines({3,line_pos.r("x","y"),channel,cam}),-1.0);
    
    
    Mesh line = mesh_line(origin-dir,origin+dir);
    
    line_mesh.merge(line);
  }
  
  mesh = line_mesh;
}
#endif


int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsopts(argc, argv, &group);

  cliini_arg *input = cliargs_get(args, "input");
  cliini_arg *camsel = cliargs_get(args, "cam");
  
  if (!input || cliarg_sum(input) != 1)
    errorexit("need exactly on input file");
  
  string name(cliarg_str(input));
  
  ClifFile f(name, H5F_ACC_RDONLY);
  
  Dataset *set = f.openDataset(0);

  Mat_<double> extrinsics;
  Mat_<double> extrinsics_rel;
  Mat_<double> lines;
  set->get(set->getSubGroup("calibration/intrinsics")/"extrinsics", extrinsics);
  set->get(set->getSubGroup("calibration/intrinsics")/"extrinsics_cams", extrinsics_rel);
  
  
  Datastore *line_store = set->getStore(set->getSubGroup("calibration/intrinsics")/"lines");
  line_store->read(lines);
  
  extrinsics.names({"extrinsics","views"});
  extrinsics_rel.names({"extrinsics","channels","cams"});
  lines.names({"lines","x","y","channels","cams"});
  
#ifdef CLIF_WITH_LIBIGL
    
  Mesh cams;
  
  if (camsel) {
    int channel = cliarg_nth_int(camsel, 0);
    int cam = cliarg_nth_int(camsel, 1);
    single_cam_lines(cams, lines, channel, cam);
  }
  else
    update_cams_mesh(cams, extrinsics, extrinsics_rel, lines);
    
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
