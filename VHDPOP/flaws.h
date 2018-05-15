#pragma once

#include "chain.h"
#include "domains.h"
#include "formulas.h"
#include "plans.h"

class Domain;
class Effect;
class Link;


// An abstract flaw
class Flaw {
public:
	// Print this object on the given stream.
	virtual void print(ostream& os, const Bindings& bindings) const = 0;
};

// An open condition.
class OpenCondition :public Flaw {
	// Id of step to which this open condition belongs.
	size_t step_id;
	// The open condition.
	const Formula* condition;
	// Time stamp associated with a literal open condition.
	FormulaTime when;

public:
	// Construct an open condition.
	OpenCondition(size_t step_id, const Formula& condition);

	// Construct an open condition.
	OpenCondition(size_t step_id, const Literal& condition, FormulaTime when);

	// Construct an open condition.
	OpenCondition(const OpenCondition& oc);

	// Deletes this open condition.
	virtual ~OpenCondition();

	// Return the step id.
	size_t get_step_id() const { return step_id; }

	// Return the open condition.
	const Formula& get_condition() const { return *condition; }

	// Check if this is a static open condition.
	bool is_static() const;

	// Return a literal, or NULL if this is not a literal open condition.
	const Literal* literal() const;

	// Return the time stamp associated with a literal open condition.
	FormulaTime get_when() const { return when; }

	// Return a inequality, or NULL if this is not an inequality open condition.
	const Inequality* inequality() const;

	// Return a disjunction, or NULL if this is not a disjunctive open condition.
	const Disjunction* disjunction() const;

	// Print this object on the given stream.
	virtual void print(ostream& os, const Bindings& bindings) const {
		os << "#<OPEN ";
		get_condition().print(os, get_step_id(), bindings);
		os << ' ' << get_step_id() << ">";
	}
};

// Equality operator for open conditions.
inline bool operator==(const OpenCondition& oc1, const OpenCondition& oc2) {
	return &oc1 == &oc2;
}


// ======================================================================
// Unsafe

// A threatened causal link.
class Unsafe :public Flaw {
	// Threatened link.
	const Link* link;
	// Id of threatening step.
	size_t step_id;
	// Threatening effect.
	const Effect* effect;

public:
	// Construct a threatened causal link.
	Unsafe(const Link& link, size_t step_id, const Effect& effect)
		: link(&link), step_id(step_id), effect(&effect) {}

	// Return the threatened link.
	const Link& get_link() const { return *link; }

	// Return the id of threatening step.
	size_t get_step_id() const { return step_id; }

	// Return the threatening effect.
	const Effect& get_effect() const { return *effect; }

	// Print this object on the given stream.
	virtual void print(ostream& os, const Bindings& bindings) const {
		os << "#<UNSAFE " << get_link().get_from_id() << ' ';
		get_link().get_condition().print(os, get_link().get_to_id(), bindings);
		os << ' ' << get_link().get_to_id() << " step " << get_step_id() << ">";
	}
};

// Equality operator for unsafe links.
inline bool operator==(const Unsafe& u1, const Unsafe& u2) {
	return &u1 == &u2;
}


// ======================================================================
// MutexThreat

// A mutex threat between effects of two separate steps. ???
class MutexThreat : public Flaw {
	// The id for the first step.
	size_t step_id1;
	// The threatening effect for the first step.
	const Effect* effect1;
	// The id for the second step.
	size_t step_id2;
	// The threatening effect for the second step.
	const Effect* effect2;

public:
	// Construct a mutex threat place hoder.
	MutexThreat() : step_id1(0) {}

	// Construct a mutex threat.
	MutexThreat(size_t step_id1, const Effect& effect1,
		size_t step_id2, const Effect& effect2)
		: step_id1(step_id1), effect1(&effect1),
		step_id2(step_id2), effect2(&effect2) {}

	// Return the id for the first step.
	size_t get_step_id1() const { return step_id1; }

	// Return the threatening effect for the first step.
	const Effect& get_effect1() const { return *effect1; }

	// Return the id for the second step.
	size_t get_step_id2() const { return step_id2; }

	// Return the threatening effect for the second step.
	const Effect& get_effect2() const { return *effect2; }

	// Print this object on the given stream.
	virtual void print(ostream& os, const Bindings& bindings) const {
		os << "#<MUTEX " << get_step_id1() << ' ';
		get_effect1().get_literal().print(os, get_step_id1(), bindings);
		os << ' ' << get_step_id2() << ' ';
		get_effect2().get_literal().print(os, get_step_id2(), bindings);
		os << '>';
	}
};

// Equality operator for mutex threats.
inline bool operator==(const MutexThreat& mt1, const MutexThreat& mt2) {
	return &mt1 == &mt2;
}