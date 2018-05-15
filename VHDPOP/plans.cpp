#include "plans.h"
#include "heuristics.h"
#include "bindings.h"
#include "problems.h"
#include "domains.h"
#include "formulas.h"
#include "requirements.h"
#include "parameters.h"
#include <algorithm>
#include <limits>
#include <queue>
#include <typeinfo>

extern int verbosity;

//=================== PlanQueue ====================

// A plan queue.
class PlanQueue : public priority_queue<const Plan*> {
};

//=================== PredicateAchieverMap ====================

// A mapping of predicate names to achievers.
class PredicateAchieverMap :public map<Predicate, ActionEffectMap> {
};


//Planning parameters. 
static const Parameters* params;
//Domain of problem currently being solved. 
static const Domain* domain = NULL;
//Problem currently being solved. 
static const Problem* problem = NULL;
//Planning graph. 
static const PlanningGraph* planning_graph;
//The goal action. 
static Action* goal_action;
//Maps predicates to actions. 
static PredicateAchieverMap achieves_pred;
//Maps negated predicates to actions. 
static PredicateAchieverMap achieves_neg_pred;
//Whether last flaw was a static predicate. 
static bool static_pred_flaw;


//=================== Plan ====================

// Less-than specialization for plan pointers.
namespace std {
	template<>
	struct less<const Plan*>
		: public binary_function<const Plan*, const Plan*, bool> {
		// Comparison function operator.
		bool operator()(const Plan* p1, const Plan* p2) const {
			return *p1 < *p2;
		}
	};
}

// Id of goal step.
const size_t Plan::GOAL_ID = numeric_limits<size_t>::max();

// Construct a plan.
Plan::Plan(const Chain<Step>* steps, size_t num_steps,
	const Chain<Link>* links, size_t num_links,
	const Orderings& orderings, const Bindings& bindings,
	const Chain<Unsafe>* unsafes, size_t num_unsafes,
	const Chain<OpenCondition>* open_conds, size_t num_open_conds,
	const Chain<MutexThreat>* mutex_threats, const Plan* parent)
	:steps(steps), num_steps(num_steps),
	links(links), num_links(num_links),
	orderings(&orderings), bindings(&bindings),
	unsafes(unsafes), num_unsafes(num_unsafes),
	open_conds(open_conds), num_open_conds(num_open_conds),
	mutex_threats(mutex_threats) {
	RCObject::ref(steps);
	RCObject::ref(links);
	Orderings::register_use(&orderings);
	Bindings::register_use(&bindings);
	RCObject::ref(unsafes);
	RCObject::ref(open_conds);
	RCObject::ref(mutex_threats);
#ifdef DEBUG_MEMORY
	created_plans++;
#endif //DEBUG_MEMORY
#ifdef DEBUG //???
	depth_ = (parent != NULL) ? parent->depth() + 1 : 0;
#endif
}

// Adds goal to chain of open conditions, and returns true if and only if the goal is consistent.
static bool add_goal(const Chain<OpenCondition>*& open_conds,
	size_t& num_open_conds, BindingList& new_bindings,
	const Formula& goal, size_t step_id,
	bool test_only = false) {
	if (goal.is_tautology()) {
		return true;
	}
	else if (goal.is_contradiction()) {
		return false;
	}
	vector<const Formula*> goals(1, &goal);
	while (!goals.empty()) {
		const Formula* goal = goals.back();
		goals.pop_back();
		const Literal* l;
		FormulaTime when;
		const TimedLiteral* tl = dynamic_cast<const TimedLiteral*>(goal);
		if (tl != NULL) {
			l = &tl->get_literal();
			when = tl->get_when();
		}
		else {
			l = dynamic_cast<const Literal*>(goal);
			when = AT_START_F;
		}
		if (l != NULL) {
			if (!test_only
				&& !(params->strip_static_preconditions()
					&& PredicateTable::is_static(l->get_predicate()))) {
				open_conds =
					new Chain<OpenCondition>(OpenCondition(step_id, *l, when),
						open_conds);
			}
			num_open_conds++;
		}
		else {
			const Conjunction* conj = dynamic_cast<const Conjunction*>(goal);
			if (conj != NULL) {
				const FormulaList& gs = conj->get_conjuncts();
				for (FormulaList::const_iterator fi = gs.begin();
					fi != gs.end(); fi++) {
					if (params->random_open_conditions) {
						size_t pos = size_t((goals.size() + 1.0)*rand() / (RAND_MAX + 1.0));
						if (pos == goals.size()) {
							goals.push_back(*fi);
						}
						else {
							const Formula* tmp = goals[pos];
							goals[pos] = *fi;
							goals.push_back(tmp);
						}
					}
					else {
						goals.push_back(*fi);
					}
				}
			}
			else {
				const Disjunction* disj = dynamic_cast<const Disjunction*>(goal);
				if (disj != NULL) {
					if (!test_only) {
						open_conds =
							new Chain<OpenCondition>(OpenCondition(step_id, *disj),
								open_conds);
					}
					num_open_conds++;
				}
				else {
					const BindingLiteral* bl = dynamic_cast<const BindingLiteral*>(goal);
					if (bl != NULL) {
						bool is_eq = (typeid(*bl) == typeid(Equality));
						new_bindings.push_back(Binding(bl->get_variable(),
							bl->step_id1(step_id),
							bl->get_term(),
							bl->step_id2(step_id), is_eq));
#ifdef BRANCH_ON_INEQUALITY //???
						const Inequality* neq = dynamic_cast<const Inequality*>(bl);
						if (params->domain_constraints
							&& neq != NULL && bl->get_term().is_variable()) {
							// Both terms are variables, so handle specially.
							if (!test_only) {
								open_conds =
									new Chain<OpenCondition>(OpenCondition(step_id, *neq),
										open_conds);
							}
							num_open_conds++;
							new_bindings.pop_back();
						}
#endif
					}
					else {
						const Exists* exists = dynamic_cast<const Exists*>(goal);
						if (exists != NULL) {
							if (params->random_open_conditions) {
								size_t pos =
									size_t((goals.size() + 1.0)*rand() / (RAND_MAX + 1.0));
								if (pos == goals.size()) {
									goals.push_back(&exists->get_body());
								}
								else {
									const Formula* tmp = goals[pos];
									goals[pos] = &exists->get_body();
									goals.push_back(tmp);
								}
							}
							else {
								goals.push_back(&exists->get_body());
							}
						}
						else {
							const Forall* forall = dynamic_cast<const Forall*>(goal);
							if (forall != NULL) {
								const Formula& g = forall->get_universal_base(SubstitutionMap(),
									*problem);
								if (params->random_open_conditions) {
									size_t pos =
										size_t((goals.size() + 1.0)*rand() / (RAND_MAX + 1.0));
									if (pos == goals.size()) {
										goals.push_back(&g);
									}
									else {
										const Formula* tmp = goals[pos];
										goals[pos] = &g;
										goals.push_back(tmp);
									}
								}
								else {
									goals.push_back(&g);
								}
							}
							else {
								throw logic_error("unknown kind of goal");
							}
						}
					}
				}
			}
		}
	}
	return true;
}

// Return a set of achievers for the given literal.
static const ActionEffectMap* literal_achievers(const Literal& literal) {
	if (params->ground_actions) {
		return planning_graph->literal_achievers(literal);
	}
	else if (typeid(literal) == typeid(Atom)) {
		PredicateAchieverMap::const_iterator pai =
			achieves_pred.find(literal.get_predicate());
		return (pai != achieves_pred.end()) ? &(*pai).second : NULL;
	}
	else {
		PredicateAchieverMap::const_iterator pai =
			achieves_neg_pred.find(literal.get_predicate());
		return (pai != achieves_neg_pred.end()) ? &(*pai).second : NULL;
	}
}

// Find threats to the given link.
static void link_threats(const Chain<Unsafe>*& unsafes, size_t& num_unsafes,
	const Link& link, const Chain<Step>* steps,
	const Orderings& orderings,
	const Bindings& bindings) {
	StepTime lt1 = link.get_effect_time();
	StepTime lt2 = end_time(link.get_condition_time());
	for (const Chain<Step>* sc = steps; sc != NULL; sc = sc->tail) {
		const Step& s = sc->head;
		if (orderings.possibly_not_after(link.get_from_id(), lt1,
			s.get_id(), StepTime::AT_END)
			&& orderings.possibly_not_before(link.get_to_id(), lt2,
				s.get_id(), StepTime::AT_START)) {
			const EffectList& effects = s.get_action().get_effects();
			for (EffectList::const_iterator ei = effects.begin();
				ei != effects.end(); ei++) {
				const Effect& e = **ei;
				if (!domain->requirements.durative_actions
					&& e.get_link_condition().is_contradiction()) {
					continue;
				}
				StepTime et = end_time(e);
				if (!(s.get_id() == link.get_to_id() && et >= lt2)
					&& orderings.possibly_not_after(link.get_from_id(), lt1, s.get_id(), et)
					&& orderings.possibly_not_before(link.get_to_id(), lt2, s.get_id(), et)) {
					if (typeid(link.get_condition()) == typeid(Negation)
						|| !(link.get_from_id() == s.get_id() && lt1 == et)) {
						if (bindings.affects(e.get_literal(), s.get_id(),
							link.get_condition(), link.get_to_id())) {
							unsafes = new Chain<Unsafe>(Unsafe(link, s.get_id(), e), unsafes);
							num_unsafes++;
						}
					}
				}
			}
		}
	}
}

