#include<vector>

#ifndef INSTANCE_H
#define INSTANCE_H
struct Powerplant_t1
{
    std::string name;
    int index;
    std::vector<std::vector<float> > pmin;
    std::vector<std::vector<float> > pmax;
    std::vector<std::vector<float> > cost;

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

struct Powerplant_t2
{
    std::string name;
    int index;
    float stock;
    int campaigns;
    std::vector<int> durations;
    int current_max_modulus;
    std::vector<int> max_modulus;
    std::vector<int> max_refuel;
    std::vector<int> min_refuel;
    std::vector<int> refuel_ratio;
    int current_stock_threshold;
    std::vector<int> stock_threshold;
    std::vector<float> pmax;
    std::vector<int> max_stock_before_refueling;
    std::vector<int> max_stock_after_refueling;
    std::vector<float> refueling_cost;
    float fuel_price;
    std::vector<int> current_decrease_profile_idx;
    std::vector<float> current_decrease_profile_val;
    std::vector<std::vector<int> > decrease_profile_idx;
    std::vector<std::vector<float> > decrease_profile_val;
};

struct Constraint_13
{
    int powerplant_idx;
    int campaign_idx;
    int earliest_stop_time;
    int latest_stop_time;
};

#endif