#include "gloutony.h"
#include "constraints.h"
#include <gecode/driver.hh>

using namespace Gecode;

int main(int argc, char * argv[])
{
    Options opt("NoAtom!");
    opt.model(0, "0", "data0.txt");
    opt.model(1, "1", "data1.txt");
    opt.model(2, "2", "data2.txt");
    opt.model(3, "3", "data3.txt");
    opt.model(4, "4", "data4.txt");
    opt.model(5, "5", "data5.txt");
    opt.model(0); //default value
    opt.parse(argc, argv);

    Script::run<Constraints, DFS, Options>(opt);
    //while (Constraints * s = e.next()) {
    //    s->print(std::cout); delete s;
    //}

}