// Find the threatened links by the given step. * /
static void step_threats(const Chain<Unsafe>*& unsafes, size_t& num_unsafes,
	const Step& step, const Chain<Link>* links,
	const Orderings& orderings,
	const Bindings& bindings) {
	const EffectList& effects = step.get_action().get_effects();
	for (const Chain<Link>* lc = links; lc != NULL; lc = lc->tail) {
		const Link& l = lc->head;
		StepTime lt1 = l.get_effect_time();
		StepTime lt2 = end_time(l.get_condition_time());
		if (orderings.possibly_not_after(l.get_from_id(), lt1,
			step.get_id(), StepTime::AT_END)
			&& orderings.possibly_not_before(l.get_to_id(), lt2,
				step.get_id(), StepTime::AT_START)) {
			for (EffectList::const_iterator ei = effects.begin();
				ei != effects.end(); ei++) {
				const Effect& e = **ei;
				if (!domain->requirements.durative_actions
					&& e.get_link_condition().is_contradiction()) {
					continue;
				}
				StepTime et = end_time(e);
				if (!(step.get_id() == l.get_to_id() && et >= lt2)
					&& orderings.possibly_not_after(l.get_from_id(), lt1, step.get_id(), et)
					&& orderings.possibly_not_before(l.get_to_id(), lt2, step.get_id(), et)) {
					if (typeid(l.get_condition()) == typeid(Negation)
						|| !(l.get_from_id() == step.get_id() && lt1 == et)) {
						if (bindings.affects(e.get_literal(), step.get_id(),
							l.get_condition(), l.get_to_id())) {
							unsafes = new Chain<Unsafe>(Unsafe(l, step.get_id(), e), unsafes);
							num_unsafes++;
						}
					}
				}
			}
		}
	}
}

// Finds the mutex threats by the given step.
static void mutex_threats(const Chain<MutexThreat>*& mutex_threats,
	const Step& step, const Chain<Step>* steps,
	const Orderings& orderings,
	const Bindings& bindings) {
	const EffectList& effects = step.get_action().get_effects();
	for (const Chain<Step>* sc = steps; sc != NULL; sc = sc->tail) {
		const Step& s = sc->head;
		bool ss, se, es, ee;
		if (orderings.possibly_concurrent(step.get_id(), s.get_id(), ss, se, es, ee)) {
			const EffectList& effects2 = s.get_action().get_effects();
			for (EffectList::const_iterator ei = effects.begin();
				ei != effects.end(); ei++) {
				const Effect& e = **ei;
				if (e.get_when() == EffectTime::AT_START) {
					if (!ss && !se) {
						continue;
					}
				}
				else if (!es && !ee) {
					continue;
				}
				for (EffectList::const_iterator ej = effects2.begin();
					ej != effects2.end(); ej++) {
					const Effect& e2 = **ej;
					if (e.get_when() == EffectTime::AT_START) {
						if (e2.get_when() == EffectTime::AT_START) {
							if (!ss) {
								continue;
							}
						}
						else if (!se) {
							continue;
						}
					}
					else {
						if (e2.get_when() == EffectTime::AT_START) {
							if (!es) {
								continue;
							}
						}
						else if (!ee) {
							continue;
						}
					}
					if (bindings.unify(e.get_literal().get_atom(), step.get_id(),
						e2.get_literal().get_atom(), s.get_id())) {
						mutex_threats = new Chain<MutexThreat>(MutexThreat(step.get_id(), e,
							s.get_id(), e2),
							mutex_threats);
					}
				}
			}
		}
	}
}

// Returns binding constraints that make the given steps fully instantiated, or NULL if no consistent binding constraints can be found.
static const Bindings* step_instantiation(const Chain<Step>* steps, size_t n,
	const Bindings& bindings) {
	if (steps == NULL) {
		return &bindings;
	}
	else {
		const Step& step = steps->head;
		const ActionSchema* as = dynamic_cast<const ActionSchema*>(&step.get_action());
		if (as == NULL || as->get_parameters().size() <= n) {
			return step_instantiation(steps->tail, 0, bindings);
		}
		else {
			const Variable& v = as->get_parameters()[n];
			if (v != bindings.get_binding(v, step.get_id())) {
				return step_instantiation(steps, n + 1, bindings);
			}
			else {
				const Type& t = TermTable::type(v);
				const ObjectList& arguments = problem->get_terms().compatible_objects(t);
				for (ObjectList::const_iterator oi = arguments.begin();
					oi != arguments.end(); oi++) {
					BindingList bl;
					bl.push_back(Binding(v, step.get_id(), *oi, 0, true));
					const Bindings* new_bindings = bindings.add(bl);
					if (new_bindings != NULL) {
						const Bindings* result = step_instantiation(steps, n + 1,
							*new_bindings);
						if (result != new_bindings) {
							delete new_bindings;
						}
						if (result != NULL) {
							return result;
						}
					}
				}
				return NULL;
			}
		}
	}
}

// Return the initial plan representing the given problem, or NULL if initial conditions or goals of the problem are inconsistent.
const Plan* Plan::make_initial_plan(const Problem& problem) {
	// Create goal of problem.
	if (params->ground_actions) {
		goal_action = new GroundAction("", false);
		const Formula& goal_formula =
			problem.get_goal().get_instantiation(SubstitutionMap(), problem);
		goal_action->set_condition(goal_formula);
	}
	else {
		goal_action = new ActionSchema("", false);
		goal_action->set_condition(problem.get_goal());
	}
	// Chain of open conditions.
	const Chain<OpenCondition>* open_conds = NULL;
	// Number of open conditions.
	size_t num_open_conds = 0;
	// Bindings introduced by goal.
	BindingList new_bindings;
	// Add goals as open conditions.
	if (!add_goal(open_conds, num_open_conds, new_bindings,
		goal_action->get_condition(), GOAL_ID)) {
		// Goals are inconsistent.
		RCObject::ref(open_conds);
		RCObject::destructive_deref(open_conds);
		return NULL;
	}
	// Make chain of mutex threat place holder.
	const Chain<MutexThreat>* mutex_threats =
		new Chain<MutexThreat>(MutexThreat(), NULL);
	// Make chain of initial steps.
	const Chain<Step>* steps =
		new Chain<Step>(Step(0, problem.get_init_action()),
			new Chain<Step>(Step(GOAL_ID, *goal_action), NULL));
	size_t num_steps = 0;
	// Variable bindings.
	const Bindings* bindings = &Bindings::EMPTY;
	// Step orderings.
	const Orderings* orderings;
	if (domain->requirements.durative_actions) {
		const TemporalOrderings* to = new TemporalOrderings();
		// Add steps for timed initial literals.
		for (TimedActionTable::const_iterator ai = problem.get_timed_actions().begin();
			ai != problem.get_timed_actions().end(); ai++) {
			num_steps++;
			steps = new Chain<Step>(Step(num_steps, *(*ai).second), steps);
			const TemporalOrderings* tmp = to->refine((*ai).first, steps->head);
			delete to;
			if (tmp == NULL) {
				RCObject::ref(open_conds);
				RCObject::destructive_deref(open_conds);
				RCObject::ref(steps);
				RCObject::destructive_deref(steps);
				return NULL;
			}
			to = tmp;
		}
		orderings = to;
	}
	else {
		orderings = new BinaryOrderings();
	}
	// Return initial plan.
	return new Plan(steps, num_steps, NULL, 0, *orderings, *bindings,
		NULL, 0, open_conds, num_open_conds, mutex_threats, NULL);
}

// Return the next flaw to work on.
const Flaw& Plan::get_flaw(const FlawSelectionOrder& flaw_order) const {
	const Flaw& flaw = flaw_order.select(*this, *problem, planning_graph);
	if (!params->ground_actions) {
		const OpenCondition* open_cond = dynamic_cast<const OpenCondition*>(&flaw);
		static_pred_flaw = (open_cond != NULL && open_cond->is_static());
	}
	return flaw;
}

