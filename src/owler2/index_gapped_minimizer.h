/*
 * index_gapped_minimizer.h
 *
 *  Created on: Jun 24, 2016
 *      Author: isovic
 */

#ifndef SRC_OWLER2_INDEX_GAPPED_MINIMIZER_H_
#define SRC_OWLER2_INDEX_GAPPED_MINIMIZER_H_

#include "sparsehash/dense_hash_map"

using google::dense_hash_map;      // namespace where class lives by default

typedef unsigned __int128 uint128_t;
struct SeedHashValue {
  int64_t start = 0, num = 0;
};
static const uint64_t empty_hash_key = 0xFFFFFFFFFFFFFFFF;
typedef dense_hash_map<uint64_t, SeedHashValue, std::hash<uint64_t> > SeedHashType;     // SeedHashType encodes the following: key is the hash key of a seed, and value is the position in the list of seeds where the seed starts as well as the number of positions with the same key.

//typedef dense_hash_map<int64_t, DenseType2, std::hash<int64_t> > DenseType;
//std::vector<DenseType> bins_map1;
//bins_map1[i].set_empty_key(-1);
//struct DenseType2 {
//  int32_t timestamp = 0;
//  float count = 0.0;
//};
//DenseType2 &hit = temp_map[position_bin];
//if (hit.timestamp == (i + 1)) { continue; }
//hit.count += 1.0f;
//hit.timestamp = (i + 1);

#include <vector>
#include <map>
#include <stdint.h>
#include <stdint.h>
#include "sequences/sequence_file.h"
#include "utility/utility_general.h"
#include "compiled_shape.h"

const uint32_t kIndexIdReverse = ((uint32_t) 1) << 31;
const uint32_t kIndexMaskStrand = ~((uint32_t) kIndexIdReverse);
const uint64_t kIndexMaskLowerBits = 0x00000000FFFFFFFF;
const uint64_t kIndexMaskUpperBits = 0xFFFFFFFF00000000;

class IndexPos {
 public:
  // This structure holds the info of a seed position in the form of (seq_id, pos, strand) 'tuple'.
  // The strand info is coded as the MSB of the pos. This way, when sorting the hits
  // All hits belonging to the same sequence will be grouped, but the fwd and the rev strand will be separate.
  uint32_t id;     // ID of the originating sequence.
  uint32_t pos;   // Position within the sequence. If >= (1 << 31), the sequence is reverse complemented.

  IndexPos() : id(0), pos(0) { }
  IndexPos(uint32_t nid, uint32_t npos) : id(nid), pos(npos) { }
  IndexPos(uint64_t coded_pos) {
    id = coded_pos >> 32;
    pos = coded_pos & (0x00000000FFFFFFFF);
  }
  bool is_rev() { return (pos & (kIndexIdReverse)) != 0; }
  inline uint32_t get_pos() { return (pos & kIndexMaskStrand); }
  inline uint32_t get_id() { return id; }
};

class IndexGappedMinimizer {
 public:
  IndexGappedMinimizer();
  ~IndexGappedMinimizer();

  void Clear();
  int CreateFromSequenceFile(const SequenceFile &seqs, const std::vector<CompiledShape> &compiled_shapes, float min_avg_seed_qv, bool index_reverse_strand, bool use_minimizers, int32_t num_threads);
  void DumpHash(std::string out_path, int32_t num_bases);
  void DumpSortedHash(std::string out_path, int32_t num_bases);
  void DumpSeeds(std::string out_path, int32_t num_bases);

 private:
  std::vector<uint128_t> seeds_;      // A seed is encoded with: upper (MSB) 64 bits are the seed key, and lower (LSB) 64 bits are the ID of the sequence and the position (1-based, and encoded as in the IndexPos class). The position is 1-based to allow for undefined values.
  SeedHashType hash_;                 // A lookup for seeds by their key.
  std::vector<std::vector<int8_t> > data_;      // Actual sequences which have been indexed. The outer vector contains both fwd and revcmp sequences (fwd come first).

  int64_t num_sequences_;
  int64_t num_sequences_forward_;
  std::vector<int64_t> reference_lengths_;
  std::vector<std::string> headers_;
//  // Seeds are extracted for each sequence separately, but are stored in a giant array. Each sequence 'i' is designated to belong to a part of that array, starting with seq_seed_starts_[i] position in seed_list_.
//  std::vector<int64_t> seq_seed_starts_;
//  std::vector<int64_t> seq_seed_counts_;
  double count_cutoff_;

  void AssignData_(const SequenceFile &seqs, bool index_reverse_strand);
  void AllocateSpaceForSeeds_(const SequenceFile &seqs, bool index_reverse_strand, int64_t num_shapes, int64_t max_seed_len, int64_t num_fwd_seqs, std::vector<int64_t> &seed_starts_for_seq, int64_t *total_num_seeds);
  int AddSeedsForSeq_(int8_t *seqdata, int8_t *seqqual, int64_t seqlen, float min_avg_seed_qv, uint64_t seq_id, const std::vector<CompiledShape> &compiled_shapes, uint128_t *seed_list);
  inline uint64_t SeedHashFunction_(uint64_t seed);
  inline uint64_t ReverseComplementSeed_(uint64_t seed, int32_t num_bases);
  int MakeMinimizers_(uint128_t *seed_list, int64_t num_seeds);
  int FlagDuplicates_(uint128_t *seed_list, int64_t num_seeds);
  int OccurrenceStatistics_(double percentil, int32_t num_threads, double* ret_avg, double* ret_stddev, double *ret_percentil_val);

  // Helper functions for debugging.
  inline std::string SeedToString_(uint64_t seed, int32_t num_bases);
};

#endif /* SRC_OWLER2_INDEX_GAPPED_MINIMIZER_H_ */
