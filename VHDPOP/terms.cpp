#include "terms.h"
#include <typeinfo>

// Convert the object to a term.
Object::operator Term() const { return Term(index); }

// Convert the variable to a term.
Variable::operator Term() const { return Term(index); }

// ================ Term =================
// Convert the term to an object.  Fail if the term is not an object.
Object Term::as_object() const {
	if (is_object()) {
		return Object(index);
	}
	else {
		throw bad_cast();
	}
}

// Convert the term to a variable.  Fail if the term is not a variable.
Variable Term::as_variable() const {
	if (is_variable()) {
		return Variable(index);
	}
	else {
		throw bad_cast();
	}
}

// Output operator for terms.
ostream& operator<<(ostream& os, const Term& t) {
	if (t.is_object()) {
		os << TermTable::names[t.index];
	}
	else {
		os << "?v" << -t.index;
	}
	return os;
}

// ================ TermTable =================
// Object names. 
vector<string> TermTable::names;

// Object types. 
TypeList TermTable::object_types;

// Variable types. 
TypeList TermTable::variable_types;

// Destructor. Delete the term table.
TermTable::~TermTable() {
	// Delete the object lists in the compatible object queries.
	for (map<Type, const ObjectList*>::const_iterator oi =
		compatible.begin(); oi != compatible.end(); oi++) {
		delete (*oi).second;
	}
}

// Add a fresh variable with the given type to the term table and return it.
Variable TermTable::add_variable(const Type& type) {
	variable_types.push_back(type);
	return Variable(-variable_types.size());
}

// Set the type of the given term.
void TermTable::set_type(const Term& term, const Type& type) {
	if (term.is_object()) {
		object_types[term.index] = type;
	}
	else {
		variable_types[-term.index - 1] = type;
	}
}

// Return the type of the given term.
const Type& TermTable::type(const Term& term) {
	if (term.is_object()) {
		return object_types[term.index];
	}
	else {
		return variable_types[-term.index - 1];
	}
}

// Add an object with the given name and type to this term table and return the object. 
const Object& TermTable::add_object(const string& name, const Type& type) {
	pair<map<string, Object>::const_iterator, bool> oi =
		objects.insert(make_pair(name, Object(names.size())));
	names.push_back(name);
	object_types.push_back(type);
	return (*oi.first).second;
}

// Return the object (pointer) with the given name, or 0 if no object with the given name exists. 
const Object* TermTable::find_object(const string& name) const {
	map<string, Object>::const_iterator oi = objects.find(name);
	if (oi != objects.end()) {
		return &(*oi).second;
	}
	// Find the object in the parent term table.
	else if (parent != 0) {
		return parent->find_object(name);
	}
	else {
		return 0;
	}
}

// Return a list with objects that are compatible with the given type. 
const ObjectList& TermTable::compatible_objects(const Type& type) const {
	map<Type, const ObjectList*>::const_iterator oi =
		compatible.find(type);
	if (oi != compatible.end()) {
		return *(*oi).second;
	}
	else {
		ObjectList* comp_objects;
		if (parent != 0) {
			comp_objects = new ObjectList(parent->compatible_objects(type));
		}
		else {
			comp_objects = new ObjectList();
		}
		for (map<string, Object>::const_iterator oi = objects.begin();
			oi != objects.end(); oi++) {
			const Object& o = (*oi).second;
			if (TypeTable::is_subtype(TermTable::type(o), type)) {
				comp_objects->push_back(o);
			}
		}
		return *comp_objects;
	}
}

// Output operator for term tables.
ostream& operator<<(ostream& os, const TermTable& t) {
	if (t.parent != 0) {
		os << *t.parent;
	}
	for (map<string, Object>::const_iterator oi = t.objects.begin();
		oi != t.objects.end(); oi++) {
		const Object& o = (*oi).second;
		os << endl << "  " << o;
		os << " - " << TermTable::type(o);
	}
	return os;
}