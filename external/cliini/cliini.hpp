#ifndef _CLIINI_HPP
#define _CLIINI_HPP

#include <cstddef>
#include <string>

namespace cliini
{

#include "cliini.h"
  
class cliarg {
public:
  cliini_arg *arg = NULL;
  
  cliarg(cliini_arg *a) : arg(a) {};
  
  int count();
  
  int num();
  double flt();
  std::string str();
  
  int num(int n);
  double flt(int n);
  std::string str(int n);
  
  bool valid();

};
  
class cliargs {
public:
  cliini_args *args = NULL;
  
  cliargs(cliini_args *args_) : args(args_) {};
  cliargs(const int argc, const char *argv[], cliini_optgroup *group);
  cliargs(const char *filename, cliini_optgroup *group);
  cliargs(char *buf, cliini_optgroup *group);
  
  cliarg operator[](const char *name);
  bool have(const char *name);
};

}

/*#ifdef __cplusplus
extern "C" {
#endif
  
#define CLIINI_ARGCOUNT_ANY -1
  
//#define CLIINI_ALLOW_UNKNOWN_NONOPT 1
#define CLIINI_ALLOW_UNKNOWN_OPT    2
  
//unknown opts with the same name will not be merged!
#define CLIINI_OPT_UNKNOWN  1
//FIXME implement!
#define CLIINI_OPT_REQUIRED 2

enum cliini_types{CLIINI_NONE, CLIINI_INT, CLIINI_DOUBLE, CLIINI_STRING, CLIINI_ENUM};

typedef struct {
  const char *longflag;
  int argcount_min;
  int argcount_max;
  int type;
  int flags; 
  char flag;
  char **enums;
  char *help;
  char *help_args;
} cliini_opt;

typedef struct _cliini_optgroup {
  cliini_opt *opts;
  struct _cliini_optgroup *groups;
  int opt_count;
  int group_count;
  int flags;
} cliini_optgroup;

typedef struct {
  cliini_opt *opt; //found opt
  void *vals; //values - for each found opt instance - argcount elements of the respective type;
  int inst_count; //number of instances of opt
  int *counts; //the number of arguments for each instance
  int sum; //sum of counts
} cliini_arg;

typedef struct _cliini_args {
  cliini_arg *args;
  int count;
  int max;
} cliini_args;

CLIINI_EXPORT cliini_args *cliini_parsopts(const int argc, const char *argv[], cliini_optgroup *group);
CLIINI_EXPORT cliini_args *cliini_parsefile(const char *filename, cliini_optgroup *group);
CLIINI_EXPORT cliini_args *cliini_parsebuf(char *buf, cliini_optgroup *group);

CLIINI_EXPORT int cliini_fit_typeopts(cliini_args *args, cliini_args *typeargs);

CLIINI_EXPORT int cliargs_count(cliini_args *args);
CLIINI_EXPORT cliini_arg *cliargs_nth(cliini_args *args, int n);

CLIINI_EXPORT void cliini_print_arg(cliini_arg *arg);

CLIINI_EXPORT cliini_arg *cliargs_get(cliini_args *args, const char *name);
//assumes that the args flags are wildcard patterns
CLIINI_EXPORT cliini_arg *cliargs_get_glob(cliini_args *args, const char *name);

CLIINI_EXPORT int cliarg_inst_count(cliini_arg *arg); //number of instances of this options
CLIINI_EXPORT int cliarg_sum(cliini_arg *arg); //sum of all arguments for all instances

CLIINI_EXPORT int cliarg_arg_count(cliini_arg *arg); //number of arguments for the first instance
CLIINI_EXPORT int cliarg_inst_arg_count(cliini_arg *arg, int inst); //number of arguments for the respective instance

CLIINI_EXPORT char  *cliarg_str(cliini_arg *arg);
CLIINI_EXPORT int    cliarg_int(cliini_arg *arg);
CLIINI_EXPORT double cliarg_double(cliini_arg *arg);

CLIINI_EXPORT char  *cliarg_nth_str(cliini_arg *arg, int n);
CLIINI_EXPORT int    cliarg_nth_int(cliini_arg *arg, int n);
CLIINI_EXPORT double cliarg_nth_double(cliini_arg *arg, int n);

//flat - return all values from all instances
CLIINI_EXPORT void cliarg_strs(cliini_arg *arg, char **vals);
CLIINI_EXPORT void cliarg_ints(cliini_arg *arg, int *vals);
CLIINI_EXPORT void cliarg_doubles(cliini_arg *arg, double *vals);

//flat - return all values from single instance
CLIINI_EXPORT void cliarg_inst_strs(cliini_arg *arg, char **vals);
CLIINI_EXPORT void cliarg_inst_ints(cliini_arg *arg, int *vals);
CLIINI_EXPORT void cliarg_inst_doubles(cliini_arg *arg, double *vals);

CLIINI_EXPORT void cliini_help(cliini_optgroup *group);

#ifdef __cplusplus
}
#endif*/

#endif
