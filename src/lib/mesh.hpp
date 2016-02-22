#ifndef CLIF_MESH_H
#define CLIF_MESH_H
 
#include <Eigen/Dense> 

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
  
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
};

Mesh mesh_cam();
Mesh mesh_plane();
Mesh mesh_line(const Eigen::Vector3d &p1, const Eigen::Vector3d &p2);

}

#endif