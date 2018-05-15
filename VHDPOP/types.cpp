#include "types.h"
#include <stdexcept>

// ================= Declaration of the static variable members in TypeTable. =======================
// The object type. (Index 0.)
const Type TypeTable::OBJECT(0);
// Name of object type.
const string TypeTable::OBJECT_NAME("object");
// Name of number type.
const string TypeTable::NUMBER_NAME("number");

// Type names.
vector<string> TypeTable::names;
// Transitive closure of subtype relation.
vector<vector<bool> > TypeTable::subtype;
// Union types. (A vector of the set of types.)
vector<TypeSet> TypeTable::utypes;

// Output operator for types.
ostream& operator<<(ostream& os, const Type& t) {
	if (!t.is_simple()) {
		const TypeSet& types = TypeTable::utypes[-t.index - 1];
		os << "(either";
		for (TypeSet::const_iterator ti = types.begin(); ti != types.end(); ti++) {
			os << ' ' << TypeTable::names[(*ti).index];
		}
		os << ")";
	}
	else if (t == TypeTable::OBJECT) {
		os << TypeTable::OBJECT_NAME;
	}
	else {
		os << TypeTable::names[t.index - 1];
	}
	return os;
}


// Add a union type of the given types to this table and Return the union type.
Type TypeTable::union_type(const TypeSet& t) {
	// Cannot add an empty set of types.
	if (t.empty()) {
		throw logic_error("empty union type");
	}

	// not push back if only one single type is to be added.
	// index non-negative
	else if (t.size() == 1) {
		return *t.begin();
	}

	// index for a set of types (more than one): negative
	else {
		utypes.push_back(t);
		return Type(-utypes.size());
	}
}

// Add the second type as a supertype of the first type.  Return false if the second type is a proper subtype of the first type.
bool TypeTable::add_supertype(const Type& t1, const Type& t2) {

	// Add all component types of t2 as supertypes of t1, if t2 is not simple.
	if (!t2.is_simple()) {
		const TypeSet& types = utypes[-t2.index - 1];
		for (TypeSet::const_iterator ti = types.begin(); ti != types.end(); ti++) {
			if (!add_supertype(t1, *ti)) {
				return false;
			}
		}
		return true;
	}

	// If t2 is simple and t1 is already a subtype of t2.
	else if (is_subtype(t1, t2)) {
		return true;
	}

	// If t2 is simple and t2 is already a subtype of t1.
	else if (is_subtype(t2, t1)) {
		return false;
	}

	// Make all subtypes of t1 subtypes of all supertypes of t2.
	else {
		size_t n = names.size();
		for (size_t k = 1; k <= n; k++) {
			if (is_subtype(Type(k), t1) && !is_subtype(Type(k), t2)) {
				for (size_t l = 1; l <= n; l++) {
					if (is_subtype(t2, Type(l))) {
						if (k > l) {
							subtype[k - 2][2 * k - l - 2] = true;
						}
						else { // l > k
							subtype[l - 2][k - 1] = true;
						}
					}
				}
			}
		}
		return true;
	}
}

// Test if the first type is a subtype of the second type.
bool TypeTable::is_subtype(const Type& t1, const Type& t2) {
	// Same types are mutually subtypes.
	if (t1 == t2) {
		return true;
	}
	
	// If t1 is not simple,  every component type of t1 must be a subtype of t2.
	else if (!t1.is_simple()) {
		const TypeSet& types = utypes[-t1.index - 1];
		for (TypeSet::const_iterator ti = types.begin(); ti != types.end(); ti++) {
			if (!is_subtype(*ti, t2)) {
				return false;
			}
		}
		return true;
	}
	else if (!t2.is_simple()) {
		// t1 one must be a subtype of some component type of t2. 
		const TypeSet& types = utypes[-t2.index - 1];
		for (TypeSet::const_iterator ti = types.begin(); ti != types.end(); ti++) {
			if (is_subtype(t1, *ti)) {
				return true;
			}
		}
		return false;
	}

	// OBJECT is the supertype of all types!
	else if (t1 == OBJECT) {
		return false;
	}
	else if (t2 == OBJECT) {
		return true;
	}
	else if (t2 < t1) {
		return subtype[t1.index - 2][2 * t1.index - t2.index - 2];
	}
	else {
		return subtype[t2.index - 2][t1.index - 1];
	}
}

// Test if the given types are compatible. (One is the other's sybtype.)
bool TypeTable::is_compatible(const Type& t1, const Type& t2) {
	return is_subtype(t1, t2) || is_subtype(t2, t1);
}

// Fill the provided set with the components of the given type.
// If the given type is not simple, just fill with the composite type; else, insert the simple type.
void TypeTable::components(TypeSet& components, const Type& type) {
	if (!type.is_simple()) {
		components = utypes[-type.index - 1];
	}
	else if (type != OBJECT) {
		components.insert(type);
	}
}

// Return a pointer to the most specific of the given types, or 0 if the two types are incompatible.
const Type* TypeTable::most_specific(const Type& t1, const Type& t2) {
	if (is_subtype(t1, t2)) {
		return &t1;
	}
	else if (is_subtype(t2, t1)) {
		return &t2;
	}
	else {
		return 0;
	}
}

// Add a simple type with the given name to this table and return the type.
const Type& TypeTable::add_type(const string& name) {
	names.push_back(name);
	pair<map<string, Type>::const_iterator, bool> ti =
		types.insert(make_pair(name, names.size()));
	const Type& type = (*ti.first).second;
	
	// Also initialize all the closure to false.
	if (type.index > 1) {
		subtype.push_back(vector<bool>(2 * (type.index - 1), false));
	}
	return type;
}

// Return a pointer to the type with the given name, or 0 if no type with the given name exists in this table.
const Type* TypeTable::find_type(const string& name) const {
	map<string, Type>::const_iterator ti = types.find(name);
	if (ti != types.end()) {
		return &(*ti).second;
	}
	else {
		return 0;
	}
}

// Output operator for type tables.
ostream& operator<<(ostream& os, const TypeTable& tt) {
	for (map<string, Type>::const_iterator ti = tt.types.begin();
		ti != tt.types.end(); ++ti) {
		const Type& t1 = (*ti).second;
		os << endl << "  " << t1;
		bool first = true;
		for (map<string, Type>::const_iterator tj = tt.types.begin();
			tj != tt.types.end(); ++tj) {
			const Type& t2 = (*tj).second;
			if (t1 != t2 && TypeTable::is_subtype(t1, t2)) {
				if (first) {
					os << " <:";
					first = false;
				}
				os << ' ' << t2;
			}
		}
	}
	return os;
}