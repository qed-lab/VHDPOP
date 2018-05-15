#pragma once

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

// A type.
class Type {
	// Type index.
	int index;

	friend bool operator==(const Type& t1, const Type& t2);
	friend bool operator!=(const Type& t1, const Type& t2);
	friend bool operator<(const Type& t1, const Type& t2);
	friend ostream& operator<<(ostream& os, const Type& t);
	friend class TypeTable;

public:
	// Constructor of a type.
	explicit Type(int idx) :index(idx) {}

	// Test if it is a simple type.
	bool is_simple() const { return index > 0; }

};

// Equality operator for types.
inline bool operator==(const Type& t1, const Type& t2) {
	return t1.index == t2.index;
}

// Inequality operator for types.
inline bool operator!=(const Type& t1, const Type& t2) {
	return t1.index != t2.index;
}

// Less than operator for types.
inline bool operator<(const Type& t1, const Type& t2) {
	return t1.index < t2.index;
}


// Vector of types.
class TypeList :public vector<Type> {};

// Set of types. (Ranking by type index.)
class TypeSet :public set<Type> {};

// A type table.
class TypeTable {
	// Type names.
	static vector<string> names;
	// Transitive closure of subtype relation.
	static vector<vector<bool> > subtype;
	// Union types. (A vector of the set of types.)
	static vector<TypeSet> utypes;

	// Mapping of type names to types.
	map<string, Type> types;

	friend ostream& operator<<(ostream& os, const Type& t);
	friend ostream& operator<<(ostream& os, const TypeTable& tt);

public:
	// The object type.
	static const Type OBJECT;
	// Name of object type.
	static const string OBJECT_NAME;
	// Name of number type.
	static const string NUMBER_NAME;

	// Add a union type of the given types to this table and Return the union type.
	static Type union_type(const TypeSet& types);

	// Add the second type as a supertype of the first type.  Return false if the second type is a proper subtype of the first type.
	static bool add_supertype(const Type& t1, const Type& t2);

	// Test if the first type is a subtype of the second type.
	static bool is_subtype(const Type& t1, const Type& t2);

	// Test if the given types are compatible.
	static bool is_compatible(const Type& t1, const Type& t2);

	// Fill the provided set with the components of the given type.
	static void components(TypeSet& components, const Type& type);

	// Return a pointer to the most specific of the given types, or 0 if the two types are incompatible.
	static const Type* most_specific(const Type& t1, const Type& t2);

	// Add a simple type with the given name to this table and return the type.
	const Type& add_type(const string& name);

	// Return a pointer to the type with the given name, or 0 if no type with the given name exists in this table.
	const Type* find_type(const string& name) const;

};
