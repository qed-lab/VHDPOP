#pragma once

#include "chain.h"
#include "flaws.h"
#include "actions.h"
#include "orderings.h"

class Parameters;
class BindingList;
class Literal;
class Atom;
class Negation;
class Effect;
class EffectList;
class Action;
class Problem;
class Bindings;
class ActionEffectMap;
class FlawSelectionOrder;


//=================== Link ====================

// Causal link.
class Link {
	// Id of step that link goes from.
	size_t from_id;
	// Time of effect satisfying link.
	StepTime effect_time;
	// Id of step that link goes to.
	size_t to_id;
	// Condition satisfied by link.
	const Literal* condition;
	// Time of condition satisfied by link.
	FormulaTime condition_time;

public:
	// Construct a causal link.
	Link(size_t from_id, StepTime effect_time, const OpenCondition& open_cond)
		:from_id(from_id), effect_time(effect_time), to_id(open_cond.get_step_id()),
		condition(open_cond.literal()), condition_time(open_cond.get_when()) {
		Formula::register_use(this->condition);
	}

	// Copy constructor of a causal link.
	Link(const Link& l)
		:from_id(l.from_id), effect_time(l.effect_time), to_id(l.to_id),
		condition(l.condition), condition_time(l.condition_time) {
		Formula::register_use(condition);
		
	}

	// Destruct this causal link.
	~Link() {
		Formula::unregister_use(condition);
	}

	// Return the id of step that link goes from.
	size_t get_from_id() const { return from_id; }

	// Return the time of effect satisfying link.
	StepTime get_effect_time() const { return effect_time; }

	// Return the id of step that link goes to.
	size_t get_to_id() const { return  to_id; }

	// Return the condition satisfied by link.
	const Literal& get_condition() const { return *condition; }

	// Return the time of the condition satisfied by this link.
	FormulaTime get_condition_time() const { return condition_time; }
};

// Equality operator for links.
inline bool operator==(const Link& l1, const Link& l2) {
	return &l1 == &l2;
}


//=================== Step ====================

// A plan step
class Step {
	// Step id.
	size_t id;
	// Action that this step is instantiated from.
	const Action* action;

public:
	// Construct a step instantiated from an action.
	Step(size_t id, const Action& action)
		: id(id), action(&action) {}

	// Construct a step.
	Step(const Step& s)
		: id(s.id), action(s.action) {}

	// Return the step id.
	size_t get_id() const { return id; }

	// Return the action that this step is instantiated from.
	const Action& get_action() const { return *action; }
};


//=================== StepSorter ====================

// Sorting of steps based on distance from initial conditions.
struct StepSorter {
	StepSorter(map<size_t, float>& dist)
		: dist(dist) {}

	bool operator()(const Step* s1, const Step* s2) const {
		return dist[s1->get_id()] < dist[s2->get_id()];
	}

	map<size_t, float>& dist;
};


//=================== Plan ====================

// A plan.
class Plan {
private:
	// List of plans.
	class PlanList :public vector<const Plan*> {
	};

	// Chain of steps.
	const Chain<Step>* steps;
	// Number of unique steps in plan.
	size_t num_steps;
	// Chain of causal links.
	const Chain<Link>* links;
	// Number of causal links.
	size_t num_links;
	// Ordering constraints of this plan.
	const Orderings* orderings;
	// Binding constraints of this plan.
	const Bindings* bindings;
	// Chain of potentially threatened links.
	const Chain<Unsafe>* unsafes;
	// Number of potentially threatened links.
	size_t num_unsafes;
	// Chain of open conditions.
	const Chain<OpenCondition>* open_conds;
	// Number of open conditions.
	const size_t num_open_conds;
	// Chain of mutex threats.
	const Chain<MutexThreat>* mutex_threats;
	// Rank of this plan.
	mutable vector<float> rank;
	// Plan id (serial number).
	mutable size_t id;

#ifdef DEBUG
	// Depth of this plan in the search space.
	size_t depth_;
#endif

	// Return the initial plan representing the given problem, or NULL if goals of problem are inconsistent.
	static const Plan* make_initial_plan(const Problem& problem);

