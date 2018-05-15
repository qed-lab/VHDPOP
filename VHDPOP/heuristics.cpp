#include "heuristics.h"

#include "bindings.h"
#include "parameters.h"
#include "plans.h"
#include "problems.h"
#include <algorithm>

extern int verbosity;


// Some static functions for later use.

// Generate a random number in the interval [0,1).
static double rand01ex() {
	return rand() / (RAND_MAX + 1.0);
}

// Return the sum of two integers, avoiding overflow.
static int sum(int n, int m) {
	return (numeric_limits<int>::max() - n > m) ?
		n + m : numeric_limits<int>::max();
}

// Compute the heuristic value of the given formula.
static void formula_value(HeuristicValue& h, HeuristicValue& hs,
	const Formula& formula, size_t step_id,
	const Plan& plan, const PlanningGraph& pg,
	bool reuse = false) {
	const Bindings* bindings = plan.get_bindings();
	if (reuse) {
		const Literal* literal;
		FormulaTime when;
		const TimedLiteral* tl = dynamic_cast<const TimedLiteral*>(&formula);
		if (tl != NULL) {
			literal = &tl->get_literal();
			when = tl->get_when();
		}
		else {
			literal = dynamic_cast<const Literal*>(&formula);
			when = AT_START_F;
		}
		if (literal != NULL) {
			StepTime gt = start_time(when);
			if (!PredicateTable::is_static(literal->get_predicate())) {
				for (const Chain<Step>* sc = plan.get_steps(); sc != NULL; sc = sc->tail) {
					const Step& step = sc->head;
					if (step.get_id() != 0
						&& plan.get_orderings().possibly_before(step.get_id(),
							StepTime::AT_START,
							step_id, gt)) {
						const EffectList& effs = step.get_action().get_effects();
						for (EffectList::const_iterator ei = effs.begin();
							ei != effs.end(); ei++) {
							const Effect& e = **ei;
							StepTime et = end_time(e);
							if (plan.get_orderings().possibly_before(step.get_id(), et,
								step_id, gt)) {
								if (typeid(*literal) == typeid(e.get_literal())) {
									if ((bindings != NULL
										&& bindings->unify(*literal, step_id,
											e.get_literal(), step.get_id()))
										|| (bindings == NULL && literal == &e.get_literal())) {
										h = HeuristicValue::ZERO_COST_UNIT_WORK;
										if (when != AT_END) {
											hs = HeuristicValue::ZERO_COST_UNIT_WORK;
										}
										else {
											hs = HeuristicValue::ZERO;
										}
										return;
									}
								}
							}
						}
					}
				}
			}
		}
		else {
			const Disjunction* disj = dynamic_cast<const Disjunction*>(&formula);
			if (disj != NULL) {
				h = hs = HeuristicValue::INFINITE;
				for (FormulaList::const_iterator fi = disj->get_disjuncts().begin();
					fi != disj->get_disjuncts().end(); fi++) {
					HeuristicValue hi, hsi;
					formula_value(hi, hsi, **fi, step_id, plan, pg, true);
					h = min(h, hi);
					hs = min(hs, hsi);
				}
			}
			else {
				const Conjunction* conj = dynamic_cast<const Conjunction*>(&formula);
				if (conj != NULL) {
					h = hs = HeuristicValue::ZERO;
					for (FormulaList::const_iterator fi = conj->get_conjuncts().begin();
						fi != conj->get_conjuncts().end(); fi++) {
						HeuristicValue hi, hsi;
						formula_value(hi, hsi, **fi, step_id, plan, pg, true);
						h += hi;
						hs += hsi;
					}
				}
				else {
					const Exists* exists = dynamic_cast<const Exists*>(&formula);
					if (exists != NULL) {
						formula_value(h, hs, exists->get_body(), step_id, plan, pg, true);
					}
					else {
						const Forall* forall = dynamic_cast<const Forall*>(&formula);
						if (forall != NULL) {
							formula_value(h, hs, forall->get_universal_base(SubstitutionMap(),
								pg.get_problem()),
								step_id, plan, pg, true);
						}
					}
				}
			}
			return;
		}
	}
	formula.get_heuristic_value(h, hs, pg, step_id, bindings);
}


// ============== Heuristic evaluation functions for formulas. ==============

// Return the heuristic value of this formula.
void Constant::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	h = hs = HeuristicValue::ZERO;
}

// Return the heuristic value of this formula (atom). 
void Atom::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	h = hs = pg.heuristic_value(*this, step_id, b);
}

// Return the heuristic value of this formula (negated atom). 
void Negation::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	h = hs = pg.heuristic_value(*this, step_id, b); // same as atom
}

// Return the heuristic value of this formula (equality). ???
void Equality::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	if (b == NULL) {
		h = hs = HeuristicValue::ZERO;
	}
	else if (b->is_consistent_with(*this, step_id)) {
		h = hs = HeuristicValue::ZERO;
	}
	else {
		h = hs = HeuristicValue::INFINITE;
	}
}

// Return the heuristic value of this formula (inequality). ???
void Inequality::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	if (b == NULL) {
		h = hs = HeuristicValue::ZERO;
	}
	else if (b->is_consistent_with(*this, step_id)) {
		h = hs = HeuristicValue::ZERO;
	}
	else {
		h = hs = HeuristicValue::INFINITE;
	}
}

// Return the heuristic value of this formula (conjunction).
void Conjunction::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	h = hs = HeuristicValue::ZERO;
	for (FormulaList::const_iterator fi = get_conjuncts().begin();
		fi != get_conjuncts().end() && !h.is_infinite(); fi++) {
		HeuristicValue hi, hsi;
		(*fi)->get_heuristic_value(hi, hsi, pg, step_id, b);
		h += hi;
		hs += hsi;
	}
}

// Return the heuristic value of this formula (disjunction).
void Disjunction::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	h = hs = HeuristicValue::INFINITE;
	for (FormulaList::const_iterator fi = get_disjuncts().begin();
		fi != get_disjuncts().end() && !h.is_zero(); fi++) {
		HeuristicValue hi, hsi;
		(*fi)->get_heuristic_value(hi, hsi, pg, step_id, b);
		h = min(h, hi);
		hs = min(hs, hsi);
	}
}

// Return the heuristic value of this formula (existential quantification).
void Exists::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	get_body().get_heuristic_value(h, hs, pg, step_id, b);
}

// Return the heuristic value of this formula (universal quantification).
void Forall::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	const Formula& f = get_universal_base(SubstitutionMap(), pg.get_problem());
	f.get_heuristic_value(h, hs, pg, step_id, b);
}

// Return the heuristic value of this formula (timed literal).
void TimedLiteral::get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
	const PlanningGraph& pg, size_t step_id,
	const Bindings* b) const {
	get_literal().get_heuristic_value(h, hs, pg, step_id, b);
	if (get_when() == AT_END) {
		hs = HeuristicValue::ZERO;
	}
} //!!!


//=================== HeuristicValue ====================

// A zero heuristic value.
const HeuristicValue HeuristicValue::ZERO =
HeuristicValue(0.0f, 0, Orderings::threshold);
// A zero cost, unit work, heuristic value.
const HeuristicValue HeuristicValue::ZERO_COST_UNIT_WORK =
HeuristicValue(0.0f, 1, Orderings::threshold);
// An infinite heuristic value.
const HeuristicValue
HeuristicValue::INFINITE = HeuristicValue(
	numeric_limits<float>::infinity(),
	numeric_limits<int>::max(),
	numeric_limits<float>::infinity());


// Check if this heuristic value is zero.
bool HeuristicValue::is_zero() const {
	return get_add_cost() == 0.0f;
}


// Checks if this heuristic value is infinite.
bool HeuristicValue::is_infinite() const {
	return get_makespan() == numeric_limits<float>::infinity();
}


// Adds the given heuristic value to this heuristic value.
HeuristicValue& HeuristicValue::operator+=(const HeuristicValue& v) {
	add_cost += v.get_add_cost();
	add_work = sum(get_add_work(), v.get_add_work());
	if (get_makespan() < v.get_makespan()) {
		makespan = v.get_makespan();
	}
	return *this;
}


