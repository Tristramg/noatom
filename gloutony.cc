#include "gloutony.h"
#include "constraints.h"
#include <gecode/driver.hh>
#include "instance.h"

using namespace Gecode;

void build_solution(const Constraints & s)
{
    Instance & data = *Instance::get();
    int steps_per_week = data.timesteps / data.weeks;

    for(int s = 0; s < data.scenario; s++)
    {
        for(size_t t = 0; t < data.timesteps; t++)
        {
            double p1min = 0;
            for(int j = 0; j < data.powerplant1; j++)
                p1min += data.plants1[j].pmin[s][t];
            double demand = data.demand[s][t];

            double p2max = 0;
            for(int i = 0; i < data.powerplant2; i++)
                p2max += data.plants2[i].pmax[t];
        }
    }

}

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
    std::string sets[] = {"../data0.txt", "../data1.txt", "../data2.txt", "../data3.txt", "../data4.txt", "../data5.txt"};
    
    Instance::build(sets[opt.model()]);

    Constraints c(opt);
    c.status();
    c.print(std::cout);
    Script::run<Constraints, DFS, Options>(opt);
    //while (Constraints * s = e.next()) {
    //    s->print(std::cout); delete s;
    //}
    Instance::destroy();

}
