#include "functions.h"

// Output operator for functions.
ostream& operator<<(ostream& os, const Function& f) {
	os << FunctionTable::names[f.index];
	return os;
}

// ================= FunctionTable ==========================

// ================= Declaration of the static variable members in FunctionTable. =======================
// Function names.
vector<string> FunctionTable::names;
// Function parameters.
vector<TypeList> FunctionTable::parameters;
// Static functions.
FunctionSet FunctionTable::static_functions;


// Add a parameter with the given type to the given function.
void FunctionTable::add_parameter(const Function& function, const Type& type) {
	parameters[function.index].push_back(type);
}

// Return the name of the given function.
const string& FunctionTable::get_name(const Function& function) {
	return names[function.index];
}

// Return the parameter types of the given function.
const TypeList& FunctionTable::get_parameters(const Function& function) {
	return parameters[function.index];
}

// Make the given function dynamic.
void FunctionTable::make_dynamic(const Function& function) {
	static_functions.erase(function);
}

// Test if the given function is static.
bool FunctionTable::is_static(const Function& function) {
	return static_functions.find(function) != static_functions.end();
}

// Add a function with the given name to this table and return the function.
const Function& FunctionTable::add_function(const string& name) {
	pair<map<string, Function>::const_iterator, bool> fi =
		functions.insert(make_pair(name, Function(names.size())));
	const Function& function = (*fi.first).second;
	names.push_back(name);
	parameters.push_back(TypeList());
	static_functions.insert(function);
	return function;
}

// Return a pointer to the function with the given name, or 0 if no function with the given name exists.
const Function* FunctionTable::find_function(const string& name) const {
	map<string, Function>::const_iterator fi = functions.find(name);
	if (fi != functions.end()) {
		return &(*fi).second;
	}
	else {
		return 0;
	}
}

// Output operator for function tables.
ostream& operator<<(ostream& os, const FunctionTable& ft) {
	for (map<string, Function>::const_iterator fi
		= ft.functions.begin(); fi != ft.functions.end(); ++fi) {
		const Function& f = (*fi).second;
		os << endl << " (" << f;
		const TypeList& types = FunctionTable::get_parameters(f);
		for (TypeList::const_iterator ti = types.begin();
			ti != types.end(); ti++) {
			os << " ?v - " << *ti;
		}
		os << ") - " << TypeTable::NUMBER_NAME;
		if (FunctionTable::is_static(f)) {
			os << " <static>";
		}
	}
	return os;
}