// Increase the cost of this heuristic value.
void HeuristicValue::increase_cost(float x) {
	add_cost += x;
}


// Increment the work of this heuristic value.
void HeuristicValue::increment_work() {
	add_work = sum(add_work, 1); // diff
}


// Increase the makespan of this heuristic value.
void HeuristicValue::increase_makespan(float x) {
	makespan += x;
}

// Inequality operator for heuristic values.
bool operator!=(const HeuristicValue& v1, const HeuristicValue& v2) {
	return (v1.get_add_cost() != v2.get_add_cost() || v1.get_add_work() != v2.get_add_work()
		|| v1.get_makespan() != v2.get_makespan());
}

// diff here

// Return the componentwise minimum heuristic value, given two heuristic values.
HeuristicValue min(const HeuristicValue& v1, const HeuristicValue& v2) {
	float add_cost;
	int add_work;
	if (v1.get_add_cost() == v2.get_add_cost()) {
		add_cost = v1.get_add_cost();
		add_work = std::min(v1.get_add_work(), v2.get_add_work());
	}
	else if (v1.get_add_cost() < v2.get_add_cost()) {
		add_cost = v1.get_add_cost();
		add_work = v1.get_add_work();
	}
	else {
		add_cost = v2.get_add_cost();
		add_work = v2.get_add_work();
	}
	return HeuristicValue(add_cost, add_work,
		min(v1.get_makespan(), v2.get_makespan()));
}

// Output operator for heuristic values.
ostream& operator<<(ostream& os, const HeuristicValue& v) {
	os << "ADD<" << v.get_add_cost() << ',' << v.get_add_work() << '>'
		<< " MS<" << v.get_makespan() << '>';
	return os;
}


//=================== Heuristic ====================

// Construct a heuristic from a name.
Heuristic::Heuristic(const string& name) {
	*this = name;
}


// Select a heuristic from a name.
Heuristic& Heuristic::operator=(const string& name) {
	h_.clear();
	needs_pg = false;
	size_t pos = 0;
	while (pos < name.length()) {
		size_t next_pos = name.find('/', pos);
		string key = name.substr(pos, next_pos - pos);
		const char* n = key.c_str();
		if (strcasecmp(n, "LIFO") == 0) {
			h_.push_back(LIFO);
		}
		else if (strcasecmp(n, "FIFO") == 0) {
			h_.push_back(FIFO);
		}
		else if (strcasecmp(n, "OC") == 0) {
			h_.push_back(OC);
		}
		else if (strcasecmp(n, "UC") == 0) {
			h_.push_back(UC);
		}
		else if (strcasecmp(n, "BUC") == 0) {
			h_.push_back(BUC);
		}
		else if (strcasecmp(n, "S+OC") == 0) {
			h_.push_back(S_PLUS_OC);
		}
		else if (strcasecmp(n, "UCPOP") == 0) {
			h_.push_back(UCPOP);
		}
		else if (strcasecmp(n, "ADD") == 0) {
			h_.push_back(ADD);
			needs_pg = true;
		}
		else if (strcasecmp(n, "ADD_COST") == 0) {
			h_.push_back(ADD_COST);
			needs_pg = true;
		}
		else if (strcasecmp(n, "ADD_WORK") == 0) {
			h_.push_back(ADD_WORK);
			needs_pg = true;
		}
		else if (strcasecmp(n, "ADDR") == 0) {
			h_.push_back(ADDR);
			needs_pg = true;
		}
		else if (strcasecmp(n, "ADDR_COST") == 0) {
			h_.push_back(ADDR_COST);
			needs_pg = true;
		}
		else if (strcasecmp(n, "ADDR_WORK") == 0) {
			h_.push_back(ADDR_WORK);
			needs_pg = true;
		}
		else if (strcasecmp(n, "MAKESPAN") == 0) {
			h_.push_back(MAKESPAN);
			needs_pg = true;
		}
		else {
			throw InvalidHeuristic(name);
		}
		pos = next_pos;
		if (name[pos] == '/') {
			pos++;
			if (pos >= name.length()) {
				throw InvalidHeuristic(name);
			}
		}
	}
	return *this;
}


// Check if this heuristic needs a planning graph.
bool Heuristic::needs_planning_graph() const {
	return needs_pg;
}


// Fill the provided vector with the ranks for the given plan.
void Heuristic::plan_rank(vector<float>& rank, const Plan& plan,
	float weight, const Domain& domain,
	const PlanningGraph* planning_graph) const {
	bool add_done = false;
	float add_cost = 0.0f;
	int add_work = 0;
	bool addr_done = false;
	float addr_cost = 0.0f;
	int addr_work = 0;
	for (vector<HVal>::const_iterator hi = h_.begin();
		hi != h_.end(); hi++) {
		HVal h = *hi;
		switch (h) {
		case LIFO:
			rank.push_back(-1.0*plan.get_serial_no());
			break;
		case FIFO:
			rank.push_back(plan.get_serial_no());
			break;
		case OC:
			rank.push_back(plan.get_num_open_conds());
			break;
		case UC:
			rank.push_back(plan.get_num_unsafes());
			break;
		case BUC:
			rank.push_back((plan.get_num_unsafes() > 0) ? 1 : 0);
			break;
		case S_PLUS_OC:
			rank.push_back(plan.get_num_steps() + weight * plan.get_num_open_conds());
			break;
		case UCPOP:
			rank.push_back(plan.get_num_steps()
				+ weight * (plan.get_num_open_conds() + plan.get_num_unsafes()));
			break;
		case ADD:
		case ADD_COST:
		case ADD_WORK:
			if (!add_done) {
				add_done = true;
				for (const Chain<OpenCondition>* occ = plan.get_open_conds();
					occ != NULL; occ = occ->tail) {
					const OpenCondition& open_cond = occ->head;
					HeuristicValue v, vs;
					formula_value(v, vs, open_cond.get_condition(), open_cond.get_step_id(),
						plan, *planning_graph);
					add_cost += v.get_add_cost();
					add_work = sum(add_work, v.get_add_work());
				}
			}
			if (h == ADD) {
				if (add_cost < numeric_limits<int>::max()) {
					rank.push_back(plan.get_num_steps() + weight * add_cost);
				}
				else {
					rank.push_back(numeric_limits<float>::infinity());
				}
			}
			else if (h == ADD_COST) {
				if (add_cost < numeric_limits<int>::max()) {
					rank.push_back(add_cost);
				}
				else {
					rank.push_back(numeric_limits<float>::infinity());
				}
			}
			else {
				if (add_work < numeric_limits<int>::max()) {
					rank.push_back(add_work);
				}
				else {
					rank.push_back(numeric_limits<float>::infinity());
				}
			}
			break;
		case ADDR:
		case ADDR_COST:
		case ADDR_WORK:
			if (!addr_done) {
				addr_done = true;
				for (const Chain<OpenCondition>* occ = plan.get_open_conds();
					occ != NULL; occ = occ->tail) {
					const OpenCondition& open_cond = occ->head;
					HeuristicValue v, vs;
					formula_value(v, vs, open_cond.get_condition(), open_cond.get_step_id(),
						plan, *planning_graph, true);
					addr_cost += v.get_add_cost();
					addr_work = sum(addr_work, v.get_add_work());
				}
			}
			if (h == ADDR) {
				if (addr_cost < numeric_limits<int>::max()) {
					rank.push_back(plan.get_num_steps() + weight * addr_cost);
				}
				else {
					rank.push_back(numeric_limits<float>::infinity());
				}
			}
			else if (h == ADDR_COST) {
				if (addr_cost < numeric_limits<int>::max()) {
					rank.push_back(addr_cost);
				}
				else {
					rank.push_back(numeric_limits<float>::infinity());
				}
			}
			else {
				if (addr_work < numeric_limits<int>::max()) {
					rank.push_back(addr_work);
				}
				else {
					rank.push_back(numeric_limits<float>::infinity());
				}
			}
			break;
		case MAKESPAN:
			map<pair<size_t, StepTime::StepPoint>, float> min_times;
			for (const Chain<OpenCondition>* occ = plan.get_open_conds();
				occ != NULL; occ = occ->tail) {
				const OpenCondition& open_cond = occ->head;
				HeuristicValue v, vs;
				formula_value(v, vs, open_cond.get_condition(), open_cond.get_step_id(),
					plan, *planning_graph);
				map<pair<size_t, StepTime::StepPoint>, float>::iterator di =
					min_times.find(make_pair(open_cond.get_step_id(), StepTime::START));
				if (di != min_times.end()) {
					if (weight*vs.get_makespan() > (*di).second) {
						(*di).second = weight * vs.get_makespan();
					}
				}
				else {
					min_times.insert(make_pair(make_pair(open_cond.get_step_id(),
						StepTime::START),
						weight*vs.get_makespan()));
				}
				di = min_times.find(make_pair(open_cond.get_step_id(),
					StepTime::END));
				if (di != min_times.end()) {
					if (weight*v.get_makespan() > (*di).second) {
						(*di).second = weight * v.get_makespan();
					}
				}
				else {
					min_times.insert(make_pair(make_pair(open_cond.get_step_id(),
						StepTime::END),
						weight*v.get_makespan()));
				}
			}
			rank.push_back(plan.get_orderings().makespan(min_times));
			break;
		}
	}
}



