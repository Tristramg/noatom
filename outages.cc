#include "outages.h"

Outages::Outages(const Instance & data) : data(data)
{
    // Par défaut on va dire que les campagnes sont équitablement réparties
    campaign_start.resize(data.powerplant2);
    decoupling.resize(data.powerplant2);
    int campaign_duration = data.weeks / data.campaigns;
    for(int i = 0; i < data.powerplant2; i++)
    {
        campaign_start[i].push_back(0);
        decoupling[i].push_back(-1);
        for(int k = 1; k < data.campaigns ; k++)
        {
            campaign_start[i].push_back(campaign_start[i].back() + campaign_duration); 
            // Par défaut, pas de découplage pour aucune des campagnes et aucune des centrales
            decoupling[i].push_back(-1);
         }
   }
}

// Outage timesteps
std::vector<int> Outages::ea(int i, int k) const
{
    int steps_per_week = data.timesteps / data.weeks;
    std::vector<int> ret;
    if(decoupling[i][k] >= 0)
    {
        int start = (campaign_start[i][k] + decoupling[i][k]) * steps_per_week;
        int end = start + data.plants2[i].durations.at(k) * steps_per_week;
        //Attention ! Vérifier la borne sup
        for(int t = start; t <= end; t++)
        {
            ret.push_back(t);
        }
    }

    return ret;
}

// Production timesteps
std::vector<int> Outages::ec(int i, int k) const
{
    int steps_per_week = data.timesteps / data.weeks;
    std::vector<int> ret;
    if(decoupling[i][k] >= 0)
    {
        int start = campaign_start[i][k] * steps_per_week;
        int end;
        if(k >= data.campaigns - 1)
            end = data.timesteps;
        else
            end = campaign_start[i][k+1] * steps_per_week;
        int dec_start = (campaign_start[i][k] + decoupling[i][k]) * steps_per_week;
        int dec_end = start + data.plants2[i].durations.at(k) * steps_per_week;

        for(int t = start; t <= end; t++)
        {
            //Attention ! Vérifier la borne sup
            if( t < dec_start || t > dec_end)
            ret.push_back(t);
        }
    }

    return ret;
}

// Week of decoupling during a campaign. -1 if no decoupling happens
int Outages::ha(int i, int k) const
{
    return decoupling[i][k];
}

