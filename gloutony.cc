#include "gloutony.h"
#include <gecode/driver.hh>
#include <gecode/minimodel.hh>
#include <fstream>
#include <boost/program_options.hpp>
#include <limits>
struct infeasible{
    enum what {OUTAGE_LATER};
    what w;
    size_t t;
    int i;
    int k;
};
//TODO (29/12)
// Gérer proprement les les courbes de décroissance DONE
// Gérer la mise à jour des stocks en début de période DONE
// Générer le fichier de sortie DONE
//
// Problèmes potentiels:
// Le stock min à respecter avant l'outage [CT7] [CT11], probablement trivial UPDATE : bof... chiant en fait
// La [CT12] de max modulation (OUH PUTAIN MERDE)
//
// TODO long-terme
// Rajouter des contraintes globales de prod min/max sur les centrales
// et optimiser dessus (bloc contraintes)

using namespace Gecode;
using namespace boost::posix_time;
namespace po = boost::program_options;

float pb(int i, int k, float x)
{
    float x1, x2;
    float y1, y2;
    Instance & data = *Instance::get();

    std::vector<int> c;
    std::vector<float> f;
    if(k < 0)
    {
        c = data.plants2[i].current_decrease_profile_idx;
        f = data.plants2[i].current_decrease_profile_val;
    }
    else
    {
        c = data.plants2[i].decrease_profile_idx[k];
        f = data.plants2[i].decrease_profile_val[k];
    }

    for(size_t z = 0; z < c.size(); z++)
    {
        if( c[z] <= x )
        {
            x1 = c[z]; x2 = c.at(z-1);
            y1 = f[z]; y2 = f.at(z-1);
            //Pff... 15 min pour retrouver cette fonction affine
            //Niveau 3e...
            float a = (y1 - y2) / (x1 - x2);
            float b = y1 - a * x1;
            return a * x + b;
        }
    }
    std::cout << "Uho..." << x << std::endl;
    return -1;
}

float get_pmax(int i, int t, int k, float x)
{
    Instance & data = *Instance::get();

    int BO;
    if(k < 0)
        BO = data.plants2[i].current_stock_threshold;
    else
        BO = data.plants2[i].stock_threshold[k];

    if(x < BO)
    {
        float ratio = pb(i, k, x);
        BOOST_ASSERT(ratio >= 0 && ratio <= 1);
        return data.plants2[i].pmax[t] * ratio;
    }
    else
        return data.plants2[i].pmax[t];

    return -1;
}