//=================== PlanningGraph ====================

// Construct a planning graph.
PlanningGraph::PlanningGraph(const Problem& problem, const Parameters& params)
	: problem(&problem) {
	// Find all consistent action instantiations.
	GroundActionList actions;
	problem.instantiated_actions(actions);
	if (verbosity > 0) {
		cerr << endl << "Instantiated actions: " << actions.size()
			<< endl;
	}
	// Find duration scaling factors for literals.
	map<const Literal*, float> duration_factor;
	if (params.action_cost == Parameters::RELATIVE) {
		for (GroundActionList::const_iterator ai = actions.begin();
			ai != actions.end(); ai++) {
			const GroundAction& action = **ai;
			const Value* min_v = dynamic_cast<const Value*>(&action.get_min_duration());
			if (min_v == NULL) {
				throw runtime_error("non-constant minimum duration");
			}
			float d = max(Orderings::threshold, min_v->get_value());
			for (EffectList::const_iterator ei = action.get_effects().begin();
				ei != action.get_effects().end(); ei++) {
				const Literal& literal = (*ei)->get_literal();
				map<const Literal*, float>::const_iterator di =
					duration_factor.find(&literal);
				if (di == duration_factor.end()) {
					duration_factor.insert(make_pair(&literal, d));
				}
				else if (d < (*di).second) {
					duration_factor[&literal] = d;
				}
			}
		}
		for (TimedActionTable::const_iterator ai = problem.get_timed_actions().begin();
			ai != problem.get_timed_actions().end(); ai++) {
			float d = (*ai).first;
			const GroundAction& action = *(*ai).second;
			for (EffectList::const_iterator ei = action.get_effects().begin();
				ei != action.get_effects().end(); ei++) {
				const Literal& literal = (*ei)->get_literal();
				map<const Literal*, float>::const_iterator di =
					duration_factor.find(&literal);
				if (di == duration_factor.end()) {
					duration_factor.insert(make_pair(&literal, d));
				}
				else if (d < (*di).second) {
					duration_factor[&literal] = d;
				}
			}
		}
	}
	if (verbosity > 2) {
		cerr << "Duration factors:" << endl;
		for (map<const Literal*, float>::const_iterator di =
			duration_factor.begin(); di != duration_factor.end(); di++) {
			cerr << "  ";
			(*di).first->print(cerr, 0, Bindings::EMPTY);
			cerr << ": " << (*di).second << endl;
		}
	}

	// Add initial conditions at level 0.
	const GroundAction& ia = problem.get_init_action();
	for (EffectList::const_iterator ei = ia.get_effects().begin();
		ei != ia.get_effects().end(); ei++) {
		const Atom& atom = dynamic_cast<const Atom&>((*ei)->get_literal());
		achievers[&atom].insert(make_pair(&ia, *ei));
		if (PredicateTable::is_static(atom.get_predicate())) {
			atom_values.insert(make_pair(&atom, HeuristicValue::ZERO));
		}
		else {
			atom_values.insert(make_pair(&atom,
				HeuristicValue::ZERO_COST_UNIT_WORK));
		}
	}
	for (TimedActionTable::const_iterator ai = problem.get_timed_actions().begin();
		ai != problem.get_timed_actions().end(); ai++) {
		float time = (*ai).first;
		const GroundAction& action = *(*ai).second;
		for (EffectList::const_iterator ei = action.get_effects().begin();
			ei != action.get_effects().end(); ei++) {
			const Literal& literal = (*ei)->get_literal();
			achievers[&literal].insert(make_pair(&action, *ei));
			float d = (params.action_cost == Parameters::UNIT_COST) ? 1.0f : time;
			map<const Literal*, float>::const_iterator di =
				duration_factor.find(&literal);
			if (di != duration_factor.end()) {
				d /= (*di).second;
			}
			const Atom* atom = dynamic_cast<const Atom*>(&literal);
			if (atom != NULL) {
				if (atom_values.find(atom) == atom_values.end()) {
					atom_values.insert(make_pair(atom,
						HeuristicValue(d, 1, time)));
				}
			}
			else {
				const Negation& negation = dynamic_cast<const Negation&>(literal);
				if (negation_values.find(&negation.get_atom()) == negation_values.end()
					&& heuristic_value(negation.get_atom(), 0).is_zero()) {
					negation_values.insert(make_pair(&negation.get_atom(),
						HeuristicValue(d, 1, time)));
				}
			}
		}
	}

	// Generate the rest of the levels until no change occurs.
	bool changed;
	int level = 0;
	//
	// Keep track of both applicable and useful actions.  When planning
	// with durative actions, it is possible that there are useful
	// actions that are not applicable.  At the end, we make sure to
	// create action domain constraints only for actions that are both
	// applicable and useful.
	//

	GroundActionSet applicable_actions;
	GroundActionSet useful_actions;
	do {
		if (verbosity > 3) {
			// Print literal values at this level.
			cerr << "Literal values at level " << level << ":" << endl;
			for (AtomValueMap::const_iterator vi = atom_values.begin();
				vi != atom_values.end(); vi++) {
				cerr << "  ";
				(*vi).first->print(cerr, 0, Bindings::EMPTY);
				cerr << " -- " << (*vi).second << endl;
			}
			for (AtomValueMap::const_iterator vi = negation_values.begin();
				vi != negation_values.end(); vi++) {
				cerr << "  (not ";
				(*vi).first->print(cerr, 0, Bindings::EMPTY);
				cerr << ") -- " << (*vi).second << endl;
			}
		}
		level++;
		changed = false;

		// Find applicable actions at current level and add effects to the next level.
		AtomValueMap new_atom_values;
		AtomValueMap new_negation_values;
		for (GroundActionList::const_iterator ai = actions.begin();
			ai != actions.end(); ai++) {
			const GroundAction& action = **ai;
			HeuristicValue pre_value;
			HeuristicValue start_value;
			action.get_condition().get_heuristic_value(pre_value, start_value, *this, 0);
			if (!start_value.is_infinite()) {
				// Precondition is achievable at this level.
				if (!pre_value.is_infinite()
					&& applicable_actions.find(&action) == applicable_actions.end()) {
					// First time this action is applicable.
					applicable_actions.insert(&action);
				}
				for (EffectList::const_iterator ei = action.get_effects().begin();
					ei != action.get_effects().end(); ei++) {
					const Effect& effect = **ei;
					if (effect.get_when() == EffectTime::AT_END && pre_value.is_infinite()) {
						continue;
					}
					HeuristicValue cond_value, cond_value_start;
					effect.get_condition().get_heuristic_value(cond_value, cond_value_start,
						*this, 0);
					if (!cond_value.is_infinite()
						&& !effect.get_link_condition().is_contradiction()) {
						// Effect condition is achievable at this level.
						if (effect.get_when() == EffectTime::AT_START) {
							cond_value += start_value;
						}
						else {
							cond_value += pre_value;
						}
						const Value* min_v =
							dynamic_cast<const Value*>(&action.get_min_duration());
						if (min_v == NULL) {
							throw runtime_error("non-constant minimum duration");
						}
						cond_value.increase_makespan(Orderings::threshold
							+ min_v->get_value());

						// Update heuristic values of literal added by effect.
						const Literal& literal = effect.get_literal();
						float d = ((params.action_cost == Parameters::UNIT_COST)
							? 1.0f : Orderings::threshold + min_v->get_value());
						map<const Literal*, float>::const_iterator di =
							duration_factor.find(&literal);
						if (di != duration_factor.end()) {
							d /= (*di).second;
						}
						cond_value.increase_cost(d);
						if (!find(achievers, literal, action, effect)) {
							if (!pre_value.is_infinite()) {
								achievers[&literal].insert(make_pair(&action, &effect));
							}
							if (useful_actions.find(&action) == useful_actions.end()) {
								useful_actions.insert(&action);
							}
							if (verbosity > 4) {
								cerr << "  ";
								action.print(cerr, 0, Bindings::EMPTY);
								cerr << " achieves ";
								literal.print(cerr, 0, Bindings::EMPTY);
								cerr << " with ";
								effect.print(cerr);
								cerr << ' ' << cond_value << endl;
							}
						}
						const Atom* atom = dynamic_cast<const Atom*>(&literal);
						if (atom != NULL) {
							AtomValueMap::const_iterator vi = new_atom_values.find(atom);
							if (vi == new_atom_values.end()) {
								vi = atom_values.find(atom);
								if (vi == atom_values.end()) {
									// First level this atom is achieved.
									HeuristicValue new_value = cond_value;
									new_value.increment_work();
									new_atom_values.insert(make_pair(atom, new_value));
									changed = true;
									continue;
								}
							}
							// This atom has been achieved earlier.
							HeuristicValue old_value = (*vi).second;
							HeuristicValue new_value = cond_value;
							new_value.increment_work();
							new_value = min(new_value, old_value);
							if (new_value != old_value) {
								new_atom_values[atom] = new_value;
								changed = true;
							}
						}
						else {
							const Negation& negation =
								dynamic_cast<const Negation&>(literal);
							AtomValueMap::const_iterator vi =
								new_negation_values.find(&negation.get_atom());
							if (vi == new_negation_values.end()) {
								vi = negation_values.find(&negation.get_atom());
								if (vi == negation_values.end()) {
									if (heuristic_value(negation.get_atom(), 0).is_zero()) {
										// First level this negated atom is achieved.
										HeuristicValue new_value = cond_value;
										new_value.increment_work();
										new_negation_values.insert(make_pair(&negation.get_atom(),
											new_value));
										changed = true;
										continue;
									}
									else {
										// Closed world assumption.
										continue;
									}
								}
							}
							// This negated atom has been achieved earlier.
							HeuristicValue old_value = (*vi).second;
							HeuristicValue new_value = cond_value;
							new_value.increment_work();
							new_value = min(new_value, old_value);
							if (new_value != old_value) {
								new_negation_values[&negation.get_atom()] = new_value;
								changed = true;
							}
						}
					}
				}
			}
		}

		// Add achieved atoms to previously achieved atoms.
		for (AtomValueMap::const_iterator vi = new_atom_values.begin();
			vi != new_atom_values.end(); vi++) {
			atom_values[(*vi).first] = (*vi).second;
		}
		// Add achieved negated atoms to previously achieved negated atoms.
		for (AtomValueMap::const_iterator vi = new_negation_values.begin();
			vi != new_negation_values.end(); vi++) {
			negation_values[(*vi).first] = (*vi).second;
		}
	} while (changed);

	// Map predicates to achievable ground atoms.
	for (AtomValueMap::const_iterator vi = atom_values.begin();
		vi != atom_values.end(); vi++) {
		const Atom& atom = *(*vi).first;
		predicate_atoms.insert(make_pair(atom.get_predicate(), &atom));
	}

	// Map predicates to achievable negated ground atoms.
	for (AtomValueMap::const_iterator vi = negation_values.begin();
		vi != negation_values.end(); vi++) {
		const Atom& atom = *(*vi).first;
		predicate_negations.insert(make_pair(atom.get_predicate(), &atom));
	}

	// Collect actions that are both applicable and useful.  Create actions domains constraints for these actions, if called for.

	GroundActionSet good_actions;
	if (verbosity > 1 || params.domain_constraints) {
		for (GroundActionSet::const_iterator ai = applicable_actions.begin();
			ai != applicable_actions.end(); ai++) {
			const GroundAction& action = **ai;
			if (useful_actions.find(&action) != useful_actions.end()) {
				good_actions.insert(&action);
				if (params.domain_constraints && !action.get_arguments().empty()) {
					ActionDomainMap::const_iterator di =
						action_domains.find(action.get_name());
					if (di == action_domains.end()) {
						ActionDomain* domain = new ActionDomain(action.get_arguments());
						ActionDomain::register_use(domain);
						action_domains.insert(make_pair(action.get_name(), domain));
					}
					else {
						(*di).second->add(action.get_arguments());
					}
				}
			}
		}
	}

	if (verbosity > 0) {
		cerr << "Applicable actions: " << applicable_actions.size()
			<< endl
			<< "Useful actions: " << useful_actions.size() << endl;
		if (verbosity > 1) {
			cerr << "Good actions: " << good_actions.size() << endl;
		}
	}


	// Delete all actions that are not useful.
	for (GroundActionList::const_iterator ai = actions.begin();
		ai != actions.end(); ai++) {
		if (useful_actions.find(*ai) == useful_actions.end()) {
			delete *ai;
		}
	}

	if (verbosity > 2) {
		// Print good actions.
		for (GroundActionSet::const_iterator ai = good_actions.begin();
			ai != good_actions.end(); ai++) {
			cerr << "  ";
			(*ai)->print(cerr, 0, Bindings::EMPTY);
			cerr << endl;
		}
		// Print literal values.
		cerr << "Achievable literals:" << endl;
		for (AtomValueMap::const_iterator vi = atom_values.begin();
			vi != atom_values.end(); vi++) {
			cerr << "  ";
			(*vi).first->print(cerr, 0, Bindings::EMPTY);
			cerr << " -- " << (*vi).second << endl;
		}
		for (AtomValueMap::const_iterator vi = negation_values.begin();
			vi != negation_values.end(); vi++) {
			cerr << "  (not ";
			(*vi).first->print(cerr, 0, Bindings::EMPTY);
			cerr << ") -- " << (*vi).second << endl;
		}
	}
}


