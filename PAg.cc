#include "my_pred.h"
#include <cassert>
#include <sstream>

std::string PAg::get_br_id(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    std::stringstream ss;
    ss << seq_no << piece << pc;
    return ss.str();
}

uint16_t PAg::get_bht_index(uint64_t pc)
{
    pc >>= 2;  // remove 4-byte alignment bits
    pc ^= pc >> 10;
    pc ^= pc >> 20;
    pc ^= pc >> 30;

    return pc & (BHT_SIZE - 1);
}

void PAg::init() {}

void PAg::fini() {}

bool PAg::predict(uint64_t seq_no, uint8_t piece, uint64_t pc)
{
    uint16_t bht_index = get_bht_index(pc);
    assert(bht_index < BHT_SIZE);

    uint16_t bhr = bht[bht_index];
    assert(bhr < PHT_SIZE);

    std::string br_id = get_br_id(seq_no, piece, pc);
    br_id_to_bhr_map.insert(std::pair<std::string, uint16_t>(br_id, bhr));

    return (pht[bhr] >= 2);
}

void PAg::spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
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

    uint16_t bht_index = get_bht_index(pc);
    assert(bht_index < BHT_SIZE);
    
    uint16_t bhr = bht[bht_index];
    bhr <<= 1;
    bhr |= resolve_dir;
    bhr %= (1 << BHR_LEN);
    
    bht[bht_index] = bhr;
}

void PAg::update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc)
{
    std::string br_id = get_br_id(seq_no, piece, pc);
    auto it = br_id_to_bhr_map.find(br_id);
    uint16_t bhr = it->second;
    
    uint8_t two_bit_counter = pht[bhr];

    if (resolve_dir)
    {
        if (two_bit_counter < 3)
            pht[bhr]++;
    }
    else
    {
        if (two_bit_counter)
            pht[bhr]--;
    }
    
    br_id_to_bhr_map.erase(br_id);
}
