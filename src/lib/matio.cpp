#include "matio.hpp"

#include "matio.h"

void dump_mat(std::string filename)
{
  mat_t
  *matfp;
  matvar_t *matvar;
  matfp = Mat_Open(filename.c_str(),MAT_ACC_RDONLY);
  if ( NULL == matfp ) {
    fprintf(stderr,"Error opening MAT file %s\n",filename.c_str());
    return;
    matvar = Mat_VarReadInfo(matfp,"x");
    if ( NULL == matvar ) {
      fprintf(stderr,"Variable ’x’ not found, or error "
      "reading MAT file\n");
    } else {
      if ( !matvar->isComplex )
        fprintf(stderr,"Variable ’x’ is not complex!\n");
      if ( matvar->rank != 2 ||
        (matvar->dims[0] > 1 && matvar->dims[1] > 1) )
        fprintf(stderr,"Variable ’x’ is not a vector!\n");
      Mat_VarFree(matvar);
    }
    
    
  }
}

void list_mat(std::string filename)
{
  mat_t
  *matfp;
  matvar_t *matvar;
  matfp = Mat_Open(filename.c_str(),MAT_ACC_RDONLY);
  if ( NULL == matfp ) {
    fprintf(stderr,"Error opening MAT file %s\n",filename.c_str());
    return;
  }
  while ( (matvar = Mat_VarReadNextInfo(matfp)) != NULL ) {
    printf("%s\n",matvar->name);
    Mat_VarFree(matvar);
    matvar = NULL;
  }
  Mat_Close(matfp);
}
