#include "cliini.hpp"

namespace cliini
{
  
int cliarg::count() { return cliarg_sum(arg); }

int cliarg::num() { return num(0); }
double cliarg::flt() { return flt(0); }
std::string cliarg::str() { return str(0); }

int cliarg::num(int n)
{
  if (count() > n)
    return cliarg_nth_int(arg, n);
  else
    return 0;
}
double cliarg::flt(int n)
{
  if (count() > n)
    return cliarg_nth_double(arg, n);
  else
    return 0;
}
std::string cliarg::str(int n)
{
  if (count() > n)
    return cliarg_nth_str(arg, n);
  else
    return std::string();
}

bool cliarg::valid()
{
  if (arg)
    return true;
  else
    return false;
}


cliargs::cliargs(const int argc, const char *argv[], cliini_optgroup *group)
{
  args = cliini_parsopts(argc, argv, group);
};
cliargs::cliargs(const char *filename, cliini_optgroup *group)
{
  args = cliini_parsefile(filename, group);
}
cliargs::cliargs(char *buf, cliini_optgroup *group)
{
  args = cliini_parsebuf(buf, group);
}

cliarg cliargs::operator[](const char *name)
{
  return cliargs_get(args, name);
}

bool cliargs::have(const char *name)
{
  if (cliargs_get(args, name))
    return true;
  else
    return false;
}

} // namespace cliini
