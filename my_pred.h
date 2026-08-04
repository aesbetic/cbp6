#ifndef MY_PRED_H_
#define MY_PRED_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <array>

#define GHR_LEN 54
#define GHR_MASK ((1ul << GHR_LEN) - 1)
#define PATH_HISTORY_LEN 2
#define NUM_WEIGHTS 8
#define WEIGHT_TABLE_LEN 512
#define NUM_PATH_WEIGHTS 2 // {mi, P, 0}
#define NUM_GLOBAL_WEIGHTS 6  // {mi, AxG, 0}
#define GHR_SUBSET_LEN (GHR_LEN / NUM_GLOBAL_WEIGHTS)
#define PC_MASK ((1ul << GHR_SUBSET_LEN) - 1)
#define THETA (int)(NUM_WEIGHTS * 2.43)

typedef std::array<uint16_t, NUM_PATH_WEIGHTS> PathWeightIndices;
typedef std::array<uint16_t, NUM_GLOBAL_WEIGHTS> GlobalWeightIndices;
typedef std::array<uint16_t, NUM_WEIGHTS> WeightIndices;

struct PredictionInfo {
    WeightIndices indices;
    int64_t sum;
};

//--------------------------------------------------------//
// This implements a Hashed Perceptron Predictor
//--------------------------------------------------------//
class MyPred {
    private:
        uint64_t ghr;
        uint64_t path_history[PATH_HISTORY_LEN];
        int8_t weight_matrix[WEIGHT_TABLE_LEN][NUM_WEIGHTS];
        
        PathWeightIndices compute_path_indices(uint64_t* path_history);
        GlobalWeightIndices compute_global_indices(uint64_t ghr, uint64_t PC);

        // this metadata is only to keep track of weight indices
        // to properly update the BP structures at execute stage.
        // Technically, this information could have been stored
        // along with the instruction itself. But since we don't
        // have access to such APIs, we are storing it by ourselves.
        std::unordered_map<std::string, PredictionInfo> branch_indices_table;
        std::string get_br_id(uint64_t seq_no, uint8_t piece, uint64_t pc);

    public:
        MyPred() {}
        ~MyPred() {}

        // interface functions
        void init();
        void fini();
        bool predict(uint64_t seq_no, uint8_t piece, uint64_t pc);
        void spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc,
                         const bool resolve_dir, const bool pred_dir,
                         const uint64_t next_pc);
        void update(uint64_t seq_no, uint8_t piece, uint64_t pc,
                    const bool resolve_dir, const bool pred_dir,
                    const uint64_t next_pc);
        void commit(uint64_t seq_no, uint8_t piece, uint64_t pc);
};

#endif // MY_PRED_H_
