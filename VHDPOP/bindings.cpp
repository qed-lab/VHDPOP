#include "bindings.h"
//#include "plans.h"
#include "domains.h"
#include "heuristics.h"
#include "problems.h"
#include "formulas.h"
#include <algorithm>
#include <iterator>
#include <stdexcept>

// =================== ActionDomain ======================

// Construct an action domain with a single tuple.
ActionDomain::ActionDomain(const ObjectList& tuple) {
#ifdef DEBUG_MEMORY
	++created_action_domains;
#endif // DEBUG_MEMORY
	add(tuple);
}

// Destruct this action domain.
ActionDomain::~ActionDomain() {
#ifdef DEBUG_MEMORY
	deleted_action_domains++;
#endif // DEBUG_MEMORY
	for (ProjectionMap::const_iterator pi = projections.begin();
		pi != projections.end(); pi++) {
		delete (*pi).second;
	}
}

// Add a tuple to this action domain.
void ActionDomain::add(const ObjectList& tuple) {
	tuples.push_back(&tuple);
}

// Return the set of objects from the given column. ??? what is column
const ObjectSet& ActionDomain::get_projection(size_t column) const {
	ProjectionMap::const_iterator pi = projections.find(column);
	if (pi != projections.end()) {
		return *(*pi).second;
	}
	else {
		ObjectSet* projection = new ObjectSet();
		for (TupleList::const_iterator ti = get_tuples().begin();
			ti != get_tuples().end(); ++ti) {
			const ObjectList& tuple = **ti;
			// should pay attention to the tuple index???
			if (column >= tuple.size()) {
				throw logic_error("Column exceeds the range!");
			}
			projection->insert(tuple[column]);
		}
		projections.insert(make_pair(column, projection));
		return *projection;
	}
}

// Return the size of the projection of the given column.
const size_t ActionDomain::projection_size(size_t column) const {
	return get_projection(column).size();
}

// Return a domain where the given column has been restricted to the given object, or 0 if this would leave an empty domain.
const ActionDomain* ActionDomain::get_restricted_domain(const Object& obj, size_t column) const {
	ActionDomain* new_domain = 0;
	for (TupleList::const_iterator ti = get_tuples().begin();
		ti != get_tuples().end(); ++ti) {
		const ObjectList& tuple = **ti;
		if (column >= tuple.size()) {
			throw logic_error("Column exceeds the range!");
		}
		if (tuple[column] == obj) {
			if (new_domain == 0) {
				new_domain = new ActionDomain(tuple);
			}
			else {
				new_domain->add(tuple);
			}
		}
	}
	if (new_domain != 0 && new_domain->size() == size()) {
		// For storage efficiency???
		ActionDomain::register_use(new_domain);
		ActionDomain::unregister_use(new_domain);
		return this;
	}
	else {
		return new_domain;
	}
}

// Return a domain where the given column has been restricted to the given set of objects, or 0 if this would leave an empty domain.
const ActionDomain* ActionDomain::get_restricted_domain(const ObjectSet& objs, size_t column) const {
	ActionDomain* new_domain = 0;
	for (TupleList::const_iterator ti = get_tuples().begin();
		ti != get_tuples().end(); ti++) {
		const ObjectList& tuple = **ti;
		if (column >= tuple.size()) {
			throw logic_error("Column exceeds the range!");
		}
		if (objs.find(tuple[column]) != objs.end()) {
			if (new_domain == 0) {
				new_domain = new ActionDomain(tuple);
			}
			else {
				new_domain->add(tuple);
			}
		}
	}
	if (new_domain != 0 && new_domain->size() == size()) {
		ActionDomain::register_use(new_domain);
		ActionDomain::unregister_use(new_domain);
		return this;
	}
	else {
		return new_domain;
	}
}

// Return a domain where the given column excludes the given object, or 0 if this would leave an empty domain.
const ActionDomain* ActionDomain::get_excluded_domain(const Object& obj, size_t column) const {
	ActionDomain* new_domain = 0;
	for (TupleList::const_iterator ti = get_tuples().begin();
		ti != get_tuples().end(); ++ti) {
		const ObjectList& tuple = **ti;
		if (column >= tuple.size()) {
			throw logic_error("Column exceeds the range!");
		}
		if (tuple[column] != obj) {
			if (new_domain == 0) {
				new_domain = new ActionDomain(tuple);
			}
			else {
				new_domain->add(tuple);
			}
		}
	}
	if (new_domain != 0 && new_domain->size() == size()) {
		// For storage efficiency???
		ActionDomain::register_use(new_domain);
		ActionDomain::unregister_use(new_domain);
		return this;
	}
	else {
		return new_domain;
	}
}

// Return a domain where the given column excludes the given set of objects, or 0 if this would leave an empty domain.
const ActionDomain* ActionDomain::get_excluded_domain(const ObjectSet& objs, size_t column) const {
	ActionDomain* new_domain = 0;
	for (TupleList::const_iterator ti = get_tuples().begin();
		ti != get_tuples().end(); ti++) {
		const ObjectList& tuple = **ti;
		if (column >= tuple.size()) {
			throw logic_error("Column exceeds the range!");
		}
		if (objs.find(tuple[column]) == objs.end()) {
			if (new_domain == 0) {
				new_domain = new ActionDomain(tuple);
			}
			else {
				new_domain->add(tuple);
			}
		}
	}
	if (new_domain != 0 && new_domain->size() == size()) {
		ActionDomain::register_use(new_domain);
		ActionDomain::unregister_use(new_domain);
		return this;
	}
	else {
		return new_domain;
	}
}

// Print this action domain on the given stream.
void ActionDomain::print(ostream& os) const {
	os << '{';
	for (TupleList::const_iterator ti = get_tuples().begin();
		ti != get_tuples().end(); ti++) {
		if (ti != get_tuples().begin()) {
			os << ' ';
		}
		os << '<';
		const ObjectList& tuple = **ti;
		for (ObjectList::const_iterator ni = tuple.begin();
			ni != tuple.end(); ni++) {
			if (ni != tuple.begin()) {
				os << ' ';
			}
			os << *ni;
		}
		os << '>';
	}
	os << '}';
}


