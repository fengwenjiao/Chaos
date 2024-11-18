#pragma once



#include "constellation_commons.h"
#include "./algorithm_helper.h"

namespace constellation::algorithm::basic {

GlobalTransTopo dfs_method(const AdjacencyList& overlay);

GlobalTransTopo random_choose_method(const AdjacencyList& overlay);

}  // namespace constellation::algorithm::basic