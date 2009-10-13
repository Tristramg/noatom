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

    rule_t header_r = str_p("begin main\n")
        >> str_p("timesteps ") >> int_p[assign_a(timesteps)] >> '\n'
        >> str_p("weeks ") >> int_p[assign_a(weeks)] >> '\n'
        >> str_p("campaigns ") >> int_p[assign_a(campaigns)] >> '\n'
        >> str_p("scenario ") >> int_p[assign_a(scenario)] >> '\n'
        >> str_p("epsilon ") >> real_p[assign_a(epsilon)] >> '\n'
        >> str_p("powerplant1 ") >> int_p[assign_a(powerplant1)] >> '\n'
        >> str_p("powerplant2 ") >> int_p[assign_a(powerplant2)] >> '\n'
        >> str_p("constraint13 ") >> int_p[assign_a(constraint13)] >> '\n'
        >> str_p("constraint14 ") >> int_p[assign_a(constraint14)] >> '\n'
        >> str_p("constraint15 ") >> int_p[assign_a(constraint15)] >> '\n'
        >> str_p("constraint16 ") >> int_p[assign_a(constraint16)] >> '\n'
        >> str_p("constraint17 ") >> int_p[assign_a(constraint17)] >> '\n'
        >> str_p("constraint18 ") >> int_p[assign_a(constraint18)] >> '\n'
        >> str_p("constraint19 ") >> int_p[assign_a(constraint19)] >> '\n'
        >> str_p("constraint20 ") >> int_p[assign_a(constraint20)] >> '\n'
        >> str_p("constraint21 ") >> int_p[assign_a(constraint21)] >> '\n';

    std::cout << "Parsing header: " << std::flush;
    parse_info<iterator_t> info = parse(first, last, header_r);
    if(!info.hit)
    {
        std::cout << "Fail" << std::endl;
        return 1;
    }
    else
    {
        std::cout << "Ok" << std::endl;
    }


    std::cout << "Parsing durations: " << std::flush;
    std::deque<int> durations;
    rule_t durations_r = "durations" >> *(' '|int_p[push_back_a(durations)]) >> '\n';

    info = parse(info.stop, last, durations_r);
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
    std::deque<std::deque<float> > demand(scenario);

    for(int i=0; i < scenario; i++)
    {
    
        rule_t demand_r = "demand" >> *(' '|real_p[push_back_a(demand[i])]) >> '\n';
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

    info = parse(info.stop, last, str_p("end main\n"));
    std::cout << "Main section done" << std::endl << std::endl;

    return 0;
}