// Return the refinements for the next flaw to work on.
void Plan::refinements(PlanList& plans,
	const FlawSelectionOrder& flaw_order) const {
	const Flaw& flaw = get_flaw(flaw_order);
	if (verbosity > 1) {
		cerr << endl << "handle ";
		flaw.print(cerr, *bindings);
		cerr << endl;
	}
	const Unsafe* unsafe = dynamic_cast<const Unsafe*>(&flaw);
	if (unsafe != NULL) {
		handle_unsafe(plans, *unsafe);
	}
	else {
		const OpenCondition* open_cond = dynamic_cast<const OpenCondition*>(&flaw);
		if (open_cond != NULL) {
			handle_open_condition(plans, *open_cond);
		}
		else {
			const MutexThreat* mutex_threat =
				dynamic_cast<const MutexThreat*>(&flaw);
			if (mutex_threat != NULL) {
				handle_mutex_threat(plans, *mutex_threat);
			}
			else {
				throw logic_error("unknown kind of flaw");
			}
		}
	}
}

// Handle an unsafe link.
void Plan::handle_unsafe(PlanList& plans, const Unsafe& unsafe) const {
	BindingList unifier;
	const Link& link = unsafe.get_link();
	StepTime lt1 = link.get_effect_time();
	StepTime lt2 = end_time(link.get_condition_time());
	StepTime et = end_time(unsafe.get_effect());
	if (get_orderings().possibly_not_after(link.get_from_id(), lt1,
		unsafe.get_step_id(), et)
		&& get_orderings().possibly_not_before(link.get_to_id(), lt2,
			unsafe.get_step_id(), et)
		&& bindings->affects(unifier, unsafe.get_effect().get_literal(),
			unsafe.get_step_id(),
			link.get_condition(), link.get_to_id())) {
		separate(plans, unsafe, unifier);
		promote(plans, unsafe);
		demote(plans, unsafe);
	}
	else {
		// bogus flaw
		plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(), get_num_links(),
			get_orderings(), *bindings,
			get_unsafes()->remove(unsafe), get_num_unsafes() - 1,
			get_open_conds(), get_num_open_conds(),
			get_mutex_threats(), this));
	}
}

// Handle an unsafe link through separation.
int Plan::separate(PlanList& plans, const Unsafe& unsafe,
	const BindingList& unifier, bool test_only) const {
	const Formula* goal = &Formula::FALSE_FORMULA;
	for (BindingList::const_iterator si = unifier.begin();
		si != unifier.end(); si++) {
		const Binding& subst = *si;
		if (!unsafe.get_effect().quantifies(subst.get_var())) {
			const Formula& g = Inequality::make(subst.get_var(), subst.get_var_id(),
				subst.get_term(), subst.get_term_id());
			const Inequality* neq = dynamic_cast<const Inequality*>(&g);
			if (neq == 0 || bindings->is_consistent_with(*neq, 0)) {
				goal = &(*goal || g);
			}
			else {
				Formula::register_use(&g);
				Formula::unregister_use(&g);
			}
		}
	}
	const Formula& effect_cond = unsafe.get_effect().get_condition();
	if (!effect_cond.is_tautology()) {
		size_t n = unsafe.get_effect().get_arity();
		if (n > 0) {
			Forall* forall = new Forall();
			SubstitutionMap forall_subst;
			for (size_t i = 0; i < n; i++) {
				Variable vi = unsafe.get_effect().get_parameter(i);
				Variable v =
					test_only ? vi : TermTable::add_variable(TermTable::type(vi));
				forall->add_parameter(v);
				if (!test_only) {
					forall_subst.insert(make_pair(vi, v));
				}
			}
			if (test_only) {
				forall->set_body(!effect_cond);
			}
			else {
				forall->set_body(!effect_cond.get_substitution(forall_subst));
			}
			const Formula* forall_cond;
			if (forall->get_body().is_tautology() || forall->get_body().is_contradiction()) {
				forall_cond = &forall->get_body();
				delete forall;
			}
			else {
				forall_cond = forall;
			}
			goal = &(*goal || *forall_cond);
		}
		else {
			goal = &(*goal || !effect_cond);
		}
	}
	const Chain<OpenCondition>* new_open_conds = test_only ? NULL : get_open_conds();
	size_t new_num_open_conds = test_only ? 0 : get_num_open_conds();
	BindingList new_bindings;
	bool added = add_goal(new_open_conds, new_num_open_conds, new_bindings,
		*goal, unsafe.get_step_id(), test_only);
	if (!test_only) {
		RCObject::ref(new_open_conds);
	}
	int count = 0;
	if (added) {
		const Bindings* bindings_t = bindings->add(new_bindings, test_only);
		if (bindings_t != NULL) {
			if (!test_only) {
				const Orderings* new_orderings = orderings;
				if (!goal->is_tautology() && planning_graph != NULL) {
					const TemporalOrderings* to =
						dynamic_cast<const TemporalOrderings*>(new_orderings);
					if (to != NULL) {
						HeuristicValue h, hs;
						goal->get_heuristic_value(h, hs, *planning_graph, unsafe.get_step_id(),
							params->ground_actions ? NULL : bindings_t);
						new_orderings = to->refine(unsafe.get_step_id(),
							hs.get_makespan(), h.get_makespan());
					}
				}
				if (new_orderings != NULL) {
					plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(), get_num_links(),
						*new_orderings, *bindings_t,
						get_unsafes()->remove(unsafe),
						get_num_unsafes() - 1,
						new_open_conds, new_num_open_conds,
						get_mutex_threats(), this));
				}
				else {
					Bindings::register_use(bindings_t);
					Bindings::unregister_use(bindings_t);
				}
			}
			count++;
		}
	}
	if (!test_only) {
		RCObject::destructive_deref(new_open_conds);
	}
	Formula::register_use(goal);
	Formula::unregister_use(goal);
	return count;
}

// Handle an unsafe link through demotion.
int Plan::demote(PlanList& plans, const Unsafe& unsafe,
	bool test_only) const {
	const Link& link = unsafe.get_link();
	StepTime lt1 = link.get_effect_time();
	StepTime et = end_time(unsafe.get_effect());
	if (get_orderings().possibly_before(unsafe.get_step_id(), et, link.get_from_id(), lt1)) {
		if (!test_only) {
			new_ordering(plans, unsafe.get_step_id(), et, link.get_from_id(), lt1, unsafe);
		}
		return 1;
	}
	else {
		return 0;
	}
}

// Handle an unsafe link through promotion.
int Plan::promote(PlanList& plans, const Unsafe& unsafe,
	bool test_only) const {
	const Link& link = unsafe.get_link();
	StepTime lt2 = end_time(link.get_condition_time());
	StepTime et = end_time(unsafe.get_effect());
	if (get_orderings().possibly_before(link.get_to_id(), lt2, unsafe.get_step_id(), et)) {
		if (!test_only) {
			new_ordering(plans, link.get_to_id(), lt2, unsafe.get_step_id(), et, unsafe);
		}
		return 1;
	}
	else {
		return 0;
	}
}

// Add a plan to the given plan list with an ordering added.
void Plan::new_ordering(PlanList& plans, size_t before_id, StepTime t1,
	size_t after_id, StepTime t2,
	const Unsafe& unsafe) const {
	const Orderings* new_orderings =
		get_orderings().refine(Ordering(before_id, t1, after_id, t2));
	if (new_orderings != NULL) {
		plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(), get_num_links(),
			*new_orderings, *bindings,
			get_unsafes()->remove(unsafe), get_num_unsafes() - 1,
			get_open_conds(), get_num_open_conds(),
			get_mutex_threats(), this));
	}
}

// Handle a mutex threat.
void Plan::handle_mutex_threat(PlanList& plans,
	const MutexThreat& mutex_threat) const {
	if (mutex_threat.get_step_id1() == 0) {
		const Chain<MutexThreat>* new_mutex_threats = NULL;
		for (const Chain<Step>* sc = get_steps(); sc != NULL; sc = sc->tail) {
			const Step& s = sc->head;
			::mutex_threats(new_mutex_threats, s, get_steps(), get_orderings(), *bindings);
		}
		plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(), get_num_links(),
			get_orderings(), *bindings, get_unsafes(), get_num_unsafes(),
			get_open_conds(), get_num_open_conds(),
			new_mutex_threats, this));
		return;
	}
	BindingList unifier;
	size_t id1 = mutex_threat.get_step_id1();
	StepTime et1 = end_time(mutex_threat.get_effect1());
	size_t id2 = mutex_threat.get_step_id2();
	StepTime et2 = end_time(mutex_threat.get_effect2());
	if (get_orderings().possibly_not_before(id1, et1, id2, et2)
		&& get_orderings().possibly_not_after(id1, et1, id2, et2)
		&& bindings->unify(unifier,
			mutex_threat.get_effect1().get_literal().get_atom(), id1,
			mutex_threat.get_effect2().get_literal().get_atom(), id2)) {
		separate(plans, mutex_threat, unifier);
		promote(plans, mutex_threat);
		demote(plans, mutex_threat);
	}
	else {
		// bogus flaw
		plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(), get_num_links(),
			get_orderings(), *bindings, get_unsafes(), get_num_unsafes(),
			get_open_conds(), get_num_open_conds(),
			get_mutex_threats()->remove(mutex_threat), this));
	}
}

