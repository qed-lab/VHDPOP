#pragma once

#include "domains.h"
#include "formulas.h"
#include <stdexcept>

class Action;
class ActionList;
class Problem;
class ActionDomain;
class Bindings;
class Flaw;
class Unsafe;
class OpenCondition;
class Plan;
class Parameters;


// =================== HeuristicValue ======================

// A heuristic value.
class HeuristicValue {
	// Cost according to additive heuristic.
	float add_cost;
	// Work according to additive heuristic.
	int add_work;
	// Value according to the makespan heursitic.
	float makespan;

public:
	// A zero heuristic value.
	static const HeuristicValue ZERO;
	// A zero cost, unit work, heuristic value.
	static const HeuristicValue ZERO_COST_UNIT_WORK;
	// An infinite heuristic value.
	static const HeuristicValue INFINITE;

	// Construct a zero heuristic value.
	HeuristicValue()
		: add_cost(0.0f), add_work(0), makespan(0.0f) {}

	// Construct a heuristic value.
	HeuristicValue(float add_cost, int add_work, float makespan)
		: add_cost(add_cost), add_work(add_work), makespan(makespan) {}

	// Return the cost according to the additive heurisitc.
	float get_add_cost() const { return add_cost; }

	// Return the work according to the additive heuristic.
	int get_add_work() const { return add_work; }

	// Return the value according to the makespan heuristic.
	float get_makespan() const { return makespan; }

	// Check if this heuristic value is zero.
	bool is_zero() const;

	// Check if this heuristic value is infinite.
	bool is_infinite() const;

	// Add the given heuristic value to this heuristic value.
	HeuristicValue& operator+=(const HeuristicValue& v);

	// Increase the cost of this heuristic value.
	void increase_cost(float x);

	// Increment the work of this heuristic value.
	void increment_work();

	// Increase the makespan of this heuristic value.
	void increase_makespan(float x);
};

// diff here


// =================== PlanningGraph ======================

// A planning graph.
class PlanningGraph {

	// Atom value map.
	class AtomValueMap : public map<const Atom*, HeuristicValue> {
	};

	// Mapping of literals to actions.
	class LiteralAchieverMap
		: public map<const Literal*, ActionEffectMap> {
	};

	// Mapping of predicate names to ground atoms.
	class PredicateAtomsMap : public multimap<Predicate, const Atom*> {
	};

	// Mapping of action name to parameter domain.
	class ActionDomainMap : public map<string, ActionDomain*> {
	};

	// Problem associated with this planning graph.
	const Problem* problem;
	// Atom values.
	AtomValueMap atom_values;
	// Negated atom values.
	AtomValueMap negation_values;
	// Maps formulas to actions that achieve those formulas.
	LiteralAchieverMap achievers;
	// Maps predicates to ground atoms.
	PredicateAtomsMap predicate_atoms;
	// Maps predicates to negated ground atoms.
	PredicateAtomsMap predicate_negations;
	// Maps action names to possible parameter lists.
	ActionDomainMap action_domains;

	// Find an element in a LiteralActionsMap.
	bool find(const LiteralAchieverMap& m, const Literal& l,
		const Action& a, const Effect& e) const;

public:
	// Construct a planning graph.
	PlanningGraph(const Problem& problem, const Parameters& params);

	// Destruct this planning graph.
	~PlanningGraph();

	// Return the problem associated with this planning graph.
	const Problem& get_problem() const { return *problem; }

	// Return the heurisitc value of an atom.
	HeuristicValue heuristic_value(const Atom& atom, size_t step_id,
		const Bindings* bindings = NULL) const;

	// Return the heuristic value of a negated atom.
	HeuristicValue heuristic_value(const Negation& negation, size_t step_id,
		const Bindings* bindings = NULL) const;

	// Return a set of achievers for the given literal.
	const ActionEffectMap* literal_achievers(const Literal& literal) const;

	// Return the parameter domain for the given action, or NULL if the parameter domain is empty.
	const ActionDomain* action_domain(const string& name) const;

};


// =================== InvalidHeuristic  ======================

// An invalid heuristic exception.
class InvalidHeuristic : public runtime_error {
public:
	// Construct an invalid heuristic exception.
	InvalidHeuristic(const string& name)
		:runtime_error("invalid heuristic `" + name + "'") {}
};



// =================== Heuristic  ======================

//
//
// LIFO gives priority to plans created later.
// FIFO gives priority to plans created earlier.
// OC gives priority to plans with few open conditions.
// UC gives priority to plans with few threatened links.
// BUC gives priority to plans with no threatened links.
// S+OC uses h(p) = |S(p)| + w//|OC(p)|.
// UCPOP uses h(p) = |S(p)| + w//(|OC(p)| + |UC(p)|).
// ADD_COST uses the additive cost heuristic.
// ADD_WORK uses the additive work heuristic.
// ADD uses h(p) = |S(p)| + w//ADD_COST.
// ADDR is like ADD, but tries to take reuse into account.
// MAKESPAN gives priority to plans with low makespan.