// =================== StepDomain ======================

// Return the (positive) index of the variable in this step domain, or -1 if the variable is not included.
int StepDomain::index_of(const Variable& var) const {
	VariableList::const_iterator vi = find(get_parameters().begin(),
		get_parameters().end(), var);
	return (vi != get_parameters().end()) ? vi - get_parameters().begin() : -1;
}

// Check if this step domain includes the given object in the given column.
bool StepDomain::includes(const Object& obj, size_t column) const {
	for (TupleList::const_iterator ti = get_domain().get_tuples().begin();
		ti != get_domain().get_tuples().end(); ti++) {
		if ((**ti)[column] == obj) {
			return true;
		}
	}
	return false;
}

// Return the set of objects from the given column.
const ObjectSet& StepDomain::get_projection(size_t column) const {
	return get_domain().get_projection(column);
}

// Return the size of the projection of the given column.
const size_t StepDomain::get_projection_size(size_t column) const {
	return get_domain().projection_size(column);
}

// Return a domain where the given column has been restricted to the given object, or 0 if this would leave an empty domain.
const StepDomain* StepDomain::get_restricted_domain(const Chain<StepDomain>*& sdc,
	const Object& obj, size_t column) const {
	const ActionDomain* ad = get_domain().get_restricted_domain(obj, column);
	if (ad == 0) {
		return 0;
	}
	// diff here!
	else if (ad == domain) {
		return this;
	}
	else {
		sdc = new Chain<StepDomain>(StepDomain(id, get_parameters(), *ad), sdc);
		return &sdc->get_head();
	}
}

// Return a domain where the given column has been restricted to the given set of objects, or 0 if this would leave an empty domain.
const StepDomain* StepDomain::get_restricted_domain(const Chain<StepDomain>*& sdc,
	const ObjectSet& objs, size_t column) const {
	const ActionDomain* ad = get_domain().get_restricted_domain(objs, column);
	if (ad == 0) {
		return 0;
	}
	// diff here!
	else if (ad == domain) {
		return this;
	}
	else {
		sdc = new Chain<StepDomain>(StepDomain(id, get_parameters(), *ad), sdc);
		return &sdc->get_head();
	}
}

// Return a domain where the given column exclues the given object, or 0 if this would leave an empty domain.
const StepDomain* StepDomain::get_excluded_domain(const Chain<StepDomain>*& sdc,
	const Object& obj, size_t column) const {
	const ActionDomain* ad = get_domain().get_excluded_domain(obj, column);
	if (ad == 0) {
		return 0;
	}
	// diff here
	else if (ad == domain) {
		return this;
	}
	else {
		sdc = new Chain<StepDomain>(StepDomain(id, get_parameters(), *ad), sdc);
		return &sdc->get_head();
	}
}

// Return a domain where the given column excludes the given set of objects, or 0 if this would leave an empty domain.
const StepDomain* StepDomain::get_excluded_domain(const Chain<StepDomain>*& sdc,
	const ObjectSet& objs, size_t column) const {
	const ActionDomain* ad = get_domain().get_excluded_domain(objs, column);
	if (ad == 0) {
		return 0;
	}
	// diff here!
	else if (ad == domain) {
		return this;
	}
	else {
		sdc = new Chain<StepDomain>(StepDomain(id, get_parameters(), *ad), sdc);
		return &sdc->get_head();
	}
}

// Print this step domain on the given stream.
void StepDomain::print(ostream& os) const {
	os << "<";
	for (VariableList::const_iterator vi = get_parameters().begin();
		vi != get_parameters().end(); vi++) {
		if (vi != get_parameters().begin()) {
			os << ' ';
		}
		os << *vi << '(' << id << ')';
	}
	os << "> in ";
	get_domain().print(os);
}

// Return the step domain containing the given variable and the column of the variable, or 0 no step domain contains the variable.
static pair<const StepDomain*, size_t>
find_step_domain(const Chain<StepDomain>* step_domains,
	const Variable& var, size_t step_id) {
	if (step_id > 0) {
		for (const Chain<StepDomain>* sd = step_domains; sd != 0; sd = sd->get_tail()) {
			const StepDomain& step_domain = sd->get_head();
			if (step_domain.get_id() == step_id) {
				int column = step_domain.index_of(var);
				if (column >= 0) {
					return make_pair(&step_domain, column);
				}
				else {
					break;
				}
			}
		}
	}
	return pair<const StepDomain*, size_t>(0, 0);
}


// =================== VarSet ======================

// Construct a varset.
VarSet::VarSet(const Object* constant, const Chain<StepVariable>* cd_set,
	const Chain<StepVariable>* ncd_set, const Type& type)
	:constant((constant != 0) ? new Object(*constant) : 0),
	cd_set(cd_set), ncd_set(ncd_set), type(type) {
	RCObject::ref(cd_set);
	RCObject::ref(ncd_set);
}

// Copy constructor for a varset.
VarSet::VarSet(const VarSet& vs)
	:constant((vs.constant != 0) ? new Object(*vs.constant) : 0),
	cd_set(vs.cd_set), ncd_set(vs.ncd_set), type(vs.type) {
	RCObject::ref(cd_set);
	RCObject::ref(ncd_set);
}

// Destruct this varset.
VarSet::~VarSet() {
	if (constant != 0) {
		delete constant;
	}
	RCObject::destructive_deref(cd_set);
	RCObject::destructive_deref(ncd_set);
}

// Check if this varset includes the given variable with the given step id.
bool VarSet::includes(const Variable& var, size_t step_id) const {
	for (const Chain<StepVariable>* vc = cd_set; vc != 0; vc = vc->get_tail()) {
		if (vc->get_head().first == var && vc->get_head().second == step_id) {
			return true;
		}
	}
	return false;
}