// Handle a mutex threat through separation.
void Plan::separate(PlanList& plans, const MutexThreat& mutex_threat,
	const BindingList& unifier) const {
	if (!unifier.empty()) {
		const Formula* goal = &Formula::FALSE_FORMULA;
		for (BindingList::const_iterator si = unifier.begin();
			si != unifier.end(); si++) {
			const Binding& subst = *si;
			if (!mutex_threat.get_effect1().quantifies(subst.get_var())
				&& !mutex_threat.get_effect2().quantifies(subst.get_var())) {
				const Formula& g = Inequality::make(subst.get_var(), subst.get_var_id(),
					subst.get_term(), subst.get_term_id());
				const Inequality* neq = dynamic_cast<const Inequality*>(&g);
				if (neq == 0 || bindings->is_consistent_with(*neq, 0)) {
					goal = &(*goal || g);
				}
				else {
					Formula::register_use(&g);
					Formula::unregister_use(&g);
				}
			}
		}
		const Chain<OpenCondition>* new_open_conds = get_open_conds();
		size_t new_num_open_conds = get_num_open_conds();
		BindingList new_bindings;
		bool added = add_goal(new_open_conds, new_num_open_conds, new_bindings,
			*goal, 0);
		RCObject::ref(new_open_conds);
		if (added) {
			const Bindings* bindings_t = bindings->add(new_bindings);
			if (bindings_t != NULL) {
				plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(),
					get_num_links(), get_orderings(), *bindings_t,
					get_unsafes(), get_num_unsafes(),
					new_open_conds, new_num_open_conds,
					get_mutex_threats()->remove(mutex_threat), this));
			}
			else {
				Bindings::register_use(bindings_t);
				Bindings::unregister_use(bindings_t);
			}
		}
		RCObject::destructive_deref(new_open_conds);
		Formula::register_use(goal);
		Formula::unregister_use(goal);
	}
	for (size_t i = 1; i <= 2; i++) {
		size_t step_id = ((i == 1) ? mutex_threat.get_step_id1()
			: mutex_threat.get_step_id2());
		const Effect& effect = ((i == 1) ? mutex_threat.get_effect1()
			: mutex_threat.get_effect2());
		const Formula& effect_cond = effect.get_condition();
		const Formula* goal;
		if (!effect_cond.is_tautology()) {
			size_t n = effect.get_arity();
			if (n > 0) {
				Forall* forall = new Forall();
				SubstitutionMap forall_subst;
				for (size_t i = 0; i < n; i++) {
					Variable vi = effect.get_parameter(i);
					Variable v = TermTable::add_variable(TermTable::type(vi));
					forall->add_parameter(v);
					forall_subst.insert(make_pair(vi, v));
				}
				forall->set_body(!effect_cond.get_substitution(forall_subst));
				if (forall->get_body().is_tautology() || forall->get_body().is_contradiction()) {
					goal = &forall->get_body();
					delete forall;
				}
				else {
					goal = forall;
				}
			}
			else {
				goal = &!effect_cond;
			}
			const Chain<OpenCondition>* new_open_conds = get_open_conds();
			size_t new_num_open_conds = get_num_open_conds();
			BindingList new_bindings;
			bool added = add_goal(new_open_conds, new_num_open_conds, new_bindings,
				*goal, step_id);
			RCObject::ref(new_open_conds);
			if (added) {
				const Bindings* bindings_t = bindings->add(new_bindings);
				if (bindings_t != NULL) {
					const Orderings* new_orderings = orderings;
					if (!goal->is_tautology() && planning_graph != NULL) {
						const TemporalOrderings* to =
							dynamic_cast<const TemporalOrderings*>(new_orderings);
						if (to != NULL) {
							HeuristicValue h, hs;
							goal->get_heuristic_value(h, hs, *planning_graph, step_id,
								params->ground_actions ? NULL : bindings_t);
							new_orderings = to->refine(step_id, hs.get_makespan(), h.get_makespan());
						}
					}
					if (new_orderings != NULL) {
						plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(),
							get_num_links(), *new_orderings, *bindings_t,
							get_unsafes(), get_num_unsafes(),
							new_open_conds, new_num_open_conds,
							get_mutex_threats()->remove(mutex_threat),
							this));
					}
					else {
						Bindings::register_use(bindings_t);
						Bindings::unregister_use(bindings_t);
					}
				}
			}
			RCObject::destructive_deref(new_open_conds);
			Formula::register_use(goal);
			Formula::unregister_use(goal);
		}
	}
}

// Handle a mutex threat through demotion.
void Plan::demote(PlanList& plans, const MutexThreat& mutex_threat) const {
	size_t id1 = mutex_threat.get_step_id1();
	StepTime et1 = end_time(mutex_threat.get_effect1());
	size_t id2 = mutex_threat.get_step_id2();
	StepTime et2 = end_time(mutex_threat.get_effect2());
	if (get_orderings().possibly_before(id1, et1, id2, et2)) {
		new_ordering(plans, id1, et1, id2, et2, mutex_threat);
	}
}

// Handle a mutex threat through promotion.
void Plan::promote(PlanList& plans, const MutexThreat& mutex_threat) const {
	size_t id1 = mutex_threat.get_step_id1();
	StepTime et1 = end_time(mutex_threat.get_effect1());
	size_t id2 = mutex_threat.get_step_id2();
	StepTime et2 = end_time(mutex_threat.get_effect2());
	if (get_orderings().possibly_before(id2, et2, id1, et1)) {
		new_ordering(plans, id2, et2, id1, et1, mutex_threat);
	}
}

// Add a plan to the given plan list with an ordering added.
void Plan::new_ordering(PlanList& plans, size_t before_id, StepTime t1,
	size_t after_id, StepTime t2, const MutexThreat& mutex_threat) const {
	const Orderings* new_orderings =
		get_orderings().refine(Ordering(before_id, t1, after_id, t2));
	if (new_orderings != NULL) {
		plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(), get_num_links(),
			*new_orderings, *bindings,
			get_unsafes(), get_num_unsafes(),
			get_open_conds(), get_num_open_conds(),
			get_mutex_threats()->remove(mutex_threat), this));
	}
}

// Handle an open condition.
void Plan::handle_open_condition(PlanList& plans,
	const OpenCondition& open_cond) const {
	const Literal* literal = open_cond.literal();
	if (literal != NULL) {
		const ActionEffectMap* achievers = literal_achievers(*literal);
		if (achievers != NULL) {
			add_step(plans, *literal, open_cond, *achievers);
			reuse_step(plans, *literal, open_cond, *achievers);
		}
		const Negation* negation = dynamic_cast<const Negation*>(literal);
		if (negation != NULL) {
			new_cw_link(plans, problem->get_init_action().get_effects(),
				*negation, open_cond);
		}
	}
	else {
		const Disjunction* disj = open_cond.disjunction();
		if (disj != NULL) {
			handle_disjunction(plans, *disj, open_cond);
		}
		else {
			const Inequality* neq = open_cond.inequality();
			if (neq != NULL) {
				handle_inequality(plans, *neq, open_cond);
			}
			else {
				throw logic_error("unknown kind of open condition");
			}
		}
	}
}

// Handle a disjunctive open condition.
int Plan::handle_disjunction(PlanList& plans, const Disjunction& disj,
	const OpenCondition& open_cond,
	bool test_only = false) const {
	int count = 0;
	const FormulaList& disjuncts = disj.get_disjuncts();
	for (FormulaList::const_iterator fi = disjuncts.begin();
		fi != disjuncts.end(); fi++) {
		BindingList new_bindings;
		const Chain<OpenCondition>* new_open_conds =
			test_only ? NULL : get_open_conds()->remove(open_cond);
		size_t new_num_open_conds = test_only ? 0 : get_num_open_conds() - 1;
		bool added = add_goal(new_open_conds, new_num_open_conds, new_bindings,
			**fi, open_cond.get_step_id(), test_only);
		if (!test_only) {
			RCObject::ref(new_open_conds);
		}
		if (added) {
			const Bindings* bindings_t = bindings->add(new_bindings, test_only);
			if (bindings != NULL) {
				if (!test_only) {
					plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(), get_num_links(),
						get_orderings(), *bindings_t,
						get_unsafes(), get_num_unsafes(),
						new_open_conds, new_num_open_conds,
						get_mutex_threats(), this));
				}
				count++;
			}
		}
		if (!test_only) {
			RCObject::destructive_deref(new_open_conds);
		}
	}
	return count;
}

