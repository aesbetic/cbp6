#include "my_pred.h"
#include "GAg.h"
#include "PAg.h"
#include <cassert>
#include <cstdint>
#include <sstream>

GAg globalPredictor;
PAg localPredictor;

std::string MyPred::get_br_id(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    std::stringstream ss;
    ss << seq_no << piece << pc;
    return ss.str();
}

uint32_t MyPred::get_cpt_index(uint64_t pc, uint32_t ghr)
{
    constexpr uint32_t cpt_mask = CPT_SIZE - 1;

    // Fold the aligned PC into the width of the choice-table index, then
    // combine it with the prediction-time global history (gshare-style).
    uint64_t pc_hash = pc >> 2;
    pc_hash ^= pc_hash >> GHR_LEN;
    pc_hash ^= pc_hash >> (2 * GHR_LEN);
    pc_hash ^= pc_hash >> (3 * GHR_LEN);
    pc_hash ^= pc_hash >> (4 * GHR_LEN);

    return (static_cast<uint32_t>(pc_hash) ^ ghr) & cpt_mask;
}

void MyPred::init() {
    globalPredictor.init();
    localPredictor.init();
}

void MyPred::fini() {
    globalPredictor.fini();
    localPredictor.fini();
}

bool MyPred::predict(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    bool localPred = localPredictor.predict(seq_no, piece, pc);
    bool globalPred = globalPredictor.predict(seq_no, piece, pc);
    
    uint32_t cpt_index = get_cpt_index(pc, globalPredictor.get_ghr());
    bool chosenPredictor = cpt[cpt_index] >= 2;

    std::string br_id = get_br_id(seq_no, piece, pc);
    Predictions brPreds {
    (localPred == globalPred),
    chosenPredictor
};
    predictions_map.insert(std::pair<std::string, Predictions>(br_id, brPreds));

    return (chosenPredictor ? localPred : globalPred);
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

    globalPredictor.spec_update(seq_no, piece, pc, resolve_dir, pred_dir, next_pc);
    localPredictor.spec_update(seq_no, piece, pc, resolve_dir, pred_dir, next_pc);
}

void MyPred::update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
{
    globalPredictor.update(seq_no, piece, pc, resolve_dir, pred_dir, next_pc);
    localPredictor.update(seq_no, piece, pc, resolve_dir, pred_dir, next_pc);

    std::string br_id = get_br_id(seq_no, piece, pc);
    auto it = predictions_map.find(br_id);
    Predictions brPreds = it->second;
    
    uint32_t cpt_index =
        get_cpt_index(pc, globalPredictor.get_br_hist(seq_no, piece, pc));
    uint8_t two_bit_counter = cpt[cpt_index];

    if (pred_dir == resolve_dir && !brPreds.samePred) {
        if (brPreds.chosenPred) { // PAg chosen
            if (two_bit_counter < 3)
                cpt[cpt_index]++;
        } else if (two_bit_counter) { // GAg chosen
            cpt[cpt_index]--;
        }
    } else {
        if (!brPreds.samePred) {
            bool otherPredictor = !brPreds.chosenPred;
            if (otherPredictor) {
                if (two_bit_counter < 3) {
                    cpt[cpt_index]++;
                }
            } else if (two_bit_counter) {
                cpt[cpt_index]--;
            }
        }

    }
    
}

void MyPred::commit(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    globalPredictor.commit(seq_no, piece, pc);
    localPredictor.commit(seq_no, piece, pc);

    std::string br_id = get_br_id(seq_no, piece, pc);
    predictions_map.erase(br_id);
}
