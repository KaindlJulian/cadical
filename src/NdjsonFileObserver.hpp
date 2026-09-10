#include "SolverObserver.hpp"

#include <cinttypes>
#include <cstdio>
#include <string>

// SolverObserver that serializes each event as a single NDJSON line to a .jsonl file.

class NdjsonFileObserver final : public SolverObserver {
  FILE* out_;

  // Block buffer for the event stream. target will be a virtual file in wasm
  static const size_t BUFFER_BYTES = 256 * 1024;

  void print_ints(const std::vector<int>& v) {
    fputc('[', out_);
    for (size_t i = 0; i < v.size(); ++i) {
      if (i) {
        fputc(',', out_);
      }
      fprintf(out_, "%d", v[i]);
    }
    fputc(']', out_);
  }

public:
  // Does not throw: the caller checks 'ok()' and reports the failure itself,
  // which keeps the solver buildable with '-fno-exceptions'.
  explicit NdjsonFileObserver(const std::string& path) {
    out_ = fopen(path.c_str(), "w");
    if (out_) {
      setvbuf(out_, nullptr, _IOFBF, BUFFER_BYTES);
    }
  }

  bool ok() const { return out_ != nullptr; }

  ~NdjsonFileObserver() {
    if (out_) {
      fclose(out_);
    }
  }

  NdjsonFileObserver(const NdjsonFileObserver&) = delete;
  NdjsonFileObserver& operator=(const NdjsonFileObserver&) = delete;

  void on_init(int variables, int clauses, const std::vector<int>& variable_ids, const std::vector<ClauseInfo>& clause_list) override {
    fprintf(out_, "{\"event\":\"init\",\"protocol_version\":\"%s\","
      "\"variables\":%d,\"clauses\":%d,"
      "\"variable_ids\":",
      NDJSON_PROTOCOL_VERSION, variables, clauses);
    print_ints(variable_ids);
    fputs(",\"clause_list\":[", out_);
    for (size_t i = 0; i < clause_list.size(); ++i) {
      if (i) {
        fputc(',', out_);
      }
      fprintf(out_, "{\"id\":%" PRId64 ",\"literals\":", clause_list[i].id);
      print_ints(clause_list[i].literals);
      fputc('}', out_);
    }
    fputs("]}\n", out_);
  }

  void on_decide(int literal, int level, const char* heuristic) override {
    fprintf(out_, "{\"event\":\"decide\",\"literal\":%d,\"level\":%d,"
      "\"heuristic\":\"%s\"}\n",
      literal, level, heuristic);
  }

  void on_propagate(int literal, int level, int64_t reason_clause_id, const std::vector<int>& _reason_literals) override {
    if (reason_clause_id == -1) {
      fprintf(out_, "{\"event\":\"propagate\",\"literal\":%d,\"level\":0,"
        "\"reason_clause_id\":null}\n",
        literal);
    } else {
      fprintf(out_, "{\"event\":\"propagate\",\"literal\":%d,\"level\":%d,"
        "\"reason_clause_id\":%" PRId64 "}\n",
        literal, level, reason_clause_id);
    }
  }

  void on_conflict(int64_t clause_id, const std::vector<int>& literals, int level, const std::vector<int>& trail) override {
    fprintf(out_, "{\"event\":\"conflict\",\"clause_id\":%" PRId64 ",\"literals\":",
      clause_id);
    print_ints(literals);
    fprintf(out_, ",\"level\":%d,\"trail\":", level);
    print_ints(trail);
    fputs("}\n", out_);
  }

  void on_learn(const std::vector<int>& learned_literals, int glue, int64_t clause_id, int jump_level) override {
    fputs("{\"event\":\"learn\",\"learned_literals\":", out_);
    print_ints(learned_literals);
    fprintf(out_, ",\"glue\":%d,\"clause_id\":%" PRId64
      ",\"jump_level\":%d}\n",
      glue, clause_id, jump_level);
  }

  void on_backtrack(int from_level, int to_level, BacktrackKind kind, const char* reason) override {
    fprintf(out_, "{\"event\":\"backtrack\",\"from_level\":%d,"
      "\"to_level\":%d,\"kind\":\"%s\"",
      from_level, to_level, to_string(kind));
    if (reason && *reason) {  // optional detail, omitted when absent
      fprintf(out_, ",\"reason\":\"%s\"", reason);
    }
    fputs("}\n", out_);
  }

  void on_restart(int64_t count) override {
    fprintf(out_, "{\"event\":\"restart\",\"count\":%" PRId64 "}\n", count);
  }

  void on_delete_clause(int64_t clause_id, const std::vector<int>& literals) override {
    fprintf(out_, "{\"event\":\"delete_clause\",\"clause_id\":%" PRId64
      ",\"literals\":",
      clause_id);
    print_ints(literals);
    fputs("}\n", out_);
  }

  void on_inspect(int64_t clause_id, InspectOutcome outcome, int w0, int w1, int n0, int n1) override {
    fprintf(out_, "{\"event\":\"inspect\",\"clause_id\":%" PRId64
      ",\"outcome\":\"%s\"",
      clause_id, to_string(outcome));
    fprintf(out_, ",\"watched\":[%d,%d]", w0, w1);
    if (n0) {  // only when this inspection replaced a watch
      fprintf(out_, ",\"next_watched\":[%d,%d]", n0, n1);
    }

    fputs("}\n", out_);
  }

  void on_result(const char* result, const std::vector<int>& model) override {
    fprintf(out_, "{\"event\":\"result\",\"result\":\"%s\",\"model\":", result);
    print_ints(model);
    fputs("}\n", out_);
  }
};