// Handle an inequality open condition.
int Plan::handle_inequality(PlanList& plans, const Inequality& neq,
	const OpenCondition& open_cond,
	bool test_only = false) const {
	int count = 0;
	size_t step_id = open_cond.get_step_id();
	Variable variable2 = neq.get_term().as_variable();
	const ObjectSet& d1 = bindings->get_domain(neq.get_variable(), neq.step_id1(step_id),
		*problem);
	const ObjectSet& d2 = bindings->get_domain(variable2, neq.step_id2(step_id),
		*problem);

	// Branch on the variable with the smallest domain.
	const Variable& var1 = (d1.size() < d2.size()) ? neq.get_variable() : variable2;
	size_t id1 =
		(d1.size() < d2.size()) ? neq.step_id1(step_id) : neq.step_id2(step_id);
	const Variable& var2 = (d1.size() < d2.size()) ? variable2 : neq.get_variable();
	size_t id2 =
		(d1.size() < d2.size()) ? neq.step_id2(step_id) : neq.step_id1(step_id);
	const ObjectSet& var_domain = (d1.size() < d2.size()) ? d1 : d2;
	for (ObjectSet::const_iterator ni = var_domain.begin();
		ni != var_domain.end(); ni++) {
		Object name = *ni;
		BindingList new_bindings;
		new_bindings.push_back(Binding(var1, id1, name, 0, true));
		new_bindings.push_back(Binding(var2, id2, name, 0, false));
		const Bindings* bindings_t = bindings->add(new_bindings, test_only);
		if (bindings != NULL) {
			if (!test_only) {
				plans.push_back(new Plan(get_steps(), get_num_steps(), get_links(), get_num_links(),
					get_orderings(), *bindings_t,
					get_unsafes(), get_num_unsafes(),
					get_open_conds()->remove(open_cond),
					get_num_open_conds() - 1,
					get_mutex_threats(), this));
			}
			count++;
		}
	}
	if (planning_graph == NULL) {
		delete &d1;
		delete &d2;
	}
	return count;
}


//void Plan::add_step(PlanList& plans, const Literal& literal,
//	const OpenCondition& open_cond,
//	const ActionEffectMap& achievers) const {
//	for (ActionEffectMap::const_iterator ai = achievers.begin();
//		ai != achievers.end(); ai++) {
//		const Action& action = *(*ai).first;
//		if (action.get_name().substr(0, 1) != "<") {
//			const Effect& effect = *(*ai).second;
//			new_link(plans, Step(get_num_steps() + 1, action), effect,
//				literal, open_cond);
//		}
//	}
//}

// Add plans to the given plan list with a link from the given step to the given open condition added.
int Plan::new_link(PlanList& plans, const Step& step, const Effect& effect,
	const Literal& literal, const OpenCondition& open_cond,
	bool test_only) const {
	BindingList mgu;
	if (bindings->unify(mgu, effect.get_literal(), step.get_id(),
		literal, open_cond.get_step_id())) {
		return make_link(plans, step, effect, literal, open_cond, mgu, test_only);
	}
	else {
		return 0;
	}
}

// Add plans to the given plan list with a link from the given step to the given open condition added, using the closed world assumption.
int Plan::new_cw_link(PlanList& plans, const EffectList& effects,
	const Negation& negation, const OpenCondition& open_cond,
	bool test_only = false) const {
	const Atom& goal = negation.get_atom();
	const Formula* goals = &Formula::TRUE_FORMULA;
	for (EffectList::const_iterator ei = effects.begin();
		ei != effects.end(); ei++) {
		const Effect& effect = **ei;
		BindingList mgu;
		if (bindings->unify(mgu, effect.get_literal(), 0,
			goal, open_cond.get_step_id())) {
			if (mgu.empty()) {
				// Impossible to separate goal and initial condition.
				return 0;
			}
			const Formula* binds = &Formula::FALSE_FORMULA;
			for (BindingList::const_iterator si = mgu.begin();
				si != mgu.end(); si++) {
				const Binding& subst = *si;
				binds = &(*binds || Inequality::make(subst.get_var(), subst.get_var_id(),
					subst.get_term(), subst.get_term_id()));
			}
			goals = &(*goals && *binds);
		}
	}
	BindingList new_bindings;
	const Chain<OpenCondition>* new_open_conds =
		test_only ? NULL : get_open_conds()->remove(open_cond);
	size_t new_num_open_conds = test_only ? 0 : get_num_open_conds() - 1;
	bool added = add_goal(new_open_conds, new_num_open_conds, new_bindings,
		*goals, 0, test_only);
	Formula::register_use(goals);
	Formula::unregister_use(goals);
	if (!test_only) {
		RCObject::ref(new_open_conds);
	}
	int count = 0;
	if (added) {
		const Bindings* bindings_t = bindings->add(new_bindings, test_only);
		if (bindings_t != NULL) {
			if (!test_only) {
				const Chain<Unsafe>* new_unsafes = get_unsafes();
				size_t new_num_unsafes = get_num_unsafes();
				const Chain<Link>* new_links =
					new Chain<Link>(Link(0, StepTime::AT_END, open_cond), get_links());
				link_threats(new_unsafes, new_num_unsafes, new_links->head, get_steps(),
					get_orderings(), *bindings_t);
				plans.push_back(new Plan(get_steps(), get_num_steps(),
					new_links, get_num_links() + 1,
					get_orderings(), *bindings_t,
					new_unsafes, new_num_unsafes,
					new_open_conds, new_num_open_conds,
					get_mutex_threats(), this));
			}
			count++;
		}
	}
	if (!test_only) {
		RCObject::destructive_deref(new_open_conds);
	}
	return count;
}

// Return a plan with a link added from the given effect to the given open condition.
int Plan::make_link(PlanList& plans, const Step& step, const Effect& effect,
	const Literal& literal, const OpenCondition& open_cond,
	const BindingList& unifier, bool test_only = false) const {
	BindingList new_bindings;
	SubstitutionMap forall_subst;
	if (test_only) {
		new_bindings = unifier;
	}
	else {
		for (BindingList::const_iterator si = unifier.begin();
			si != unifier.end(); si++) {
			const Binding& subst = *si;
			if (effect.quantifies(subst.get_var())) {
				Variable v = TermTable::add_variable(TermTable::type(subst.get_var()));
				forall_subst.insert(make_pair(subst.get_var(), v));
				new_bindings.push_back(Binding(v, subst.get_var_id(),
					subst.get_term(), subst.get_term_id(), true));
			}
			else {
				new_bindings.push_back(subst);
			}
		}
	}

	// If the effect is conditional, add condition as goal.
	const Chain<OpenCondition>* new_open_conds =
		test_only ? NULL : get_open_conds()->remove(open_cond);
	size_t new_num_open_conds = test_only ? 0 : get_num_open_conds() - 1;
	const Formula* cond_goal = &(effect.get_condition() && effect.get_link_condition());
	if (!cond_goal->is_tautology()) {
		if (!test_only) {
			size_t n = effect.get_arity();
			if (n > 0) {
				for (size_t i = 0; i < n; i++) {
					Variable vi = effect.get_parameter(i);
					if (forall_subst.find(vi) == forall_subst.end()) {
						Variable v = TermTable::add_variable(TermTable::type(vi));
						forall_subst.insert(make_pair(vi, v));
					}
				}
				const Formula* old_cond_goal = cond_goal;
				cond_goal = &cond_goal->get_substitution(forall_subst);
				if (old_cond_goal != cond_goal) {
					Formula::register_use(old_cond_goal);
					Formula::unregister_use(old_cond_goal);
				}
			}
		}
		bool added = add_goal(new_open_conds, new_num_open_conds, new_bindings,
			*cond_goal, step.get_id(), test_only);
		Formula::register_use(cond_goal);
		Formula::unregister_use(cond_goal);
		if (!added) {
			if (!test_only) {
				RCObject::ref(new_open_conds);
				RCObject::destructive_deref(new_open_conds);
			}
			return 0;
		}
	}

	// See if this is a new step.
	const Bindings* bindings_t = bindings;
	const Chain<Step>* new_steps = test_only ? NULL : get_steps();
	size_t new_num_steps = test_only ? 0 : get_num_steps();
	if (step.get_id() > get_num_steps()) {
		if (!add_goal(new_open_conds, new_num_open_conds, new_bindings,
			step.get_action().get_condition(), step.get_id(), test_only)) {
			if (!test_only) {
				RCObject::ref(new_open_conds);
				RCObject::destructive_deref(new_open_conds);
			}
			return 0;
		}
		if (params->domain_constraints) {
			bindings_t = bindings_t->add(step.get_id(), step.get_action(), *planning_graph);
			if (bindings_t == NULL) {
				if (!test_only) {
					RCObject::ref(new_open_conds);
					RCObject::destructive_deref(new_open_conds);
				}
				return 0;
			}
		}
		if (!test_only) {
			new_steps = new Chain<Step>(step, new_steps);
			new_num_steps++;
		}
	}
	const Bindings* tmp_bindings = bindings_t->add(new_bindings, test_only);
	if ((test_only || tmp_bindings != bindings_t) && bindings_t != bindings) {
		delete bindings_t;
	}
	if (tmp_bindings == NULL) {
		if (!test_only) {
			RCObject::ref(new_open_conds);
			RCObject::destructive_deref(new_open_conds);
			RCObject::ref(new_steps);
			RCObject::destructive_deref(new_steps);
			return 0;
		}
	}
	if (!test_only) {
		bindings_t = tmp_bindings;
		StepTime et = end_time(effect);
		StepTime gt = start_time(open_cond.get_when());
		const Orderings* new_orderings =
			get_orderings().refine(Ordering(step.get_id(), et, open_cond.get_step_id(), gt),
				step, planning_graph,
				params->ground_actions ? NULL : bindings_t);
		if (new_orderings != NULL && !cond_goal->is_tautology()
			&& planning_graph != NULL) {
			const TemporalOrderings* to =
				dynamic_cast<const TemporalOrderings*>(new_orderings);
			if (to != NULL) {
				HeuristicValue h, hs;
				cond_goal->get_heuristic_value(h, hs, *planning_graph, step.get_id(),
					params->ground_actions ? NULL : bindings_t);
				const Orderings* tmp_orderings = to->refine(step.get_id(), hs.get_makespan(),
					h.get_makespan());
				if (tmp_orderings != new_orderings) {
					delete new_orderings;
					new_orderings = tmp_orderings;
				}
			}
		}
		if (new_orderings == NULL) {
			if (bindings_t != bindings) {
				delete bindings_t;
			}
			RCObject::ref(new_open_conds);
			RCObject::destructive_deref(new_open_conds);
			RCObject::ref(new_steps);
			RCObject::destructive_deref(new_steps);
			return 0;
		}

		// Add a new link.
		const Chain<Link>* new_links =
			new Chain<Link>(Link(step.get_id(), end_time(effect), open_cond), get_links());

		// Find any threats to the newly established link.
		const Chain<Unsafe>* new_unsafes = get_unsafes();
		size_t new_num_unsafes = get_num_unsafes();
		link_threats(new_unsafes, new_num_unsafes, new_links->head, new_steps,
			*new_orderings, *bindings_t);

		// If this is a new step, find links it threatens.
		const Chain<MutexThreat>* new_mutex_threats = get_mutex_threats();
		if (step.get_id() > get_num_steps()) {
			step_threats(new_unsafes, new_num_unsafes, step,
				get_links(), *new_orderings, *bindings_t);
		}

		// Add the new plan.
		plans.push_back(new Plan(new_steps, new_num_steps, new_links,
			get_num_links() + 1, *new_orderings, *bindings_t,
			new_unsafes, new_num_unsafes,
			new_open_conds, new_num_open_conds,
			new_mutex_threats, this));
	}
	return 1;
}