	// Construct a plan.
	Plan(const Chain<Step>* steps, size_t num_steps,
		const Chain<Link>* links, size_t num_links,
		const Orderings& orderings, const Bindings& bindings,
		const Chain<Unsafe>* unsafes, size_t num_unsafes,
		const Chain<OpenCondition>* open_conds, size_t num_open_conds,
		const Chain<MutexThreat>* mutex_threats, const Plan* parent);

	// Return the next flaw to work on.
	const Flaw& get_flaw(const FlawSelectionOrder& flaw_order) const;

	// Return the refinements for the next flaw to work on.
	void refinements(PlanList& plans,
		const FlawSelectionOrder& flaw_order) const;

	// Handle an unsafe link.
	void handle_unsafe(PlanList& plans, const Unsafe& unsafe) const;

	// Handle an unsafe link through separation.
	int separate(PlanList& plans, const Unsafe& unsafe,
		const BindingList& unifier, bool test_only = false) const;

	// Handle an unsafe link through demotion.
	int demote(PlanList& plans, const Unsafe& unsafe,
		bool test_only = false) const;

	// Handle an unsafe link through promotion.
	int promote(PlanList& plans, const Unsafe& unsasfe,
		bool test_only = false) const;

	// Add a plan to the given plan list with an ordering added.
	void new_ordering(PlanList& plans, size_t before_id, StepTime t1,
		size_t after_id, StepTime t2,
		const Unsafe& unsafe) const;

	// Handle a mutex threat.
	void handle_mutex_threat(PlanList& plans,
		const MutexThreat& mutex_threat) const;

	// Handle a mutex threat through separation.
	void separate(PlanList& plans, const MutexThreat& mutex_threat,
		const BindingList& unifier) const;

	// Handle a mutex threat through demotion.
	void demote(PlanList& plans, const MutexThreat& mutex_threat) const;

	// Handle a mutex threat through promotion.
	void promote(PlanList& plans, const MutexThreat& mutex_threat) const;

	// Add a plan to the given plan list with an ordering added.
	void new_ordering(PlanList& plans, size_t before_id, StepTime t1,
		size_t after_id, StepTime t2,
		const MutexThreat& mutex_threat) const;

	// Handle an open condition.
	void handle_open_condition(PlanList& plans,
		const OpenCondition& open_cond) const;

	// Handle a disjunctive open condition.
	int handle_disjunction(PlanList& plans, const Disjunction& disj,
		const OpenCondition& open_cond,
		bool test_only = false) const;

	// Handle an inequality open condition.
	int handle_inequality(PlanList& plans, const Inequality& neq,
		const OpenCondition& open_cond,
		bool test_only = false) const;

	// Handle a literal open condition by adding a new step.
	void add_step(PlanList& plans, const Literal& literal,
		const OpenCondition& open_cond,
		const ActionEffectMap& achievers) const {
		for (ActionEffectMap::const_iterator ai = achievers.begin();
			ai != achievers.end(); ai++) {
			const Action& action = *(*ai).first;
			if (action.get_name().substr(0, 1) != "<") {
				const Effect& effect = *(*ai).second;
				new_link(plans, Step(get_num_steps() + 1, action), effect,
					literal, open_cond);
			}
		}
	}

	// Handle a literal open condition by reusing an existing step.
	void reuse_step(PlanList& plans, const Literal& literal,
		const OpenCondition& open_cond,
		const ActionEffectMap& achievers) const {
		StepTime gt = start_time(open_cond.get_when());
		for (const Chain<Step>* sc = get_steps(); sc != NULL; sc = sc->tail) {
			const Step& step = sc->head;
			if (get_orderings().possibly_before(step.get_id(), StepTime::AT_START,
				open_cond.get_step_id(), gt)) {
				std::pair<ActionEffectMap::const_iterator,
					ActionEffectMap::const_iterator> b =
					achievers.equal_range(&step.get_action());
				for (ActionEffectMap::const_iterator ei = b.first;
					ei != b.second; ei++) {
					const Effect& effect = *(*ei).second;
					StepTime et = end_time(effect);
					if (get_orderings().possibly_before(step.get_id(), et,
						open_cond.get_step_id(), gt)) {
						new_link(plans, step, effect, literal, open_cond);
					}
				}
			}
		}
	}