// Check if this varset excludes the given variable.
bool VarSet::excludes(const Variable& var, size_t step_id) const {
	for (const Chain<StepVariable>* vc = ncd_set; vc != 0; vc = vc->get_tail()) {
		if (vc->get_head().first == var && vc->get_head().second == step_id) {
			return true;
		}
	}
	return false;
}

// Return the varset obtained by adding the given object to this varset, or 0 if the object is excluded from this varset.
const VarSet* VarSet::add(const Chain<VarSet>*& vsc, const Object& obj) const {
	if (get_constant() != 0) {
		return (*get_constant() == obj) ? this : 0;
	}
	else {
		const Type& ot = TermTable::type(obj);
		if (TypeTable::is_subtype(ot, type)) {
			// why not assign obj to constant???
			vsc = new Chain<VarSet>(VarSet(&obj, cd_set, ncd_set, ot), vsc);
			return &vsc->get_head();
			// why not return vsc???
		}
		else {
			return 0;
		}
	}
}

// Return the varset obtained by adding the given variable to this varset, or 0 if the variable is excluded from this varset.
const VarSet* VarSet::add(const Chain<VarSet>*& vsc, const Variable& var,
	size_t step_id) const {
	if (excludes(var, step_id)) {
		return 0;
	}
	else {
		const Type* tt;
		if (constant != 0) {
			if (!TypeTable::is_subtype(type, TermTable::type(var))) {
				return 0;
			}
			tt = &type;
		}
		else {
			tt = TypeTable::most_specific(type, TermTable::type(var));
			if (tt == 0) {
				return 0;
			}
		}
		const Chain<StepVariable>* new_cd =
			new Chain<StepVariable>(make_pair(var, step_id), cd_set);
		vsc = new Chain<VarSet>(VarSet(constant, new_cd, ncd_set, *tt), vsc);
		return &vsc->get_head();
	}
}

// Return the varset obtained by adding the given term to this varset, or 0 if the term is excluded from this varset.
const VarSet* VarSet::add(const Chain<VarSet>*& vsc, const Term& term,
	size_t step_id) const {
	if (term.is_object()) {
		return add(vsc, term.as_object());
	}
	else {
		return add(vsc, term.as_variable(), step_id);
	}
}

// Return the varset obtained by adding the given variable to the non-codesignation list of this varset; N.B.assumes that the variable is not included in the varset already. ??? what if already exists?
const VarSet* VarSet::restrict(const Chain<VarSet>*& vsc,
	const Variable& var, size_t step_id) const {
	const Chain<StepVariable>* new_ncd = new Chain<StepVariable>(make_pair(var, step_id), ncd_set);
	vsc = new Chain<VarSet>(VarSet(constant, cd_set, new_ncd, type), vsc);
	return &vsc->get_head();
}

// Return the combination of this and the given varset, or 0 if the combination is inconsistent.
const VarSet* VarSet::combine(const Chain<VarSet>*& vsc, const VarSet& vs) const {
	const Object* comb_obj;
	const Type* tt;
	if (constant != 0) {
		if (vs.constant != 0) {
			if (constant != vs.constant) {
				return 0;
			}
		}
		else if (!TypeTable::is_subtype(type, vs.type)) {
			return 0;
		}
		comb_obj = constant;
		tt = &type;
	}
	else if (vs.constant != 0) {
		if (!TypeTable::is_subtype(vs.type, type)) {
			return 0;
		}
		comb_obj = vs.constant;
		tt = &vs.type;
	}
	else {
		comb_obj = 0;
		tt = TypeTable::most_specific(type, vs.type);
		if (tt == 0) {
			return 0;
		}
	}
	const Chain<StepVariable>* comb_cd = cd_set;
	for (const Chain<StepVariable>* vc = vs.cd_set; vc != 0; vc = vc->get_tail()) {
		const StepVariable& step_var = vc->get_head();
		if (excludes(step_var.first, step_var.second)) {
			RCObject::ref(comb_cd);
			RCObject::destructive_deref(comb_cd);
			return 0;
		}
		else {
			comb_cd = new Chain<StepVariable>(step_var, comb_cd);
		}
	}
	const Chain<StepVariable>* comb_ncd = ncd_set;
	for (const Chain<StepVariable>* vc = vs.ncd_set;
		vc != 0; vc = vc->get_tail()) {
		const StepVariable& step_var = vc->get_head();
		if (includes(step_var.first, step_var.second)) {
			RCObject::ref(comb_cd);
			RCObject::destructive_deref(comb_cd);
			RCObject::ref(comb_ncd);
			RCObject::destructive_deref(comb_ncd);
			return 0;
		}
		else if (!excludes(step_var.first, step_var.second)) {
			comb_ncd = new Chain<StepVariable>(step_var, comb_ncd);
		}
	}
	vsc = new Chain<VarSet>(VarSet(comb_obj, comb_cd, comb_ncd, *tt), vsc);
	return &vsc->get_head();
}