// Return plan for given problem.
const Plan* Plan::plan(const Problem& problem, const Parameters& p,
	bool last_problem) {
	// Set planning parameters.
	params = &p;
	// Set current domain.
	domain = &problem.get_domain();
	::problem = &problem;

	
	// Initialize planning graph and maps from predicates to actions.
	
	bool need_pg = (params->ground_actions || params->domain_constraints
		|| params->heuristic.needs_planning_graph());
	for (size_t i = 0; !need_pg && i < params->flaw_orders.size(); i++) {
		if (params->flaw_orders[i].needs_planning_graph()) {
			need_pg = true;
		}
	}
	if (need_pg) {
		planning_graph = new PlanningGraph(problem, *params);
	}
	else {
		planning_graph = NULL;
	}
	if (!params->ground_actions) {
		achieves_pred.clear();
		achieves_neg_pred.clear();
		for (ActionSchemaMap::const_iterator ai = domain->get_actions().begin();
			ai != domain->get_actions().end(); ai++) {
			const ActionSchema* as = (*ai).second;
			for (EffectList::const_iterator ei = as->get_effects().begin();
				ei != as->get_effects().end(); ei++) {
				const Literal& literal = (*ei)->get_literal();
				if (typeid(literal) == typeid(Atom)) {
					achieves_pred[literal.get_predicate()].insert(make_pair(as, *ei));
				}
				else {
					achieves_neg_pred[literal.get_predicate()].insert(make_pair(as,
						*ei));
				}
			}
		}
		const GroundAction& ia = problem.get_init_action();
		for (EffectList::const_iterator ei = ia.get_effects().begin();
			ei != ia.get_effects().end(); ei++) {
			const Literal& literal = (*ei)->get_literal();
			achieves_pred[literal.get_predicate()].insert(make_pair(&ia, *ei));
		}
		for (TimedActionTable::const_iterator ai = problem.get_timed_actions().begin();
			ai != problem.get_timed_actions().end(); ai++) {
			const GroundAction& action = *(*ai).second;
			for (EffectList::const_iterator ei = action.get_effects().begin();
				ei != action.get_effects().end(); ei++) {
				const Literal& literal = (*ei)->get_literal();
				if (typeid(literal) == typeid(Atom)) {
					achieves_pred[literal.get_predicate()].insert(make_pair(&action,
						*ei));
				}
				else {
					achieves_neg_pred[literal.get_predicate()].insert(make_pair(&action,
						*ei));
				}
			}
		}
	}
	static_pred_flaw = false;

	// Number of visited plan.
	size_t num_visited_plans = 0;
	// Number of generated plans.
	size_t num_generated_plans = 0;
	// Number of static preconditions encountered.
	size_t num_static = 0;
	// Number of dead ends encountered.
	size_t num_dead_ends = 0;

	// Generated plans for different flaw selection orders.
	vector<size_t> generated_plans(params->flaw_orders.size(), 0);
	// Queues of pending plans.
	vector<PlanQueue> plans(params->flaw_orders.size(), PlanQueue());
	// Dead plan queues.
	vector<PlanQueue*> dead_queues;
	// Construct the initial plan.
	const Plan* initial_plan = make_initial_plan(problem);
	if (initial_plan != NULL) {
		initial_plan->id = 0;
	}

	// Variable for progress bar (number of generated plans).
	size_t last_dot = 0;
	// Variable for progress bar (time).
	size_t last_hash = 0;

	// Search for complete plan.
	size_t current_flaw_order = 0;
	size_t flaw_orders_left = params->flaw_orders.size();
	size_t next_switch = 1000;
	const Plan* current_plan = initial_plan;
	generated_plans[current_flaw_order]++;
	num_generated_plans++;
	if (verbosity > 1) {
		cerr << "using flaw order " << current_flaw_order << endl;
	}
	float f_limit;
	if (current_plan != NULL
		&& params->search_algorithm == Parameters::IDA_STAR) {
		f_limit = current_plan->primary_rank();
	}
	else {
		f_limit = numeric_limits<float>::infinity();
	}
	do {
		float next_f_limit = numeric_limits<float>::infinity();
		while (current_plan != NULL && !current_plan->is_complete()) {
			// Do a little amortized cleanup of dead queues.
			for (size_t dq = 0; dq < 4 && !dead_queues.empty(); dq++) {
				PlanQueue& dead_queue = *dead_queues.back();
				delete dead_queue.top();
				dead_queue.pop();
				if (dead_queue.empty()) {
					dead_queues.pop_back();
				}
			}

			// diff here

			// Visiting a new plan.

			num_visited_plans++;
			if (verbosity == 1) {
				while (num_generated_plans - num_static - last_dot >= 1000) {
					cerr << '.';
					last_dot += 1000;
				}
				// diff here!
			}
			if (verbosity > 1) {
				cerr << endl << (num_visited_plans - num_static) << ": "
					<< "!!!!CURRENT PLAN (id " << current_plan->id << ")"
					<< " with rank (" << current_plan->primary_rank();
				for (size_t ri = 1; ri < current_plan->rank.size(); ri++) {
					cerr << ',' << current_plan->rank[ri];
				}
				cerr << ")" << endl << *current_plan << endl;
			}
			// List of children to current plan.
			PlanList refinements;
			// Get plan refinements. 
			current_plan->refinements(refinements,
				params->flaw_orders[current_flaw_order]);
			// Add children to queue of pending plans. 
			bool added = false;
			for (PlanList::const_iterator pi = refinements.begin();
				pi != refinements.end(); pi++) {
				const Plan& new_plan = **pi;
				// N.B. Must set id before computing rank, because it may be used.
				new_plan.id = num_generated_plans;
				if (new_plan.primary_rank() != numeric_limits<float>::infinity()
					&& (generated_plans[current_flaw_order]
						< params->search_limits[current_flaw_order])) {
					if (params->search_algorithm == Parameters::IDA_STAR
						&& new_plan.primary_rank() > f_limit) {
						next_f_limit = min(next_f_limit, new_plan.primary_rank());
						delete &new_plan;
						continue;
					}
					if (!added && static_pred_flaw) {
						num_static++;
					}
					added = true;
					plans[current_flaw_order].push(&new_plan);
					generated_plans[current_flaw_order]++;
					num_generated_plans++;
					if (verbosity > 2) {
						cerr << endl << "####CHILD (id " << new_plan.id << ")"
							<< " with rank (" << new_plan.primary_rank();
						for (size_t ri = 1; ri < new_plan.rank.size(); ri++) {
							cerr << ',' << new_plan.rank[ri];
						}
						cerr << "):" << endl << new_plan << endl;
					}
				}
				else {
					delete &new_plan;
				}
			}
			if (!added) {
				num_dead_ends++;
			}

			// Process next plan.
			bool limit_reached = false;
			if ((limit_reached = (generated_plans[current_flaw_order]
				>= params->search_limits[current_flaw_order]))
				|| generated_plans[current_flaw_order] >= next_switch) {
				if (verbosity > 1) {
					cerr << "time to switch ("
						<< generated_plans[current_flaw_order] << ")" << endl;
				}
				if (limit_reached) {
					flaw_orders_left--;
					// Discard the rest of the plan queue.
					dead_queues.push_back(&plans[current_flaw_order]);
				}
				if (flaw_orders_left > 0) {
					do {
						current_flaw_order++;
						if (verbosity > 1) {
							cerr << "use flaw order "
								<< current_flaw_order << "?" << endl;
						}
						if (current_flaw_order >= params->flaw_orders.size()) {
							current_flaw_order = 0;
							next_switch *= 2;
						}
					} while ((generated_plans[current_flaw_order]
						>= params->search_limits[current_flaw_order]));
					if (verbosity > 1) {
						cerr << "using flaw order " << current_flaw_order
							<< endl;
					}
				}
			}
			if (flaw_orders_left > 0) {
				if (generated_plans[current_flaw_order] == 0) {
					current_plan = initial_plan;
					generated_plans[current_flaw_order]++;
					num_generated_plans++;
				}
				else {
					if (current_plan != initial_plan) {
						delete current_plan;
					}
					if (plans[current_flaw_order].empty()) {
						// Problem lacks solution.
						current_plan = NULL;
					}
					else {
						current_plan = plans[current_flaw_order].top();
						plans[current_flaw_order].pop();
					}
				}
				// Instantiate all actions if the plan is otherwise complete.
	
				bool instantiated = params->ground_actions;
				while (current_plan != NULL && current_plan->is_complete()
					&& !instantiated) {
					const Bindings* new_bindings =
						step_instantiation(current_plan->get_steps(), 0,
							*current_plan->bindings);
					if (new_bindings != NULL) {
						instantiated = true;
						if (new_bindings != current_plan->bindings) {
							const Plan* inst_plan =
								new Plan(current_plan->get_steps(), current_plan->get_num_steps(),
									current_plan->get_links(), current_plan->get_num_links(),
									current_plan->get_orderings(), *new_bindings,
									NULL, 0, NULL, 0, NULL, current_plan);
							delete current_plan;
							current_plan = inst_plan;
						}
					}
					else if (plans[current_flaw_order].empty()) {
						// Problem lacks solution.
						current_plan = NULL;
					}
					else {
						current_plan = plans[current_flaw_order].top();
						plans[current_flaw_order].pop();
					}
				}
			}
			else {
				if (next_f_limit != numeric_limits<float>::infinity()) {
					current_plan = NULL;
				}
				break;
			}
		}
		if (current_plan != NULL && current_plan->is_complete()) {
			break;
		}
		f_limit = next_f_limit;
		if (f_limit != numeric_limits<float>::infinity()) {
			// Restart search.
			if (current_plan != NULL && current_plan != initial_plan) {
				delete current_plan;
			}
			current_plan = initial_plan;
		}
	} while (f_limit != numeric_limits<float>::infinity());
	if (verbosity > 0) {
		
		// Print statistics.
		
		cerr << endl << "Plans generated: " << num_generated_plans;
		if (num_static > 0) {
			cerr << " [" << (num_generated_plans - num_static) << "]";
		}
		cerr << endl << "Plans visited: " << num_visited_plans;
		if (num_static > 0) {
			cerr << " [" << (num_visited_plans - num_static) << "]";
		}
		cerr << endl << "Dead ends encountered: " << num_dead_ends
			<< endl;
	}
	
	// Discard the rest of the plan queue and some other things, unless
	// this is the last problem in which case we can save time by just
	// letting the operating system reclaim the memory for us.
	
	if (!last_problem) {
		if (current_plan != initial_plan) {
			delete initial_plan;
		}
		for (size_t i = 0; i < plans.size(); i++) {
			while (!plans[i].empty()) {
				delete plans[i].top();
				plans[i].pop();
			}
		}
	}
	// Return last plan, or NULL if problem does not have a solution.
	return current_plan;
}

