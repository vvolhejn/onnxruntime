#include "sequences.h"
#ifdef _MSC_VER
// Could reduce the chance of arithmetic overflow. TODO: fix it
#pragma warning(disable : 26451)
#endif
namespace onnxruntime {
namespace contrib {
namespace transformers {

void Sequences::Init(gsl::span<int64_t> buffer, int batch_beam_size, int sequence_length, int max_length) {
  size_t sequences_size = SafeInt<size_t>(batch_beam_size) * max_length;
  assert(buffer.length() == sequences_size + sequences_size);

  sequences[0] = buffer.subspan(0, sequences_size);
  sequences[1] = buffer.subspan(sequences_size);

  current_sequences_buffer = 0;

  batch_beam_size_ = batch_beam_size;
  max_length_ = max_length;
  current_length_ = sequence_length;
}

gsl::span<const int64_t> Sequences::GetSequence(int beam_index) const {
  gsl::span<const int64_t> buffer(sequences[current_sequences_buffer].data(), sequences[current_sequences_buffer].size());
  gsl::span<const int64_t> sequence = buffer.subspan(beam_index * max_length_, current_length_);
  return sequence;
}

int Sequences::GetSequenceLength() const {
  return current_length_;
}

void Sequences::PrintSequences(const IConsoleDumper* dumper) const {
#ifdef DEBUG_BEAM_SEARCH
  for (int i = 0; i < batch_beam_size_; i++) {
    gsl::span<const int64_t> sequence = GetSequence(i);
    dumper->Print("sequences", i, false);
    dumper->Print(nullptr, sequence.data(), 1, current_length_);
  }
#endif
}

void Sequences::AppendNextTokenToSequences(
    gsl::span<int64_t>& beam_indices,
    gsl::span<int64_t>& beam_next_tokens) {
  gsl::span<const int64_t> input(sequences[current_sequences_buffer].data(), sequences[current_sequences_buffer].size());
  gsl::span<int64_t> output = sequences[1 - current_sequences_buffer];

  for (int i = 0; i < batch_beam_size_; i++) {
    int beam_index = static_cast<int>(beam_indices[i]);
    gsl::span<const int64_t> source = input.subspan(beam_index * max_length_, current_length_);
    gsl::span<int64_t> target = output.subspan(i * max_length_, current_length_);
    gsl::copy(source, target);
  }

  // Append next token to each beam.
  for (int i = 0; i < batch_beam_size_; i++) {
    output[i * max_length_ + current_length_] = beam_next_tokens[i];
  }

  ++current_length_;

  // Rotate buffer for next round.
  current_sequences_buffer = 1 - current_sequences_buffer;
}

}  // namespace transformers
}  // namespace contrib
}  // namespace onnxruntime