// Destruct this planning graph.
PlanningGraph::~PlanningGraph() {
	for (ActionDomainMap::const_iterator di = action_domains.begin();
		di != action_domains.end(); di++) {
		ActionDomain::unregister_use((*di).second);
	}
	GroundActionSet useful_actions;
	for (LiteralAchieverMap::const_iterator lai = achievers.begin();
		lai != achievers.end(); lai++) {
		for (ActionEffectMap::const_iterator aei = (*lai).second.begin();
			aei != (*lai).second.end(); aei++) {
			if ((*aei).first->get_name().substr(0, 1) != "<") {
				useful_actions.insert(dynamic_cast<const GroundAction*>((*aei).first));
			}
		}
	}
	for (GroundActionSet::const_iterator ai = useful_actions.begin();
		ai != useful_actions.end(); ai++) {
		delete *ai;
	}
}


// Return the heuristic value of a ground atom.
HeuristicValue PlanningGraph::heuristic_value(const Atom& atom, size_t step_id,
	const Bindings* bindings) const {
	if (bindings == NULL) {
		// Assume ground atom.
		AtomValueMap::const_iterator vi = atom_values.find(&atom);
		return ((vi != atom_values.end())
			? (*vi).second : HeuristicValue::INFINITE);
	}
	else {
		// Take minimum value of ground atoms that unify.
		HeuristicValue value = HeuristicValue::INFINITE;
		pair<PredicateAtomsMap::const_iterator,
			PredicateAtomsMap::const_iterator> bounds =
			predicate_atoms.equal_range(atom.get_predicate());
		for (PredicateAtomsMap::const_iterator gi = bounds.first;
			gi != bounds.second; gi++) {
			const Atom& a = *(*gi).second;
			if (bindings->unify(atom, step_id, a, 0)) {
				HeuristicValue v = heuristic_value(a, 0);
				value = min(value, v);
				if (value.is_zero()) {
					return value;
				}
			}
		}
		return value;
	}
}