// Clean up after planning.
void Plan::cleanup() {
	if (planning_graph != NULL) {
		delete planning_graph;
		planning_graph = NULL;
	}
	if (goal_action != NULL) {
		delete goal_action;
		goal_action = NULL;
	}
}

// Destruct this plan.
Plan::~Plan() {
#ifdef DEBUG_MEMORY
	deleted_plans++;
#endif
	RCObject::destructive_deref(steps);
	RCObject::destructive_deref(links);
	Orderings::unregister_use(orderings);
	Bindings::unregister_use(bindings);
	RCObject::destructive_deref(unsafes);
	RCObject::destructive_deref(open_conds);
	RCObject::destructive_deref(mutex_threats);
}

// Return the bindings of this plan.
const Bindings* Plan::get_bindings() const {
	return params->ground_actions ? NULL : bindings;
}

// Check if this plan is complete.
bool Plan::is_complete() const {
	return get_unsafes() == NULL && get_open_conds() == NULL && get_mutex_threats() == NULL;
}

// Return the primary rank of this plan, where a lower rank signifies a better plan.
float Plan::primary_rank() const {
	if (rank.empty()) {
		params->heuristic.plan_rank(rank, *this, params->weight, *domain,
			planning_graph);
	}
	return rank[0];
}

// Return the serial number of this plan.
size_t Plan::get_serial_no() const {
	return id;
}

// Count the number of refinements for the given threat, and returns true iff the number of refinements does not exceed the given limit.
bool Plan::unsafe_refinements(int& refinements, int& separable,
	int& promotable, int& demotable,
	const Unsafe& unsafe, int limit) const {
	if (refinements >= 0) {
		return refinements <= limit;
	}
	else {
		int ref = 0;
		BindingList unifier;
		const Link& link = unsafe.get_link();
		StepTime lt1 = link.get_effect_time();
		StepTime lt2 = end_time(link.get_condition_time());
		StepTime et = end_time(unsafe.get_effect());
		if (get_orderings().possibly_not_after(link.get_from_id(), lt1,
			unsafe.get_step_id(), et)
			&& get_orderings().possibly_not_before(link.get_to_id(), lt2,
				unsafe.get_step_id(), et)
			&& bindings->affects(unifier, unsafe.get_effect().get_literal(),
				unsafe.get_step_id(),
				link.get_condition(), link.get_to_id())) {
			PlanList dummy;
			if (separable < 0) {
				separable = separate(dummy, unsafe, unifier, true);
			}
			ref += separable;
			if (ref <= limit) {
				if (promotable < 0) {
					promotable = promote(dummy, unsafe, true);
				}
				ref += promotable;
				if (ref <= limit) {
					if (demotable < 0) {
						demotable = demote(dummy, unsafe, true);
					}
					refinements = ref + demotable;
					return refinements <= limit;
				}
			}
			return false;
		}
		else {
			separable = promotable = demotable = 0;
			refinements = 1;
			return refinements <= limit;
		}
	}
}

// Check if the given threat is separable.
int Plan::is_separable(const Unsafe& unsafe) const {
	BindingList unifier;
	const Link& link = unsafe.get_link();
	StepTime lt1 = link.get_effect_time();
	StepTime lt2 = end_time(link.get_condition_time());
	StepTime et = end_time(unsafe.get_effect());
	if (get_orderings().possibly_not_after(link.get_from_id(), lt1,
		unsafe.get_step_id(), et)
		&& get_orderings().possibly_not_before(link.get_to_id(), lt2,
			unsafe.get_step_id(), et)
		&& bindings->affects(unifier,
			unsafe.get_effect().get_literal(),
			unsafe.get_step_id(),
			link.get_condition(), link.get_to_id())) {
		PlanList dummy;
		return separate(dummy, unsafe, unifier, true);
	}
	else {
		return 0;
	}
}

