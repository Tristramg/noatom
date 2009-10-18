#include <iostream>
#include <deque>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_lists.hpp> 
#include <boost/spirit/include/classic_file_iterator.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>

using namespace BOOST_SPIRIT_CLASSIC_NS;
typedef file_iterator<char>   iterator_t;
typedef scanner<iterator_t>     scanner_t;
typedef rule<scanner_t>         rule_t;

struct Powerplant_t1
{
    std::string name;
    int index;
    std::deque<std::vector<float> > pmin;
    std::deque<std::vector<float> > pmax;
    std::deque<std::vector<float> > cost;

    Powerplant_t1(size_t scenarios, size_t timesteps) :
        pmin(scenarios),
        pmax(scenarios),
        cost(scenarios)
    {
        for(size_t i=0; i < scenarios; i++)
        {
            pmin[i].reserve(timesteps);
            pmax[i].reserve(timesteps);
            cost[i].reserve(timesteps);
        }
    }

};

int main(int argc, char ** argv)
{
    if(argc != 2)
    {
        std::cout << "Use: " << argv[0] << " data_file" << std::endl;
        return 1;
    }

    file_iterator<> first(argv[1]);
    if(!first)
    {
        std::cout << "Unable to open file " << argv[1] << std::endl;
        return 1;
    }
    file_iterator<> last = first.make_end();

    int timesteps = 0;
    int weeks = 0;
    int campaigns = 0;
    int scenario = 0;
    float epsilon = 0;
    int powerplant1 = 0;
    int powerplant2 = 0;
    int constraint13 = 0;
    int constraint14 = 0;
    int constraint15 = 0;
    int constraint16 = 0;
    int constraint17 = 0;
    int constraint18 = 0;
    int constraint19 = 0;
    int constraint20 = 0;
    int constraint21 = 0;
    std::deque<int> durations;

    rule_t header_r = "begin main" >> eol_p
        >> "timesteps " >> int_p[assign_a(timesteps)] >> eol_p
        >> "weeks " >> int_p[assign_a(weeks)] >> eol_p
        >> "campaigns " >> int_p[assign_a(campaigns)] >> eol_p
        >> "scenario " >> int_p[assign_a(scenario)] >> eol_p
        >> "epsilon " >> real_p[assign_a(epsilon)] >> eol_p
        >> "powerplant1 " >> int_p[assign_a(powerplant1)] >> eol_p
        >> "powerplant2 " >> int_p[assign_a(powerplant2)] >> eol_p
        >> "constraint13 " >> int_p[assign_a(constraint13)] >> eol_p
        >> "constraint14 " >> int_p[assign_a(constraint14)] >> eol_p
        >> "constraint15 " >> int_p[assign_a(constraint15)] >> eol_p
        >> "constraint16 " >> int_p[assign_a(constraint16)] >> eol_p
        >> "constraint17 " >> int_p[assign_a(constraint17)] >> eol_p
        >> "constraint18 " >> int_p[assign_a(constraint18)] >> eol_p
        >> "constraint19 " >> int_p[assign_a(constraint19)] >> eol_p
        >> "constraint20 " >> int_p[assign_a(constraint20)] >> eol_p
        >> "constraint21 " >> int_p[assign_a(constraint21)] >> eol_p
        >> "durations" >> *(' '|int_p[push_back_a(durations)]) >> eol_p;


    std::cout << "Parsing header: " << std::flush;
    parse_info<iterator_t> info = parse(first, last, header_r);
    if(!info.hit)
    {
        std::cout << "Fail" << std::endl;
        return 1;
    }
    else if(durations.size() != timesteps)
    {
        std::cout << "wrong number of durations" << std::endl;
        return 1;
    }
    else
    {
        std::cout << "Ok" << std::endl; 
    }


    std::cout << "Parsing demand: " << std::flush;
    std::vector<std::vector<float> > demand(scenario);

    for(int i=0; i < scenario; i++) { demand[i].reserve(timesteps);
        rule_t demand_r = "demand" >> *(' '|real_p[push_back_a(demand[i])]) >> eol_p;
        info = parse(info.stop, last, demand_r);
        if(!info.hit)
        {
            std::cout << "Fail at scenario " << i << std::endl;
            return 1;
        }
        else if(demand[i].size() != timesteps)
        {
            std::cout << "wrong number of demand, scenario" << std::endl;
            return 1;
        }
    }
    std::cout << "Ok" << std::endl;

    info = parse(info.stop, last, "end main" >> eol_p);
    std::cout << "Main section done" << std::endl << std::endl;

    std::cout << "Parsing Powerplant type1: " << std::flush;
    std::deque<Powerplant_t1> plants1 (powerplant1, Powerplant_t1(scenario, timesteps));
    for(int i=0; i < powerplant1; i++)
    {
        rule_t type1_r = "begin powerplant" >> eol_p
            >> "name " >> (*~space_p)[assign_a(plants1[i].name)] >> eol_p
            >> "type 1" >> eol_p
            >> "index " >> int_p[assign_a(plants1[i].index)] >> eol_p
            >> "scenario " >> int_p >> eol_p
            >> "timesteps " >> int_p >> eol_p;

        info = parse(info.stop, last, type1_r);
        if(!info.hit)
        {
            std::cout << "Fail header of powerplant #" << i << std::endl;
            return 1;
        }

        for(int s=0; s < scenario; s++)
        {

            rule_t data_r = "pmin" >> *(' '|real_p[push_back_a(plants1[i].pmin[s])]) >> eol_p
                >> "pmax" >> *(' '|real_p[push_back_a(plants1[i].pmax[s])]) >> eol_p
                >> "cost" >> *(' '|real_p[push_back_a(plants1[i].cost[s])]) >> eol_p;
            info = parse(info.stop, last, data_r);
            if(!info.hit)
            {
                std::cout << "Fail data of powerplant #" << i << std::endl;
                return 1;
            }
            else if(plants1[i].pmin[s].size() != timesteps
                    || plants1[i].pmax[s].size() != timesteps
                    || plants1[i].cost[s].size() != timesteps)
            {
                std::cout << "Wrong number of data of powerplant #" << i << " and scenario #" << s << std::endl;
                return 1;
            }
        }
        info = parse(info.stop, last, "end powerplant" >> eol_p);
        if(!info.hit)
        {
            std::cout << "Missing \"end powerplant\" of powerplant #" << i << std::endl;
            return 1;
        }

    }
    std::cout << "Ok" << std::endl;

    return 0;
}

