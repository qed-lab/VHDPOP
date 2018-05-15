#pragma once

#include "types.h"
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

// An object.

class Term;

// An object.
class Object {
	// Object index.
	int index;

 public:
	// Construct an object with an index.
	explicit Object(int idx) : index(idx) {}

	// Convert the object to a term.
	operator Term() const;
};


// A variable.
class Variable {
	// Variable index.
	int index;
public:
	// Construct a variable.
	explicit Variable(int idx) : index(idx) {}

	// Convert the variable to a term.
	operator Term() const;
};


// A term
class Term {
	
	// Term index.
	int index;

	friend bool operator==(const Term& t1, const Term& t2);
	friend bool operator!=(const Term& t1, const Term& t2);
	friend bool operator<(const Term& t1, const Term& t2);
	friend ostream& operator<<(ostream& os, const Term& t);
	friend class TermTable;

public:
	// Construct a term with an index.
	explicit Term(int idx) : index(idx) {}

	// Test if this term is an object.
	bool is_object() const { return index >= 0; }

	// Test if this term is a variable.
	bool is_variable() const { return index < 0; }

	// Convert the term to an object.  Fail if the term is not an object.
	Object as_object() const;

	// Convert the term to a variable.  Fail if the term is not a variable.
	Variable as_variable() const;
};

// Equality operator for terms.
inline bool operator==(const Term& t1, const Term& t2) {
	return t1.index == t2.index;
}

// Inequality operator for terms.
inline bool operator!=(const Term& t1, const Term& t2) {
	return t1.index == t2.index;
}

// Less-than operator for terms.
inline bool operator<(const Term& t1, const Term& t2) {
	return t1.index < t2.index;
}

// Output operator for terms.
ostream& operator<<(ostream& os, const Term& t);

// Variable substitutions map. (Mapping from variables to terms.)
class SubstitutionMap : public map<Variable, Term> {
};


// Term list: a vector of terms.
class TermList : public vector<Term> {
};


// Object list: a vector of objects.
class ObjectList : public vector<Object> {
};


// Variable list: a vector of variables.
class VariableList : public vector<Variable> {
};


// Term table.
class TermTable {
	// Object names. 
	static vector<string> names;

	// Object types. 
	static TypeList object_types;

	// Variable types. 
	static TypeList variable_types;

	// Pointer to parent term table. 
	const TermTable* parent;

	// Mapping of object names to objects. 
	map<string, Object> objects;

	// Cached results of compatible objects queries. 
	mutable map<Type, const ObjectList*> compatible;

	friend ostream& operator<<(ostream& os, const TermTable& t);
	friend ostream& operator<<(ostream& os, const Term& t);

public:
	// Construct an empty term table.
	TermTable() : parent(0) {};

	// Construct a term table extending the given term table. 
	TermTable(const TermTable& par) : parent(&par) {}

	// Destructor. Delete the term table.
	~TermTable();
	
	// Add a fresh variable with the given type to the term table and return it.
	static Variable add_variable(const Type& type);

	// Set the type of the given term.
	static void set_type(const Term& term, const Type& type);

	// Return the type of the given term.
	static const Type& type(const Term& term);

	// Add an object with the given name and type to this term table and return the object. 
	const Object& add_object(const string& name, const Type& type);

	// Return the object (pointer) with the given name, or 0 if no object with the given name exists. 
	const Object* find_object(const string& name) const;

	// Return a list with objects that are compatible with the given type. 
	const ObjectList& compatible_objects(const Type& type) const;	
};