// Check if the given open condition is threatened.
bool Plan::is_unsafe_open_condition(const OpenCondition& open_cond) const {
	const Literal* literal = open_cond.literal();
	if (literal != NULL) {
		const Literal& goal = *literal;
		StepTime gt = end_time(open_cond.get_when());
		for (const Chain<Step>* sc = get_steps(); sc != NULL; sc = sc->tail) {
			const Step& s = sc->head;
			if (get_orderings().possibly_not_before(open_cond.get_step_id(), gt,
				s.get_id(), StepTime::AT_START)) {
				const EffectList& effects = s.get_action().get_effects();
				for (EffectList::const_iterator ei = effects.begin();
					ei != effects.end(); ei++) {
					const Effect& e = **ei;
					StepTime et = end_time(e);
					if (get_orderings().possibly_not_before(open_cond.get_step_id(), gt,
						s.get_id(), et)
						&& bindings->affects(e.get_literal(), s.get_id(),
							goal, open_cond.get_step_id())) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

// Count the number of refinements for the given open condition, and returns true iff the number of refinements does not exceed the given limit.
bool Plan::open_cond_refinements(int& refinements, int& addable, int& reusable,
	const OpenCondition& open_cond, int limit) const {
	if (refinements >= 0) {
		return refinements <= limit;
	}
	else {
		const Literal* literal = open_cond.literal();
		if (literal != NULL) {
			int ref = 0;
			if (addable < 0) {
				if (!addable_steps(addable, *literal, open_cond, limit)) {
					return false;
				}
			}
			ref += addable;
			if (ref <= limit) {
				if (reusable < 0) {
					if (!reusable_steps(reusable, *literal, open_cond, limit)) {
						return false;
					}
				}
				refinements = ref + reusable;
				return refinements <= limit;
			}
		}
		else {
			PlanList dummy;
			const Disjunction* disj = open_cond.disjunction();
			if (disj != NULL) {
				refinements = handle_disjunction(dummy, *disj, open_cond, true);
				return refinements <= limit;
			}
			else {
				const Inequality* neq = open_cond.inequality();
				if (neq != NULL) {
					refinements = handle_inequality(dummy, *neq, open_cond, true);
				}
				else {
					throw logic_error("unknown kind of open condition");
				}
			}
		}
	}
	return false;
}

// Count the number of add-step refinements for the given literal open condition, and returns true iff the number of refinements does not exceed the given limit.
bool Plan::addable_steps(int& refinements, const Literal& literal,
	const OpenCondition& open_cond, int limit) const {
	int count = 0;
	PlanList dummy;
	const ActionEffectMap* achievers = literal_achievers(literal);
	if (achievers != NULL) {
		for (ActionEffectMap::const_iterator ai = achievers->begin();
			ai != achievers->end(); ai++) {
			const Action& action = *(*ai).first;
			if (action.get_name().substr(0, 1) != "<") {
				const Effect& effect = *(*ai).second;
				count += new_link(dummy, Step(get_num_steps() + 1, action), effect,
					literal, open_cond, true);
				if (count > limit) {
					return false;
				}
			}
		}
	}
	refinements = count;
	return count <= limit;
}

// Count the number of reuse-step refinements for the given literal open condition, and returns true iff the number of refinements does not exceed the given limit.
bool Plan::reusable_steps(int& refinements, const Literal& literal,
	const OpenCondition& open_cond, int limit) const {
	int count = 0;
	PlanList dummy;
	const ActionEffectMap* achievers = literal_achievers(literal);
	if (achievers != NULL) {
		StepTime gt = start_time(open_cond.get_when());
		for (const Chain<Step>* sc = get_steps(); sc != NULL; sc = sc->tail) {
			const Step& step = sc->head;
			if (get_orderings().possibly_before(step.get_id(), StepTime::AT_START,
				open_cond.get_step_id(), gt)) {
				pair<ActionEffectMap::const_iterator,
					ActionEffectMap::const_iterator> b =
					achievers->equal_range(&step.get_action());
				for (ActionEffectMap::const_iterator ei = b.first;
					ei != b.second; ei++) {
					const Effect& effect = *(*ei).second;
					StepTime et = end_time(effect);
					if (get_orderings().possibly_before(step.get_id(), et,
						open_cond.get_step_id(), gt)) {
						count += new_link(dummy, step, effect, literal, open_cond, true);
						if (count > limit) {
							return false;
						}
					}
				}
			}
		}
	}
	const Negation* negation = dynamic_cast<const Negation*>(&literal);
	if (negation != NULL) {
		count += new_cw_link(dummy, problem->get_init_action().get_effects(),
			*negation, open_cond, true);
	}
	refinements = count;
	return count <= limit;
}



// Less-than operator for plans.
bool operator<(const Plan& p1, const Plan& p2) {
	float diff = p1.primary_rank() - p2.primary_rank();
	for (size_t i = 1; i < p1.rank.size() && diff == 0.0; i++) {
		diff = p1.rank[i] - p2.rank[i];
	}
	return diff > 0.0;
}

// Output operator for plans.
ostream& operator<<(ostream& os, const Plan& p) {
	const Step* init = NULL;
	const Step* goal = NULL;
	const Bindings* bindings = p.bindings;
	vector<const Step*> ordered_steps;
	for (const Chain<Step>* sc = p.get_steps(); sc != NULL; sc = sc->tail) {
		const Step& step = sc->head;
		if (step.get_id() == 0) {
			init = &step;
		}
		else if (step.get_id() == Plan::GOAL_ID) {
			goal = &step;
		}
		else {
			ordered_steps.push_back(&step);
		}
	}
	map<size_t, float> start_times;
	map<size_t, float> end_times;
	float makespan = p.get_orderings().schedule(start_times, end_times);
	sort(ordered_steps.begin(), ordered_steps.end(), StepSorter(start_times));

	// not dealing with link conditions on effects
	if (verbosity < 2) {
		cerr << "Makespan: " << makespan << endl;
		bool first = true;
		for (vector<const Step*>::const_iterator si = ordered_steps.begin();
			si != ordered_steps.end(); si++) {
			const Step& s = **si;
			if (s.get_action().get_name().substr(0, 1) != "<") {
				if (verbosity > 0 || !first) {
					os << endl;
				}
				first = false;
				os << start_times[s.get_id()] << ':';
				s.get_action().print(os, s.get_id(), *bindings);
				if (s.get_action().is_durative()) {
					os << '[' << (end_times[s.get_id()] - start_times[s.get_id()]) << ']';
				}
			}
		}
	}
	else {
		os << "Initial  :";
		const EffectList& effects = init->get_action().get_effects();
		for (EffectList::const_iterator ei = effects.begin();
			ei != effects.end(); ei++) {
			os << ' ';
			(*ei)->get_literal().print(os, 0, *bindings);
		}
		ordered_steps.push_back(goal);
		for (vector<const Step*>::const_iterator si = ordered_steps.begin();
			si != ordered_steps.end(); si++) {
			const Step& step = **si;
			if (step.get_id() == Plan::GOAL_ID) {
				os << endl << endl << "Goal     : ";
			}
			else {
				os << endl << endl << "Step " << step.get_id();
				if (step.get_id() < 100) {
					if (step.get_id() < 10) {
						os << ' ';
					}
					os << ' ';
				}
				os << " : ";
				step.get_action().print(os, step.get_id(), *bindings);
				for (const Chain<MutexThreat>* mc = p.get_mutex_threats();
					mc != NULL; mc = mc->tail) {
					const MutexThreat& mt = mc->head;
					if (mt.get_step_id1() == step.get_id()) {
						os << " <" << mt.get_step_id2() << '>';
					}
					else if (mt.get_step_id2() == step.get_id()) {
						os << " <" << mt.get_step_id1() << '>';
					}
				}
			}
			for (const Chain<Link>* lc = p.get_links(); lc != NULL; lc = lc->tail) {
				const Link& link = lc->head;
				if (link.get_to_id() == step.get_id()) {
					os << endl << "          " << link.get_from_id();
					if (link.get_from_id() < 100) {
						if (link.get_from_id() < 10) {
							os << ' ';
						}
						os << ' ';
					}
					os << " -> ";
					link.get_condition().print(os, link.get_to_id(), *bindings);
					for (const Chain<Unsafe>* uc = p.get_unsafes();
						uc != NULL; uc = uc->tail) {
						const Unsafe& unsafe = uc->head;
						if (unsafe.get_link() == link) {
							os << " <" << unsafe.get_step_id() << '>';
						}
					}
				}
			}
			for (const Chain<OpenCondition>* occ = p.get_open_conds();
				occ != NULL; occ = occ->tail) {
				const OpenCondition& open_cond = occ->head;
				if (open_cond.get_step_id() == step.get_id()) {
					os << endl << "           ?? -> ";
					open_cond.get_condition().print(os, open_cond.get_step_id(), *bindings);
				}
			}
		}
		os << endl << "orderings = " << p.get_orderings();
		if (p.get_bindings() != NULL) {
			os << endl << "bindings = ";
			bindings->print(os);
		}
	}
	return os;
}