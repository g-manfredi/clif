#ifndef _METAMAT_CLIINI
#define _METAMAT_CLIINI

#include "cliini.h"

BaseType cliini_type_to_BaseType(int type)
{
  switch (type) {
    case CLIINI_STRING : return BaseType::STRING;
    case CLIINI_DOUBLE : return BaseType::DOUBLE;
    case CLIINI_INT : return BaseType::INT;
  }
  
  printf("ERROR: unknown argument type!\n");
  abort();
}

#endif