	// Add plans to the given plan list with a link from the given step to the given open condition added.
	int new_link(PlanList& plans, const Step& step, const Effect& effect,
		const Literal& literal, const OpenCondition& open_cond,
		bool test_only = false) const;

	// Add plans to the given plan list with a link from the given step to the given open condition added using the closed world assumption.
	int new_cw_link(PlanList& plans, const EffectList& effects,
		const Negation& negation, const OpenCondition& open_cond,
		bool test_only = false) const;

	// Return a plan with a link added from the given effect to the given open condition.
	int make_link(PlanList& plans, const Step& step, const Effect& effect,
		const Literal& literal, const OpenCondition& open_cond,
		const BindingList& unifier, bool test_only = false) const;

	friend bool operator<(const Plan& p1, const Plan& p2);
	friend ostream& operator<<(ostream& os, const Plan& p);


public:
	// Id of goal step.
	static const size_t GOAL_ID;

	// Return plan for given problem.
	static const Plan* plan(const Problem& problem, const Parameters& params,
		bool last_problem);

	// Cleans up after planning.
	static void cleanup();

	// Destruct this plan.
	~Plan();

	// Return the steps of this plan.
	const Chain<Step>* get_steps() const { return steps; }

	// Return the number of unique steps in this plan.
	size_t get_num_steps() const { return num_steps; }

	// Return the links of this plan.
	const Chain<Link>* get_links() const { return links; }

	// Return the number of links in this plan.
	size_t get_num_links() const { return num_links; }

	// Return the ordering constraints of this plan.
	const Orderings& get_orderings() const { return *orderings; }

	// Return the bindings of this plan.
	const Bindings* get_bindings() const;

	// Return the potentially threatened links of this plan.
	const Chain<Unsafe>* get_unsafes() const { return unsafes; }

	// Return the number of potentially threatened links in this plan.
	size_t get_num_unsafes() const { return num_unsafes; }

	// Return the open conditions of this plan.
	const Chain<OpenCondition>* get_open_conds() const { return open_conds; }

	// Return the number of open conditions in this plan.
	size_t get_num_open_conds() const { return num_open_conds; }

	// Return the mutex threats of this plan.
	const Chain<MutexThreat>* get_mutex_threats() const { return mutex_threats; }

	// Check if this plan is complete.
	bool is_complete() const;

	// Return the primary rank of this plan, where a lower rank signifies a better plan.
	float primary_rank() const;

	// Return the serial number of this plan.
	size_t get_serial_no() const;

#ifdef DEBUG
	// Return the depth of this plan.
	size_t depth() const { return depth_; }
#endif

	// Count the number of refinements for the given threat, and returns true iff the number of refinements does not exceed the given limit.
	bool unsafe_refinements(int& refinements, int& separable,
		int& promotable, int& demotable,
		const Unsafe& unsafe, int limit) const;

	// Check if the given threat is separable.
	int is_separable(const Unsafe& unsafe) const;

	// Check if the given open condition is threatened.
	bool is_unsafe_open_condition(const OpenCondition& open_cond) const;

	// Count the number of refinements for the given open condition, and returns true iff the number of refinements does not exceed the given limit.
	bool open_cond_refinements(int& refinements, int& addable, int& reusable,
		const OpenCondition& open_cond, int limit) const;

	// Count the number of add-step refinements for the given literal open condition, and returns true iff the number of refinements does not exceed the given limit.
	bool addable_steps(int& refinements, const Literal& literal,
		const OpenCondition& open_cond, int limit) const;

	// Count the number of reuse-step refinements for the given literal open condition, and returns true iff the number of refinements does not exceed the given limit.
	bool reusable_steps(int& refinements, const Literal& literal,
		const OpenCondition& open_cond, int limit) const;
};