#include "message_handler.h"

#include <boost/tokenizer.hpp>

#include <iterator>
#include <fstream>
#include <chrono>
#include <ctime>
#include <cstdlib>

int mem_allocs = 0;

void* operator new(std::size_t n)
{
    ++mem_allocs;
    return malloc(n);
}

template <typename StringT, typename Callable>
void load_file(StringT&& filename, Callable f)
{
    std::ifstream ifs(filename);
    boost::char_separator<char> sep(",");

    for (std::string line; std::getline(ifs, line); )
    {
        boost::tokenizer<boost::char_separator<char>> tok(line, sep);
        assert(std::distance(tok.begin(), tok.end()) == 3);

        auto it = tok.begin();
        ++it; // skip the 1st field
        std::string ref = *it;
        ++it;
        double price = std::stof(*it);

        f(ref, price);
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::srand(std::time(NULL));

    market_data_provider_mic_string mdp_mic_string;
    market_data_provider_mic_string_view mdp_mic_string_view;
    market_data_provider_umap_string mdp_umap_string;
    market_data_provider_umap_string_view mdp_umap_string_view;
    std::vector<stock> stocks;

    load_file(argv[1], [&](const std::string& ref, double price)
    {
        mdp_mic_string.add_stock(stock{ref, ref, price, 100});
        mdp_mic_string_view.add_stock(stock{ref, ref, price, 100});
        mdp_umap_string.add_stock(stock{ref, ref, price, 100});
        mdp_umap_string_view.add_stock(stock{ref, ref, price, 100});
        stocks.push_back({ref, ref, price, 100});
    });

    auto benchmark_lookup = [&](auto&& market_data_provider)
    {
        mem_allocs = 0;
        counter<std::string>::reset();
        counter<std::experimental::string_view>::reset();

        auto start = std::chrono::steady_clock::now();

        static const int Iterations = 1e4;
        for (int i = 0; i < Iterations; ++i)
        {
            const stock& s = stocks[std::rand() % stocks.size()];
            market_data_provider.on_price_change(s.market_ref.get().c_str(), s.market_ref.get().size(), 10.0);
        }

        auto end = std::chrono::steady_clock::now();
        std::cout << market_data_provider.name() << " --- mem allocs: " << mem_allocs
                  << " - time elapsed: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " - "
                  << counter<std::string>() << " - " << counter<std::string>() << std::endl;
    };

    benchmark_lookup(mdp_mic_string);
    benchmark_lookup(mdp_mic_string_view);
    benchmark_lookup(mdp_umap_string);
    benchmark_lookup(mdp_umap_string_view);

    return 0;
}
