#pragma once
class Requirements
{
public:
	// Required action style. 
	bool strips;

	// Whether support for types is required. 
	bool typing;

	// Whether support for negative preconditions is required. 
	bool negative_preconditions;

	// Whether support for disjunctive preconditions is required. 
	bool disjunctive_preconditions;

	// Whether support for equality predicate is required.
	bool equality;

	// Whether support for existentially quantified preconditions is required.
	bool existential_preconditions;

	// Whether support for universally quantified preconditions is required. 
	bool universal_preconditions;

	// Whether support for conditional effects is required. 
	bool conditional_effects;

	// Whether support for durative actions is required. 
	bool durative_actions;

	// Whether support for duration inequalities is required. 
	bool duration_inequalities;

	// Whether support for fluents is required. 
	bool fluents;

	// Whether support for timed initial literals is required. 
	bool timed_initial_literals;

	// Whether support for decompositions is required.
	bool decompositions;

public:
	// Construct a default requirements object.
	Requirements();

	// Enable quantified preconditions.
	void enable_quantified_preconditions();

	// Enable ADL style actions.
	void enable_adl();

	// Enable decompositions.
	void enable_decompositions();
};

