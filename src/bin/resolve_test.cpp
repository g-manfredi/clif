#include "cliini.h"

#include <clif/dataset.hpp>

#include <iostream>

using namespace clif;
using namespace std;

int main(const int argc, const char *argv[])
{
  Attributes attrs;
  
  Attribute a;
  
  a.setName("blub/dir/hut");
  a.set("a");
  attrs.append(a);
  
  a.setName("blub/dir/hallo");
  a.set("b");
  attrs.append(a);
  
  Attribute link("rudi");
  link.setLink("blub");
  attrs.append(link);

  cpath r = attrs.resolve("rudi/flop");
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
  
  ClifFile f;
  f.create("_test.clif");
  Dataset *s0 = f.createDataset();
  s0->append(attrs);
  //FIXME should be deleted by f.close!
  delete s0;
  f.close();
  
  ClifFile f2;
  f2.open("_test.clif");
  Dataset *s = f2.openDataset();
  
  r = s->resolve("rudi/flop");
  cout << r << endl;
  r = s->resolve("blub/dir");
  cout << r << endl;
  r = s->resolve("blub/rudi");
  cout << r << endl;
  r = s->resolve("rudi");
  cout << r << endl;
  r = s->resolve("rudi/dir/hut");
  cout << r << endl;
  r = s->resolve("rudi/dir/randeluff");
  cout << r << endl;
  

  return EXIT_SUCCESS;
}
