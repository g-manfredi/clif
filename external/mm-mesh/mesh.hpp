#ifndef CLIF_MESH_H
#define CLIF_MESH_H
 
#include <Eigen/Dense>

#ifdef MM_MESH_WITH_VIEWER
namespace igl { namespace  viewer {
  class Viewer;
}}
namespace std {
  class thread;
}
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
  void color(bool use_color);
  
  bool show(bool block = true);
  
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
  Eigen::MatrixXd C;
private:
#ifdef MM_MESH_WITH_VIEWER
  igl::viewer::Viewer *_viewer = NULL;
  std::thread *_viewer_thread = NULL;
public :
  ~Mesh();
#endif
};

Mesh mesh_cam();
Mesh mesh_plane();
Mesh mesh_line(const Eigen::Vector3d &p1, const Eigen::Vector3d &p2);

}

#endif
