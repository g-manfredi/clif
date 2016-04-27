#include "cliini.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

//may be our local fnmatch
#include "fnmatch.h"

const int maxsize = 1024*1024;

#define RETONFALSE(A) if (!(A)) return;
#define RETNULLONFALSE(A) if (!(A)) return NULL;

#define MAXPARTS 1000
#define SECTION_MAXLEN 4096

CLIINI_EXPORT cliini_opt *cliini_opt_new(const char *longflag,
                           int argcount_min,
                           int argcount_max,
                           int type,
                           int flags,
                           char flag,
                           char **enums)
{
	cliini_opt *opt = (cliini_opt*)calloc(sizeof(cliini_opt), 1);
  
  opt->longflag = longflag;
  opt->argcount_min = argcount_min;
  opt->argcount_max = argcount_max;
  opt->type = type;
  opt->flags = flags;
  opt->flag = flag;
  
  return opt;
}

static cliini_opt *_cliini_opt_match(const char *str, cliini_optgroup *group)
{
  int i;
  const char *optstring = str;
  cliini_opt *opt;
  
  while (*optstring == '-')
    optstring++;
  
  if (!group)
    return NULL;
  
  if (strlen(optstring) == 1)
    for(i=0;i<group->opt_count;i++) {
      if (group->opts[i].flag == optstring[0])
        return &group->opts[i];
    }
  else    
    for(i=0;i<group->opt_count;i++) {
      if (!strcmp(optstring, group->opts[i].longflag))
        return &group->opts[i];
    }
  
  for(i=0;i<group->group_count;i++) {
    opt = _cliini_opt_match(str, &group->groups[i]);
    if (opt)
      return opt;
  }
  
  //no valid option found
  if (group->flags && CLIINI_ALLOW_UNKNOWN_OPT)
    return cliini_opt_new(strdup(str), 0, CLIINI_ARGCOUNT_ANY, CLIINI_STRING, CLIINI_OPT_UNKNOWN, '\0', NULL);
  
  return NULL;
}

static int _cliini_opt_arg_invalid(cliini_opt *opt, const char *arg)
{
  int len = 0;
  int dots = 0;
  int exp = 0;
  
  switch (opt->type) {
    case CLIINI_STRING : return 0;
    case CLIINI_INT :
      if (arg[0] == '+' || arg[0] == '-')
        len++;
      while (arg[len] != '\0')
        if (arg[len] < '0' || arg[len] > '9')
          return len+1;
        else
          len++;
        break;
    case CLIINI_DOUBLE : 
      if (arg[0] == '+' || arg[0] == '-')
        len++;
      while (arg[len] != '\0')
        if (arg[len] == '.') {
          if (dots)
            return len+1;
          else
            dots++;
          len++;
        }
        else if (arg[len] == 'e' || arg[len] == 'E') {
          len++;
          if (arg[len] == '+' || arg[len] == '-')
            len++;
          while (arg[len] != '\0')
            if (arg[len] < '0' || arg[len] > '9')
              return len+1;
            else
              len++;
        }
        else if (arg[len] < '0' || arg[len] > '9') {
          return len+1;
        }
        else
          len++;
        break;
	case CLIINI_ENUM: {
		if (!opt->enums)
			return 1;
		char **checkval = opt->enums;
		while (*checkval)
			if (!strcmp(arg, *checkval))
				return 0;
			else
				checkval++;
		return 1;
	}
    default:
      printf("invalid opt specification!\n");
      abort();
  }
}

static int _type_size(int type)
{ 
  switch (type) {
    case CLIINI_NONE :   return 0;
    case CLIINI_ENUM : 
    case CLIINI_STRING : return sizeof(char*);
    case CLIINI_INT :    return sizeof(int);
    case CLIINI_DOUBLE : return sizeof(double);
    default:
      printf("invalid type!\n");
      abort();
  }
}

