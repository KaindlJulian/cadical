#include "SolverObserver.hpp"

#include <cinttypes>
#include <cstdio>
#include <stdexcept>
#include <string>

// SolverObserver that serializes each event as a single NDJSON line to a .jsonl file.

class NdjsonFileObserver final : public SolverObserver {
  FILE* out_;

  void print_ints(const std::vector<int>& v) {
    fputc('[', out_);
    for (size_t i = 0; i < v.size(); ++i) {
      if (i)
        fputc(',', out_);
      fprintf(out_, "%d", v[i]);
    }
    fputc(']', out_);
  }

public:
  explicit NdjsonFileObserver(const std::string& path) {
    out_ = fopen(path.c_str(), "w");
    if (!out_)
      throw std::runtime_error("NdjsonFileObserver: cannot open " + path);
  }

  ~NdjsonFileObserver() {
    if (out_)
      fclose(out_);
  }

  NdjsonFileObserver(const NdjsonFileObserver&) = delete;
  NdjsonFileObserver& operator=(const NdjsonFileObserver&) = delete;

  void on_init(int variables, int clauses,
    const std::vector<int>& variable_ids,
    const std::vector<ClauseInfo>& clause_list) override {
    fprintf(out_, "{\"event\":\"init\",\"protocol_version\":%d,"
      "\"variables\":%d,\"clauses\":%d,"
      "\"variable_ids\":",
      NDJSON_PROTOCOL_VERSION, variables, clauses);
    print_ints(variable_ids);
    fputs(",\"clause_list\":[", out_);
    for (size_t i = 0; i < clause_list.size(); ++i) {
      if (i)
        fputc(',', out_);
      fprintf(out_, "{\"id\":%" PRId64 ",\"literals\":", clause_list[i].id);
      print_ints(clause_list[i].literals);
      fputc('}', out_);
    }
    fputs("]}\n", out_);
    fflush(out_);
  }

  void on_decide(int literal, int level, const char* heuristic) override {
    fprintf(out_, "{\"event\":\"decide\",\"literal\":%d,\"level\":%d,"
      "\"heuristic\":\"%s\"}\n",
      literal, level, heuristic);
    fflush(out_);
  }

  void on_propagate(int literal, int level, int64_t reason_clause_id,
    const std::vector<int>& reason_literals) override {
    if (reason_clause_id == -1) {
      fprintf(out_, "{\"event\":\"propagate\",\"literal\":%d,\"level\":0,"
        "\"reason_clause_id\":null,\"reason_literals\":[]}\n",
        literal);
    } else {
      fprintf(out_, "{\"event\":\"propagate\",\"literal\":%d,\"level\":%d,"
        "\"reason_clause_id\":%" PRId64 ",\"reason_literals\":",
        literal, level, reason_clause_id);
      print_ints(reason_literals);
      fputs("}\n", out_);
    }
    fflush(out_);
  }

  void on_conflict(int64_t clause_id, const std::vector<int>& literals,
    int level, const std::vector<int>& trail) override {
    fprintf(out_, "{\"event\":\"conflict\",\"clause_id\":%" PRId64 ",\"literals\":",
      clause_id);
    print_ints(literals);
    fprintf(out_, ",\"level\":%d,\"trail\":", level);
    print_ints(trail);
    fputs("}\n", out_);
    fflush(out_);
  }

  void on_learn(const std::vector<int>& learned_literals, int glue,
    int64_t clause_id, int jump_level) override {
    fputs("{\"event\":\"learn\",\"learned_literals\":", out_);
    print_ints(learned_literals);
    fprintf(out_, ",\"glue\":%d,\"clause_id\":%" PRId64
      ",\"jump_level\":%d}\n",
      glue, clause_id, jump_level);
    fflush(out_);
  }

  void on_backtrack(int from_level, int to_level, BacktrackKind kind,
    const char* reason) override {
    fprintf(out_, "{\"event\":\"backtrack\",\"from_level\":%d,"
      "\"to_level\":%d,\"kind\":\"%s\"",
      from_level, to_level, to_string(kind));
    if (reason && *reason)  // optional detail, omitted when absent
      fprintf(out_, ",\"reason\":\"%s\"", reason);
    fputs("}\n", out_);
    fflush(out_);
  }

  void on_restart(int64_t count) override {
    fprintf(out_, "{\"event\":\"restart\",\"count\":%" PRId64 "}\n", count);
    fflush(out_);
  }

  void on_delete_clause(int64_t clause_id,
    const std::vector<int>& literals) override {
    fprintf(out_, "{\"event\":\"delete_clause\",\"clause_id\":%" PRId64
      ",\"literals\":",
      clause_id);
    print_ints(literals);
    fputs("}\n", out_);
    fflush(out_);
  }

  void on_result(const char* result,
    const std::vector<int>& model) override {
    fprintf(out_, "{\"event\":\"result\",\"result\":\"%s\",\"model\":", result);
    print_ints(model);
    fputs("}\n", out_);
    fflush(out_);
  }
};
