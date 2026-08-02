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

static const int NDJSON_PROTOCOL_VERSION = 2;

class SolverObserver {
public:
  struct ClauseInfo {
    int64_t id;
    std::vector<int> literals;
  };

  enum class BacktrackKind {
    Conflict, // unwind that resolves a conflict: backjump, chronological unwind before analysis, or otf subsumption
    Restart,  // the unwind belonging to a restart
    Other     // inprocessing, preprocessing, incremental API, cleanup
  };

  static const char *to_string(BacktrackKind kind) {
    switch (kind) {
    case BacktrackKind::Conflict: return "conflict";
    case BacktrackKind::Restart: return "restart";
    default: return "other";
    }
  }

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

  // Fired after the 1st-UIP clause is derived. Reports the clause only.
  // jump_level is the raw second-highest decision level in the learned clause.
  virtual void on_learn(const std::vector<int>& learned_literals, int glue,
    int64_t clause_id, int jump_level) = 0;

  // Fired before every trail unwind, from any site in the solver, and only
  // when something is actually unwound (to_level < from_level).
  //
  // reason is optional free-form detail naming the phase that requested the unwind.
  virtual void on_backtrack(int from_level, int to_level, BacktrackKind kind,
    const char* reason) = 0;

  // Fired before a restart. A marker only, the restart's own unwind arrives
  // as an on_backtrack with kind Restart (which may be absent when trail
  // reuse leaves the level unchanged).
  virtual void on_restart(int64_t count) = 0;

  // Fired before a clause is freed during garbage collection.
  virtual void on_delete_clause(int64_t clause_id,
    const std::vector<int>& literals) = 0;

  // Fired immediately after the CDCL loop returns.
  // model is empty for UNSAT/unknown.
  virtual void on_result(const char* result,
    const std::vector<int>& model) = 0;
};
