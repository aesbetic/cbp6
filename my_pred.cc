#include "my_pred.h"
#include <cassert>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <cstdlib>

std::string MyPred::get_br_id(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    std::stringstream ss;
    ss << seq_no << piece << pc;
    return ss.str();
}

PathWeightIndices MyPred::compute_path_indices(uint64_t *path_history) {
  PathWeightIndices temp;

  for (int i = 0; i < PATH_HISTORY_LEN; ++i) {
      temp[i] = (path_history[i] >> 2) % WEIGHT_TABLE_LEN;
  }
  return temp;
}

GlobalWeightIndices MyPred::compute_global_indices(uint64_t ghr, uint64_t PC) {
    GlobalWeightIndices temp;

    PC >>= 2;
    for (int i = 1; i <= 6; ++i) {
        PC ^= PC >> (i * GHR_SUBSET_LEN);
    }
    PC &= PC_MASK;

    uint64_t AxG;
    for (int i = 1; i <= NUM_GLOBAL_WEIGHTS; ++i) {
        AxG = PC ^ (ghr >> (GHR_SUBSET_LEN * (i - 1)));
        AxG &= PC_MASK;
        temp[NUM_GLOBAL_WEIGHTS - i] = AxG % WEIGHT_TABLE_LEN;
    }

    return temp;
}

void MyPred::init() {}

void MyPred::fini() {}

bool MyPred::predict(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    // create current branch pair
    std::pair<std::string, PredictionInfo> curr_branch;
    std::string branch_id = get_br_id(seq_no, piece, pc);
    curr_branch.first = branch_id;
    WeightIndices curr_branch_weight_indices;
    int64_t sum = 0;

    // compute path and global weight indices
    PathWeightIndices path_indices = compute_path_indices(path_history);
    GlobalWeightIndices global_indices = compute_global_indices(ghr, pc);
    
    // concatenate path and global weight indices into curr_branch_weight_indices
    std::copy(path_indices.begin(), path_indices.end(), curr_branch_weight_indices.begin());
    std::copy(global_indices.begin(), global_indices.end(), curr_branch_weight_indices.begin() + path_indices.size());
    
    // fetch and sum the weights using curr branch weight indices
    for (int i = 0; i < NUM_WEIGHTS; ++i) {
        sum += weight_matrix[curr_branch_weight_indices[i]][i];
    }

    // insert curr_branch into the branch indices table
    curr_branch.second = PredictionInfo{curr_branch_weight_indices, sum};
    branch_indices_table.insert(curr_branch);
    
    bool prediction = (sum >= 0) ? 1 : 0;
    return prediction;
}

void MyPred::spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
{
    //---------------------------------------------------------------------------------------//
    // Remember that the spec_update function is called right after the BP predicted for
    // the branch. In a real processor, you WON'T know the real outcome of the branch
    // (i.e., `resolve_dir` and `next_pc` arguments), at this point. It will only be
    // known AFTER the branch is executed (when the `update` function is called).
    // Then why do we provide you the `resolve_dir` and `next_pc` arguments here?
    // This is to update any path history structures that you may use to make
    // subsequent predictions, without taking complex branch recovery code into account.
    // For example, here we use the `resolve_dir` argument to update the GHR, which
    // will be used to make subsequent branch predictions.
    // But observe that: we *WILL NOT* use any of this information to update the PHT
    // at this stage. That will only happen at the `update` function call.
    //
    // If you are unsure whether your usage of `resolve_dir` and `next_pc`
    // in this spec_update function is valid or not, please email us.
    //---------------------------------------------------------------------------------------//

    // speculatively update ghr
    ghr <<= 1;
    ghr &= GHR_MASK;
    ghr |= resolve_dir;
    
    // speculatively update path history
    path_history[0] = path_history[1];
    path_history[1] = pc;
}

void MyPred::update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
{
    std::string br_id = get_br_id(seq_no, piece, pc);
    auto it = branch_indices_table.find(br_id);
    PredictionInfo branch_info = it->second;

    if (pred_dir != resolve_dir || std::abs(branch_info.sum) <= THETA) {
        int update = resolve_dir ? 1 : -1;
        for (int i = 0; i < NUM_WEIGHTS; ++i) {
            weight_matrix[branch_info.indices[i]][i] += update;
        }
    }
}

void MyPred::commit(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    std::string br_id = get_br_id(seq_no, piece, pc);
    branch_indices_table.erase(br_id);
}
