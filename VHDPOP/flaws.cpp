#include "flaws.h"

// Constructs an open condition.
OpenCondition::OpenCondition(size_t step_id, const Formula& condition)
	: step_id(step_id), condition(&condition) {
	Formula::register_use(this->condition);
}


// Constructs an open condition.
OpenCondition::OpenCondition(size_t step_id, const Literal& condition,
	FormulaTime when)
	: step_id(step_id), condition(&condition), when(when) {
	Formula::register_use(this->condition);
}


// Copy constructor of an open condition.
OpenCondition::OpenCondition(const OpenCondition& oc)
	: step_id(oc.step_id), condition(oc.condition), when(oc.when) {
	Formula::register_use(this->condition);
}


// Deletes this open condition.
OpenCondition::~OpenCondition() {
	Formula::unregister_use(this->condition);
}


// Checks if this is a static open condition.
bool OpenCondition::is_static() const {
	const Literal* lit = literal();
	return (lit != NULL && get_step_id() != Plan::GOAL_ID
		&& PredicateTable::is_static(lit->get_predicate()));
}


// Return a literal, or NULL if this is not a literal open condition.
const Literal* OpenCondition::literal() const {
	return dynamic_cast<const Literal*>(condition);
}


// Return a inequality, or NULL if this is not an inequality open condition.
const Inequality* OpenCondition::inequality() const {
	return dynamic_cast<const Inequality*>(condition);
}


// Return a disjunction, or NULL if this is not a disjunctive open condition.
const Disjunction* OpenCondition::disjunction() const {
	return dynamic_cast<const Disjunction*>(condition);
}