CLIINI_EXPORT void _cliini_opt_arg_store_val(cliini_opt *opt, const char *arg, void *val)
{  char *tmp;
  switch (opt->type) {
    //FIXME duplicate string?
    case CLIINI_ENUM :
    case CLIINI_STRING : tmp = strdup(arg); *(char **)val = tmp; break;
    case CLIINI_INT :    *(int*)val    = atoi(arg); break;
    case CLIINI_DOUBLE : *(double*)val = atof(arg); break;
    default:
      printf("invalid opt type!\n");
      abort();
  }
}


static int _isopt(const char *str)
{
  if (str[0] == '-')
    return 1;
  return 0;
}

CLIINI_EXPORT cliini_args *cliini_args_new()
{
  cliini_args *args = (cliini_args*)calloc(1, sizeof(cliini_args));
  
  args->max = 16;
  args->args = (cliini_arg*)calloc(sizeof(cliini_arg)*args->max, 1);

  return args;
}

CLIINI_EXPORT cliini_arg *cliini_args_add(cliini_args *args, cliini_opt *opt)
{
  while (args->count >= args->max) {
    args->args = (cliini_arg*)realloc(args->args, sizeof(cliini_arg)*args->max*2);
    memset(args->args+args->max, 0, sizeof(cliini_arg)*args->max);
    args->max *= 2;
  }
  
  args->args[args->count].opt = opt;
  
  return &args->args[args->count++];
}

//get arg for option opt from args - if not yet existing then create it
CLIINI_EXPORT cliini_arg *cliini_args_get_add(cliini_args *args, cliini_opt *opt)
{
  int i;
  
  for(i=0;i<args->count;i++)
    if (args->args[i].opt == opt)
      return &args->args[i];
    
  return cliini_args_add(args, opt);
}

//int blind: assume all argv[] are args
static int _cliini_opt_parse(const int argc, const char *argv[], int argpos, cliini_arg *arg, int *len, int blind)
{
  int count;
  int i;
  int errpos;
  int errors = 0;
  cliini_opt *opt = arg->opt;
  
  *len = 1;
  
  if (argpos + opt->argcount_min >= argc) {
    printf("ERROR: not enough arguments for option %s (required at least %d, found max %d)\n", opt->longflag, opt->argcount_min,argc-argpos-1);
    return 1;
  }
  
  arg->inst_count++;
  if (opt->argcount_min >= 1 || opt->argcount_max == CLIINI_ARGCOUNT_ANY) {   
    for(i=1; i<=opt->argcount_max || opt->argcount_max == CLIINI_ARGCOUNT_ANY;i++) {
        if (argpos + i >= argc || (opt->argcount_max == CLIINI_ARGCOUNT_ANY && !blind && _isopt(argv[argpos+i]))) {
          if (i < opt->argcount_min && opt->argcount_min != CLIINI_ARGCOUNT_ANY) {
            printf("ERROR: not enough arguments for option %s (required at least %d, found %d)\n", opt->longflag, opt->argcount_min, i-1);
            errors++;
          }
          break;
        }
        if ((errpos = _cliini_opt_arg_invalid(opt, argv[argpos+i]))) {
          printf("ERROR: invalid argument \"%s\" for opt %s, error at string positions %d\n", argv[argpos+i], opt->longflag, errpos-1);
          errors++;
          continue;
        }
    }
    //found i-1 arguments:
    count = i-1;
    *len = count+1;
    arg->sum += count;
    arg->counts = (int*)realloc(arg->counts, sizeof(int)*arg->inst_count);
    arg->counts[arg->inst_count-1] = count;
    arg->vals = realloc(arg->vals, _type_size(opt->type)*arg->sum);
    for(i=1;i<=count;i++) {
      _cliini_opt_arg_store_val(opt, argv[argpos+i], (char*)arg->vals + _type_size(opt->type)*(arg->sum-count+i-1));
    }
  }
  
  return errors;
}

