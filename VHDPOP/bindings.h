#pragma once

#include "terms.h"
#include "chain.h"
#include "formulas.h"
#include <set>

class Literal;
class Equality;
class Inequality;
class Action;
class Problem;
class Step;
class PlanningGraph;


// =================== Binding ======================

// A variable binding.
class Binding {
	// A variable.
	Variable var;
	// Step id for the variable.
	size_t var_id;
	// A term bound to or separated from the variable.
	Term term;
	// Step id for the term.
	size_t term_id;
	// If this is an equality binding or not.
	bool equality;

public:
	// Construct a variable binding.
	Binding(const Variable& var, size_t var_id, const Term& term,
		size_t term_id, bool equality)
		:var(var), var_id(var_id), term(term), term_id(term_id),
		equality(equality) {}

	// Copy constructor of a variable binding
	Binding(const Binding& b)
		: var(b.var), var_id(b.var_id), term(b.term), term_id(b.term_id),
		equality(b.equality) {}

	// Return the variable of this binding.
	const Variable& get_var() const { return var; }

	// Return the step id for the variable.
	size_t get_var_id() const { return var_id; }

	// Return the term of this binding.
	const Term& get_term() const { return term; }

	// Return the step id for the term.
	size_t get_term_id() const { return term_id; }

	// Check if this is an equality binding.
	bool is_equality() const { return equality; }
};


// =================== BindingList ======================

// A list (vector) of bindings.
class BindingList :public vector<Binding> {

};


// =================== ObjectSet ======================

// A set of objects.
class ObjectSet :public set<Object> {

};


// =================== TupleList ======================

// A list (vector) of parameter tuples (pointers to ObjectList).
class TupleList :public vector<const ObjectList*> {
};


// =================== ActionDomain ======================

// A domain for action parameters.
class ActionDomain :public RCObject {

	// A projection map.
	class ProjectionMap :public map<size_t, const ObjectSet*> {
	};

	// Possible parameter tuples.
	TupleList tuples;

	// Projections.
	mutable ProjectionMap projections;

	// diff here! no ref_count

public:

	// Construct an action domain with a single tuple.
	ActionDomain(const ObjectList& tuple);

	// Destruct this action domain.
	~ActionDomain();

	// diff here!

	// Register use of this action domain.
	static void register_use(const ActionDomain* a) {
		ref(a);
	}

	// Unregister use of this action domain.
	static void unregister_use(const ActionDomain* a) {
		destructive_deref(a);
	}

	// Return number of tuples.
	size_t size() const { return get_tuples().size(); }

	// Return the tuples of this action domain.
	const TupleList& get_tuples() const { return tuples; }

	// Add a tuple to this action domain.
	void add(const ObjectList& tuple);

	// Return the set of names from the given column. ??? what is column
	const ObjectSet& get_projection(size_t column) const;

	// Return the size of the projection of the given column.
	const size_t projection_size(size_t column) const;

	// Return a domain where the given column has been restricted to the given object, or 0 if this would leave an empty domain.
	const ActionDomain* get_restricted_domain(const Object& obj, size_t column) const;

	// Return a domain where the given column has been restricted to the given set of objects, or 0 if this would leave an empty domain.
	const ActionDomain* get_restricted_domain(const ObjectSet& objs, size_t column) const;

	// Return a domain where the given column excludes the given object, or 0 if this would leave an empty domain.
	const ActionDomain* get_excluded_domain(const Object& obj, size_t column) const;

	// Return a domain where the given column excludes the given set of objects, or 0 if this would leave an empty domain.
	const ActionDomain* get_excluded_domain(const ObjectSet& objs, size_t column) const;

	// Print this action domain on the given stream.
	void print(ostream& os) const;
};


// =================== StepDomain ======================

// A step domain
class StepDomain {

	// The id of the step.
	size_t id;
	// (Pointers to) Parameters of the step.
	const VariableList* parameters;
	// (Pointers to) Domain of the parameters.
	const ActionDomain* domain;

public:
	// Construct a step domain.
	StepDomain(size_t id, const VariableList& parameters,
		const ActionDomain& domain)
		: id(id), parameters(&parameters), domain(&domain) {
		ActionDomain::register_use(this->domain);
	}

