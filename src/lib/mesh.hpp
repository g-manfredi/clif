#ifndef CLIF_MESH_H
#define CLIF_MESH_H
 
#include <Eigen/Dense>

#include "clif/config.h"

#ifdef CLIF_WITH_LIBIGL_VIEWER
namespace igl { namespace  viewer {
  class Viewer;
}}
#endif

namespace clif {

class Mesh
{
public:
  void writeOBJ(const char *filename);
  Mesh& operator+=(const Eigen::Vector3d &rhs);
  Mesh& operator-=(const Eigen::Vector3d &rhs);
  Mesh& scale(double s);
  void rotate(const Eigen::Vector3d &r_v);
  void merge(const Mesh &other);
  void size(int v_count, int f_count);
  
  bool show();
  
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
private:
#ifdef CLIF_WITH_LIBIGL_VIEWER
  igl::viewer::Viewer *_viewer = NULL;
#endif
};

Mesh mesh_cam();
Mesh mesh_plane();
Mesh mesh_line(const Eigen::Vector3d &p1, const Eigen::Vector3d &p2);

}

#endif
