#include "internal.hpp"

#include "NdjsonObserver.hpp"
#include "NdjsonFileObserver.hpp"

#include <memory>

namespace CaDiCaL {

  // Chosen once in hook_init: file observer when eventlog_path is set,
  // stdout observer otherwise.
  static std::unique_ptr<SolverObserver> g_observer;

  static SolverObserver& observer() { return *g_observer; }

  // Root-level propagations fire during parsing, before the observer exists
  // (on_init needs the fully parsed clause list). They are buffered here and
  // replayed as ordinary propagate events right after on_init.
  struct PendingPropagate {
    int lit;
    int level;
    int64_t reason_id;
    std::vector<int> reason_lits;
  };
  static std::vector<PendingPropagate> g_pending;

  void Internal::hook_init() {
    if (!opts.eventlog)
      return;
    if (g_observer)  // already initialized (guards re-entry / incremental solve)
      return;

    if (eventlog_path)
      g_observer = std::make_unique<NdjsonFileObserver>(eventlog_path);
    else
      g_observer = std::make_unique<NdjsonObserver>();

    std::vector<int> variable_ids;
    variable_ids.reserve(max_var);
    for (int i = 1; i <= max_var; i++)
      variable_ids.push_back(i);

    std::vector<SolverObserver::ClauseInfo> clause_list;
    for (auto c : clauses) {
      if (!c->redundant && !c->garbage) {
        SolverObserver::ClauseInfo info;
        info.id = c->id;
        info.literals.assign(c->begin(), c->end());
        clause_list.push_back(std::move(info));
      }
    }

    observer().on_init(max_var, (int)clause_list.size(), variable_ids,
      clause_list);

    // Replay propagations buffered during parsing, in trail order, now that
    // the observer exists and the init snapshot has been emitted.
    for (const auto& p : g_pending)
      observer().on_propagate(p.lit, p.level, p.reason_id, p.reason_lits);
    g_pending.clear();
  }

  void Internal::hook_decide(int lit, bool random_dec) {
    if (!opts.eventlog || !g_observer)
      return;

    const char* heuristic =
      random_dec ? "random" : (use_scores() ? "vsids" : "vmtf");
    observer().on_decide(lit, level, heuristic);
  }

  void Internal::hook_propagate(int lit, int lit_level, Clause* reason) {
    if (!opts.eventlog || searching_lucky_phases)
      return;

    int level;
    int64_t reason_id;
    std::vector<int> reason_lits;
    if (reason) {
      level = lit_level;
      reason_id = reason->id;
      reason_lits.assign(reason->begin(), reason->end());
    } else {
      level = 0;
      reason_id = -1;
    }

    // Before hook_init runs (root propagations during parsing) buffer the
    // event; it is flushed in trail order once the observer is created.
    if (!g_observer) {
      g_pending.push_back({lit, level, reason_id, std::move(reason_lits)});
      return;
    }

    observer().on_propagate(lit, level, reason_id, reason_lits);
  }

  void Internal::hook_conflict() {
    if (!opts.eventlog || !g_observer || searching_lucky_phases)
      return;

    assert(conflict);
    std::vector<int> lits(conflict->begin(), conflict->end());
    observer().on_conflict(conflict->id, lits, level, trail);
  }

  void Internal::hook_learn_and_backtrack(int glue, int jump, int new_level,
    Clause* driving) {
    if (!opts.eventlog || !g_observer || searching_lucky_phases)
      return;

    int64_t cid = driving ? driving->id : -1;
    observer().on_learn_and_backtrack(clause, glue, cid, jump, new_level);
  }

  void Internal::hook_restart(int to_level) {
    if (!opts.eventlog || !g_observer)
      return;

    observer().on_restart(stats.restarts, level, to_level);
  }

  void Internal::hook_delete_clause(Clause* c) {
    if (!opts.eventlog || !g_observer)
      return;

    std::vector<int> lits(c->begin(), c->end());
    observer().on_delete_clause(c->id, lits);
  }

  void Internal::hook_result(int res) {
    if (!opts.eventlog || !g_observer)
      return;

    const char* result_str =
      (res == 10) ? "sat" : (res == 20) ? "unsat" : "unknown";

    std::vector<int> model;
    if (res == 10) {
      for (int idx = 1; idx <= max_var; idx++) {
        const signed char v = val(idx);
        if (v)
          model.push_back(v > 0 ? idx : -idx);
      }
    }

    observer().on_result(result_str, model);
  }

}