	// Copy constructor of a step domain.
	StepDomain(const StepDomain& sd)
		: id(sd.id), parameters(sd.parameters), domain(sd.domain) {
		ActionDomain::register_use(domain);
	}

	// Destruct this step domain.
	~StepDomain() {
		ActionDomain::unregister_use(domain);
	}

	// Return the step id.
	size_t get_id() const { return id; }

	// Return the step parameters.
	const VariableList& get_parameters() const { return *parameters; }

	// Return the parameter domains.
	const ActionDomain& get_domain() const { return *domain; }

	// Return the index of the variable in this step domain, or -1 if the variable is not included.
	int index_of(const Variable& var) const;

	// Check if this step domain includes the given object in the given column.
	bool includes(const Object& obj, size_t column) const;

	// Return the set of objects from the given column.
	const ObjectSet& get_projection(size_t column) const;

	// Return the size of the projection of the given column.
	const size_t get_projection_size(size_t column) const;

	// Return a domain where the given column has been restricted to the given object, or 0 if this would leave an empty domain.
	const StepDomain* get_restricted_domain(const Chain<StepDomain>*& sdc,
		const Object& obj, size_t column) const;

	// Return a domain where the given column has been restricted to the given set of objects, or 0 if this would leave an empty domain.
	const StepDomain* get_restricted_domain(const Chain<StepDomain>*& sdc,
		const ObjectSet& objs, size_t column) const;

	// Return a domain where the given column excludes the given object, or 0 if this would leave an empty domain.
	const StepDomain* get_excluded_domain(const Chain<StepDomain>*& sdc,
		const Object& obj, size_t column) const;

	// Return a domain where the given column excludes the given set of objects, or 0 if this would leave an empty domain.
	const StepDomain* get_excluded_domain(const Chain<StepDomain>*& sdc,
		const ObjectSet& objs, size_t column) const;

	// Print this object on the given stream.
	void print(ostream& os) const;
};


// =================== StepVariable ======================

// A variable with its step id.
typedef pair<Variable, size_t> StepVariable;


// =================== VariableSet ======================

// A set of variables (with the step id).
class VariableSet :public set<StepVariable> {
};


// =================== VarSet ======================

// Variable codesignation, and non-codesignation.
class VarSet {
private:
	// Pointer to the constant object of this varset, or 0.
	const Object* constant;
	// The codesignation list.
	const Chain<StepVariable>* cd_set;
	// The non-codesignation list.
	const Chain<StepVariable>* ncd_set;
	// The most specific type of any variable in this set.
	Type type;

public:

	// Construct a varset.
	VarSet(const Object* constant, const Chain<StepVariable>* cd_set,
		const Chain<StepVariable>* ncd_set, const Type& type);

	// Copy constructor of a varset.
	VarSet(const VarSet& vs);

	// Destruct this varset.
	~VarSet();

	// Pointer to the constant of this varset, or 0.
	const Object* get_constant() const { return constant; }

	// The codesignation list.
	const Chain<StepVariable>* get_cd_set() const { return cd_set; }

	// The non-codesignation list.
	const Chain<StepVariable>* get_ncd_set() const { return ncd_set; }

	// Check if this varset includes the given object.
	bool includes(const Object& obj) const {
		return get_constant() != 0 && *get_constant() == obj;
	}

	// Check if this varset includes the given variable with the given step id.
	bool includes(const Variable& var, size_t step_id) const;

	// Check if this varset excludes the given variable.
	bool excludes(const Variable& var, size_t step_id) const;

	// Return the varset obtained by adding the given object to this varset, or 0 if the object is excluded from this varset.
	const VarSet* add(const Chain<VarSet>*& vsc, const Object& obj) const;

	// Return the varset obtained by adding the given variable to this varset, or 0 if the variable is excluded from this varset.
	const VarSet* add(const Chain<VarSet>*& vsc, const Variable& var,
		size_t step_id) const;