// Heuristic for ranking plans.
class Heuristic {
	// Heuristics.
	typedef enum {
		LIFO, FIFO, OC, UC, BUC, S_PLUS_OC, UCPOP,
		ADD, ADD_COST, ADD_WORK, ADDR, ADDR_COST, ADDR_WORK,
		MAKESPAN
	} HVal;

	// The selected heuristics.
	vector<HVal> h_;
	// Whether a planning graph is needed by this heuristic.
	bool needs_pg;
public:
	// Construct a heuristic from a name.
	Heuristic(const string& name = "UCPOP");

	// Select a heuristic from a name.
	Heuristic& operator=(const string& name);

	// Check if this heuristic needs a planning graph.
	bool needs_planning_graph() const;

	// Fill the provided vector with the ranks for the given plan.
	void plan_rank(vector<float>& rank, const Plan& plan,
		float weight, const Domain& domain,
		const PlanningGraph* planning_graph) const;
};



// =================== InvalidFlawSelectionOrder  ======================

// An invalid flaw selection order exception.
class InvalidFlawSelectionOrder : public runtime_error {
public:
	// Construct an invalid flaw selection order exception.
	InvalidFlawSelectionOrder(const string& name)
		:runtime_error("invalid flaw selection order `" + name + "'") {}
};



// =================== SelectionCriterion  ======================

//
//
// The specification of a selection criterion more or less follows the
// notation of (Pollack, Joslin, and Paolucci 1997).  Their LC is here
// called LR (least refinements) because we use LC to denote least
// heuristic cost.  While not mentioned by Pollack et al., we have
// implemented MR and REUSE for completeness.  These are the opposites
// of LR and NEW, respectively.  We define four completely new
// tie-breaking strategies based on heuristic evaluation functions.
// These are LC (least cost), MC (most cost), LW (least work), and MW
// (most work).  It is required that a heuristic is specified in
// conjunction with the use of any of these four strategies.  Finally,
// we introduce three new flaw types.  These are 't' for static open
// conditions, 'u' for unsafe open conditions, and 'l' for local open
// conditions.  All three select subsets of 'o', so {t,o}, {u,o}, and
// {t,o} reduce to {o}.
//

// A selection criterion.
struct SelectionCriterion {
	// A selection order.
	typedef enum {
		LIFO, FIFO, RANDOM, LR, MR,
		NEW, REUSE, LC, MC, LW, MW
	} OrderType;
	// A heuristic.
	typedef enum { ADD, MAKESPAN } RankHeuristic;

	// Whether this criterion applies to non-separable threats.
	bool non_separable;
	// Whether this criterion applies to separable threats.
	bool separable;
	// Whether this criterion applies to open conditions.
	bool open_cond;
	// Whether this criterion applies to local open conditions.
	bool local_open_cond;
	// Whether this criterion applies to static open conditions.
	bool static_open_cond;
	// Whether this criterion applies to unsafe open conditions.
	bool unsafe_open_cond;
	// The maximum number of refinements allowed for a flaw that this criterion applies to.
	int max_refinements;
	// The ordering criterion.
	OrderType order;
	// Heuristic used to rank open conditions (if applicable).
	RankHeuristic heuristic;
	// Whether the above heuristic should take reuse into account.
	bool reuse;
};


// =================== FlawSelectionOrder ======================

// Flaw selection order. This is basically a list of selection criteria.
class FlawSelectionOrder {
	// A flaw selection.
	struct FlawSelection {
		// The selected flaw.
		const Flaw* flaw;
		// Index of criterion used to select this flaw.
		int criterion;
		// Rank of this flaw if selected by a ranking criterion.
		float rank;
		// Counts the length of a streak, for use with random order. ???
		int streak;
	};

	// Selection criteria.
	vector<SelectionCriterion> selection_criteria;
	// Whether a planning graph is needed by this flaw selection order.
	bool needs_pg;
	// Index of the first selection criterion involving threats.
	int first_unsafe_criterion;
	// Index of the last selection criterion involving threats.
	int last_unsafe_criterion;
	// Index of the first selection criterion involving open conditions.
	int first_open_cond_criterion;
	// Index of the last selection criterion involving open conditions.
	int last_open_cond_criterion;

	// Search threats for a flaw to select.
	int select_unsafe(FlawSelection& selection, const Plan& plan,
		const Problem& problem,
		int first_criterion, int last_criterion) const;

	// Search open conditions for a flaw to select.
	int select_open_cond(FlawSelection& selection, const Plan& plan,
		const Problem& problem, const PlanningGraph* pg,
		int first_criterion, int last_criterion) const;

public:
	// Construct a default flaw selection order.
	FlawSelectionOrder(const string& name = "UCPOP");

	// Select a flaw selection order from a name.
	FlawSelectionOrder& operator=(const string& name);

	// Check if this flaw order needs a planning graph.
	bool needs_planning_graph() const;

	// Select a flaw from the flaws of the given plan.
	const Flaw& select(const Plan& plan, const Problem& problem,
		const PlanningGraph* pg) const;
};