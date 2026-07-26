#include "GAg.h"
#include <cassert>
#include <sstream>

std::string GAg::get_br_id(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    std::stringstream ss;
    ss << seq_no << piece << pc;
    return ss.str();
}

uint32_t GAg::get_pht_index(uint64_t key)
{
    return key % PHT_SIZE;
}

void GAg::init() {}

void GAg::fini() {}

bool GAg::predict(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    uint32_t index = get_pht_index(ghr);
    assert(index < PHT_SIZE);

    std::string br_id = get_br_id(seq_no, piece, pc);
    br_hist.insert(std::pair<std::string, uint64_t>(br_id, ghr));

    return (pht[index] >= 2);
}

void GAg::spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
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

    ghr <<= 1;
    ghr &= GHR_MASK;
    ghr |= resolve_dir;
}

void GAg::update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
{
    std::string br_id = get_br_id(seq_no, piece, pc);
    auto it = br_hist.find(br_id);
    assert(it != br_hist.end());

    uint64_t ghr_to_use = it->second;
    uint32_t index = get_pht_index(ghr);
    assert(index < PHT_SIZE);

    if (resolve_dir)
    {
        if (pht[index] < 3)
            pht[index]++;
    }
    else
    {
        if (pht[index])
            pht[index]--;
    }
}

void GAg::commit(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    std::string br_id = get_br_id(seq_no, piece, pc);
    br_hist.erase(br_id);
}
