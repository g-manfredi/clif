#include "cliini.h"

#include <stdlib.h>
#include <stdio.h>

cliini_opt opts[] = {
  {
    "help",
    0, //argcount
    0,
    CLIINI_NONE, //type
    0, //flags
    'h'
  },
  {
    "input",
    1, //argcount
    CLIINI_ARGCOUNT_ANY,
    CLIINI_STRING, //type
    0, //flags
    'i'
  },
  {
    "output",
    1, //argcount
    1,
    CLIINI_STRING, //type
    0, //flags
    'o'
  },
  {
    "translate",
    3, //argcount
    3,
    CLIINI_DOUBLE, //type
    0, //flags
    't'
  }
};

cliini_optgroup group = {
  opts,
  NULL,
  sizeof(opts)/sizeof(*opts),
  0,
  CLIINI_ALLOW_UNKNOWN_OPT
};

int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsefile(argv[1], &group);
  
  if (!args) {
    printf("error parsing command line\n");
    return EXIT_FAILURE;
  }
  
  if (cliargs_get(args, "help\n"))
    printf("print help!");
  
  cliini_arg *arg;
  
  printf("opt count: %d\n", cliargs_count(args));
  
  arg = cliargs_get(args, "input");
  if (arg)
    printf("input %s\n", cliarg_str(arg));
  
  arg = cliargs_get(args, "output");
  if (arg)
    printf("output %s\n", cliarg_str(arg));
  
  arg = cliargs_get(args, "translate");
  if (arg) {
    double v[3];
    cliarg_doubles(arg, v);
    printf("translate: %f %f %f\n", v[0], v[1], v[2]);
  }

}