Solution::Solution() {}
Solution::Solution(const Constraints & c, const Instance & data):
        p1(boost::extents[data.powerplant1][data.timesteps][data.scenario]),
        p2(boost::extents[data.powerplant2][data.timesteps][data.scenario]),
        x(boost::extents[data.powerplant2][data.timesteps + 1][data.scenario]),
    r(boost::extents[data.powerplant2][data.campaigns])
{
    float fuel_cost = 0;
    float p1_cost = 0;
    float remaining_fuel = 0; 
    int steps_per_week = data.timesteps / data.weeks;

    int low_demand = 0;


    for(int s = 0; s < data.scenario; s++)
    {
        //CT8
        for(int i = 0; i < data.powerplant2; i++)
        {
            x[i][0][s] = data.plants2[i].stock;
            remaining_fuel -= x[i][data.timesteps][s] * data.plants2[i].fuel_price;
        }

        for(size_t t = 0; t < data.timesteps; t++)
        {
            float ratio = 1.0;
            float p1min = 0;
            for(int j = 0; j < data.powerplant1; j++)
                p1min += data.plants1[j].pmin[s][t];
            float demand = data.demand[s][t];

            float p2mod = 0;
            float p2dec = 0;
            for(int i = 0; i < data.powerplant2; i++)
            {
                int k = c.get_campaign(i,t);
                float pmax = get_pmax(i, t, k, x[i][t][s]);
                if(pmax < data.plants2[i].pmax[t])
                    p2dec += pmax;
                else
                    p2mod += pmax;
            }
            for(int i = 0; i < data.powerplant2; i++)
                if (demand - p1min - p2dec < p2mod)
                {
                    low_demand++;
                    ratio = (demand - p1min - p2dec) / p2mod;
                }

            for(int i = 0; i < data.powerplant2; i++)
            {
                int k = c.get_campaign(i,t);
                if( !c.is_out(i, t / steps_per_week) )
                { 
                    int k = c.get_campaign(i,t);
                    float pmax = get_pmax(i, t, k, x[i][t][s]);
                    if(pmax < data.plants2[i].pmax[t])
                        p2[i][t][s] = pmax;
                    else
                        p2[i][t][s] = pmax * ratio;

                    if(x[i][t][s] < p2[i][t][s] * data.durations[t])
                        p2[i][t][s] = 0;

                    x[i][t+1][s] = x[i][t][s] - p2[i][t][s] * data.durations[t]; // CT9
                    BOOST_ASSERT(x[i][t+1][s] >= 0);
                }
                else 
                {
                    p2[i][t][s] = 0;
                    if(c.first_outage(i, t))
                    {
                        if(k >= 0)
                        {
                            //CT10
                            float max = data.plants2[i].max_stock_after_refueling[k];
                            x[i][t+1][s] = max;
                            int BOk = data.plants2[i].stock_threshold[k];
                            int BO1;
                            if(k < 1)
                                BO1 = data.plants2[i].current_stock_threshold;
                            else
                                BO1 = data.plants2[i].stock_threshold[k - 1];

                            max -= BOk;
                            max -= ((data.plants2[i].refuel_ratio[k] -1) / data.plants2[i].refuel_ratio[k]) * (x[i][t][s] - BO1);
                            r[i][k] = max;
                            fuel_cost += r[i][k] * data.plants2[i].refueling_cost[k];


                            //CT7 
                            //CT11
                            //TODO se casser le cul à trouver un moyen de gérer ça
                            // Possibilité 1 : retarder le outage
                            // Possibilité 2 : se casser le cul à faire produire la centrale plus (boarf...)

                            // Tentative de la technique 1
                            if(x[i][t][s] > data.plants2[i].max_stock_before_refueling[k] || r[i][k] < data.plants2[i].min_refuel[k])
                            {
                                infeasible e;
                                e.i = i;
                                e.t = t;
                                e.k = k;
                                e.w = infeasible::OUTAGE_LATER;
                                throw e;
                            }
                        }
                    }
                    else
                    {
                        x[i][t+1][s] = x[i][t][s];
                    }
                }
            }

            //TODO partir de la centrale qui coûte le plus cher
            // Après tout, c'est ça qui coute le plus cher !
            float to_produce = demand - (p2mod * ratio) - p1min - p2dec;
            for(int j = 0; j < data.powerplant1; j++)
            {
                float prod = std::min(to_produce, data.plants1[j].pmax[s][t] - data.plants1[j].pmin[s][t]);
                to_produce -= prod;
                p1[j][t][s] = data.plants1[j].pmin[s][t] + prod;
                p1_cost += data.plants1[j].cost[s][t] * p1[j][t][s] * data.durations[t];
            }
            BOOST_ASSERT(to_produce <= 0);
        }
    }

    //TODO : déduire le coût du fuel à la fin

    std::cout << "Refuel_cost: " << fuel_cost << std::endl;
    std::cout << "P1 cost: " << p1_cost/data.scenario << std::endl;
    cost = fuel_cost + (p1_cost - remaining_fuel) / data.scenario;
    std::cout << "Total cost: " << cost << std::endl;
}