CLIINI_EXPORT cliini_args *cliini_parsopts(const int argc, const char *argv[], cliini_optgroup *group)
{
  int error = 0;
  int i = 1;
  int len;
  cliini_args *args = cliini_args_new();
  cliini_arg *arg;
  cliini_opt *opt;
  
  while(i<argc) {
    if (!_isopt(argv[i])) {
      printf("ERROR: unknown non-option string \"%s\"\n", argv[i]);
      i++;
      error++;
      continue;
    }
    
    opt = _cliini_opt_match(argv[i], group);
    
    if (!opt) {
      printf("ERROR: unknown option string \"%s\"\n", argv[i]);
      i++;
      error++;
      continue;
    }
    arg = cliini_args_get_add(args, opt);
    error += _cliini_opt_parse(argc, argv, i, arg, &len, 0);
    i += len;
  }
  
  if (error) {
    printf("encontered %d errors when parsing command line\n", error);
    return NULL;
  }

  return args;
}

char *static_equal_sign = "=";

//changes line!
static int split_string(char *line, char **parts, int maxparts)
{
  int count = 0;
  char *eq_found = strchr(line, '=');
  char *part = strtok(line, " \t=");
  char *lastpart = NULL;
  
  while (part) {
    if (count >= maxparts-1)
      return -1;
    if (eq_found && part > eq_found && lastpart < eq_found) {
      parts[count] = static_equal_sign;
      count++;
    }
    parts[count] = part;
    count++;
    lastpart = part;
    part = strtok(NULL, " \t");
  }
  
  return count;
}

static char *parse_parts_section(int count, char **parts)
{
  if (count == 1) {
    if (parts[0][strlen(parts[0])-1] != ']') return NULL;
    
    parts[0][strlen(parts[0])-1] = '\0';
    return parts[0]+1;
  }
  else if (count == 2) {
    if (parts[1][strlen(parts[1])-1] != ']') return NULL;
    if (strlen(parts[0]) > 1 && strlen(parts[1]) > 1) return NULL;

    //this one contains section
    if (strlen(parts[0]) > 1)
      return parts[0]+1;
    else {
      parts[1][strlen(parts[1])-1] = '\0';
      return parts[1];
    }
  }
  else if (count == 3) {
    //easy
    if (strcmp(parts[0],"[") || strcmp(parts[2],"]")) return NULL;
    
    return parts[1];
  }
  else {
    return NULL;
  }
}


static int parts_option_valid(int count, char **parts)
{
  if (count < 3)
    return 0;
  
  if (strcmp(parts[1],"="))
    return 0;
  
  return 1;
}

static void strreplacec(char *str, char search, char replace)
{
  while(*str) {
    if (*str == search)
      *str = replace;
    str++;
  }
}

static int parse_line(char *line, cliini_args *args, cliini_optgroup *group, char *currsection)
{
  int len;
  int error = 0;
  int count;
  char *parts[MAXPARTS];
  char *section;
  char optsstring[SECTION_MAXLEN];
  
  //remove '\r' by unix newline
  for(int i=0;i<strlen(line);i++)
    if (line[i] == '\r')
      line[i] = '\n';
    
  count = split_string(line, parts, MAXPARTS);
  
  if (count == -1) {
    printf("FIXME: error: max line argument count reached!");
    abort();
  }
  
  if (!count)
    return 0 ;
  if (parts[0][0] == ';' || parts[0][0] == '#')
    return 0;
  if (strlen(parts[0]) >= 2 && parts[0][0] == '/' && parts[0][1] == '/')
    return 0;
  
  if (parts[0][0] == '[') {
    //parse section

    section = parse_parts_section(count, parts);
    
    if (section) {
      //printf("section: [%s]\n", section);
      if (section[0] != '.')
        strcpy(currsection, section);
      else if (strlen(section) >= 2) {
        if (section[1] != '.')
          strcat(currsection, section);
        else {
          char *last = strrchr(currsection, '.');
          if (!last)
            last = currsection;
          *last = '\0';
          
          if (strlen(section) >= 3)
            strcat(currsection, section+1);
        }
      }
      else {
        printf("error, could not parse section string \"%\"", section);
        return 1;
      }
      
      //we have produced an absoulte path - now replace '.' with '/' to get unix path names usefule for globbing
      return 0;
    }
    else {
      printf("error! unable to parse line: \"%s\"\n", line);
      return 1;
    }
  }
  
  /*int i;
  for(i=0;i<count;i++)
    printf("<%s>",parts[i]);
  if (count)
    printf("\n");*/
  
  if (parts_option_valid(count, parts)) {
    cliini_arg *arg;
    cliini_opt *opt;
    
    sprintf(optsstring, "%s.%s", currsection, parts[0]);
    
    //FIXME should integrate this somewhere else...
    //produce posix-style path for hierarchical representation
    strreplacec(optsstring, '.', '/');
    
    opt = _cliini_opt_match(optsstring, group);
    
    
    if (!opt) {
      printf("ERROR: unknown option string \"%s\"\n", parts[0]);
      return 1;
    }
    
    arg = cliini_args_get_add(args, opt);
    error += _cliini_opt_parse(count, (const char**)parts, 1, arg, &len, 1);
    if (len != count-1) {
      printf("ERROR: could not parse all arguments for option %s (%d/%d)\n", opt->longflag,len,count);
      return 1;
    }
  }
  else {
    error++;
    
    printf("could not parse: ");
    int i;
    for(i=0;i<count;i++)
      printf("<%s>",parts[i]);
    if (count)
      printf("\n");
  }
  
  return error;
}

