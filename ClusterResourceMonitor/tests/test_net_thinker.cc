#include "net_thinker.h"
#include "util.h"
using namespace moniter;
    

int main(){
    std::unordered_map<int, std::vector<int>> topo = {
        {1, {2, 3, 4, 5}},
        {2, {1, 3, 4, 5}},
        {3, {1, 2, 4, 5}},
        {4, {1, 2, 3, 5}}

    };

    NetThinker::edge_coloring(topo);

    return 0;
}

