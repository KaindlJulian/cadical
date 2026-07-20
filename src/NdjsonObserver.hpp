#include "SolverObserver.hpp"

#include <cinttypes>
#include <cstdio>

// Concrete SolverObserver that serializes each event as a single NDJSON line
// to stdout.

class NdjsonObserver final : public SolverObserver {
  static void print_ints(const std::vector<int>& v) {
    fputc('[', stdout);
    for (size_t i = 0; i < v.size(); ++i) {
      if (i)
        fputc(',', stdout);
      printf("%d", v[i]);
    }
    fputc(']', stdout);
  }

public:
  void on_init(int variables, int clauses,
    const std::vector<int>& variable_ids,
    const std::vector<ClauseInfo>& clause_list) override {
    printf("{\"event\":\"init\",\"variables\":%d,\"clauses\":%d,"
      "\"variable_ids\":",
      variables, clauses);
    print_ints(variable_ids);
    fputs(",\"clause_list\":[", stdout);
    for (size_t i = 0; i < clause_list.size(); ++i) {
      if (i)
        fputc(',', stdout);
      printf("{\"id\":%" PRId64 ",\"literals\":", clause_list[i].id);
      print_ints(clause_list[i].literals);
      fputc('}', stdout);
    }
    fputs("]}\n", stdout);
    fflush(stdout);
  }

  void on_decide(int literal, int level, const char* heuristic) override {
    printf("{\"event\":\"decide\",\"literal\":%d,\"level\":%d,"
      "\"heuristic\":\"%s\"}\n",
      literal, level, heuristic);
    fflush(stdout);
  }

  void on_propagate(int literal, int level, int64_t reason_clause_id,
    const std::vector<int>& reason_literals) override {
    if (reason_clause_id == -1) {
      printf("{\"event\":\"propagate\",\"literal\":%d,\"level\":0,"
        "\"reason_clause_id\":null,\"reason_literals\":[]}\n",
        literal);
    } else {
      printf("{\"event\":\"propagate\",\"literal\":%d,\"level\":%d,"
        "\"reason_clause_id\":%" PRId64 ",\"reason_literals\":",
        literal, level, reason_clause_id);
      print_ints(reason_literals);
      fputs("}\n", stdout);
    }
    fflush(stdout);
  }

  void on_conflict(int64_t clause_id, const std::vector<int>& literals,
    int level, const std::vector<int>& trail) override {
    printf("{\"event\":\"conflict\",\"clause_id\":%" PRId64 ",\"literals\":",
      clause_id);
    print_ints(literals);
    printf(",\"level\":%d,\"trail\":", level);
    print_ints(trail);
    fputs("}\n", stdout);
    fflush(stdout);
  }

  void on_learn_and_backtrack(const std::vector<int>& learned_literals,
    int glue, int64_t clause_id, int jump_level,
    int backtrack_level) override {
    printf("{\"event\":\"learn_and_backtrack\",\"learned_literals\":");
    print_ints(learned_literals);
    printf(",\"glue\":%d,\"clause_id\":%" PRId64
      ",\"jump_level\":%d,\"backtrack_level\":%d}\n",
      glue, clause_id, jump_level, backtrack_level);
    fflush(stdout);
  }

  void on_restart(int64_t count, int from_level, int to_level) override {
    printf("{\"event\":\"restart\",\"count\":%" PRId64
      ",\"from_level\":%d,\"to_level\":%d}\n",
      count, from_level, to_level);
    fflush(stdout);
  }

  void on_delete_clause(int64_t clause_id,
    const std::vector<int>& literals) override {
    printf("{\"event\":\"delete_clause\",\"clause_id\":%" PRId64
      ",\"literals\":",
      clause_id);
    print_ints(literals);
    fputs("}\n", stdout);
    fflush(stdout);
  }

  void on_result(const char* result,
    const std::vector<int>& model) override {
    printf("{\"event\":\"result\",\"result\":\"%s\",\"model\":", result);
    print_ints(model);
    fputs("}\n", stdout);
    fflush(stdout);
  }
};
