#include "requirements.h"


// Construct a default requirements object.
Requirements::Requirements()
	:strips(true), typing(false), negative_preconditions(false),
	disjunctive_preconditions(false), equality(false),
	existential_preconditions(false), universal_preconditions(false),
	conditional_effects(false), durative_actions(false),
	duration_inequalities(false), fluents(false),
	timed_initial_literals(false), decompositions(false)
{
}


// Enable quantified preconditions.
void Requirements::enable_quantified_preconditions() {
	existential_preconditions = true;
	universal_preconditions = true;
}

// Enable ADL style actions.
void Requirements::enable_adl() {
	typing = true;
	negative_preconditions = true;
	disjunctive_preconditions = true;
	equality = true;
	enable_quantified_preconditions();
	conditional_effects = true;
}

// Enable decompositions.
void Requirements::enable_decompositions() {
	decompositions = true;
}