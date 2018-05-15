#include "predicates.h"


// ================= Declaration of the static variable members in PredicateTable. =======================
// Predicate names.
vector<string> PredicateTable::names;
// Predicate parameters.
vector<TypeList> PredicateTable::parameters;
// Static predicates.
PredicateSet PredicateTable::static_predicates;

// Output operator for predicates. 
ostream& operator<<(ostream& os, const Predicate& p) {
	os << PredicateTable::names[p.index];
	return os;
}

//================ Predicate Table ========================
// Add a parameter with the given type to the given predicate.
void PredicateTable::add_parameter(const Predicate& predicate, const Type& type) {
	parameters[predicate.index].push_back(type);
}

// Return the name of the given predicate.
const string& PredicateTable::get_name(const Predicate& predicate) {
	return names[predicate.index];
}

// Return the parameter types of the given predicate.
const TypeList& PredicateTable::get_parameters(const Predicate& predicate) {
	return parameters[predicate.index];
}

// Make the given predicate dynamic. (Remove from static predicates set.)
void PredicateTable::make_dynamic(const Predicate& predicate) {
	static_predicates.erase(predicate);
}

// Test if the given predicate is static.
bool PredicateTable::is_static(const Predicate& predicate) {
	return static_predicates.find(predicate) != static_predicates.end();
}

// Add a predicate with the given name to the table and return it.
const Predicate& PredicateTable::add_predicate(const string& name) {
	pair<map<string, Predicate>::const_iterator, bool> pi =
		predicates.insert(make_pair(name, Predicate(names.size())));
	const Predicate& predicate = (*pi.first).second;
	names.push_back(name);
	parameters.push_back(TypeList()); // At this point, just add no parameters.
	static_predicates.insert(predicate); // Initially, all predicates are taken as static by default.
	return predicate;
}

// Return a pointer to the predicate with the given name, or 0 if no predicate with the given name exists.
const Predicate* PredicateTable::find_predicate(const string& name) const {
	map<string, Predicate>::const_iterator pi = predicates.find(name);
	if (pi != predicates.end()) {
		return &(*pi).second;
	}
	else {
		return 0;
	}
}


// Output operator for predicate tables.
ostream& operator<<(ostream& os, const PredicateTable& pt) {
	for (map<string, Predicate>::const_iterator pi =
		pt.predicates.begin();
		pi != pt.predicates.end(); ++pi) {
		const Predicate& p = (*pi).second;
		os << endl << "  (" << p;
		const TypeList& types = PredicateTable::get_parameters(p);
		for (TypeList::const_iterator ti = types.begin();
			ti != types.end(); ti++) {
			os << " ?v - " << *ti;
		}
		os << ")";
		if (PredicateTable::is_static(p)) {
			os << " <static>";
		}
	}
	return os;
}