#include "mesh.hpp"

#ifdef MM_MESH_WITH_LIBIGL
  #include <igl/writeOBJ.h>
#endif

#ifdef MM_MESH_WITH_VIEWER
  #include <igl/viewer/Viewer.h>
  #include <thread>
#endif

namespace clif {

void Mesh::writeOBJ(const char *filename)
{
#ifdef MM_MESH_WITH_LIBIGL
  igl::writeOBJ(filename,V,F);
#endif
}

Mesh& Mesh::operator+=(const Eigen::Vector3d &rhs)
{
  for(int i=0;i<V.rows();i++) {
    V(i,0) += rhs(0);
    V(i,1) += rhs(1);
    V(i,2) += rhs(2);
  }

  return *this;
}

Mesh& Mesh::operator-=(const Eigen::Vector3d &rhs)
{
  for(int i=0;i<V.rows();i++) {
    V(i,0) -= rhs(0);
    V(i,1) -= rhs(1);
    V(i,2) -= rhs(2);
  }

  return *this;
}


Mesh& Mesh::scale(double s)
{
  for(int i=0;i<V.rows();i++) {
    V(i,0) *= s;
    V(i,1) *= s;
    V(i,2) *= s;
  }
  
  return *this;
}

void Mesh::rotate(const Eigen::Vector3d &r_v)
{
  double len = r_v.norm();
  
  if (len == 0.0)
    return;
  
  Eigen::Matrix3d r(Eigen::AngleAxisd(len, r_v*(1.0/len)));
  
  for(int i=0;i<V.rows();i++)
    V.row(i) = V.row(i)*r;
}

void Mesh::merge(const Mesh &other)
{
  int v_old = V.rows();
  int f_old = F.rows();
  
  V.conservativeResize(V.rows()+other.V.rows(), 3);
  F.conservativeResize(F.rows()+other.F.rows(), 3);
  
  for(int i=0;i<other.V.rows();i++) {
    V(i+v_old, 0) = other.V(i,0);
    V(i+v_old, 1) = other.V(i,1);
    V(i+v_old, 2) = other.V(i,2);
  }
  
  for(int i=0;i<other.F.rows();i++) {
    F(i+f_old, 0) = other.F(i,0)+v_old;
    F(i+f_old, 1) = other.F(i,1)+v_old;
    F(i+f_old, 2) = other.F(i,2)+v_old;
  }
}

void Mesh::size(int v_count, int f_count)
{
  V.resize(v_count,3);
  F.resize(f_count, 3);
}

void Mesh::color(bool use_color)
{
  if (use_color)
    C.resize(V.rows(), V.cols());
  else
    C.resize(0, 0);
}

Mesh mesh_cam()
{
  Mesh m;
  
  Eigen::MatrixXd V(5, 3);
  Eigen::MatrixXi F(4, 3);
  
  V(0,0) = 0;
  V(0,1) = 0;
  V(0,2) = 0;
  
  V(1,0) = -0.5;
  V(1,1) = -0.5;
  V(1,2) = -1;
  
  V(2,0) = 0.5;
  V(2,1) = -0.5;
  V(2,2) = -1;
  
  V(3,0) = 0.5;
  V(3,1) = 0.5;
  V(3,2) = -1;
  
  V(4,0) = -0.5;
  V(4,1) = 0.5;
  V(4,2) = -1;
  
  F(0,0) = 0;
  F(0,1) = 1;
  F(0,2) = 2;
  
  F(1,0) = 0;
  F(1,1) = 2;
  F(1,2) = 3;
  
  F(2,0) = 0;
  F(2,1) = 3;
  F(2,2) = 4;
  
  F(3,0) = 0;
  F(3,1) = 4;
  F(3,2) = 1;
  
  m.V = V;
  m.F = F;
  return m;
}


Mesh mesh_plane()
{
  Mesh m;
  
  Eigen::MatrixXd V(4, 3);
  Eigen::MatrixXi F(2, 3);
   
  V(0,0) = -0.5;
  V(0,1) = -1;
  V(0,2) = 0;
  
  V(1,0) = 0.5;
  V(1,1) = -1;
  V(1,2) = 0;
  
  V(2,0) = 0.5;
  V(2,1) = 1;
  V(2,2) = 0;
  
  V(3,0) = -0.5;
  V(3,1) = 1;
  V(3,2) = 0;
  
  F(0,0) = 1;
  F(0,1) = 3;
  F(0,2) = 0;
  
  F(1,0) = 3;
  F(1,1) = 1;
  F(1,2) = 2;
  
  m.V = V;
  m.F = F;
  return m;
}

Mesh mesh_line(const Eigen::Vector3d &p1, const Eigen::Vector3d &p2)
{
  Mesh m;
  
  Eigen::MatrixXd V(3, 3);
  Eigen::MatrixXi F(1, 3);
   
  V(0,0) = p1(0);
  V(0,1) = p1(1);
  V(0,2) = p1(2);
  
  V(1,0) = p2(0);
  V(1,1) = p2(1);
  V(1,2) = p2(2);
  
  V(2,0) = p2(0);
  V(2,1) = p2(1);
  V(2,2) = p2(2);
  
  F(0,0) = 0;
  F(0,1) = 1;
  F(0,2) = 2;
  
  m.V = V;
  m.F = F;
  return m;
}

#ifdef MM_MESH_WITH_VIEWER

Mesh::~Mesh()
{
  if (_viewer_thread) {
    //glfwSetWindowShouldClose(_viewer->window, 1); 
    _viewer_thread->join();
  }
}

static void _run_viewer(const Mesh *mesh, igl::viewer::Viewer **viewer)
{
  if (!(*viewer))
    *viewer =  new igl::viewer::Viewer;
  
  (*viewer)->core.set_rotation_type(
    igl::viewer::ViewerCore::RotationType::ROTATION_TYPE_TRACKBALL);
  (*viewer)->data.set_mesh(mesh->V, mesh->F);
  if (mesh->C.rows())
    (*viewer)->data.set_colors(mesh->C);
  
  (*viewer)->core.show_lines = false;
  //(*viewer)->core.shininess = 0.0;
  (*viewer)->core.lighting_factor = 0.0;
  
  (*viewer)->launch();
}
#endif

bool Mesh::show(bool block)
{
#ifdef MM_MESH_WITH_VIEWER
  if (_viewer)
    return false;
  
  if (block) {
    _run_viewer(this, &_viewer);
    delete _viewer;
    _viewer = NULL;
  }
  else
    _viewer_thread = new std::thread(_run_viewer, this, &_viewer);
  return true;
#else
  return false;
#endif
}

#ifdef MM_MESH_WITH_VIEWER
static bool _update_viewer(igl::viewer::Viewer& viewer, Mesh *m)
{ 

  viewer.data.clear();
  viewer.data.set_mesh(m->V, m->F);
  viewer.callback_pre_draw = nullptr;
  
  glfwPostEmptyEvent();
  
  return false;
}
#endif

Mesh& Mesh::operator=(const Mesh &m)
{
  V = m.V;
  F = m.F;
  C = m.C;

#ifdef MM_MESH_WITH_VIEWER
  if (_viewer)
    _viewer->callback_pre_draw = std::bind(_update_viewer, std::placeholders::_1, this);
#endif

  return *this;
}

} //namespace clif
