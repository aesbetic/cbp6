#ifndef __PAg_H__
#define __PAg_H__

#include <cstdint>
#include <string>
#include <unordered_map>

#define BHT_SIZE 1024 // Branch History Table Size
#define BHR_LEN 10 // Branch History Register Length
#define PHT_SIZE (1 << BHR_LEN) // Pattern History Table Size

//--------------------------------------------------------//
// This implements a Local-History Predictor (PAg)
//--------------------------------------------------------//
class PAg
{
    private:
        uint16_t bht[BHT_SIZE];
        uint8_t pht[PHT_SIZE];

        // this metadata is only to keep track of bhr values
        // to properly update the BP structures at execute stage.
        // Technically, this information could have been stored
        // along with the instruction itself. But since we don't
        // have access to such APIs, we are storing it by ourselves.
        std::unordered_map<std::string, uint16_t> br_id_to_bhr_map;
        std::string get_br_id(uint64_t seq_no, uint8_t piece, uint64_t pc);

        uint16_t get_bht_index(uint64_t pc);

    public:
        PAg() {}
        ~PAg() {}

        // interface functions
        void init();
        void fini();
        bool predict(uint64_t seq_no, uint8_t piece, uint64_t pc);
        void spec_update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc);
        void update(uint64_t seq_no, uint8_t piece, uint64_t pc, const bool resolve_dir, const bool pred_dir, const uint64_t next_pc);
        void commit(uint64_t seq_no, uint8_t piece, uint64_t pc);
};

#endif
