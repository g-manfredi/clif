#include "clifview.h"
#include <QApplication>

#include "cliini.h"

cliini_opt opts[] = {
  {
    "input",
    1, //argcount min
    1, //argcount max
    CLIINI_STRING, //type
    0, //flags
    'i'
  },
  {
    "dataset",
    1, //argcount
    1, //argcount
    CLIINI_STRING, //type
    0, //flags
    'd'
  },
  {
    "store",
    1, //argcount
    1, //argcount
    CLIINI_STRING, //type
    0, //flags
    's'
  }
};

cliini_optgroup group = {
  opts,
  NULL,
  sizeof(opts)/sizeof(*opts),
  0,
  0
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    cliini_args *args = cliini_parsopts(argc, const_cast<const char**>(argv), &group);

    cliini_arg *input = cliargs_get(args, "input");
    cliini_arg *dataset = cliargs_get(args, "dataset");
    cliini_arg *store = cliargs_get(args, "store");
    
    ClifView w(cliarg_str(input),cliarg_str(dataset),cliarg_str(store));
    w.showMaximized();

    return a.exec();
}
