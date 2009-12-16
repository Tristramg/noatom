#include "instance.h"
#include "outages.h"

#include "coin/ClpSimplex.hpp"

#include <boost/assert.hpp>
#include <iostream>

struct VarIndex
{
    int offset;
    static int max;
    int max1, max2, max3;

    VarIndex(int max1, int max2, int max3 = 1) : offset(max), max1(max1), max2(max2), max3(max3)
    {
        max += max1 * max2 * max3;
    }

    int operator()(int v1, int v2, int v3 = 0) const
    {
        BOOST_ASSERT(v1 < max1);
        BOOST_ASSERT(v2 < max2);
        BOOST_ASSERT(v3 < max3);
        return offset + v1 + v2 * max1 + v3 * max1 * max2; 
    }
};

int VarIndex::max = 0;

int main(int argc, char ** argv)
{
    if(argc != 2)
    {
        std::cout << "Use: " << argv[0] << " data_file" << std::endl;
        return 1;
    }

    Instance data(argv[1]);

    std::cout << std::endl << "Building the model: " << std::flush;

    //Let's manage the variables !
    VarIndex p1(data.powerplant1, data.timesteps, data.scenario); //p(j,t,s)
    VarIndex p2(data.powerplant2, data.timesteps, data.scenario); //p(i,t,s)
    VarIndex r(data.powerplant2, data.campaigns); // r(i,k)
    VarIndex x(data.powerplant2, data.timesteps, data.scenario); //x(i, t, s)
    int numCols = VarIndex::max;

    std::vector<double> objective(numCols, 0);
    std::vector<double> colLower(numCols, 0);
    std::vector<double> colUpper(numCols, DBL_MAX);

    //Set the objectives
    for(int i = 0; i < data.powerplant2; i++)
    {
        for(int k = 0; k < data.campaigns; k++)
        {
            objective[r(i,k)] = data.plants2[i].refueling_cost[k];
        }
    }

    for(int s = 0; s < data.scenario; s++)
    {
        for(size_t t = 0; t < data.timesteps; t++)
        {
            for(int j = 0; j < data.powerplant1; j++)
            {
                objective[p1(j,t,s)] = data.plants1[j].cost[s][t] * data.durations[t] / data.scenario;
            }
        }
        for(int i = 0; i < data.powerplant2; i++)
        {
            // ATTENTION : est-ce que c'est vraiment 1 ?
            objective.at(x(i,data.timesteps - 1, s)) = - data.plants2[i].fuel_price / data.scenario;
        }
    }

    //[CT2] Min and max production on Type 1 plants
    for(int j = 0; j < data.powerplant1; j++)
    {
        for(size_t t = 0; t < data.timesteps; t++)
        {
            for(int s = 0; s < data.scenario; s++)
            {
                colLower[p1(j,t,s)] = data.plants1[j].pmin[s][t];
                colUpper[p1(j,t,s)] = data.plants1[j].pmax[s][t];
            }
        }
    }

    //[CT5] Max production on Type 2 plants
    // Attention !!! y'a les conditions qui sont bizares. On suppose ici qu'on a tjs pmax, outage ou non
    for(int i = 0; i < data.powerplant2; i++)
    {
        for(size_t t = 0; t < data.timesteps; t++)
        {
            for(int s = 0; s < data.scenario; s++)
            {
                colUpper[p2(i,t,s)] = data.plants2[i].pmax[i];
            }
        }
    }

    //[CT8] Initial fuel stock
    for(int i = 0; i < data.powerplant2; i++)
    {
        for(int s = 0; s < data.scenario; s++)
        {
            colLower[x(i,0,s)] = data.plants2[i].stock;
            colUpper[x(i,0,s)] = data.plants2[i].stock;
        }
    }

    std::vector<int> rows;
    std::vector<int> cols;
    std::vector<double> vals;
    std::vector<double> rowLower;
    std::vector<double> rowUpper;

    rows.reserve(data.scenario*data.timesteps*(data.powerplant1 + data.powerplant2));
    cols.reserve(data.scenario*data.timesteps*(data.powerplant1 + data.powerplant2));
    vals.reserve(data.scenario*data.timesteps*(data.powerplant1 + data.powerplant2));
    rowLower.reserve(data.scenario*data.timesteps);
    rowUpper.reserve(data.scenario*data.timesteps);
    
    //[CT1] Production must meet load
    int row = 0;
    for(int s = 0; s < data.scenario; s++)
    {
        for(size_t t = 0; t < data.timesteps; t++)
        {
            for(int j = 0; j < data.powerplant1; j++)
            {
                rows.push_back(row);
                cols.push_back(p1(j,t,s));
                vals.push_back(1);
            }
            for(int i = 0; i < data.powerplant2; i++)
            {
                rows.push_back(row);
                cols.push_back(p2(i,t,s));
                vals.push_back(1);
            }
            rowLower.push_back(data.demand[s][t]);
            rowUpper.push_back(data.demand[s][t]);
            row++;
        }
    }

    //[CT4] Minimum power: production must be positive
    // Skiped - taken in account by the bounds on variables


    // From this point, the constraints will change during resolution

    Outages out(data);

    //[CT3] during outages, production must be 0
    for(int i = 0; i < data.powerplant2; i++)
    {
        for(int k = 0; k < data.campaigns; k++)
        {
            std::vector<int> out_t = out.ea(i,k);
            for(int s = 0; s < data.scenario; s++)
            {
                //TODO: remplacer par des foreach (scrogneugneu de train sans internet)
                for(std::vector<int>::const_iterator it = out_t.begin(); it != out_t.end(); it++)
                {
                    colUpper[p2(i,*it,s)] = 0;
                }
            }
        }
    }

    //[CT7] Lower & Upper bound on refueling
    for(int i = 0; i < data.powerplant2; i++)
    {
        for(int k = 0; k < data.campaigns; k++)
        {
            if(out.ha(i,k) >= 0)
            {
                colLower[r(i,k)] = data.plants2[i].min_refuel[k];
                colUpper[r(i,k)] = data.plants2[i].max_refuel[k];
            }
            else
            {
                colLower[r(i,k)] = 0;
                colUpper[r(i,k)] = 0;
            }
        }
    }

    CoinPackedMatrix constraints(true, &rows[0], &cols[0], &vals[0], rows.size());

    ClpSimplex modele;

    modele.loadProblem(constraints, &colLower[0], &colUpper[0], &objective[0], &rowLower[0], &rowUpper[0]);

    std::cout << "Ok" << std::endl;

    modele.initialSolve();
    std::cout << "Computing lowerbound: " << std::endl;
    modele.dual();
    std::cout << "Lower bound=" << modele.rawObjectiveValue() << std::endl;

    return 0;
}

