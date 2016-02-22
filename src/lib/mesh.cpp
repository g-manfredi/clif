#include "mesh.hpp"

#ifdef CLIF_WITH_LIBIGL
  #include <igl/writeOBJ.h>
#endif

namespace clif {

void Mesh::writeOBJ(const char *filename)
{
  igl::writeOBJ(filename,V,F);
}

Mesh& Mesh::operator+=(const Eigen::Vector3d &rhs)
{
  for(int i=0;i<V.rows();i++) {
    V(i,0) += rhs(0);
    V(i,1) += rhs(1);
    V(i,2) += rhs(2);
  }
}

Mesh& Mesh::operator-=(const Eigen::Vector3d &rhs)
{
  for(int i=0;i<V.rows();i++) {
    V(i,0) -= rhs(0);
    V(i,1) -= rhs(1);
    V(i,2) -= rhs(2);
  }
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

}