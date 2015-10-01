#ifndef _CLIINI_H
#define _CLIINI_H

#ifdef __cplusplus
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

typedef struct {
  cliini_arg *args;
  int count;
  int max;
} cliini_args;

CLIF_EXPORT cliini_args *cliini_parsopts(const int argc, const char *argv[], cliini_optgroup *group);
CLIF_EXPORT cliini_args *cliini_parsefile(const char *filename, cliini_optgroup *group);

CLIF_EXPORT int cliini_fit_typeopts(cliini_args *args, cliini_args *typeargs);

CLIF_EXPORT int cliargs_count(cliini_args *args);
CLIF_EXPORT cliini_arg *cliargs_nth(cliini_args *args, int n);

CLIF_EXPORT void cliini_print_arg(cliini_arg *arg);

CLIF_EXPORT cliini_arg *cliargs_get(cliini_args *args, const char *name);
//assumes that the args flags are wildcard patterns
CLIF_EXPORT cliini_arg *cliargs_get_glob(cliini_args *args, const char *name);

CLIF_EXPORT int cliarg_inst_count(cliini_arg *arg); //number of instances of this options
CLIF_EXPORT int cliarg_sum(cliini_arg *arg); //sum of all arguments for all instances

CLIF_EXPORT int cliarg_arg_count(cliini_arg *arg); //number of arguments for the first instance
CLIF_EXPORT int cliarg_inst_arg_count(cliini_arg *arg, int inst); //number of arguments for the respective instance

CLIF_EXPORT char  *cliarg_str(cliini_arg *arg);
CLIF_EXPORT int    cliarg_int(cliini_arg *arg);
CLIF_EXPORT double cliarg_double(cliini_arg *arg);

CLIF_EXPORT char  *cliarg_nth_str(cliini_arg *arg, int n);
CLIF_EXPORT int    cliarg_nth_int(cliini_arg *arg, int n);
CLIF_EXPORT double cliarg_nth_double(cliini_arg *arg, int n);

//flat - return all values from all instances
CLIF_EXPORT void cliarg_strs(cliini_arg *arg, char **vals);
CLIF_EXPORT void cliarg_ints(cliini_arg *arg, int *vals);
CLIF_EXPORT void cliarg_doubles(cliini_arg *arg, double *vals);

//flat - return all values from single instance
CLIF_EXPORT void cliarg_inst_strs(cliini_arg *arg, char **vals);
CLIF_EXPORT void cliarg_inst_ints(cliini_arg *arg, int *vals);
CLIF_EXPORT void cliarg_inst_doubles(cliini_arg *arg, double *vals);

#ifdef __cplusplus
}
#endif

#endif