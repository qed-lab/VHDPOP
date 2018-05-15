#pragma once
#include "actions.h"
#include "functions.h"
#include "predicates.h"
#include "requirements.h"
#include "terms.h"

class Domain;

//=================== DomainMap ====================

// Table of domain definitions.
class DomainMap : public map<string, const Domain*> {
};


//=================== Domain ====================

// A domain.
class Domain {
	// Table of all defined domains.
	static DomainMap domains;
	// Name of this domain.
	string name;
	// Domain types.
	TypeTable types;
	// Domain predicates.
	PredicateTable predicates;
	// Domain functions.
	FunctionTable functions;
	// The `total-time' function.
	Function total_time;
	// Domain terms.
	TermTable terms;
	// Domain action schemas.
	ActionSchemaMap actions;

	friend ostream& operator<<(ostream& os, const Domain& d);

public:
	// Requirements for this domain.
	Requirements requirements;

	// Construct an empty domain with the given name.
	Domain(const string& name);

	// Destruct a domain.
	~Domain();

	// Return a const_iterator pointing to the first domain.
	static DomainMap::const_iterator begin() {
		return domains.begin();
	}

	// Return a const_iterator pointing beyond the last domain.
	static DomainMap::const_iterator end() {
		return domains.end();
	}

	// Return the domain with the given name, or NULL it is undefined.
	static const Domain* find(const string& name);

	// Remove all defined domains.
	static void clear();

	// Return the name of this domain.
	const string& get_name() const { return name; }

	// Domain actions.
	const ActionSchemaMap& get_actions() const { return actions; }

	// Return the type table of this domain.
	TypeTable& get_types() { return types; }

	// Return the type table of this domain.
	const TypeTable& get_types() const { return types; }

	// Return the predicate table of this domain.
	PredicateTable& get_predicates() { return predicates; }

	// Return the predicate table of this domain.
	const PredicateTable& get_predicates() const { return predicates; }

	// Return the function table of this domain.
	FunctionTable& get_functions() { return functions; }

	// Return the function table of this domain.
	const FunctionTable& get_functions() const { return functions; }

	// Return the `total-time' function.
	const Function& get_total_time() const { return total_time; }

	// Return the term table of this domain.
	TermTable& get_terms() { return terms; }

	// Return the term table of this domain.
	const TermTable& get_terms() const { return terms; }

	// Adds an action to this domain.
	void add_action(const ActionSchema& action);

	// Return the action schema with the given name, or NULL if it is undefined.
	const ActionSchema* find_action(const string& name) const;
};