// Return the heuristic value of a negated atom.
HeuristicValue PlanningGraph::heuristic_value(const Negation& negation,
	size_t step_id,
	const Bindings* bindings) const {
	if (bindings == NULL) {
		// Assume ground negated atom.
		AtomValueMap::const_iterator vi = negation_values.find(&negation.get_atom());
		if (vi != negation_values.end()) {
			return (*vi).second;
		}
		else {
			vi = atom_values.find(&negation.get_atom());
			return ((vi == atom_values.end() || !(*vi).second.is_zero())
				? HeuristicValue::ZERO_COST_UNIT_WORK
				: HeuristicValue::INFINITE);
		}
	}
	else {
		// Take minimum value of ground negated atoms that unify.
		const Atom& atom = negation.get_atom();
		if (!heuristic_value(atom, step_id, bindings).is_zero()) {
			return HeuristicValue::ZERO;
		}
		HeuristicValue value = HeuristicValue::INFINITE;
		pair<PredicateAtomsMap::const_iterator,
			PredicateAtomsMap::const_iterator> bounds =
			predicate_negations.equal_range(negation.get_predicate());
		for (PredicateAtomsMap::const_iterator gi = bounds.first;
			gi != bounds.second; gi++) {
			const Atom& a = *(*gi).second;
			if (bindings->unify(atom, step_id, a, 0)) {
				HeuristicValue v = heuristic_value(a, 0);
				value = min(value, v);
				if (value.is_zero()) {
					return value;
				}
			}
		}
		return value;
	}
}


// Return a set of achievers for the given literal.
const ActionEffectMap*
PlanningGraph::literal_achievers(const Literal& literal) const {
	LiteralAchieverMap::const_iterator lai = achievers.find(&literal);
	return (lai != achievers.end()) ? &(*lai).second : NULL;
}


// Return the parameter domain for the given action, or NULL if the parameter domain is empty.
const ActionDomain*
PlanningGraph::action_domain(const string& name) const {
	ActionDomainMap::const_iterator di = action_domains.find(name);
	return (di != action_domains.end()) ? (*di).second : NULL;
}


// Finds an element in a LiteralActionsMap.
bool PlanningGraph::find(const PlanningGraph::LiteralAchieverMap& m,
	const Literal &l, const Action& a,
	const Effect& e) const {
	LiteralAchieverMap::const_iterator lai = m.find(&l);
	if (lai != m.end()) {
		pair<ActionEffectMap::const_iterator,
			ActionEffectMap::const_iterator> bounds = (*lai).second.equal_range(&a);
		for (ActionEffectMap::const_iterator i = bounds.first;
			i != bounds.second; i++) {
			if ((*i).second == &e) {
				return true;
			}
		}
	}
	return false;
}


//=================== SelectionCriterion ====================

// Output operator for selection criterion.
ostream& operator<<(ostream& os, const SelectionCriterion& c) {
	os << '{';
	bool first = true;
	if (c.non_separable) {
		if (!first) {
			os << ',';
		}
		os << 'n';
		first = false;
	}
	if (c.separable) {
		if (!first) {
			os << ',';
		}
		os << 's';
		first = false;
	}
	if (c.open_cond) {
		if (!first) {
			os << ',';
		}
		os << 'o';
		first = false;
	}
	if (c.local_open_cond) {
		if (!first) {
			os << ',';
		}
		os << 'l';
		first = false;
	}
	if (c.static_open_cond) {
		if (!first) {
			os << ',';
		}
		os << 't';
		first = false;
	}
	if (c.unsafe_open_cond) {
		if (!first) {
			os << ',';
		}
		os << 'u';
		first = false;
	}
	os << '}';
	if (c.max_refinements < numeric_limits<int>::max()) {
		os << c.max_refinements;
	}
	switch (c.order) {
	case SelectionCriterion::LIFO:
		os << "LIFO";
		break;
	case SelectionCriterion::FIFO:
		os << "FIFO";
		break;
	case SelectionCriterion::RANDOM:
		os << "R";
		break;
	case SelectionCriterion::LR:
		os << "LR";
		break;
	case SelectionCriterion::MR:
		os << "MR";
		break;
	case SelectionCriterion::NEW:
		os << "NEW";
		break;
	case SelectionCriterion::REUSE:
		os << "REUSE";
		break;
	case SelectionCriterion::LC:
		os << "LC_";
		switch (c.heuristic) {
		case SelectionCriterion::ADD:
			os << "ADD";
			if (c.reuse) {
				os << 'R';
			}
			break;
		case SelectionCriterion::MAKESPAN:
			os << "MAKESPAN";
			break;
		}
		break;
	case SelectionCriterion::MC:
		os << "MC_";
		switch (c.heuristic) {
		case SelectionCriterion::ADD:
			os << "ADD";
			if (c.reuse) {
				os << 'R';
			}
			break;
		case SelectionCriterion::MAKESPAN:
			os << "MAKESPAN";
			break;
		}
		break;
	case SelectionCriterion::LW:
		os << "LW_";
		switch (c.heuristic) {
		case SelectionCriterion::ADD:
			os << "ADD";
			if (c.reuse) {
				os << 'R';
			}
			break;
		case SelectionCriterion::MAKESPAN:
			os << "MAKESPAN";
			break;
		}
		break;
	case SelectionCriterion::MW:
		os << "MW_";
		switch (c.heuristic) {
		case SelectionCriterion::ADD:
			os << "ADD";
			if (c.reuse) {
				os << 'R';
			}
			break;
		case SelectionCriterion::MAKESPAN:
			os << "MAKESPAN";
			break;
		}
		break;
	}
	return os;
}


//=================== FlawSelectionOrder ====================

// Construct a default flaw selection order.
FlawSelectionOrder::FlawSelectionOrder(const string& name) {
	*this = name;
}


