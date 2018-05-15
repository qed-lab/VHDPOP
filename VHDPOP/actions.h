#pragma once
#include "effects.h"

class Expression;
class Domain;
class Bindings;


// =================== Action ======================

// An abstract action.
class Action {

	// Next action id.
	static size_t next_id;

	// Unique id for actions.
	size_t id;
	// Name of this action.
	string name;
	// Action condition.
	const Formula* condition;
	// List of action effects.
	EffectList effects;
	// Whether this is a durative action.
	bool durative;
	// Minimum duration of this action.
	const Expression* min_duration;
	// Maximum duration of this action.
	const Expression* max_duration;

protected:
	// Construct an action with the given name.
	Action(const string& name, bool durative);

public:
	// Destruct this action.
	virtual ~Action();

	// Set the condition for this action.
	void set_condition(const Formula& condition);

	// Add an effect to this action.
	void add_effect(const Effect& effect);

	// Set the minimum duration for this action.
	void set_min_duration(const Expression& min_duration);

	// Set the maximum duration for this action.
	void set_max_duration(const Expression& max_duration);

	// Set the duration for this action.
	void set_duration(const Expression& duration);

	// Return the id for this action.
	size_t get_id() const { return id; }

	// Return the name of this action.
	const string& get_name() const { return name; }

	// Return the condition of this action.
	const Formula& get_condition() const { return *condition; }

	// List of action effects.
	const EffectList& get_effects() const { return effects; }

	// Whether this is a durative action.
	bool is_durative() const { return durative; }

	// Minimum duration of this action.
	const Expression& get_min_duration() const { return *min_duration; }

	// Maximum duration of this action.
	const Expression& get_max_duration() const { return *max_duration; }

	// "Strengthen" the effects of this action.
	void strengthen_effects(const Domain& domain);

	// Print this action on the given stream with the given bindings.
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const = 0;

};


// Less-than specialization object for action pointers.

namespace std {
	template<>
	struct less<const Action*>
		: public binary_function<const Action*, const Action*, bool> {
		// Comparison function operator.
		bool operator()(const Action* a1, const Action* a2) const {
			return a1->get_id() < a2->get_id();
		}
	};
}



// =================== ActionList ======================

// List of action definitions.
class ActionList :public vector<const Action*> {
};


// =================== ActionSchema ======================

class GroundAction;
class GroundActionList;

// Action schema definition.

class ActionSchema : public Action {
	// Action schema parameters.
	VariableList parameters;

	// Return an instantiation of this action schema.
	const GroundAction* get_instantiation(const SubstitutionMap& args,
		const Problem& problem,
		const Formula& condition) const;
public:
	// Construct an action schema with the given name.
	ActionSchema(const string& name, bool durative)
		:Action(name, durative) {}

	// Add a parameter to this action schema.
	void add_parameter(Variable var) {
		parameters.push_back(var);
	}

	// Return the parameters of this action schema.
	const VariableList& get_parameters() const { return parameters; }

	// Fill the provided action list with all instantiations of this action schema.
	void instantiations(GroundActionList& actions, const Problem& problem) const;

	// Print this action on the given stream.
	void print(ostream& os) const;

	// Print this action on the given stream with the given bindings.
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const;
};

// Less-than specialization for action schemas pointers.
namespace std {
	template<>
	struct less<const ActionSchema*> : public less<const Action*> {
	};
}


// =================== ActionSchemaMap ======================

// Table of action schema definitions.
class ActionSchemaMap : public map<string, const ActionSchema*> {
};


// ======================================================================
// GroundAction

// A ground action.
class GroundAction :public Action {
	// Action arguments.
	ObjectList arguments;
public:
	// Construct a ground action with the given name.
	GroundAction(const string& name, bool durative)
		:Action(name, durative) {}

	// Add an argument to this ground action.
	void add_argument(Object arg) {
		arguments.push_back(arg);
	}

	// Action arguments.
	const ObjectList& get_arguments() const { return arguments; }

	// Print this action on the given stream with the given bindings.
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const;
};

// Less-than specialization for ground action pointers.
namespace std {
	template<>
	struct less<const GroundAction*> : public less<const Action*> {
	};
}


// =================== GroundActionList ======================

// A list of ground actions.
class GroundActionList :public vector<const GroundAction*> {
};


// =================== TimedActionTable ======================

// A table of timed actions.
class TimedActionTable :public map<float, GroundAction*> {
};


// =================== ActionEffectMap ======================

// Mapping from actions to effects.
class ActionEffectMap :public multimap<const Action*, const Effect*> {
};



// =================== GroundActionSet ======================

// A set of ground actions.
class GroundActionSet :public set<const GroundAction*> {
};