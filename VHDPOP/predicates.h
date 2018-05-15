#pragma once

#include "types.h"
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

// A predicate.
class Predicate {
	// Predicate index.
	int index;

	friend bool operator==(const Predicate& p1, const Predicate& p2);
	friend bool operator!=(const Predicate& p1, const Predicate& p2);
	friend bool operator<(const Predicate& p1, const Predicate& p2);
	friend ostream& operator<<(ostream& os, const Predicate& p);
	friend class PredicateTable;

public:
	explicit Predicate(int index) :index(index) {}
};

// Equality operator for predicates.
inline bool operator==(const Predicate& p1, const Predicate& p2) {
	return p1.index == p2.index;
}

// Inequality operator for predicates.
inline bool operator!=(const Predicate& p1, const Predicate& p2) {
	return p1.index != p2.index;
}

// Less-than operator for predicates.
inline bool operator<(const Predicate& p1, const Predicate& p2) {
	return p1.index < p2.index;
}

// Set of predicates.
class PredicateSet :public set<Predicate> {};

// Predicate table.
class PredicateTable {
	// Predicate names.
	static vector<string> names;
	// Predicate parameters. (A type matrix/vector of vector.)
	static vector<TypeList> parameters;
	// Static predicates.
	static PredicateSet static_predicates;

	// Mapping of predicate names to predicates.
	map<string, Predicate> predicates;

	friend ostream& operator<<(ostream& os, const PredicateTable& t);
	friend ostream& operator<<(ostream& os, const Predicate& p);

public:
	// Add a parameter with the given type to the given predicate.
	static void add_parameter(const Predicate& predicate, const Type& type);

	// Return the name of the given predicate.
	static const string& get_name(const Predicate& predicate);

	// Return the parameter types of the given predicate.
	static const TypeList& get_parameters(const Predicate& predicate);

	// Make the given predicate dynamic.
	static void make_dynamic(const Predicate& predicate);

	// Test if the given predicate is static.
	static bool is_static(const Predicate& predicate);

	// Add a predicate with the given name to the table and return it.
	const Predicate& add_predicate(const string& name);

	// Return a pointer to the predicate with the given name, or 0 if no predicate with the given name exists.
	const Predicate* find_predicate(const string& name) const;
};

