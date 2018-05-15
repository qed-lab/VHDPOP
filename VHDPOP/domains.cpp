#include "domains.h"
#include "bindings.h"

//=================== Domain ====================

// Table of defined domains.
DomainMap Domain::domains = DomainMap();

// Construct an empty domain with the given name.
Domain::Domain(const string& name)
	:name(name), total_time(functions.add_function("total-time")) {
	const Domain* d = find(name);
	if (d != NULL) {
		delete d;
	}
	domains[name] = this;
	FunctionTable::make_dynamic(total_time);
}

// Destruct a domain.
Domain::~Domain() {
	domains.erase(get_name());
	for (ActionSchemaMap::const_iterator ai = actions.begin();
		ai != actions.end(); ai++) {
		delete (*ai).second;
	}
}

// Return the domain with the given name, or NULL it is undefined.
const Domain* Domain::find(const string& name) {
	DomainMap::const_iterator di = domains.find(name);
	return (di != domains.end()) ? (*di).second : NULL;
}

// Remove all defined domains.
void Domain::clear() {
	DomainMap::const_iterator di = begin();
	while (di != end()) {
		delete (*di).second;
		di = begin();
	}
	domains.clear();
}

// Adds an action to this domain.
void Domain::add_action(const ActionSchema& action) {
	actions.insert(make_pair(action.get_name(), &action));
}

// Return the action schema with the given name, or NULL if it is undefined.
const ActionSchema* Domain::find_action(const string& name) const {
	ActionSchemaMap::const_iterator ai = actions.find(name);
	return (ai != actions.end()) ? (*ai).second : NULL;
}

// Output operator for domains.
ostream& operator<<(ostream& os, const Domain& d) {
	os << "name: " << d.get_name();
	os << endl << "types:" << d.get_types();
	os << endl << "constants:" << d.get_terms();
	os << endl << "predicates:" << d.get_predicates();
	os << endl << "functions:" << d.get_functions();
	os << endl << "actions:";
	for (ActionSchemaMap::const_iterator ai = d.actions.begin();
		ai != d.actions.end(); ai++) {
		os << endl;
		(*ai).second->print(os);
	}
	return os;
}