#undef MAXPARTS

CLIINI_EXPORT cliini_args *cliini_parsefile(const char *filename, cliini_optgroup *group)
{
  if (!filename)
	return NULL;

  FILE *f = fopen(filename, "r");
  char *buf = (char*)malloc(maxsize);
  char *line_end;
  char currsection[SECTION_MAXLEN];
  
  int error = 0;
  int filepos = 0;
  int bufpos = 0;
  int curlen = 0;
  
  if (!f) {
    printf("error: could not open config file \"%s\"", filename);
    return NULL;
  }
  
  cliini_args *args = cliini_args_new();
  currsection[0] = '\0';
  
  while(1) {
    line_end = (char*)memchr(buf+bufpos,'\n',curlen-bufpos);
    if (!line_end) {
      if (curlen && bufpos == 0) {
        printf("FIXME: error: max line size reached");
        abort();
      }
      
      fseek(f, filepos, SEEK_SET);
      curlen = fread(buf, 1, maxsize, f);
      if (!curlen) 
        break;
      if (curlen < maxsize) {
        buf[curlen] = '\n'; //at line break at end of file
        curlen++;
      }
      //printf("read %d!\n", curlen);
      //printf("read %s\n", buf);
      bufpos = 0;
      continue;
    }
    *line_end = '\0';
    //printf("%s\n", buf+bufpos);
    error += parse_line(buf+bufpos, args, group, currsection);
    //put to 1 after last line
    //printf("proc %d\n", line_end-(buf+bufpos)+1);
    filepos += line_end-(buf+bufpos)+1;
    //printf("filepos %d\n", filepos);
    bufpos = line_end-buf+1;
  }
  
  fclose(f);
  
  if (error) {
    printf("encountered %d errors while parsing file %s\n", error, filename);
    return NULL;
  }
  
  return args;
}


CLIINI_EXPORT cliini_args *cliini_parsebuf(char *buf, cliini_optgroup *group)
{
  char *line_end;
  char currsection[SECTION_MAXLEN];
  
  int error = 0;
  int filepos = 0;
  int bufpos = 0;
  int curlen = strlen(buf);
  
  if (!buf) {
    printf("error: passed NULL buffer!\n");
    return NULL;
  }  
  cliini_args *args = cliini_args_new();
  currsection[0] = '\0';
  
  buf = strdup(buf);
  
  while(1) {
    line_end = (char*)memchr(buf+bufpos,'\n',curlen-bufpos);
    if (!line_end)
      break;
    *line_end = '\0';
    error += parse_line(buf+bufpos, args, group, currsection);
    bufpos = line_end-buf+1;
  }
  
  free(buf);
  
  if (error) {
    printf("encountered %d errors while parsing\n", error);
    return NULL;
  }
  
  return args;
}

