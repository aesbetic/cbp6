#ifndef MY_PRED_H_
#define MY_PRED_H_

#include <cstdint>
#include <string>
#include <unordered_map>

#define GHR_LEN 12 // Global History Register Length
#define CPT_SIZE (1 << GHR_LEN) // Choice Prediction Table Size

struct Predictions {
    bool samePred;
    bool chosenPred; // 0 = GAg and 1 = PAg
};

//--------------------------------------------------------//
// This implements a Tournament Branch Predictor (GAg vs PAg)
//--------------------------------------------------------//
class MyPred
{
private:
    uint8_t cpt[CPT_SIZE];

    // this metadata is only to keep track of prediction values
    // to properly update the BP structures at execute stage.
    // Technically, this information could have been stored
    // along with the instruction itself. But since we don't
    // have access to such APIs, we are storing it by ourselves.
    std::unordered_map<std::string, Predictions> predictions_map;
    std::string get_br_id(uint64_t seq_no, uint8_t piece, uint64_t pc);
    uint32_t get_cpt_index(uint64_t pc, uint32_t ghr);

public:
    MyPred() {}
    ~MyPred() {}

    // interface functions
    void init();
    void fini();
    bool predict(uint64_t seq_no, uint8_t piece, uint64_t pc);
    void spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc);
    void update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc);
    void commit(uint64_t seq_no, uint8_t piece, uint64_t pc);
};

#endif // MY_PRED_H_