// Return the varset representing the given equality binding.
const VarSet* VarSet::make(const Chain<VarSet>*& vsc, const Binding& b,
	bool reverse = false) {
	if (b.is_equality()) {
		const Chain<StepVariable>* cd_set =
			new Chain<StepVariable>(make_pair(b.get_var(), b.get_var_id()), 0);
		if (b.get_term().is_object()) {
			Object obj = b.get_term().as_object();
			vsc = new Chain<VarSet>(VarSet(&obj, cd_set, 0,
				TermTable::type(b.get_term())),
				vsc);
		}
		else {
			const Type* tt = TypeTable::most_specific(TermTable::type(b.get_var()),
				TermTable::type(b.get_term()));
			if (tt == 0) {
				RCObject::ref(cd_set);
				RCObject::destructive_deref(cd_set);
				return 0;
			}
			cd_set = new Chain<StepVariable>(make_pair(b.get_term().as_variable(),
				b.get_term_id()),
				cd_set);
			vsc = new Chain<VarSet>(VarSet(0, cd_set, 0, *tt), vsc);
		}
		return &vsc->get_head();
	}
	else {
		if (reverse) {
			const Chain<StepVariable>* ncd_set =
				new Chain<StepVariable>(make_pair(b.get_var(), b.get_var_id()), 0);
			if (b.get_term().is_object()) {
				Object obj = b.get_term().as_object();
				vsc = new Chain<VarSet>(VarSet(&obj, 0, ncd_set,
					TermTable::type(b.get_term())),
					vsc);
			}
			else {
				Variable var = b.get_term().as_variable();
				const Chain<StepVariable>* cd_set =
					new Chain<StepVariable>(make_pair(var, b.get_term_id()), 0);
				vsc = new Chain<VarSet>(VarSet(0, cd_set, ncd_set,
					TermTable::type(b.get_term())),
					vsc);
			}
			return &vsc->get_head();
		}
		else { // !reverse
			if (b.get_term().is_object()) {
				return 0;
			}
			else {
				Variable var = b.get_term().as_variable();
				const Chain<StepVariable>* cd_set =
					new Chain<StepVariable>(make_pair(b.get_var(), b.get_var_id()), 0);
				const Chain<StepVariable>* ncd_set =
					new Chain<StepVariable>(make_pair(var, b.get_term_id()), 0);
				vsc = new Chain<VarSet>(VarSet(0, cd_set, ncd_set,
					TermTable::type(b.get_var())),
					vsc);
				return &vsc->get_head();
			}
		}
	}
}

// Return the varset containing the given object, or 0 if none do.
static const VarSet* find_varset(const Chain<VarSet>* varsets,
	const Object& obj) {
	for (const Chain<VarSet>* vsc = varsets; vsc != 0; vsc = vsc->get_tail()) {
		const VarSet& vs = vsc->get_head();
		if (vs.includes(obj)) {
			return &vs;
		}
	}
	return 0;
}

// Return the varset containing the given variable, or 0 if none do.
static const VarSet* find_varset(const Chain<VarSet>* varsets,
	const Variable& var, size_t step_id) {
	for (const Chain<VarSet>* vsc = varsets; vsc != 0; vsc = vsc->get_tail()) {
		const VarSet& vs = vsc->get_head();
		if (vs.includes(var, step_id)) {
			return &vs;
		}
	}
	return 0;
}

// Return the varset containing the given term, or 0 if none do.
static const VarSet* find_varset(const Chain<VarSet>* varsets,
	const Term& term, size_t step_id) {
	if (term.is_object()) {
		return find_varset(varsets, term.as_object());
	}
	else {
		return find_varset(varsets, term.as_variable(), step_id);
	}
}



// =================== Bindings ======================

// Empty bindings.
const Bindings Bindings::EMPTY = Bindings();

// Construct a binding collection, empty by default.
Bindings::Bindings()
	:varsets(0), high_step_id(0), step_domains(0) {
	ref(this); //??? why
}

// Construct a binding collection.
Bindings::Bindings(const Chain<VarSet>* varsets, size_t high_step_id,
	const Chain<StepDomain>* step_domains)
	: varsets(varsets), high_step_id(high_step_id), step_domains(step_domains) {
#ifdef DEBUG_MEMORY
	created_bindings++;
#endif //DEBUG_MEMORY
	RCObject::ref(varsets); //???
	RCObject::ref(step_domains);
}

// Destruct this binding collection.
Bindings::~Bindings() {
#ifdef DEBUG_MEMORY
	++deleted_bindings;
#endif // DEBUG_MEMORY
	RCObject::destructive_deref(varsets);
	RCObject::destructive_deref(step_domains);
}

// Check if the given formulas (literals) can be unified.
bool Bindings::is_unifiable(const Literal& l1, size_t id1,
	const Literal& l2, size_t id2) {
	BindingList dummy;
	return is_unifiable(dummy, l1, id1, l2, id2);
}

// Check if the given formulas can be unified; the most general unifier is added to the provided substitution list.
bool Bindings::is_unifiable(BindingList& mgu,
	const Literal& l1, size_t id1, const Literal& l2, size_t id2) {
	return EMPTY.unify(mgu, l1, id1, l2, id2);
}

// Return the binding for the given term, or the term itself if it is not bound to a single object.
// Return the term itself if the step id exceeds the highest.
Term Bindings::get_binding(const Term& term, size_t step_id) const {
	if (term.is_variable()) {
		const VarSet* vs =
			((step_id <= high_step_id)
				? find_varset(varsets, term.as_variable(), step_id) : 0);
		if (vs != 0 && vs->get_constant() != 0) {
			return *vs->get_constant();
		}
	}
	return term;
}

// Return the domain for the given step variable.
const ObjectSet& Bindings::get_domain(const Variable& var, size_t step_id,
	const Problem& problem) const {
	pair<const StepDomain*, size_t> sd =
		find_step_domain(step_domains, var, step_id);
	if (sd.first != 0) {
		return sd.first->get_projection(sd.second);
	}
	else {
		const ObjectList& objects =
			problem.get_terms().compatible_objects(TermTable::type(var));
		ObjectSet* objs = new ObjectSet();
		objs->insert(objects.begin(), objects.end());
		const VarSet* vs =
			(step_id <= high_step_id) ? find_varset(varsets, var, step_id) : 0;
		if (vs != 0) {
			for (const Chain<StepVariable>* vc = vs->get_ncd_set();
				vc != 0; vc = vc->get_tail()) {
				const StepVariable& sv = vc->get_head();
				const VarSet* vs2 = ((sv.second <= high_step_id)
					? find_varset(varsets, sv.first, sv.second) : 0);
				if (vs2 != 0 && vs2->get_constant() != 0) {
					objs->erase(*vs2->get_constant());
				}
			}
		}
		return *objs;
	}
}

