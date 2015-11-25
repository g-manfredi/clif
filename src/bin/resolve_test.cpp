#include "cliini.h"

#include "attribute.hpp"

#include <iostream>

using namespace clif;
using namespace std;

int main(const int argc, const char *argv[])
{
  Attributes attrs;
  
  attrs.append(Attribute("blub/dir/hut"));
  attrs.append(Attribute("blub/dir/hallo"));
  Attribute link("rudi");
  link.setLink("blub");
  attrs.append(link);

  path r = attrs.resolve("rudi/flop");
  cout << r << endl;
  r = attrs.resolve("blub/dir");
  cout << r << endl;
  r = attrs.resolve("blub/rudi");
  cout << r << endl;
  r = attrs.resolve("rudi");
  cout << r << endl;
  r = attrs.resolve("rudi/dir/hut");
  cout << r << endl;
  r = attrs.resolve("rudi/dir/randeluff");
  cout << r << endl;

  return EXIT_SUCCESS;
}