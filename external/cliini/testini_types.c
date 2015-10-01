#include "cliini.h"

#include <stdlib.h>
#include <stdio.h>

cliini_optgroup group = {
  NULL,
  NULL,
  0,
  0,
  CLIINI_ALLOW_UNKNOWN_OPT
};

int main(const int argc, const char *argv[])
{
  cliini_args *args = cliini_parsefile(argv[1], &group);
  cliini_args *argtypes = cliini_parsefile(argv[2], &group);
  
  cliini_fit_typeopts(args, argtypes);
  
  int i;
  for(i=0;i<cliargs_count(args);i++)
    cliini_print_arg(cliargs_nth(args, i));
}