static int gettype_string(char *str)
{
  if (!strcmp(str, "STRING"))
    return CLIINI_STRING;
  else if (!strcmp(str, "DOUBLE"))
    return CLIINI_DOUBLE;
  else if (!strcmp(str, "INT"))
    return CLIINI_INT;
  else if (!strcmp(str, "ENUM"))
    return CLIINI_ENUM;
  
  return -1;
}

static char *type_str(int type)
{
  switch (type) {
    case CLIINI_STRING : return "STRING";
    case CLIINI_INT : return "INT";
    case CLIINI_DOUBLE : return "DOUBLE";
  }
  return "UNKNOWN";
}

static char *print_arg_val(int type)
{
  switch (type) {
    case CLIINI_STRING : return "STRING";
    case CLIINI_INT : return "INT";
    case CLIINI_DOUBLE : return "DOUBLE";
  }
  return "UNKNOWN";
}

static void print_strs(char **vals, int count)
{
  int i;
  for(i=0;i<count;i++)
    printf(" %s", vals[i]);
}

static void print_ints(int *vals, int count)
{
  int i;
  for(i=0;i<count;i++)
    printf(" %d", vals[i]);
}

static void print_doubles(double *vals, int count)
{
  int i;
  for(i=0;i<count;i++)
    printf(" %f", vals[i]);
}

CLIINI_EXPORT void cliini_print_arg(cliini_arg *arg)
{
  int i;
  
  printf("option [%s] type [%s] vals :", arg->opt->longflag, type_str(arg->opt->type));
  
  switch (arg->opt->type) {
    case CLIINI_STRING : 
      print_strs((char**)arg->vals, arg->sum);
      break;
    case CLIINI_INT : 
      print_ints((int*)arg->vals, arg->sum);
      break;
    case CLIINI_DOUBLE : 
      print_doubles((double*)arg->vals, arg->sum);
      break;
  }
  
  printf("\n");
}

CLIINI_EXPORT int cliini_fit_typeopts(cliini_args *args, cliini_args *typeargs)
{
  int i;
  int error = 0;
  cliini_arg *arg;
  cliini_arg *typearg;
  
  if (!typeargs) {
    printf("not types supplied!\n");
    return 1;
  }

  for(i=0;i<args->count;i++) {    
    arg = &args->args[i];
    if (!(arg->opt->flags & CLIINI_OPT_UNKNOWN))
      continue;
    typearg = cliargs_get_glob(typeargs, arg->opt->longflag);
    
    if (!typearg) {
      printf("could not find type for opt %s!\n", arg->opt->longflag);
      error++;
      continue;
    }
    
    int type = gettype_string(cliarg_str(typearg));
    int typeerror = 0;
    
    if (type < 0) {
      printf("ERROR: unknown type %s\n", cliarg_str(typearg));
      error++;
      typeerror = 1;
      continue;
    }
    
    arg->opt->type = type;
    
    char **oldvals = (char**)arg->vals;
    
    //TODO duplicate strings for ENUM and STRING?
    if (type == CLIINI_ENUM) {
      printf("possible enum values for %s: ", arg->opt->longflag);
	  arg->opt->enums = (char**)calloc(_type_size(type)*(cliarg_sum(typearg) - 1), 1);
      for(int i=1;i<cliarg_sum(typearg);i++) {
        arg->opt->enums[i-1] = cliarg_nth_str(typearg, i);
        printf("%s ", cliarg_nth_str(typearg, i));
      }
      printf("\n");
    }

    arg->vals = calloc(_type_size(type)*cliarg_sum(arg), 1);
    for(int i=0;i<cliarg_sum(arg);i++)
      if (_cliini_opt_arg_invalid(arg->opt, ((char**)oldvals)[i])) {
        printf("error parsing arg: %s\n", oldvals[i]);
        error++;
        typeerror++;
      }
      
    if (typeerror)
      continue;
    
    for(int i=0;i<cliarg_sum(arg);i++)
      _cliini_opt_arg_store_val(arg->opt, oldvals[i], (char*)arg->vals + _type_size(type)*i);
  }
  
  return error;
}