void Solution::write(const std::string & filename, const Constraints & c, ptime start) const
{
    Instance & data = *Instance::get();
    std::ofstream of(filename.c_str());
    time_facet* output_facet = new time_facet();
    of.imbue(std::locale(std::locale::classic(), output_facet));
    output_facet->format("%d/%m/%y %H:%M:%S");
    output_facet->time_duration_format("%H:%M:%S");

    ptime t(second_clock::local_time());

    time_duration duration = t - start;

    of << "begin main\n"
        << "team_identifier J10\n"
        << "solution_time_date " << t << "\n"
        << "solution_running_time " << duration << "\n"
        << "data_set " << data.dataset << "\n"
        << "cost " << cost << "\n"
        << "end main\n"

        << "begin outages\n";
        for(int i=0; i < data.powerplant2; i++)
        {
            of << "name " << data.plants2[i].name << "\n"
                << "index " << data.plants2[i].index << "\n"
                << "outage_dates";
            for(int k=0; k < data.campaigns; k++)
            {
                of << " " << c.ha[i + k * data.powerplant2].val();
            }
            of << "\n"
                << "reloaded fuel";
            for(int k=0; k < data.campaigns; k++)
            {
                of << " " << (int)r[i][k];
            }
            of << "\n";
        }
        of << "end outages\n"

            << "begin power_output\n";
        for(int s=0; s < data.scenario; s++)
        {
            of << "scenario " << s << "\n"
                << "begin type_1_plants\n";
            for(int j=0; j < data.powerplant1; j++)
            {
                of << "name " << data.plants1[j].name << " " << data.plants1[j].index;
                for(size_t t=0; t < data.timesteps; t++)
                {
                    of << " " << p1[j][t][s];
                }
                of << "\n";

            }

            of << "end type_1_plants\n"
                << "begin type_2_plants\n";
            for(int i=0; i < data.powerplant2; i++)
            {
                of << "name " << data.plants2[i].name << " " << data.plants2[i].index;
                for(size_t t=0; t < data.timesteps; t++)
                {
                    of << " " << p2[i][t][s];
                }
                of << "\n"
                    << "fuel_variation";
                for(size_t t=0; t < data.timesteps; t++)
                {
                    of << " " << (int)x[i][t][s];
                }
                of << "\n"
                    << "remaining_fuel_at_the_end " << x[i][data.timesteps][s] << "\n";
            }
            of << "end type_2_plants\n";
        }
        of << "end power_output";


        
}

int main(int argc, char * argv[])
{
    ptime start(second_clock::local_time());

    po::options_description desc("Allowed options");
    std::string infile, outfile;
    int max_time;

    desc.add_options()
        ("help,h", "Display this help message")
        ("time,t", po::value<int>(&max_time)->default_value(1800), "Maximum execution time in seconds")
        ("instance,n", po::value<std::string>(&infile), "Instance data file")
        ("team,i", "Show the team identifier")
        ("result,r", po::value<std::string>(&outfile), "Output file containing the solution");

    po::options_description cmdline_options;
    cmdline_options.add(desc);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(argc == 1 || vm.count("help"))
    {
        std::cout << desc << std::endl;
        return (EXIT_SUCCESS);
    }

    if(vm.count("team"))
    {
        std::cout << "J10" << std::endl;
        return (EXIT_SUCCESS);
    }

    if(!vm.count("result") && !vm.count("instance"))
    {
        std::cout << desc << std::endl;
        return (EXIT_SUCCESS);

    }

    Instance::build(infile);
    Instance & data = *Instance::get();

    Constraints c;
    c.status();
    c.print(std::cout);
    bool stop = false;
    Constraints * best = 0;
    float best_cost = std::numeric_limits<float>::max();
    while(!stop)
    {
        DFS<Constraints> e(&c);
        //    Script::run<Constraints, DFS, Options>(opt);
        try{
            std::cout << "Trying to get new solution" << std::endl;
            while (!stop)
            {
                Constraints * s = e.next();
                if(s)
                {
                    Solution sol(*s, data);
                    if(sol.cost < best_cost)
                    {
                        if(best != 0)
                            delete best;
                        best = s;
                        best_cost = sol.cost;
                        sol.write(outfile, *best, start);
                    }
                    else
                        delete s;
                    if( (second_clock::local_time() - start).total_seconds() > max_time - 60)
                        stop = true; 
                }
                else stop = true;
            }
            stop = true;
        }
        catch(infeasible i)
        {
            if(i.w == infeasible::OUTAGE_LATER)
            {
                std::cout << "Setting outage start later of plant " << i.i << ", campaign " << i.k << std::endl;
                int idx = i.i + i.k * data.powerplant2;
                if(c.ha[idx].min() == -1)
                {
                    post(c, c.ha[idx] == -1);
                }
                else
                {
                    post(c, c.ha[idx] > c.ha[idx].min());
                }
            }
        }
        if( (second_clock::local_time() - start).total_seconds() > max_time - 60)
            stop = true; 
    }

    Solution sol(*best, data);
    sol.write(outfile, *best, start);
    delete best;
    Instance::destroy();
}