// Check if one of the given formulas is the negation of the other, and the atomic formulas can be unified.
bool Bindings::affects(const Literal& l1, size_t id1,
	const Literal& l2, size_t id2) const {
	BindingList dummy;
	return affects(dummy, l1, id1, l2, id2);
}

// Check if one of the given formulas is the negation of the other, and the atomic formulas can be unified; the most general unifier is added to the provided substitution list.
bool Bindings::affects(BindingList& mgu, const Literal& l1, size_t id1,
	const Literal& l2, size_t id2) const {
	const Negation* negation = dynamic_cast<const Negation*>(&l1);
	if (negation != 0) {
		return unify(mgu, l2, id2, negation->get_atom(), id1);
	}
	else {
		negation = dynamic_cast<const Negation*>(&l2);
		if (negation != 0) {
			return unify(mgu, negation->get_atom(), id2, l1, id1);
		}
		else {
			return false;
		}
	}
}

// Check if the given formulas can be unified.
bool Bindings::unify(const Literal& l1, size_t id1,
	const Literal& l2, size_t id2) const {
	BindingList dummy;
	return unify(dummy, l1, id1, l2, id2);
}

// Check if the given formulas can be unified; the most general unifier is added to the provided substitution list.
bool Bindings::unify(BindingList& mgu, const Literal& l1, size_t id1,
	const Literal& l2, size_t id2) const {
	if (l1.get_id() > 0 && l2.get_id() > 0) {
		// Both literals are fully instantiated. 
		return &l1 == &l2;
	}
	else if (typeid(l1) != typeid(l2)) {
		// Not the same type of literal. 
		return false;
	}
	else if (l1.get_predicate() != l2.get_predicate()) {
		// The predicates do not match. 
		return false;
	}
	else if (l1.get_id() > 0 || l2.get_id() > 0) {
		// One of the literals is fully instantiated and the other not. 
		const Literal* ll;
		const Literal* lg;
		size_t idl;
		if (l1.get_id() > 0) {
			ll = &l2;
			lg = &l1;
			idl = id2;
		}
		else {
			ll = &l1;
			lg = &l2;
			idl = id1;
		}
		SubstitutionMap bind;
		size_t n = ll->get_arity();
		for (size_t i = 0; i < n; i++) {
			const Term& term1 = ll->get_term(i);
			Object obj2 = lg->get_term(i).as_object();
			if (term1.is_object()) {
				if (term1 != obj2) {
					return false;
				}
			}
			else {
				Variable var1 = term1.as_variable();
				SubstitutionMap::const_iterator b = bind.find(var1);
				if (b != bind.end()) {
					if ((*b).second != obj2) {
						return false;
					}
				}
				else {
					Term bt = get_binding(term1, idl);
					if (bt.is_object()) {
						if (bt != obj2) {
							return false;
						}
					}
					else {
						if (!TypeTable::is_subtype(TermTable::type(obj2),
							TermTable::type(term1))) {
							return false;
						}
						mgu.push_back(Binding(var1, idl, obj2, 0, true));
					}
					bind.insert(make_pair(var1, obj2));
				}
			}
		}
	}
	else {
		// Try to unify the terms of the literals.

		// Number of terms for the first literal. 
		size_t n = l1.get_arity();
		for (size_t i = 0; i < n; i++) {
			//Try to unify a pair of terms.

			const Term& term1 = l1.get_term(i);
			const Term& term2 = l2.get_term(i);
			if (term1.is_object()) {
				// The first term is an object. 
				if (term2.is_object()) {
					// Both terms are objects.

					if (term1 != term2) {
						// The two terms are different objects. 
						return false;
					}
				}
				else {
					// The first term is an object and the second is a variable.

					if (!TypeTable::is_subtype(TermTable::type(term1),
						TermTable::type(term2))) {
						// Incompatible term types. 
						return false;
					}
					mgu.push_back(Binding(term2.as_variable(), id2, term1, 0, true));
				}
			}
			else {
				// The first term is a variable.

				if (term2.is_object()) {
					if (!TypeTable::is_subtype(TermTable::type(term2),
						TermTable::type(term1))) {
						// Incompatible term types. 
						return false;
					}
				}
				else if (!TypeTable::is_compatible(TermTable::type(term1),
					TermTable::type(term2))) {
					// Incompatible term types. 
					return false;
				}
				mgu.push_back(Binding(term1.as_variable(), id1, term2, id2, true));
			}
		}
	}
	if (add(mgu, true) == 0) {
		// Unification is inconsistent with current bindings. 
		return false;
	}
	else {
		// Successful unification. 
		return true;
	}
}

// Check if the given equality is consistent with the current bindings.
bool Bindings::is_consistent_with(const Equality& eq, size_t step_id) const {
	size_t var_id = eq.step_id1(step_id);
	size_t term_id = eq.step_id2(step_id);
	const VarSet* vs =
		(term_id <= high_step_id) ? find_varset(varsets, eq.get_term(), term_id) : 0;
	if (vs == 0 || vs->includes(eq.get_variable(), var_id)) {
		return true;
	}
	else if (vs->excludes(eq.get_variable(), var_id)) {
		return false;
	}
	else if (vs->get_constant() != 0) {
		pair<const StepDomain*, size_t> sd =
			find_step_domain(step_domains, eq.get_variable(), var_id);
		if (sd.first != 0) {
			return sd.first->includes(*vs->get_constant(), sd.second);
		}
	}
	return true;
}

// Check if the given inequality is consistent with the current bindings.
bool Bindings::is_consistent_with(const Inequality& neq, size_t step_id) const {
	size_t var_id = neq.step_id1(step_id);
	size_t term_id = neq.step_id2(step_id);
	const VarSet* vs =
		(term_id <= high_step_id) ? find_varset(varsets, neq.get_term(), term_id) : 0;
	return (vs == 0
		|| !vs->includes(neq.get_variable(), var_id)
		|| vs->excludes(neq.get_variable(), var_id));
}

