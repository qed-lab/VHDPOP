#include "parameters.h"

// =================== InvalidSearchAlgorithm ====================

// Construct an invalid search algorithm exception.
InvalidSearchAlgorithm::InvalidSearchAlgorithm(const string& name)
	: runtime_error("invalid search algorithm `" + name + "'") {
}


// =================== InvalidSearchAlgorithm ====================

// Construct an invalid action cost exception.
InvalidActionCost::InvalidActionCost(const string& name)
	: runtime_error("invalid action cost `" + name + "'") {}


// =================== Parameters ====================


// Construct default planning parameters.
Parameters::Parameters()
	: time_limit(UINT_MAX), search_algorithm(A_STAR),
	heuristic("UCPOP"), action_cost(UNIT_COST), weight(1.0),
	random_open_conditions(false), ground_actions(false),
	domain_constraints(false), keep_static_preconditions(true) {
	flaw_orders.push_back(FlawSelectionOrder("UCPOP")),
		search_limits.push_back(UINT_MAX);
}


// Whether to strip static preconditions.
bool Parameters::strip_static_preconditions() const {
	return !ground_actions && domain_constraints && !keep_static_preconditions;
}


// Select a search algorithm from a name.
void Parameters::set_search_algorithm(const string& name) {
	const char* n = name.c_str();
	if (strcasecmp(n, "A") == 0) {
		search_algorithm = A_STAR;
	}
	else if (strcasecmp(n, "IDA") == 0) {
		search_algorithm = IDA_STAR;
	}
	else if (strcasecmp(n, "HC") == 0) {
		search_algorithm = HILL_CLIMBING;
	}
	else {
		throw InvalidSearchAlgorithm(name);
	}
}


// Select an action cost from a name.
void Parameters::set_action_cost(const string& name) {
	const char* n = name.c_str();
	if (strcasecmp(n, "UNIT") == 0) {
		action_cost = UNIT_COST;
	}
	else if (strcasecmp(n, "DURATION") == 0) {
		action_cost = DURATION;
	}
	else if (strcasecmp(n, "RELATIVE") == 0) {
		action_cost = RELATIVE;
	}
	else {
		throw InvalidActionCost(name);
	}
}
