#pragma once

#include "constellation_commons.h"
#include "./algorithm_helper.h"

namespace constellation::algorithm::basic {

/* brief: Generate a transport topology tree using DFS
 * @param overlay: the overlay topology
 * @return the transport topology tree
 */
GlobalTransTopo dfs_tree(const AdjacencyList& overlay);

/* brief: Randomly choose a transport topology tree
 * @param overlay: the overlay topology
 * @return the transport topology tree
 */
GlobalTransTopo random_choose_tree(const AdjacencyList& overlay);

/* brief: Generate  model synchronization paths to target node randomly
 * @param overlay: the overlay topology
 * @param target: the target node
 * @param maxPaths: the maximum number of paths
 * @return the model synchronization paths
 */
std::vector<TransPath> random_choose_paths(AdjacencyList overlay,
                                           int target,
                                           int maxPaths = 10);

/* brief: Generate  model synchronization paths to target node using dijsktra
 algorithm Note:
 * @param overlay: the overlay topology
 * @param target: the target node
 * @return the model synchronization paths
 */
std::vector<TransPath> dijsktra_paths(
    AdjacencyList overlay,
    AdjacencyListT<float> weights,
    int target,
    std::vector<float>* path_weights = nullptr);

}  // namespace constellation::algorithm::basic