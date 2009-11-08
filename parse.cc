#include <iostream>
#include <deque>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_lists.hpp> 
#include <boost/spirit/include/classic_file_iterator.hpp>
#include <boost/spirit/include/classic_push_back_actor.hpp>

#include "instance.h"

#include <boost/assert.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;
typedef file_iterator<char>   iterator_t;
typedef scanner<iterator_t>     scanner_t;
typedef rule<scanner_t>         rule_t;


int main(int argc, char ** argv)
{
    if(argc != 2)
    {
        std::cout << "Use: " << argv[0] << " data_file " << std::endl;
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
    BOOST_ASSERT(info.hit);
    BOOST_ASSERT(durations.size() == timesteps);
    std::cout << "Ok" << std::endl; 


    std::cout << "Parsing demand: " << std::flush;
    std::vector<std::vector<float> > demand(scenario);

    for(int i=0; i < scenario; i++)
    {
        demand[i].reserve(timesteps);
        rule_t demand_r = "demand" >> *(' '|real_p[push_back_a(demand[i])]) >> eol_p;
        info = parse(info.stop, last, demand_r);
        BOOST_ASSERT(info.hit);
        BOOST_ASSERT(demand[i].size() == timesteps);
    }
    std::cout << "Ok" << std::endl;

    info = parse(info.stop, last, "end main" >> eol_p);
    BOOST_ASSERT(info.hit);
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
        BOOST_ASSERT(info.hit);

        for(int s=0; s < scenario; s++)
        {
            rule_t data_r = "pmin" >> *(' '|real_p[push_back_a(plants1[i].pmin[s])]) >> eol_p
                >> "pmax" >> *(' '|real_p[push_back_a(plants1[i].pmax[s])]) >> eol_p
                >> "cost" >> *(' '|real_p[push_back_a(plants1[i].cost[s])]) >> eol_p;
            info = parse(info.stop, last, data_r);
            BOOST_ASSERT(info.hit);
            BOOST_ASSERT(plants1[i].pmin[s].size() == timesteps);
            BOOST_ASSERT(plants1[i].pmax[s].size() == timesteps);
            BOOST_ASSERT(plants1[i].cost[s].size() == timesteps);
        }
        info = parse(info.stop, last, "end powerplant" >> eol_p);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;

    std::cout << "Parsing Powerplant type2: " << std::flush;
    std::deque<Powerplant_t2> plants2 (powerplant2, Powerplant_t2());
    for(int i=0; i < powerplant2; i++)
    {
        rule_t type2_r = "begin powerplant" >> eol_p
            >> "name " >> (*~space_p)[assign_a(plants2[i].name)] >> eol_p
            >> "type 2" >> eol_p
            >> "index " >> int_p[assign_a(plants2[i].index)] >> eol_p
            >> "stock " >> real_p[assign_a(plants2[i].stock)] >> eol_p
            >> "campaigns " >> int_p[assign_a(plants2[i].campaigns)] >> eol_p
            >> "durations" >> *(' '|int_p[push_back_a(plants2[i].durations)]) >> eol_p 
            >> "current_campaign_max_modulus " >> int_p[assign_a(plants2[i].current_max_modulus)] >> eol_p
            >> "max_modulus" >> *(' '|int_p[push_back_a(plants2[i].max_modulus)]) >> eol_p
            >> "max_refuel" >> *(' '|int_p[push_back_a(plants2[i].max_refuel)]) >> eol_p
            >> "min_refuel" >> *(' '|int_p[push_back_a(plants2[i].min_refuel)]) >> eol_p
            >> "refuel_ratio" >> *(' '|int_p[push_back_a(plants2[i].refuel_ratio)]) >> eol_p
            >> "current_campaign_stock_threshold " >> int_p[assign_a(plants2[i].current_stock_threshold)] >> eol_p
            >> "stock_threshold" >> *(' '|int_p[push_back_a(plants2[i].stock_threshold)]) >> eol_p
            >> "pmax" >> *(' '|real_p[push_back_a(plants2[i].pmax)]) >> eol_p
            >> "max_stock_before_refueling" >> *(' '|int_p[push_back_a(plants2[i].max_stock_before_refueling)]) >> eol_p
            >> "max_stock_after_refueling" >> *(' '|int_p[push_back_a(plants2[i].max_stock_after_refueling)]) >> eol_p
            >> "refueling_cost" >> *(' '|real_p[push_back_a(plants2[i].refueling_cost)]) >> eol_p
            >> "fuel_price " >> real_p[assign_a(plants2[i].fuel_price)] >> eol_p
            >> "begin current_campaign_profile" >> eol_p
            >> "profile_points " >> int_p >> eol_p
            >> "decrease_profile" >> *(' ' >> int_p[push_back_a(plants2[i].current_decrease_profile_idx)] >> ' ' >> real_p[push_back_a(plants2[i].current_decrease_profile_val)]) >> eol_p
            >> "end current_campaign_profile" >> eol_p;
        info = parse(info.stop, last, type2_r);
        BOOST_ASSERT(info.hit);

        plants2[i].decrease_profile_idx.resize(plants2[i].campaigns);
        plants2[i].decrease_profile_val.resize(plants2[i].campaigns);
        int profile_points;
        for(int s=0; s < plants2[i].campaigns; s++)
        {
            rule_t profile_r = "begin profile" >> eol_p
                >> "campaign_profile " >> int_p >> eol_p
                >> "profile_points " >> int_p[assign_a(profile_points)] >> eol_p
                >> "decrease_profile" >> *(' ' >> int_p[push_back_a(plants2[i].decrease_profile_idx[s])] >> ' ' >> real_p[push_back_a(plants2[i].decrease_profile_val[s])]) >> eol_p
                >> "end profile" >> eol_p;

            info = parse(info.stop, last, profile_r);
            BOOST_ASSERT(info.hit);
        }
        info = parse(info.stop, last, "end powerplant" >> eol_p);
        BOOST_ASSERT(info.hit);
    }
    std::cout << "Ok" << std::endl;
    return 0;
}

