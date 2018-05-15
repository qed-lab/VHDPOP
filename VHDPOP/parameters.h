#pragma once

#include "heuristics.h"
#include <string.h>

#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

// =================== InvalidSearchAlgorithm ====================

// An invalid search algorithm exception.
class InvalidSearchAlgorithm : public runtime_error {
public:
	// Construct an invalid search algorithm exception.
	InvalidSearchAlgorithm(const string& name);
};


// =================== InvalidSearchAlgorithm ====================

// An invalid action cost exception.
class InvalidActionCost : public runtime_error {
public:
	// Constructs an invalid action cost exception.
	InvalidActionCost(const string& name);
};


// =================== InvalidSearchAlgorithm ====================

// Planning parameters.
class Parameters {
public:
	// Valid search algorithms.
	typedef enum { A_STAR, IDA_STAR, HILL_CLIMBING } SearchAlgorithm;
	// Valid action costs.
	typedef enum { UNIT_COST, DURATION, RELATIVE } ActionCost;

	// Time limit, in minutes.
	size_t time_limit;
	// Search algorithm to use.
	SearchAlgorithm search_algorithm;
	// Plan selection heuristic.
	Heuristic heuristic;
	// Action cost.
	ActionCost action_cost;
	// Weight to use with heuristic.
	float weight;
	// Flaw selecion orders.
	vector<FlawSelectionOrder> flaw_orders;
	// Search limits.
	vector<size_t> search_limits;
	// Whether to add open conditions in random order.
	bool random_open_conditions;
	// Whether to use ground actions.
	bool ground_actions;
	// Whether to use parameter domain constraints.
	bool domain_constraints;
	// Whether to keep static preconditions when using domain constraints.
	bool keep_static_preconditions;

	// Construct default planning parameters.
	Parameters();

	// Whether to strip static preconditions.
	bool strip_static_preconditions() const;

	// Select a search algorithm from a name.
	void set_search_algorithm(const string& name);

	// Select an action cost from a name.
	void set_action_cost(const string& name);
};