// Select a flaw selection order from a name.
FlawSelectionOrder& FlawSelectionOrder::operator=(const string& name) {
	const char* n = name.c_str();
	if (strcasecmp(n, "UCPOP") == 0) {
		return *this = "{n,s}LIFO/{o}LIFO";
	}
	else if (strcasecmp(n, "UCPOP-LC") == 0) {
		return *this = "{n,s}LIFO/{o}LR";
	}
	else if (strncasecmp(n, "DSep-", 5) == 0) {
		if (strcasecmp(n + 5, "LIFO") == 0) {
			return *this = "{n}LIFO/{o}LIFO/{s}LIFO";
		}
		else if (strcasecmp(n + 5, "FIFO") == 0) {
			return *this = "{n}LIFO/{o}FIFO/{s}LIFO";
		}
		else if (strcasecmp(n + 5, "LC") == 0) {
			return *this = "{n}LIFO/{o}LR/{s}LIFO";
		}
	}
	else if (strncasecmp(n, "DUnf-", 5) == 0) {
		if (strcasecmp(n + 5, "LIFO") == 0) {
			return *this = "{n,s}0LIFO/{n,s}1LIFO/{o}LIFO/{n,s}LIFO";
		}
		else if (strcasecmp(n + 5, "FIFO") == 0) {
			return *this = "{n,s}0LIFO/{n,s}1LIFO/{o}FIFO/{n,s}LIFO";
		}
		else if (strcasecmp(n + 5, "LC") == 0) {
			return *this = "{n,s}0LIFO/{n,s}1LIFO/{o}LR/{n,s}LIFO";
		}
		else if (strcasecmp(n + 5, "Gen") == 0) {
			return *this = "{n,s,o}0LIFO/{n,s,o}1LIFO/{n,s,o}LIFO";
		}
	}
	else if (strncasecmp(n, "DRes-", 5) == 0) {
		if (strcasecmp(n + 5, "LIFO") == 0) {
			return *this = "{n,s}0LIFO/{o}LIFO/{n,s}LIFO";
		}
		else if (strcasecmp(n + 5, "FIFO") == 0) {
			return *this = "{n,s}0LIFO/{o}FIFO/{n,s}LIFO";
		}
		else if (strcasecmp(n + 5, "LC") == 0) {
			return *this = "{n,s}0LIFO/{o}LR/{n,s}LIFO";
		}
	}
	else if (strncasecmp(n, "DEnd-", 5) == 0) {
		if (strcasecmp(n + 5, "LIFO") == 0) {
			return *this = "{o}LIFO/{n,s}LIFO";
		}
		else if (strcasecmp(n + 5, "FIFO") == 0) {
			return *this = "{o}FIFO/{n,s}LIFO";
		}
		else if (strcasecmp(n + 5, "LC") == 0) {
			return *this = "{o}LR/{n,s}LIFO";
		}
	}
	else if (strcasecmp(n, "LCFR") == 0) {
		return *this = "{n,s,o}LR";
	}
	else if (strcasecmp(n, "LCFR-DSep") == 0) {
		return *this = "{n,o}LR/{s}LR";
	}
	else if (strcasecmp(n, "ZLIFO") == 0) {
		return *this = "{n}LIFO/{o}0LIFO/{o}1NEW/{o}LIFO/{s}LIFO";
	}
	else if (strcasecmp(n, "ZLIFO*") == 0) {
		return *this = "{o}0LIFO/{n,s}LIFO/{o}1NEW/{o}LIFO";
	}
	else if (strcasecmp(n, "Static") == 0) {
		return *this = "{t}LIFO/{n,s}LIFO/{o}LIFO";
	}
	else if (strcasecmp(n, "LCFR-Loc") == 0) {
		return *this = "{n,s,l}LR";
	}
	else if (strcasecmp(n, "LCFR-Conf") == 0) {
		return *this = "{n,s,u}LR/{o}LR";
	}
	else if (strcasecmp(n, "LCFR-Loc-Conf") == 0) {
		return *this = "{n,s,u}LR/{l}LR";
	}
	else if (strcasecmp(n, "MC") == 0) {
		return *this = "{n,s}LR/{o}MC_add";
	}
	else if (strcasecmp(n, "MC-Loc") == 0) {
		return *this = "{n,s}LR/{l}MC_add";
	}
	else if (strcasecmp(n, "MC-Loc-Conf") == 0) {
		return *this = "{n,s}LR/[u}MC_add/{l}MC_add";
	}
	else if (strcasecmp(n, "MW") == 0) {
		return *this = "{n,s}LR/{o}MW_add";
	}
	else if (strcasecmp(n, "MW-Loc") == 0) {
		return *this = "{n,s}LR/{l}MW_add";
	}
	else if (strcasecmp(n, "MW-Loc-Conf") == 0) {
		return *this = "{n,s}LR/{u}MW_add/{l}MW_add";
	}
	selection_criteria.clear();
	needs_pg = false;
	first_unsafe_criterion = numeric_limits<int>::max();
	last_unsafe_criterion = 0;
	first_open_cond_criterion = numeric_limits<int>::max();
	last_open_cond_criterion = 0;
	int non_separable_max_refinements = -1;
	int separable_max_refinements = -1;
	int open_cond_max_refinements = -1;
	size_t pos = 0;
	while (pos < name.length()) {
		if (name[pos] != '{') {
			throw InvalidFlawSelectionOrder(name);
		}
		pos++;
		SelectionCriterion criterion;
		criterion.non_separable = false;
		criterion.separable = false;
		criterion.open_cond = false;
		criterion.local_open_cond = false;
		criterion.static_open_cond = false;
		criterion.unsafe_open_cond = false;
		do {
			switch (name[pos]) {
			case 'n':
				pos++;
				if (name[pos] == ',' || name[pos] == '}') {
					criterion.non_separable = true;
					if (first_unsafe_criterion > last_unsafe_criterion) {
						first_unsafe_criterion = selection_criteria.size();
					}
					last_unsafe_criterion = selection_criteria.size();
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
				break;
			case 's':
				pos++;
				if (name[pos] == ',' || name[pos] == '}') {
					criterion.separable = true;
					if (first_unsafe_criterion > last_unsafe_criterion) {
						first_unsafe_criterion = selection_criteria.size();
					}
					last_unsafe_criterion = selection_criteria.size();
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
				break;
			case 'o':
				pos++;
				if (name[pos] == ',' || name[pos] == '}') {
					criterion.open_cond = true;
					criterion.local_open_cond = false;
					criterion.static_open_cond = false;
					criterion.unsafe_open_cond = false;
					if (first_open_cond_criterion > last_open_cond_criterion) {
						first_open_cond_criterion = selection_criteria.size();
					}
					last_open_cond_criterion = selection_criteria.size();
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
				break;
			case 'l':
				pos++;
				if (name[pos] == ',' || name[pos] == '}') {
					if (!criterion.open_cond) {
						criterion.local_open_cond = true;
						if (first_open_cond_criterion > last_open_cond_criterion) {
							first_open_cond_criterion = selection_criteria.size();
						}
						last_open_cond_criterion = selection_criteria.size();
					}
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
				break;
			case 't':
				pos++;
				if (name[pos] == ',' || name[pos] == '}') {
					if (!criterion.open_cond) {
						criterion.static_open_cond = true;
						if (first_open_cond_criterion > last_open_cond_criterion) {
							first_open_cond_criterion = selection_criteria.size();
						}
						last_open_cond_criterion = selection_criteria.size();
					}
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
				break;
			case 'u':
				pos++;
				if (name[pos] == ',' || name[pos] == '}') {
					if (!criterion.open_cond) {
						criterion.unsafe_open_cond = true;
						if (first_open_cond_criterion > last_open_cond_criterion) {
							first_open_cond_criterion = selection_criteria.size();
						}
						last_open_cond_criterion = selection_criteria.size();
					}
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
				break;
			default:
				throw InvalidFlawSelectionOrder(name);
			}
			if (name[pos] == ',') {
				pos++;
				if (name[pos] == '}') {
					throw InvalidFlawSelectionOrder(name);
				}
			}
		} while (name[pos] != '}');
		pos++;
		size_t next_pos = pos;
		while (name[next_pos] >= '0' && name[next_pos] <= '9') {
			next_pos++;
		}
		if (next_pos > pos) {
			string number = name.substr(pos, next_pos - pos);
			criterion.max_refinements = atoi(number.c_str());
			pos = next_pos;
		}
		else {
			criterion.max_refinements = numeric_limits<int>::max();
		}
		next_pos = name.find('/', pos);
		string key = name.substr(pos, next_pos - pos);
		n = key.c_str();
		if (strcasecmp(n, "LIFO") == 0) {
			criterion.order = SelectionCriterion::LIFO;
		}
		else if (strcasecmp(n, "FIFO") == 0) {
			criterion.order = SelectionCriterion::FIFO;
		}
		else if (strcasecmp(n, "R") == 0) {
			criterion.order = SelectionCriterion::RANDOM;
		}
		else if (strcasecmp(n, "LR") == 0) {
			criterion.order = SelectionCriterion::LR;
		}
		else if (strcasecmp(n, "MR") == 0) {
			criterion.order = SelectionCriterion::MR;
		}
		else {
			if (criterion.non_separable || criterion.separable) {
				// No other orders that the above can be used with threats.
				throw InvalidFlawSelectionOrder(name);
			}
			if (strcasecmp(n, "NEW") == 0) {
				criterion.order = SelectionCriterion::NEW;
			}
			else if (strcasecmp(n, "REUSE") == 0) {
				criterion.order = SelectionCriterion::REUSE;
			}
			else if (strncasecmp(n, "LC_", 3) == 0) {
				criterion.order = SelectionCriterion::LC;
				needs_pg = true;
				if (strcasecmp(n + 3, "ADD") == 0) {
					criterion.heuristic = SelectionCriterion::ADD;
					criterion.reuse = false;
				}
				else if (strcasecmp(n + 3, "ADDR") == 0) {
					criterion.heuristic = SelectionCriterion::ADD;
					criterion.reuse = true;
				}
				else if (strcasecmp(n + 3, "MAKESPAN") == 0) {
					criterion.heuristic = SelectionCriterion::MAKESPAN;
					criterion.reuse = false;
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
			}
			else if (strncasecmp(n, "MC_", 3) == 0) {
				criterion.order = SelectionCriterion::MC;
				needs_pg = true;
				if (strcasecmp(n + 3, "ADD") == 0) {
					criterion.heuristic = SelectionCriterion::ADD;
					criterion.reuse = false;
				}
				else if (strcasecmp(n + 3, "ADDR") == 0) {
					criterion.heuristic = SelectionCriterion::ADD;
					criterion.reuse = true;
				}
				else if (strcasecmp(n + 3, "MAKESPAN") == 0) {
					criterion.heuristic = SelectionCriterion::MAKESPAN;
					criterion.reuse = false;
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
			}
			else if (strncasecmp(n, "LW_", 3) == 0) {
				criterion.order = SelectionCriterion::LW;
				needs_pg = true;
				if (strcasecmp(n + 3, "ADD") == 0) {
					criterion.heuristic = SelectionCriterion::ADD;
					criterion.reuse = false;
				}
				else if (strcasecmp(n + 3, "ADDR") == 0) {
					criterion.heuristic = SelectionCriterion::ADD;
					criterion.reuse = true;
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
			}
			else if (strncasecmp(n, "MW_", 3) == 0) {
				criterion.order = SelectionCriterion::MW;
				needs_pg = true;
				if (strcasecmp(n + 3, "ADD") == 0) {
					criterion.heuristic = SelectionCriterion::ADD;
					criterion.reuse = false;
				}
				else if (strcasecmp(n + 3, "ADDR") == 0) {
					criterion.heuristic = SelectionCriterion::ADD;
					criterion.reuse = true;
				}
				else {
					throw InvalidFlawSelectionOrder(name);
				}
			}
			else {
				throw InvalidFlawSelectionOrder(name);
			}
		}
		if (criterion.non_separable) {
			non_separable_max_refinements = max(criterion.max_refinements,
				non_separable_max_refinements);
		}
		if (criterion.separable) {
			separable_max_refinements = max(criterion.max_refinements,
				separable_max_refinements);
		}
		if (criterion.open_cond || criterion.local_open_cond) {
			open_cond_max_refinements = max(criterion.max_refinements,
				open_cond_max_refinements);
		}
		selection_criteria.push_back(criterion);
		pos = next_pos;
		if (name[pos] == '/') {
			pos++;
			if (pos >= name.length()) {
				throw InvalidFlawSelectionOrder(name);
			}
		}
	}
	if (non_separable_max_refinements < numeric_limits<int>::max()
		|| separable_max_refinements < numeric_limits<int>::max()
		|| open_cond_max_refinements < numeric_limits<int>::max()) {
		// Incomplete flaw selection order.
		throw InvalidFlawSelectionOrder(name);
	}
	return *this;
}


// Check if this flaw order needs a planning graph.
bool FlawSelectionOrder::needs_planning_graph() const {
	return needs_pg;
}


// Search threats for a flaw to select.
int FlawSelectionOrder::select_unsafe(FlawSelection& selection,
	const Plan& plan, const Problem& problem,
	int first_criterion,
	int last_criterion) const {
	if (first_criterion > last_criterion || plan.get_unsafes() == NULL) {
		return numeric_limits<int>::max();
	}
	// Loop through usafes.
	for (const Chain<Unsafe>* uc = plan.get_unsafes();
		uc != NULL && first_criterion <= last_criterion; uc = uc->tail) {
		const Unsafe& unsafe = uc->head;
		if (verbosity > 1) {
			cerr << "(considering ";
			unsafe.print(cerr, Bindings::EMPTY);
			cerr << ")" << endl;
		}
		int refinements = -1;
		int separable = -1;
		int promotable = -1;
		int demotable = -1;
		// Loop through selection criteria that are within limits.
		for (int c = first_criterion; c <= last_criterion; c++) {
			const SelectionCriterion& criterion = selection_criteria[c];
			// If criterion applies only to one type of threats, make sure we know which type of threat this is.
			if (criterion.non_separable != criterion.separable && separable < 0) {
				separable = plan.is_separable(unsafe);
				if (separable < 0) {
					refinements = separable = 0;
				}
			}
			// Test if criterion applies.
			if ((criterion.non_separable && criterion.separable)
				|| (criterion.separable && separable > 0)
				|| (criterion.non_separable && separable == 0)) {
				// Right type of threat, so now check if the refinement constraint is satisfied.
				if (criterion.max_refinements >= 3
					|| plan.unsafe_refinements(refinements, separable, promotable,
						demotable, unsafe,
						criterion.max_refinements)) {
					// Refinement constraint is satisfied, so criterion applies.
					switch (criterion.order) {
					case SelectionCriterion::LIFO:
						selection.flaw = &unsafe;
						selection.criterion = c;
						last_criterion = c - 1;
						if (verbosity > 1) {
							cerr << "selecting ";
							unsafe.print(cerr, Bindings::EMPTY);
							cerr << " by criterion " << criterion << endl;
						}
						break;
					case SelectionCriterion::FIFO:
						selection.flaw = &unsafe;
						selection.criterion = c;
						last_criterion = c;
						if (verbosity > 1) {
							cerr << "selecting ";
							unsafe.print(cerr, Bindings::EMPTY);
							cerr << " by criterion " << criterion << endl;
						}
						break;
					case SelectionCriterion::RANDOM:
						if (c == selection.criterion) {
							selection.streak++;
						}
						else {
							selection.streak = 1;
						}
						if (rand01ex() < 1.0 / selection.streak) {
							selection.flaw = &unsafe;
							selection.criterion = c;
							last_criterion = c;
							if (verbosity > 1) {
								cerr << "selecting ";
								unsafe.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion << endl;
							}
						}
						break;
					case SelectionCriterion::LR:
						if (c < selection.criterion
							|| plan.unsafe_refinements(refinements, separable, promotable,
								demotable, unsafe,
								int(selection.rank + 0.5) - 1)) {
							selection.flaw = &unsafe;
							selection.criterion = c;
							plan.unsafe_refinements(refinements, separable, promotable,
								demotable, unsafe,
								numeric_limits<int>::max());
							selection.rank = refinements;
							last_criterion = (refinements == 0) ? c - 1 : c;
							if (verbosity > 1) {
								cerr << "selecting ";
								unsafe.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion
									<< " with rank " << refinements << endl;
							}
						}
						break;
					case SelectionCriterion::MR:
						plan.unsafe_refinements(refinements, separable, promotable,
							demotable, unsafe,
							numeric_limits<int>::max());
						if (c < selection.criterion
							|| refinements > selection.rank) {
							selection.flaw = &unsafe;
							selection.criterion = c;
							selection.rank = refinements;
							last_criterion = (refinements == 3) ? c - 1 : c;
							if (verbosity > 1) {
								cerr << "selecting ";
								unsafe.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion
									<< " with rank " << refinements << endl;
							}
						}
						break;
					default:
						// No other ordering criteria apply to threats.
						break;
					}
				}
			}
		}
	}
	return last_criterion;
}


// Search open conditions for a flaw to select.
int FlawSelectionOrder::select_open_cond(FlawSelection& selection,
	const Plan& plan, const Problem& problem, const PlanningGraph* pg,
	int first_criterion, int last_criterion) const {
	if (first_criterion > last_criterion || plan.get_open_conds() == NULL) {
		return numeric_limits<int>::max();
	}
	size_t local_id = 0;
	// Loop through open conditions.
	for (const Chain<OpenCondition>* occ = plan.get_open_conds();
		occ != NULL && first_criterion <= last_criterion; occ = occ->tail) {
		const OpenCondition& open_cond = occ->head;
		if (verbosity > 1) {
			cerr << "(considering ";
			open_cond.print(cerr, Bindings::EMPTY);
			cerr << ")" << endl;
		}
		if (local_id == 0) {
			local_id = open_cond.get_step_id();
		}
		bool local = (open_cond.get_step_id() == local_id);
		int is_static = -1;
		int is_unsafe = -1;
		int refinements = -1;
		int addable = -1;
		int reusable = -1;
		// Loop through selection criteria that are within limits.
		for (int c = first_criterion; c <= last_criterion; c++) {
			const SelectionCriterion& criterion = selection_criteria[c];
			if (criterion.local_open_cond && !local
				&& !criterion.static_open_cond && !criterion.unsafe_open_cond) {
				if (c == last_criterion) {
					last_criterion--;
				}
				continue;
			}
			// If criterion applies only to one type of open condition, make sure we know which type of open condition this is.
			if (criterion.static_open_cond && is_static < 0) {
				is_static = open_cond.is_static() ? 1 : 0;
			}
			if (criterion.unsafe_open_cond && is_unsafe < 0) {
				is_unsafe = (plan.is_unsafe_open_condition(open_cond)) ? 1 : 0;
			}
			// Test if criterion applies.
			if (criterion.open_cond
				|| (criterion.local_open_cond && local)
				|| (criterion.static_open_cond && is_static > 0)
				|| (criterion.unsafe_open_cond && is_unsafe > 0)) {
				// Right type of open condition, so now check if the refinement constraint is satisfied.
				if (criterion.max_refinements == numeric_limits<int>::max()
					|| plan.open_cond_refinements(refinements, addable, reusable,
						open_cond,
						criterion.max_refinements)) {
					// Refinement constraint is satisfied, so criterion applies.
					switch (criterion.order) {
					case SelectionCriterion::LIFO:
						selection.flaw = &open_cond;
						selection.criterion = c;
						last_criterion = c - 1;
						if (verbosity > 1) {
							cerr << "selecting ";
							open_cond.print(cerr, Bindings::EMPTY);
							cerr << " by criterion " << criterion << endl;
						}
						break;
					case SelectionCriterion::FIFO:
						selection.flaw = &open_cond;
						selection.criterion = c;
						last_criterion = c;
						if (verbosity > 1) {
							cerr << "selecting ";
							open_cond.print(cerr, Bindings::EMPTY);
							cerr << " by criterion " << criterion << endl;
						}
						break;
					case SelectionCriterion::RANDOM:
						if (c == selection.criterion) {
							selection.streak++;
						}
						else {
							selection.streak = 1;
						}
						if (rand01ex() < 1.0 / selection.streak) {
							selection.flaw = &open_cond;
							selection.criterion = c;
							last_criterion = c;
							if (verbosity > 1) {
								cerr << "selecting ";
								open_cond.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion << endl;
							}
						}
						break;
					case SelectionCriterion::LR:
						if (c < selection.criterion
							|| plan.open_cond_refinements(refinements, addable, reusable,
								open_cond,
								int(selection.rank + 0.5) - 1)) {
							selection.flaw = &open_cond;
							selection.criterion = c;
							plan.open_cond_refinements(refinements, addable, reusable,
								open_cond,
								numeric_limits<int>::max());
							selection.rank = refinements;
							last_criterion = (refinements == 0) ? c - 1 : c;
							if (verbosity > 1) {
								cerr << "selecting ";
								open_cond.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion
									<< " with rank " << refinements << endl;
							}
						}
						break;
					case SelectionCriterion::MR:
						plan.open_cond_refinements(refinements, addable, reusable,
							open_cond,
							numeric_limits<int>::max());
						if (c < selection.criterion
							|| refinements > selection.rank) {
							selection.flaw = &open_cond;
							selection.criterion = c;
							selection.rank = refinements;
							last_criterion = c;
							if (verbosity > 1) {
								cerr << "selecting ";
								open_cond.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion
									<< " with rank " << refinements << endl;
							}
						}
						break;
					case SelectionCriterion::NEW:
					{
						bool has_new = false;
						if (addable < 0) {
							const Literal* literal = open_cond.literal();
							if (literal != NULL) {
								has_new = !plan.addable_steps(addable, *literal,
									open_cond, 0);
							}
						}
						else {
							has_new = (addable > 0);
						}
						if (has_new || c < selection.criterion) {
							selection.flaw = &open_cond;
							selection.criterion = c;
							last_criterion = has_new ? c - 1 : c;
							if (verbosity > 1) {
								cerr << "selecting ";
								open_cond.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion;
								if (has_new) {
									cerr << " with new";
								}
								cerr << endl;
							}
						}
					}
					break;
					case SelectionCriterion::REUSE:
					{
						bool has_reuse = false;
						if (reusable < 0) {
							const Literal* literal = open_cond.literal();
							if (literal != NULL) {
								has_reuse = !plan.reusable_steps(reusable, *literal,
									open_cond, 0);
							}
						}
						else {
							has_reuse = (reusable > 0);
						}
						if (has_reuse || c < selection.criterion) {
							selection.flaw = &open_cond;
							selection.criterion = c;
							last_criterion = has_reuse ? c - 1 : c;
							if (verbosity > 1) {
								cerr << "selecting ";
								open_cond.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion;
								if (has_reuse) {
									cerr << " with reuse";
								}
								cerr << endl;
							}
						}
					}
					break;
					case SelectionCriterion::LC:
					{
						HeuristicValue h, hs;
						formula_value(h, hs, open_cond.get_condition(), open_cond.get_step_id(),
							plan, *pg, criterion.reuse);
						float rank = ((criterion.heuristic == SelectionCriterion::ADD)
							? h.get_add_cost() : h.get_makespan());
						if (c < selection.criterion || rank < selection.rank) {
							selection.flaw = &open_cond;
							selection.criterion = c;
							selection.rank = rank;
							last_criterion = (rank == 0.0f) ? c - 1 : c;
							if (verbosity > 1) {
								cerr << "selecting ";
								open_cond.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion
									<< " with rank " << rank << endl;
							}
						}
					}
					break;
					case SelectionCriterion::MC:
					{
						HeuristicValue h, hs;
						formula_value(h, hs, open_cond.get_condition(), open_cond.get_step_id(),
							plan, *pg, criterion.reuse);
						float rank = ((criterion.heuristic == SelectionCriterion::ADD)
							? h.get_add_cost() : h.get_makespan() + 0.5);
						if (c < selection.criterion || rank > selection.rank) {
							selection.flaw = &open_cond;
							selection.criterion = c;
							selection.rank = rank;
							last_criterion = c;
							if (verbosity > 1) {
								cerr << "selecting ";
								open_cond.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion
									<< " with rank " << rank << endl;
							}
						}
					}
					break;
					case SelectionCriterion::LW:
					{
						HeuristicValue h, hs;
						formula_value(h, hs, open_cond.get_condition(), open_cond.get_step_id(),
							plan, *pg, criterion.reuse);
						int rank = h.get_add_work();
						if (c < selection.criterion || rank < selection.rank) {
							selection.flaw = &open_cond;
							selection.criterion = c;
							selection.rank = rank;
							last_criterion = (rank == 0) ? c - 1 : c;
							if (verbosity > 1) {
								cerr << "selecting ";
								open_cond.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion
									<< " with rank " << rank << endl;
							}
						}
					}
					break;
					case SelectionCriterion::MW:
					{
						HeuristicValue h, hs;
						formula_value(h, hs, open_cond.get_condition(), open_cond.get_step_id(),
							plan, *pg, criterion.reuse);
						int rank = h.get_add_work();
						if (c < selection.criterion || rank > selection.rank) {
							selection.flaw = &open_cond;
							selection.criterion = c;
							selection.rank = rank;
							last_criterion = c;
							if (verbosity > 1) {
								cerr << "selecting ";
								open_cond.print(cerr, Bindings::EMPTY);
								cerr << " by criterion " << criterion
									<< " with rank " << rank << endl;
							}
						}
					}
					break;
					}
				}
			}
		}
	}
	return last_criterion;
}


// Select a flaw from the flaws of the given plan.
const Flaw& FlawSelectionOrder::select(const Plan& plan,
	const Problem& problem,
	const PlanningGraph* pg) const {
	FlawSelection selection;
	selection.flaw = NULL;
	selection.criterion = numeric_limits<int>::max();
	int last_criterion = select_unsafe(selection, plan, problem,
		first_unsafe_criterion,
		last_unsafe_criterion);
	select_open_cond(selection, plan, problem, pg, first_open_cond_criterion,
		min(last_open_cond_criterion, last_criterion));
	if (selection.flaw != NULL) {
		return *selection.flaw;
	}
	else {
		return plan.get_mutex_threats()->head;
	}
}