// Add bindings to the list as determined by difference between the given step domains
static void add_domain_bindings(BindingList& bindings,
	const StepDomain& old_sd,
	const StepDomain& new_sd,
	size_t ex_column = UINT_MAX) {
	for (size_t c = 0; c < old_sd.get_parameters().size(); c++) {
		if (c != ex_column && new_sd.get_projection_size(c) == 1
			&& old_sd.get_projection_size(c) > 1) {
			bindings.push_back(Binding(new_sd.get_parameters()[c], new_sd.get_id(),
				*new_sd.get_projection(c).begin(), 0, true));
		}
	}
}

// Return the binding collection obtained by adding the given bindings to this binding collection, or 0 if the new bindings are inconsistent with the current.
const Bindings* Bindings::add(const BindingList& new_bindings,
	bool test_only) const {
	if (new_bindings.empty()) {
		// No new bindings.
		return this;
	}

	// Varsets for new binding collection.
	const Chain<VarSet>* varsets_new = varsets;
	// Highest step id of variable in varsets. 
	size_t high_step_new = high_step_id;
	// Variables above previous high step. 
	VariableSet high_step_vars;
	// Step domains for new binding collection 
	const Chain<StepDomain>* step_domains_new = step_domains;

	BindingList new_binds(new_bindings);
	// Add new bindings one at a time.
	
	for (size_t i = 0; i < new_binds.size(); i++) {
		//
		// N.B. Make a copy of the binding instead of just saving a
		// reference, because new_binds can be expanded in the loop in
		// which case the reference may become invalid.
		
		const Binding bind = new_binds[i];
		if (bind.is_equality()) {
			//
			// Adding equality binding.
			
			// Varset for variable. 
			const VarSet* vs1;
			StepVariable sv(bind.get_var(), bind.get_var_id());
			if (bind.get_var_id() <= high_step_id
				|| high_step_vars.find(sv) != high_step_vars.end()) {
				vs1 = find_varset(varsets_new, bind.get_var(), bind.get_var_id());
			}
			else {
				if (bind.get_var_id() > high_step_new) {
					high_step_new = bind.get_var_id();
				}
				high_step_vars.insert(sv);
				vs1 = 0;
			}
			// Varset for term. 
			const VarSet* vs2;
			if (bind.get_term().is_object()) {
				vs2 = find_varset(varsets_new, bind.get_term().as_object());
			}
			else {
				StepVariable sv(bind.get_term().as_variable(), bind.get_term_id());
				if (bind.get_term_id() <= high_step_id
					|| high_step_vars.find(sv) != high_step_vars.end()) {
					vs2 = find_varset(varsets_new, sv.first, bind.get_term_id());
				}
				else {
					if (bind.get_term_id() > high_step_new) {
						high_step_new = bind.get_term_id();
					}
					high_step_vars.insert(sv);
					vs2 = 0;
				}
			}
			// Combined varset, or 0 if binding is inconsistent with current bindings. 
			const VarSet* comb;
			if (vs1 != 0 || vs2 != 0) {
				// At least one of the terms is already bound. 
				if (vs1 != vs2) {
					// The terms are not yet bound to eachother. 
					if (vs1 == 0) {
						// The first term is unbound, so add it to the varset of the second. 
						comb = vs2->add(varsets_new, bind.get_var(), bind.get_var_id());
					}
					else if (vs2 == 0) {
						// The second term is unbound, so add it to the varset of the first. 
						comb = vs1->add(varsets_new, bind.get_term(), bind.get_term_id());
					}
					else {
						// Both terms are bound, so combine their varsets. 
						comb = vs1->combine(varsets_new, *vs2);
					}
				}
				else {
					// The terms are already bound to eachother. 
					comb = vs1;
				}
			}
			else {
				// None of the terms are already bound. 
				comb = VarSet::make(varsets_new, bind);
			}
			if (comb == 0) {
				// Binding is inconsistent with current bindings. 
				RCObject::ref(varsets_new);
				RCObject::destructive_deref(varsets_new);
				RCObject::ref(step_domains_new);
				RCObject::destructive_deref(step_domains_new);
				return 0;
			}
			else {
				// Binding is consistent with current bindings. 
				if (comb != vs1) {
					// Combined varset is new, so add it to the chain of varsets. 
					const Object* obj = comb->get_constant();
					// Restrict step domain for all codesignated variables. 
					const ObjectSet* intersection = 0;
					bool new_intersection = false;
					const Chain<StepVariable>* vc = 0;
					int phase = 0;
					while (phase < 4 || (intersection != 0 && phase < 8)) {
						const Variable* var = 0;
						size_t var_id = 0;
						switch (phase) {
						case 0:
						case 4:
							if (vs1 == 0) {
								var = new Variable(bind.get_var());
								var_id = bind.get_var_id();
								phase += 2;
							}
							else if (vs1->get_constant() == 0) {
								vc = vs1->get_cd_set();
								phase++;
							}
							else {
								phase += 2;
							}
							break;
						case 1:
						case 3:
						case 5:
						case 7:
							if (vc != 0) {
								var = new Variable(vc->get_head().first);
								var_id = vc->get_head().second;
								vc = vc->get_tail();
							}
							else {
								var = 0;
								phase++;
							}
							break;
						case 2:
						case 6:
							if (vs2 == 0) {
								var = (bind.get_term().is_variable()
									? new Variable(bind.get_term().as_variable()) : 0);
								var_id = bind.get_term_id();
								phase += 2;
							}
							else if (vs2->get_constant() == 0) {
								vc = vs2->get_cd_set();
								phase++;
							}
							else {
								phase += 2;
							}
							break;
						}
						if (var != 0) {
							// Step domain for variable. 
							pair<const StepDomain*, size_t> sd =
								find_step_domain(step_domains_new, *var, var_id);
							delete var;
							if (sd.first != 0) {
								if (obj != 0) {
									const StepDomain* new_sd =
										sd.first->get_restricted_domain(step_domains_new, *obj, sd.second);
									if (new_sd == 0) {
										// Domain became empty. 
										RCObject::ref(varsets_new);
										RCObject::destructive_deref(varsets_new);
										RCObject::ref(step_domains_new);
										RCObject::destructive_deref(step_domains_new);
										if (new_intersection) {
											delete intersection;
										}
										return 0;
									}
									if (sd.first != new_sd) {
										add_domain_bindings(new_binds, *sd.first, *new_sd,
											sd.second);
									}
								}
								else {
									if (phase > 4) {
										const StepDomain* new_sd =
											sd.first->get_restricted_domain(step_domains_new,
												*intersection, sd.second);
										if (new_sd == 0) {
											// Domain became empty. 
											RCObject::ref(varsets_new);
											RCObject::destructive_deref(varsets_new);
											RCObject::ref(step_domains_new);
											RCObject::destructive_deref(step_domains_new);
											if (new_intersection) {
												delete intersection;
											}
											return 0;
										}
										if (sd.first != new_sd) {
											add_domain_bindings(new_binds, *sd.first, *new_sd);
										}
									}
									else if (intersection == 0) {
										intersection = &sd.first->get_projection(sd.second);
									}
									else {
										ObjectSet* cut = new ObjectSet();
										const ObjectSet& set2 = sd.first->get_projection(sd.second);
										set_intersection(intersection->begin(),
											intersection->end(),
											set2.begin(), set2.end(),
											inserter(*cut, cut->begin()));
										if (new_intersection) {
											delete intersection;
										}
										intersection = cut;
										new_intersection = true;
										if (intersection->empty()) {
											// Domain became empty. 
											RCObject::ref(varsets_new);
											RCObject::destructive_deref(varsets_new);
											RCObject::ref(step_domains_new);
											RCObject::destructive_deref(step_domains_new);
											if (new_intersection) {
												delete intersection;
											}
											return 0;
										}
									}
								}
							}
						}
					}
					if (new_intersection) {
						delete intersection;
					}
				}
			}
		}
		else {
			// Adding inequality binding.
			
			// Varset for variable. 
			const VarSet* vs1;
			StepVariable sv(bind.get_var(), bind.get_var_id());
			if (bind.get_var_id() <= high_step_id
				|| high_step_vars.find(sv) != high_step_vars.end()) {
				vs1 = find_varset(varsets, bind.get_var(), bind.get_var_id());
			}
			else {
				if (bind.get_var_id() > high_step_new) {
					high_step_new = bind.get_var_id();
				}
				high_step_vars.insert(sv);
				vs1 = 0;
			}
			// Varset for term. 
			const VarSet* vs2;
			if (bind.get_term().is_object()) {
				vs2 = find_varset(varsets, bind.get_term().as_object());
			}
			else {
				StepVariable sv(bind.get_term().as_variable(), bind.get_term_id());
				if (bind.get_term_id() <= high_step_id
					|| high_step_vars.find(sv) != high_step_vars.end()) {
					vs2 = find_varset(varsets, sv.first, bind.get_term_id());
				}
				else {
					if (bind.get_term_id() > high_step_new) {
						high_step_new = bind.get_term_id();
					}
					high_step_vars.insert(sv);
					vs2 = 0;
				}
			}
			if (vs1 != 0 && vs2 != 0 && vs1 == vs2) {
				// The terms are already bound to eachother. 
				RCObject::ref(varsets_new);
				RCObject::destructive_deref(varsets_new);
				RCObject::ref(step_domains_new);
				RCObject::destructive_deref(step_domains_new);
				return 0;
			}
			else {
				// The terms are not bound to eachother. 
				bool separate1 = true;
				bool separate2 = true;
				if (vs1 == 0) {
					// The first term is unbound, so create a new varset for it. 
					vs1 = VarSet::make(varsets_new, bind);
				}
				else {
					if (bind.get_term().is_variable()) {
						// The second term is a variable. 
						Variable var = bind.get_term().as_variable();
						if (vs1->excludes(var, bind.get_term_id())) {
							// The second term is already separated from the first. 
							separate1 = false;
						}
						else {
							// Separate the second term from the first. 
							vs1 = vs1->restrict(varsets_new, var, bind.get_term_id());
						}
					}
					else {
						// The second term is an object, so the terms are separated in the varset for the second term. 
						separate1 = false;
					}
				}
				if (vs2 == 0) {
					// The second term is unbound, so create a new varset for it. 
					vs2 = VarSet::make(varsets_new, bind, true);
				}
				else if (vs2->excludes(bind.get_var(), bind.get_var_id())) {
					// The first term is already separated from the second. 
					separate2 = false;
				}
				else {
					// Separate the first term from the second. 
					vs2 = vs2->restrict(varsets_new, bind.get_var(), bind.get_var_id());
				}
				if (separate1 && vs1 != 0) {
					// The second term was not separated from the first already. 
					if (vs1->get_constant() != 0 && vs2 != 0) {
						for (const Chain<StepVariable>* vc = vs2->get_cd_set();
							vc != 0; vc = vc->get_tail()) {
							pair<const StepDomain*, size_t> sd =
								find_step_domain(step_domains_new,
									vc->get_head().first, vc->get_head().second);
							if (sd.first != 0) {
								const StepDomain* new_sd =
									sd.first->get_excluded_domain(step_domains_new, *vs1->get_constant(), sd.second);
								if (new_sd == 0) {
									// Domain became empty. 
									RCObject::ref(varsets_new);
									RCObject::destructive_deref(varsets_new);
									RCObject::ref(step_domains_new);
									RCObject::destructive_deref(step_domains_new);
									return 0;
								}
								if (sd.first != new_sd) {
									add_domain_bindings(new_binds, *sd.first, *new_sd);
								}
							}
						}
					}
				}
				if (separate2 && vs2 != 0) {
					// The first term was not separated from the second already. 
					if (vs2->get_constant() != 0) {
						const Chain<StepVariable>* vc = (vs1 != 0) ? vs1->get_cd_set() : 0;
						const Variable* var = (vc != 0) ? &(vc->head.first) : &bind.get_var();
						size_t var_id = (vc != 0) ? vc->get_head().second : bind.get_var_id();
						while (var != 0) {
							pair<const StepDomain*, size_t> sd =
								find_step_domain(step_domains_new, *var, var_id);
							if (sd.first != 0) {
								const StepDomain* new_sd =
									sd.first->get_excluded_domain(step_domains_new, *vs2->get_constant(), sd.second);
								if (new_sd == 0) {
									// Domain became empty.
									RCObject::ref(varsets_new);
									RCObject::destructive_deref(varsets_new);
									RCObject::ref(step_domains_new);
									RCObject::destructive_deref(step_domains_new);
									return 0;
								}
								if (sd.first != new_sd) {
									add_domain_bindings(new_binds, *sd.first, *new_sd);
								}
							}
							vc = (vs1 != 0) ? vc->get_tail() : 0;
							var = (vc != 0) ? &(vc->head.first) : 0;
							var_id = (vc != 0) ? vc->get_head().second : 0;
						}
					}
				}
			}
		}
	}
	// New bindings are consistent with the current bindings. 
	if (test_only
		|| (varsets_new == varsets && high_step_new == high_step_id
			&& step_domains_new == step_domains)) {
		RCObject::ref(varsets_new);
		RCObject::destructive_deref(varsets_new);
		RCObject::ref(step_domains_new);
		RCObject::destructive_deref(step_domains_new);
		return this;
	}
	else {
		return new Bindings(varsets_new, high_step_new, step_domains_new);
	}
}

