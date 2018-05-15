#include "problems.h"

#include "bindings.h"

#include <sstream>

// =================== Problem ======================

Problem::ProblemMap Problem::problems = Problem::ProblemMap();

// Return a const_iterator pointing to the first problem. 
Problem::ProblemMap::const_iterator Problem::begin() {
	return problems.begin();
}


// Return a const_iterator pointing beyond the last problem. 
Problem::ProblemMap::const_iterator Problem::end() {
	return problems.end();
}


// Return the problem with the given name, or NULL if it is undefined. 
const Problem* Problem::find(const string& name) {
	ProblemMap::const_iterator pi = problems.find(name);
	return (pi != problems.end()) ? (*pi).second : NULL;
}


// Removes all defined problems. 
void Problem::clear() {
	ProblemMap::const_iterator pi = begin();
	while (pi != end()) {
		delete (*pi).second;
		pi = begin();
	}
	problems.clear();
}


// Construct a problem. 
Problem::Problem(const string& name, const Domain& domain)
	: name(name), domain(&domain), terms(TermTable(domain.get_terms())),
	init_action(GroundAction("<init 0>", false)), goal(&Formula::TRUE_FORMULA),
	metric(new Value(0)) {
	Formula::register_use(goal);
	RCObject::ref(metric);
	const Problem* p = find(name);
	if (p != NULL) {
		delete p;
	}
	problems[name] = this;
}


// Destruct a problem. 
Problem::~Problem() {
	problems.erase(get_name());
	for (ValueMap::const_iterator vi = init_values.begin();
		vi != init_values.end(); vi++) {
		RCObject::destructive_deref((*vi).first);
	}
	for (TimedActionTable::const_iterator ai = timed_actions.begin();
		ai != timed_actions.end(); ai++) {
		delete (*ai).second;
	}
	Formula::unregister_use(goal);
	RCObject::destructive_deref(metric);
}


// Add an atomic formula to the initial conditions of this problem. 
void Problem::add_init_atom(const Atom& atom) {
	init_atoms.insert(&atom);
	init_action.add_effect(*new Effect(atom, EffectTime::AT_END));
}


// Add a timed initial literal to this problem. 
void Problem::add_init_literal(float time, const Literal& literal) {
	if (time == 0.0f) {
		const Atom* atom = dynamic_cast<const Atom*>(&literal);
		if (atom != NULL) {
			add_init_atom(*atom);
		}
	}
	else {
		GroundAction* action;
		TimedActionTable::const_iterator ai = timed_actions.find(time);
		if (ai != timed_actions.end()) {
			action = (*ai).second;
		}
		else {
			ostringstream ss;
			ss << "<init " << time << '>';
#if !HAVE_SSTREAM
			ss << '\0';
#endif
			action = new GroundAction(ss.str(), false);
			timed_actions.insert(make_pair(time, action));
		}
		action->add_effect(*new Effect(literal, EffectTime::AT_END));
	}
}


// Add a fluent value to the initial conditions of this problem. 
void Problem::add_init_value(const Fluent& fluent, float value) {
	if (init_values.find(&fluent) == init_values.end()) {
		init_values.insert(make_pair(&fluent, value));
		RCObject::ref(&fluent);
	}
	else {
		init_values[&fluent] = value;
	}
}


// Set the goal of this problem. 
void Problem::set_goal(const Formula& goal) {
	if (this->goal != &goal) {
		Formula::register_use(&goal);
		Formula::unregister_use(this->goal);
		this->goal = &goal;
	}
}


// Set the metric to minimize for this problem. 
void Problem::set_metric(const Expression& metric, bool negate) {
	const Expression* real_metric;
	if (negate) {
		real_metric = &Subtraction::make(*new Value(0), metric);
	}
	else {
		real_metric = &metric;
	}
	const Expression& inst_metric =
		real_metric->get_instantiation(SubstitutionMap(), get_init_values());
	if (&inst_metric != real_metric) {
		RCObject::ref(real_metric);
		RCObject::destructive_deref(real_metric);
		real_metric = &inst_metric;
	}
	if (real_metric != this->metric) {
		RCObject::ref(real_metric);
		RCObject::destructive_deref(this->metric);
		this->metric = real_metric;
	}
}


// Test if the metric is constant. 
bool Problem::constant_metric() const {
	return typeid(get_metric()) == typeid(Value);
}


// Fills the provided action list with ground actions instantiated from the action schemas of the domain. 
void Problem::instantiated_actions(GroundActionList& actions) const {
	for (ActionSchemaMap::const_iterator ai = get_domain().get_actions().begin();
		ai != get_domain().get_actions().end(); ai++) {
		(*ai).second->instantiations(actions, *this);
	}
}

// Output operator for problems.
ostream& operator<<(ostream& os, const Problem& p) {
	os << "name: " << p.get_name();
	os << endl << "domain: " << p.get_domain().get_name();
	os << endl << "objects:" << p.get_terms();
	os << endl << "init:";
	for (AtomSet::const_iterator ai = p.get_init_atoms().begin();
		ai != p.get_init_atoms().end(); ai++) {
		os << ' ';
		(*ai)->print(os, 0, Bindings::EMPTY);
	}
	for (TimedActionTable::const_iterator ai = p.get_timed_actions().begin();
		ai != p.get_timed_actions().end(); ai++) {
		float time = (*ai).first;
		const EffectList& effects = (*ai).second->get_effects();
		for (EffectList::const_iterator ei = effects.begin();
			ei != effects.end(); ei++) {
			os << " (at " << time << ' ';
			(*ei)->get_literal().print(os, 0, Bindings::EMPTY);
			os << ")";
		}
	}
	for (ValueMap::const_iterator vi = p.init_values.begin();
		vi != p.init_values.end(); vi++) {
		os << endl << "  (= " << *(*vi).first << ' ' << (*vi).second << ")";
	}
	os << endl << "goal: ";
	p.get_goal().print(os, 0, Bindings::EMPTY);
	os << endl << "metric: " << p.get_metric();
	return os;
}