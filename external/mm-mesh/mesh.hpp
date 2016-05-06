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
  Mesh& operator=(const Mesh &m);
  Mesh& scale(double s);
  void rotate(const Eigen::Vector3d &r_v);
  void merge(const Mesh &other);
  void size(int v_count, int f_count);
  void color(bool use_color);
  
  bool show(bool block = true);
#ifdef MM_MESH_WITH_VIEWER 
  void callback_key_pressed(std::function<bool(Mesh *mesh, unsigned int key, int modifiers)> cb);
  //c++ problem: should not be public but otherwise we'd have to expose a function (as friend) from the cpp which would expouse the viewer implementation from igl!
  std::function<bool(Mesh *mesh, unsigned int key, int modifiers)> _callback_key_pressed;
#endif
  
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
  Eigen::MatrixXd C;
private:
#ifdef MM_MESH_WITH_VIEWER
  igl::viewer::Viewer *_viewer = NULL;
  std::thread *_viewer_thread = NULL;
  void _run_viewer();
public :
  ~Mesh();
#endif
};

Mesh mesh_cam();
Mesh mesh_plane();
Mesh mesh_line(const Eigen::Vector3d &p1, const Eigen::Vector3d &p2);

}

#endif