// Return the binding collection obtained by adding the constraints associated with the given step to this binding collection, or 0 if the new binding collection would be inconsistent.
const Bindings* Bindings::add(size_t step_id, const Action& step_action,
	const PlanningGraph& pg, bool test_only = false) const {
	const ActionSchema* action = dynamic_cast<const ActionSchema*>(&step_action);
	if (action == 0 || action->get_parameters().empty()) {
		return this;
	}
	const ActionDomain* domain = pg.action_domain(action->get_name());
	if (domain == 0) {
		return 0;
	}
	const Chain<StepDomain>* step_domains_new =
		new Chain<StepDomain>(StepDomain(step_id, action->get_parameters(), *domain),
			step_domains);
	const Chain<VarSet>* varsets_new = varsets;
	size_t high_step_new = high_step_id;
	const StepDomain& step_domain_new = step_domains_new->head;
	for (size_t c = 0; c < step_domain_new.get_parameters().size(); c++) {
		if (step_domain_new.get_projection_size(c) == 1) {
			const Chain<StepVariable>* cd_set =
				new Chain<StepVariable>(make_pair(step_domain_new.get_parameters()[c],
					step_domain_new.get_id()),
					0);
			Type type = TermTable::type(step_domain_new.get_parameters()[c]);
			varsets_new = new Chain<VarSet>(VarSet(&*step_domain_new.get_projection(c).begin(),
				cd_set, 0, type),
				varsets);
			if (step_id > high_step_new) {
				high_step_new = step_id;
			}
		}
	}
	if (test_only
		|| (varsets_new == varsets && high_step_new == high_step_id
			&& step_domains_new == step_domains)) {
		RCObject::ref(varsets_new);
		RCObject::destructive_deref(varsets_new);
		RCObject::ref(step_domains_new);
		RCObject::destructive_deref(step_domains_new);
		return this;
	}
	else {
		return new Bindings(varsets_new, high_step_new, step_domains_new);
	}
}

