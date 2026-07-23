#pragma once

#include <cinttypes>
#include <cstdint>
#include <vector>

// Abstract observer interface for solver events.
//
// Adapters implement this class (mapping solver-internal
// structures to the plain c++ types below). 
// 
// Consumers implement it once per output format (e.g. NdjsonObserver writes 
// NDJSON to stdout).

class SolverObserver {
public:
  struct ClauseInfo {
    int64_t id;
    std::vector<int> literals;
  };

  virtual ~SolverObserver() = default;

  // Fired once before solving begins.
  // Captures the complete initial formula the solver will operate on.
  // Root-level literals forced during parsing are reported afterwards as
  // ordinary on_propagate events.
  virtual void on_init(int variables, int clauses,
    const std::vector<int>& variable_ids,
    const std::vector<ClauseInfo>& clause_list) = 0;

  // Fired after a decision literal is committed to the trail.
  virtual void on_decide(int literal, int level, const char* heuristic) = 0;

  // Fired for each BCP-implied assignment (not decisions).
  virtual void on_propagate(int literal, int level,
    int64_t reason_clause_id,
    const std::vector<int>& reason_literals) = 0;

  // Fired at the start of conflict analysis, before the trail is modified.
  // trail contains all currently assigned literals in assignment order.
  virtual void on_conflict(int64_t clause_id,
    const std::vector<int>& literals, int level,
    const std::vector<int>& trail) = 0;

  // Fired after the 1st-UIP clause is derived, before backtracking.
  // jump_level is the raw second-highest decision level in the learned clause.
  // backtrack_level is the actual backtrack target (may differ under
  // chronological backtracking).
  virtual void on_learn_and_backtrack(
    const std::vector<int>& learned_literals, int glue, int64_t clause_id,
    int jump_level, int backtrack_level) = 0;

  // Fired before a restart backtrack.
  // from_level is the current decision level; to_level is the restart target
  // (may be > 0 under trail reuse).
  virtual void on_restart(int64_t count, int from_level, int to_level) = 0;

  // Fired before a clause is freed during garbage collection.
  virtual void on_delete_clause(int64_t clause_id,
    const std::vector<int>& literals) = 0;

  // Fired immediately after the CDCL loop returns.
  // model is empty for UNSAT/unknown.
  virtual void on_result(const char* result,
    const std::vector<int>& model) = 0;
};
