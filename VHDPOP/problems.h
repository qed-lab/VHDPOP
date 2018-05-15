#pragma once
#include "domains.h"
#include "expressions.h"
#include "formulas.h"


// =================== Problem ======================

// Problem definition.
class Problem {

	// Name of problem.
	string name;
	// Problem domain.
	const Domain* domain;
	// Problem terms.
	mutable TermTable terms;
	// Initial atoms.
	AtomSet init_atoms;
	// Initial fluent values.
	ValueMap init_values;
	// Aciton representing initial conditions of problem.
	GroundAction init_action;
	// Actions representing timed initial conditions of problem.
	TimedActionTable timed_actions;
	// Goal of problem.
	const Formula* goal;
	// Metric to minimize.
	const Expression* metric;

	friend ostream& operator<<(ostream& os, const Problem& p);

public:
	// Table of problem definitions.
	class ProblemMap : public map<string, const Problem*> {
	};

	// Return a const_iterator pointing to the first problem.
	static ProblemMap::const_iterator begin();

	// Return a const_iterator pointing beyond the last problem.
	static ProblemMap::const_iterator end();

	// Return the problem with the given name, or NULL if it is undefined.
	static const Problem* find(const string& name);

	// Remove all defined problems.
	static void clear();

	// Constructs a problem.
	Problem(const string& name, const Domain& domain);

	// Deletes a problem.
	~Problem();

	// Return the name of this problem.
	const string& get_name() const { return name; }

	// Return the domain of this problem.
	const Domain& get_domain() const { return *domain; }

	// Return the term table of this problem. //???
	TermTable& get_terms() { return terms; }

	// Return the term table of this problem.
	const TermTable& get_terms() const { return terms; }

	// Add an atomic formula to the initial conditions of this problem.
	void add_init_atom(const Atom& atom);

	// Add a timed initial literal to this problem.
	void add_init_literal(float time, const Literal& literal);

	// Add a fluent value to the initial conditions of this problem.
	void add_init_value(const Fluent& fluent, float value);

	// Set the goal of this problem.
	void set_goal(const Formula& goal);

	// Set the metric to minimize for this problem.
	void set_metric(const Expression& metric, bool negate = false);

	// Return the initial atoms of this problem.
	const AtomSet& get_init_atoms() const { return init_atoms; }

	// Return the initial values of this problem.
	const ValueMap& get_init_values() const { return init_values; }

	// Return the action representing the initial conditions of this problem.
	const GroundAction& get_init_action() const { return init_action; }

	// Return the actions representing the timed initial literals.
	const TimedActionTable& get_timed_actions() const { return timed_actions; }

	// Return the goal of this problem.
	const Formula& get_goal() const { return *goal; }

	// Return the metric to minimize for this problem.
	const Expression& get_metric() const { return *metric; }

	// Test if the metric is constant.
	bool constant_metric() const;

	// Fill the provided action list with ground actions instantiated from the action schemas of the domain.
	void instantiated_actions(GroundActionList& actions) const;

private:
	// Table of defined problems.
	static ProblemMap problems;
};