	// Return the varset obtained by adding the given term to this varset, or 0 if the term is excluded from this varset.
	const VarSet* add(const Chain<VarSet>*& vsc, const Term& term,
		size_t step_id) const;

	// Return the varset obtained by adding the given variable to the non-codesignation list of this varset; N.B.assumes that the variable is not included in the varset already.
	const VarSet* restrict(const Chain<VarSet>*& vsc,
		const Variable& var, size_t step_id) const;

	// Return the combination of this and the given varset, or 0 if the combination is inconsistent.
	const VarSet* combine(const Chain<VarSet>*& vsc, const VarSet& vs) const;

	// Return the varset representing the given equality binding.
	static const VarSet* make(const Chain<VarSet>*& vsc, const Binding& b,
		bool reverse = false);
};

// =================== Bindings ======================

class StepDomain;

// A collection of variable bindings. diff here!!!
class Bindings :public RCObject {

	// Varsets representing the transitive closure of the bindings.
	const Chain<VarSet>* varsets;
	// Highest step id of variable in varsets.
	size_t high_step_id;
	// Step domains.
	const Chain<StepDomain>* step_domains;
	// Reference counter.
	mutable size_t ref_count;

	// Construct a binding collection, empty by default.
	Bindings();

	// Construct a binding collection.
	Bindings(const Chain<VarSet>* varsets, size_t high_step_id,
		const Chain<StepDomain>* step_domains);

public:
	// Empty bindings.
	static const Bindings EMPTY;

	// Register use of this binding collection.
	static void register_use(const Bindings* b) {
		ref(b);
	}

	// Unregister use of this binding collection.
	static void unregister_use(const Bindings* b) {
		destructive_deref(b);
	}

	// Destruct this binding collection.
	~Bindings();

	// Check if the given formulas can be unified.
	static bool is_unifiable(const Literal& l1, size_t id1,
		const Literal& l2, size_t id2);

	// Check if the given formulas can be unified; the most general unifier is added to the provided substitution list.
	static bool is_unifiable(BindingList& mgu,
		const Literal& l1, size_t id1,
		const Literal& l2, size_t id2);

	// Return the binding for the given term, or the term itself if it is not bound to a single object.
	Term get_binding(const Term& term, size_t step_id) const;

	// Return the domain for the given step variable.
	const ObjectSet& get_domain(const Variable& var, size_t step_id,
		const Problem& problem) const;

	// Check if one of the given formulas is the negation of the other, and the atomic formulas can be unified.
	bool affects(const Literal& l1, size_t id1,
		const Literal& l2, size_t id2) const;

	// Check if one of the given formulas is the negation of the other, and the atomic formulas can be unified; the most general unifier is added to the provided substitution list.
	bool affects(BindingList& mgu, const Literal& l1, size_t id1,
		const Literal& l2, size_t id2) const;

	// Check if the given formulas can be unified.
	bool unify(const Literal& l1, size_t id1,
		const Literal& l2, size_t id2) const;

	// Check if the given formulas can be unified; the most general unifier is added to the provided substitution list.
	bool unify(BindingList& mgu, const Literal& l1, size_t id1,
		const Literal& l2, size_t id2) const;

	// Check if the given equality is consistent with the current bindings.
	bool is_consistent_with(const Equality& eq, size_t step_id) const;

	// Check if the given inequality is consistent with the current bindings.
	bool is_consistent_with(const Inequality& neq, size_t step_id) const;

	// Return the binding collection obtained by adding the given bindings to this binding collection, or 0 if the new bindings are inconsistent with the current.
	const Bindings* add(const BindingList& new_bindings,
		bool test_only = false) const;

	// Return the binding collection obtained by adding the constraints associated with the given step to this binding collection, or 0 if the new binding collection would be inconsistent.
	const Bindings* add(size_t step_id, const Action& step_action,
		const PlanningGraph& pg, bool test_only = false) const;

	// Print this binding collection on the given stream.
	void print(ostream& os) const;

	// Print the given term on the given stream.
	void print_term(ostream& os, const Term& term, size_t step_id) const;
};
