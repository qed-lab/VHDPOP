#pragma once

#include "types.h"
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

// A function
class Function {
	// Function index.
	int index;

	friend bool operator==(const Function& f1, const Function& f2);
	friend bool operator!=(const Function& f1, const Function& f2);
	friend bool operator<(const Function& f1, const Function& f2);
	friend ostream& operator<<(ostream& os, const Function& f);
	friend class FunctionTable;

public:
	// Construct a function.
	explicit Function(int index) :index(index) {}
};

// Equality operator for functions.
inline bool operator==(const Function& f1, const Function& f2) {
	return f1.index == f2.index;
}

// Inequality operator for functions.
inline bool operator!=(const Function& f1, const Function& f2) {
	return f1.index != f2.index;
}

// Less-than operator for functions.
inline bool operator<(const Function& f1, const Function& f2) {
	return f1.index < f2.index;
}

// Set of functions.
class FunctionSet :public set<Function> {};

// Function table.
class FunctionTable {
	// Function names.
	static vector<string> names;
	// Function parameters.
	static vector<TypeList> parameters;
	// Static functions.
	static FunctionSet static_functions;

	// Mapping of function names to functions.
	map<string, Function> functions;

	friend ostream& operator<<(ostream& os, const FunctionTable& t);
	friend ostream& operator<<(ostream& os, const Function& f);

public:
	// Add a parameter with the given type to the given function.
	static void add_parameter(const Function& function, const Type& type);

	// Return the name of the given function.
	static const string& get_name(const Function& function);

	// Return the parameter types of the given function.
	static const TypeList& get_parameters(const Function& function);

	// Make the given function dynamic.
	static void make_dynamic(const Function& function);

	// Test if the given function is static.
	static bool is_static(const Function& function);

	// Add a function with the given name to this table and Return the function.
	const Function& add_function(const string& name);

	// Return a pointer to the function with the given name, or 0 if no function with the given name exists.
	const Function* find_function(const string& name) const;
};