// Print this binding collection on the given stream.
void Bindings::print(ostream& os) const {
	map<size_t, vector<Variable> > seen_vars;
	vector<Object> seen_objs;
	for (const Chain<VarSet>* vsc = varsets; vsc != 0; vsc = vsc->tail) {
		const VarSet& vs = vsc->head;
		if (vs.get_cd_set() != 0) {
			const Chain<StepVariable>* vc = vs.get_cd_set();
			if (find(seen_vars[vc->head.second].begin(),
				seen_vars[vc->head.second].end(), vc->head.first)
				!= seen_vars[vc->head.second].end()) {
				continue;
			}
			os << endl << "{";
			for (; vc != 0; vc = vc->tail) {
				const StepVariable& step_var = vc->head;
				os << ' ' << step_var.first << '(' << step_var.second << ')';
				seen_vars[step_var.second].push_back(step_var.first);
			}
			os << " }";
			if (vs.get_constant() != 0) {
				os << " == ";
			}
		}
		if (vs.get_constant() != 0) {
			const Object& obj = *vs.get_constant();
			if (find(seen_objs.begin(), seen_objs.end(), obj) != seen_objs.end()) {
				continue;
			}
			if (vs.get_cd_set() == 0) {
				os << endl;
			}
			os << obj;
			seen_objs.push_back(obj);
		}
		if (vs.get_ncd_set() != 0) {
			os << " != {";
			for (const Chain<StepVariable>* vc =
				vs.get_ncd_set(); vc != 0; vc = vc->tail) {
				os << ' ' << vc->head.first << '(' << vc->head.second << ')';
			}
			os << " }";
		}
	}
	set<size_t> seen_steps;
	for (const Chain<StepDomain>* sd = step_domains; sd != 0; sd = sd->tail) {
		if (seen_steps.find(sd->head.get_id()) == seen_steps.end()) {
			seen_steps.insert(sd->head.get_id());
			os << endl;
			sd->head.print(os);
		}
	}
}

// Print the given term on the given stream.
void Bindings::print_term(ostream& os, const Term& term, size_t step_id) const {
	Term t = get_binding(term, step_id);
	os << t;
	if (t.is_variable()) {
		os << '(' << step_id << ')';
	}
}