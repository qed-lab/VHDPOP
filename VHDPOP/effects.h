#pragma once

#include "terms.h"
#include <vector>

using namespace std;

class Formula;
class Literal;
class Problem;


// =================== Effect ======================

class EffectList;

// Possible temporal annotations for an effect.
typedef enum { AT_START, AT_END } EffectTime;

// An effect
class Effect {

	// A list (vector) of universally quantified variable for this effect.
	VariableList parameters;

	// Condition for this effect, or TRUE_FORMULA if it's unconditional effect.
	const Formula* condition;

	// Condition that must hold for this effect to be considered for linking.
	mutable const Formula* link_condition;

	// Literal added by this effect.
	const Literal* literal;

	// Temporal annotation for this effect.
	EffectTime when;

	// Return an instantiation of this effect.
	const Effect* get_instantiation(const SubstitutionMap& args,
		const Problem& problem, const Formula& condition) const;

public:
	
	// Construct an effect.
	Effect(const Literal& literal, EffectTime when);

	// Destruct an effect.
	~Effect();

	// Add a universally quantified variable to this effect.
	void add_parameter(Variable parameter);

	// Set the condition of this effect.
	void set_condition(const Formula& condition);

	// Set the link condition of this effect.
	void set_link_condition(const Formula& link_condition) const;

	// Return the number of universally quantified variables of this effect.
	size_t get_arity() const { return parameters.size(); }

	// Return the ith universally quantified variable of this effect.
	Variable get_parameter(size_t i) const { return parameters[i]; }

	// Return the condition for this effect.
	const Formula& get_condition() const { return *condition; }

	// Return the condition that must hold for this effect to be considered for linking.
	const Formula& get_link_condition() const { return *link_condition; }

	// Return the literal added by this effect.
	const Literal& get_literal() const { return *literal; }

	// Return the temporal annotation for this effect.
	EffectTime get_when() const { return when; }

	// Test if this effect quantifies the given variable.
	bool quantifies(Variable variable) const;

	// Fill the provided list with instantiations of this effect.
	void instantiations(EffectList& effects, size_t& useful,
		const SubstitutionMap& subst,
		const Problem& problem) const;

	// Print this effect on the given stream.
	void print(ostream& os) const;

};

// List (vector) of (pointers to) effects.
class EffectList :public vector<const Effect*> {
};