CLIINI_EXPORT cliini_arg *cliargs_get(cliini_args *args, const char *name)
{
  if (!args)
    return NULL;
  
  int i;
  for(i=0;i<args->count;i++)
    if (!strcmp(args->args[i].opt->longflag, name))
      return &args->args[i];
  return NULL;
}

CLIINI_EXPORT cliini_arg *cliargs_get_glob(cliini_args *args, const char *name)
{
  if (!args)
    return NULL;
  
  int i;
  for(i=0;i<args->count;i++)
    if (!fnmatch(args->args[i].opt->longflag, name, FNM_PATHNAME))
      return &args->args[i];
  return NULL;
}

CLIINI_EXPORT int cliarg_inst_count(cliini_arg *arg)
{
  RETNULLONFALSE(arg)

  return arg->inst_count;
}

CLIINI_EXPORT int cliarg_inst_arg_count(cliini_arg *arg, int inst)
{
  if (!arg || inst >= arg->inst_count)
    return 0;
  
  return arg->counts[inst];
}

CLIINI_EXPORT int cliargs_count(cliini_args *args)
{
  if (!args)
    return NULL;

  return args->count;
}

CLIINI_EXPORT cliini_arg *cliargs_nth(cliini_args *args, int n)
{
  return &args->args[n];
}

CLIINI_EXPORT int cliarg_sum(cliini_arg *arg)
{
  if (!arg)
    return 0;
  return arg->sum;
}

CLIINI_EXPORT void cliarg_strs(cliini_arg *arg, char **vals)
{
  RETONFALSE(arg)
  RETONFALSE(vals)
  int i;
  memcpy((void*)vals, arg->vals, sizeof(char*)*arg->sum);
}

CLIINI_EXPORT void cliarg_doubles(cliini_arg *arg, double *vals)
{
  RETONFALSE(arg)
  RETONFALSE(vals)
  int i;
  memcpy((void*)vals, arg->vals, sizeof(double)*arg->sum);
}

CLIINI_EXPORT void cliarg_ints(cliini_arg *arg, int *vals)
{
  RETONFALSE(arg)
  RETONFALSE(vals)
  int i;
  memcpy((void*)vals, arg->vals, sizeof(int)*arg->sum);
}

CLIINI_EXPORT char  *cliarg_str(cliini_arg *arg)
{
  RETNULLONFALSE(arg)
  return ((char**)arg->vals)[0];
}

CLIINI_EXPORT char  *cliarg_nth_str(cliini_arg *arg, int n)
{
  RETNULLONFALSE(arg)
  return ((char**)arg->vals)[n];
}

CLIINI_EXPORT int cliarg_nth_int(cliini_arg *arg, int n)
{
  RETNULLONFALSE(arg)
  return ((int*)arg->vals)[n];
}

//static int _max_help_len(cliini_optgroup *group)

static void print_help_opt(cliini_opt *opt)
{
  RETONFALSE(opt)
  
  if (opt->flag && !opt->longflag)
    printf("-%c ", opt->flag);
  if (opt->longflag && !opt->flag)
    printf("--%s ", opt->longflag);
  if (opt->flag && opt->longflag)
    printf("--%s/-%c ", opt->longflag, opt->flag);
  
  if (opt->help_args)
    printf("%s ", opt->help_args);
  else {
    
  }
  
  if (opt->help)
    printf("%s ", opt->help);
  
  printf("\n");
}

CLIINI_EXPORT void cliini_help(cliini_optgroup *group)
{
  RETONFALSE(group)
  
  for(int i=0;i<group->group_count;i++)
    cliini_help(&group->groups[i]);
  
  for(int i=0;i<group->opt_count;i++)
    print_help_opt(&group->opts[i]);
    
}
