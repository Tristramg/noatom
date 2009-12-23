#include "instance.h"
#include "outages.h"

#include "coin/ClpSimplex.hpp"

#include <boost/assert.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

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
    std::cout << "P1 max=" << VarIndex::max << std::endl;
    VarIndex p2(data.powerplant2, data.timesteps, data.scenario); //p(i,t,s)
    std::cout << "P2 max=" << VarIndex::max << std::endl;
    VarIndex x(data.powerplant2, data.timesteps + 1, data.scenario); //x(i, t, s)
    std::cout << "x max=" << VarIndex::max << std::endl;
    VarIndex r(data.powerplant2, data.campaigns); // r(i,k)
    std::cout << "r max=" << VarIndex::max << std::endl;
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
                objective[p1(j,t,s)] = data.plants1[j].cost.at(s).at(t) * data.durations.at(t) / data.scenario;
            }
        }
        for(int i = 0; i < data.powerplant2; i++)
        {
            objective.at(x(i,data.timesteps, s)) = - data.plants2[i].fuel_price / data.scenario;
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
                colUpper[p2(i,t,s)] = data.plants2[i].pmax[t];
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
    int row = 0;
    std::vector<std::string> ct;

    //[CT1] Production must meet load
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
            ct.push_back("ct1");
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
            std::vector<size_t> out_t = out.ea(i,k);
            for(int s = 0; s < data.scenario; s++)
            {
                //TODO: remplacer par des foreach (scrogneugneu de train sans internet)
                //PS: scrogneugneu de freebox en rade
                for(std::vector<size_t>::const_iterator it = out_t.begin(); it != out_t.end(); it++)
                {
                    colUpper[p2(i,*it,s)] = 0;
                }
            }
        }
    }

    //CT-perso. During outages, stock must be constant !
    for(int i = 0; i < data.powerplant2; i++)
    {
        for(int k = 0; k < data.campaigns; k++)
        {
            std::vector<size_t> out_t = out.ea(i,k);
            for(int s = 0; s < data.scenario; s++)
            {
                //TODO: remplacer par des foreach (scrogneugneu de train sans internet)
                //PS: scrogneugneu de freebox en rade
                for(std::vector<size_t>::const_iterator it = out_t.begin() + 1; it != out_t.end(); it++)
                {
                    cols.push_back(x(i,*it,s));
                    rows.push_back(row);
                    vals.push_back(1);
                    cols.push_back(x(i, *it + 1, s));
                    rows.push_back(row);
                    vals.push_back(-1);
                    rowUpper.push_back(0);
                    rowLower.push_back(0);
                        std::stringstream ss;
                        ss << "ctp i:" << i << " t:" << *it << " s:" << s;
                        ct.push_back(ss.str());
 
                    row++;
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
    
    //
    //[CT9] Fuel stock variation during production campaign
    for(int i = 0; i < data.powerplant2; i++)
    {
        for(int k = 0; k < data.campaigns; k++)
        {
            std::vector<size_t> out_t = out.ec(i,k);
            for(int s = 0; s < data.scenario; s++)
            {
                for(std::vector<size_t>::const_iterator it = out_t.begin() + 1; it != out_t.end(); it++)
                {
                    if(*it < data.timesteps)
                    {
                        cols.push_back(x(i, *it , s));
                        rows.push_back(row);
                        vals.push_back(1);

                        cols.push_back(x(i, *it - 1, s));
                        rows.push_back(row);
                        vals.push_back(-1);

                        cols.push_back(p2(i, *it - 1, s));
                        rows.push_back(row);
                        vals.push_back(data.durations[*it]);

                        rowLower.push_back(0);
                        rowUpper.push_back(0);

                        std::stringstream s;
                        s << "ct9 " << *it;
                        ct.push_back(s.str());
                        row++;

                    }

                }

            }

        }
    }
    
    
    //[CT10] Fuel variation during an outage
    //... and [CT11] as a bonus
    for(int i = 0 ; i < data.powerplant2; i++)
    {
        for(int k = 0; k < data.campaigns; k++)
        {
            for(int s = 0; s < data.scenario; s++)
            {
                
                if(out.ea(i,k).size() > 0)
                {
                    size_t t = out.ea(i, k).front();
                    BOOST_ASSERT(t < data.timesteps);
                    cols.push_back(x(i, t + 1, s));
                    rows.push_back(row);
                    vals.push_back(1);

                    cols.push_back(x(i, t, s));
                    rows.push_back(row);
                    int Qik = data.plants2[i].refuel_ratio[k];
//                    vals.push_back(-1);
                    vals.push_back(-(Qik-1)/Qik);

                    cols.push_back(r(i,k));
                    rows.push_back(row);
                    vals.push_back(-1);

                    //ATTENTION : ai-je bien compris le conditions en 0 ?
                    int BO1;
                    if(k == 0)
                        BO1 = data.plants2[i].current_stock_threshold;
                    else
                        BO1 = data.plants2[i].stock_threshold[k - 1];

                    int BO2 = data.plants2[i].stock_threshold[k];

                    float bound = -((Qik-1)/Qik) * BO1 + BO2;
//                    float bound = 0;
                    rowLower.push_back(bound);
                    rowUpper.push_back(bound);
                    ct.push_back("ct10");
                    row++;

                    // [CT11]
                    // ATTENTION : faut regarder si ces bornes ne sont pas déjà définies ailleurs
                    // TODO: voir si on peut pas gérer ça en partie au travers des bornes sur les variables
                    cols.push_back(x(i,t,s));
                    rows.push_back(row);
                    vals.push_back(1);
                    rowLower.push_back(0);
                    rowUpper.push_back(data.plants2[i].max_stock_before_refueling[k]);
                        std::stringstream ss;
                        ss << "ct11 i:" << i << " t:" << t << " s:" << s;
                        ct.push_back(ss.str());
                    row++;

                    cols.push_back(x(i,t+1, s));
                    rows.push_back(row);
                    vals.push_back(1);
                    rowLower.push_back(0);
                    rowUpper.push_back(data.plants2[i].max_stock_after_refueling[k]);
                    ct.push_back("ct11");
                    row++;
                        
                }
            }
        }
    }

    
    BOOST_ASSERT(rows.size() == cols.size());
    BOOST_ASSERT(cols.size() == vals.size());
    CoinPackedMatrix constraints(true, &rows[0], &cols[0], &vals[0], rows.size());

    ClpSimplex modele;
//    modele.setOptimizationDirection(-1);
//    modele.setInfeasibilityCost(10e13);
    modele.setPrimalTolerance(1e-4);
    modele.setDualTolerance(1e-4);
    modele.scaling(); // Ça sert à quoi ? ça sonne bien et ça accélère un foil


    BOOST_ASSERT(rowLower.size() == rowUpper.size());
    BOOST_ASSERT(rowLower.size() == row);
    modele.loadProblem(constraints, &colLower[0], &colUpper[0], &objective[0], &rowLower[0], &rowUpper[0]);

    std::cout << "Ok" << std::endl;

    std::cout << "Plants type1: " << data.powerplant1
        << ", plants type2: " << data.powerplant2
        << ", timesteps: " << data.timesteps
        << ", scenarios: " << data.scenario << std::endl;
    std::cout << "Variables: " << VarIndex::max << ", constraints: " << row << std::endl << out;
    
    std::cout << std::endl << "Running presolve " << std::endl;
    modele.initialSolve();
   // modele.writeMps("test.mps");
    std::cout << std::endl << "Running primal algorithm" << std::endl;
    modele.primal();
    if(modele.isProvenPrimalInfeasible())
    {
        std::cout << "Proven infeasible :'(" << std::endl;
        const double * dual = modele.infeasibilityRay();
        for(int i=0; i < modele.getNumRows(); i++)
        {
            if(dual[i] != 0)
                std::cout << i << ":" << dual[i] << " " << ct[i] << std::endl;
        }
    }
    else
        std::cout << "Lower bound="  << modele.objectiveValue() << std::endl;

    if(modele.isProvenDualInfeasible())
    {
        std::cout << "Proven dual infeasible!" << std::endl;
        std::cout << "Sum of dual infeasibilities: " << modele.sumDualInfeasibilities() << std::endl;
        std::cout << "Num of dual infeasibilities: " << modele.numberDualInfeasibilities() << std::endl;
        const double * dual = modele.dualColumnSolution();
        for(int i=0; i < modele.getNumCols(); i++)
        {
            if(dual[i] != 0)
                std::cout << i << ":" << dual[i] << " "  << std::endl;
        }
 
    }
    double * columnPrimal = modele.primalColumnSolution();
    double * columnDual = modele.dualColumnSolution();

    for(int s = 0; s < data.scenario; s++)
    {
        std::ofstream output("output.dat" );
        for(size_t t = 0; t < data.timesteps; t++)
        {
            output << t << " " << data.demand[s][t];
            for(int i = 0; i < data.powerplant2; i++)
            {
                output << "   " << columnPrimal[p2(i,t,s)] << " " << columnPrimal[x(i,t,s)];
            }
            output << "   ";
            for(int j = 0; j < data.powerplant1; j++)
            {
                output << " " << columnPrimal[p1(j,t,s)];
            }
            output << std::endl;
        }
    }

     
    return 0;
}

