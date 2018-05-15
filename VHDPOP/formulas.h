#pragma once

#include "refcount.h"
#include "predicates.h"
#include "terms.h"

class Effect;
class Problem;
class Domain;
class Bindings;
class HeuristicValue;
class PlanningGraph;


// =================== Formula ====================

class Literal;

// Abstract formula.
class Formula :public RCObject{

	// Negation operator for formulas.
	friend const Formula& operator!(const Formula& f);

protected:
	// Construct a formula.
	Formula();

	// Return the negation of this formula.
	virtual const Formula& negation() const = 0;

public:
	// The true formula.
	static const Formula& TRUE_FORMULA;
	// The false formula.
	static const Formula& FALSE_FORMULA;

	// Destruct the formula.
	virtual ~Formula();

	// diff here! original: not inherited from RCObject

	// Register use of the given formula.
	static void register_use(const Formula* f) {
		ref(f);
	}

	// Unregister use of the given formula.
	static void unregister_use(const Formula* f) {
		destructive_deref(f);
	}

	// Test if this formula is a tautology.
	bool is_tautology() const { return this == &TRUE_FORMULA; }

	// Test if this formula is a contradiction.
	bool is_contradiction() const { return this == &FALSE_FORMULA; }

	// Return a formula that separates the given effect from anything definitely asserted by this formula.
	virtual const Formula& get_separator(const Effect& effect,
		const Domain& domain) const = 0;

	// Return this formula subject to the given substitutions.
	virtual const Formula& get_substitution(const SubstitutionMap& subst) const = 0;

	// Return an instantiation of this formula.
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const = 0;

	// Return the universal base of this formula.
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const = 0;

	// Return the heuristic value of this formula.
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b = NULL) const = 0;

	// Print this formula on the given stream with the given bindings.
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const = 0;
};

// Conjunction operator for formulas.
const Formula& operator&&(const Formula& f1, const Formula& f2);

// Disjunction operator for formulas.
const Formula& operator||(const Formula& f1, const Formula& f2);


// =================== FormulaList ====================

// List (vector) of (pointers to) formulas.
class FormulaList :public vector<const Formula*> {
};



// =================== Constant ======================

// A formula with a constant truth value.
class Constant :public Formula {

	// Constant representing true.
	static const Constant TRUE_CONSTANT;
	// Constant representing false.
	static const Constant FALSE_CONSTANT;

	// Value of this constant.
	bool value;

	// Construct a constant formula.
	explicit Constant(bool value);

	friend class Formula;

protected:
	// Return a negation of the constant formula.
	virtual const Formula& negation() const;

public:
	// Return a formula that separates the given effect from anything definitely asserted by this formula.
	virtual const Formula& get_separator(const Effect& effect,
		const Domain& domain) const;

	// Return this formula subject to the given substitutions.
	virtual const Formula& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula.
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula.
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula.
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b = NULL) const;

	// Print this formula on the given stream with the given bindings.
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const;
};


// =================== Literal ======================

class Atom;

// An abstract literal.
class Literal :public Formula {
	// Next index for ground literals.
	static size_t next_id;

	// Unique id for ground literals (0 if lifted).
	size_t id;

protected:
	// Assign an index to this literal.
	void assign_id(bool ground);

public:
	// Return the id for this literal (zero if lifted).
	size_t get_id() const { return id; }

	// Return the predicate of this literal.
	virtual const Predicate& get_predicate() const = 0;

	// Return the number of terms of this literal.
	virtual size_t get_arity() const = 0;

	// Return the ith term of this literal.
	virtual const Term& get_term(size_t i) const = 0;

	// Return the atom associated with this literal.
	virtual const Atom& get_atom() const = 0;

	// Return a formula that separates the given effect from anything definitely asserted by this formula.
	virtual const Formula& get_separator(const Effect& effect,
		const Domain& domain) const;

	// Return this formula subject to the given substitutions.
	virtual const Literal& get_substitution(const SubstitutionMap& subst) const = 0;
};


// Less-than specification for literal pointers.
namespace std {
	template<>
	struct less<const Literal*>
		: public binary_function<const Literal*, const Literal*, bool> {
		// Comparison function operator.
		bool operator()(const Literal* l1, const Literal* l2) const {
			return l1->get_id() < l2->get_id();
		}
	};
}


// =================== Atom ======================

// An atom.
class Atom : public Literal {

	// Less-than comparison function object for atoms. 
	class AtomLess
		: public binary_function<const Atom*, const Atom*, bool> {
		// Comparison function. 
		bool operator()(const Atom* a1, const Atom* a2) const;
	};

	// A table of atomic formulas. 
	class AtomTable :public set<const Atom*, AtomLess> {
	};

	// Table of atomic formulas. 
	static AtomTable atoms;

	// Predicate of this atom. 
	Predicate predicate;
	// Terms of this atom. 
	TermList terms;

	// Construct an atomic formula with the given predicate. 
	explicit Atom(const Predicate& predicate) : predicate(predicate) {}

	// Add a term to this atomic formula. 
	void add_term(const Term& term) { terms.push_back(term); }

protected:
	// Return the negation of this formula. 
	virtual const Literal& negation() const;

public:
	// Return an atomic formula with the given predicate and terms. 
	static const Atom& make(const Predicate& predicate, const TermList& terms);

	// Destruct this atomic formula. 
	virtual ~Atom();

	// Return the predicate of this literal (atom). 
	virtual const Predicate& get_predicate() const { return predicate; }

	// Return the number of terms of this literal. 
	virtual size_t get_arity() const { return terms.size(); }

	// Return the ith term of this literal. 
	virtual const Term& get_term(size_t i) const { return terms[i]; }

	// Return the atom associated with this literal. 
	virtual const Atom& get_atom() const { return *this; }

	// Return this formula subject to the given substitutions. 
	virtual const Atom& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula (atom). 
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula (atom). 
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula (atom). 
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b) const;

	// Print this atom on the given stream with the given bindings. 
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const;
};

// Less-than specification for atom pointers.
namespace std {
	template<>
	struct less<const Atom*> : public less<const Literal*> {
	};
}


// =================== AtomSet ======================

// A set of atoms.
class AtomSet :public set<const Atom*> {
};


// =================== Negation ======================

// A negated atom.
class Negation : public Literal {

	// Less-than comparison function object for negated atoms. 
	class NegationLess
		:public binary_function<const Negation*, const Negation*, bool> {
		// Comparison function.
		bool operator()(const Negation* n1, const Negation* n2) const;
	};

	// A table of negated atoms. 
	class NegationTable :public set<const Negation*, NegationLess> {
	};

	// Table of negated atoms. 
	static NegationTable negations;

	// The negated atom. 
	const Atom* atom;

	// Construct a negated atom. 
	explicit Negation(const Atom& atom);

protected:
	// Return the negation of this formula. 
	virtual const Literal& negation() const;

public:
	// Return a negation of the given atom. 
	static const Negation& make(const Atom& atom);

	// Destruct this negated atom. 
	virtual ~Negation();

	// Return the predicate of this literal. 
	virtual const Predicate& get_predicate() const { return get_atom().get_predicate(); }

	// Return the number of terms of this literal. 
	virtual size_t get_arity() const { return get_atom().get_arity(); }

	// Return the ith term of this literal. 
	virtual const Term& get_term(size_t i) const { return get_atom().get_term(i); }

	// Return the atom associated with this literal. 
	virtual const Atom& get_atom() const { return *atom; }

	// Return this formula subject to the given substitutions. 
	virtual const Negation& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula. 
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula. 
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula. 
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b) const;

	// Print this formula on the given stream with the given bindings. 
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const;
};

// Less-than specification for negation pointers.
namespace std {
	template<>
	struct less<const Negation*> : public less<const Literal*> {
	};
}



// =================== BindingLiteral ======================

// A binding literal.
class BindingLiteral : public Formula {
	// Variable of binding literal.
	Variable variable;
	// Step id of variable, or zero if unassigned.
	size_t id1;
	// Term of binding literal.
	Term term;
	// Step id of term, or zero if unassigned.
	size_t id2;

protected:
	// Construct a binding literal.
	BindingLiteral(const Variable& variable, size_t id1,
		const Term& term, size_t id2)
		: variable(variable), id1(id1), term(term), id2(id2) {}

public:
	// Return the variable of this binding literal.
	const Variable& get_variable() const { return variable; }

	// Return the step id of the variable, or the given id if no step id has been assigned.
	size_t step_id1(size_t def_id) const { return (id1 != 0) ? id1 : def_id; }

	// Return the term of this binding literal.
	const Term& get_term() const { return term; }

	// Return the step id of the term, or the given id if no step id has been assigned.
	size_t step_id2(size_t def_id) const { return (id2 != 0) ? id2 : def_id; }

	// Return a formula that separates the given effect from anything definitely asserted by this formula.
	virtual const Formula& get_separator(const Effect& effect,
		const Domain& domain) const {
		return TRUE_FORMULA;
	}

};


// =================== Equality ======================

// An equality formula.
class Equality :public BindingLiteral {

	// Construct an equality with assigned step ids.
	Equality(const Variable& variable, size_t id1, const Term& term, size_t id2)
		: BindingLiteral(variable, id1, term, id2) {}

protected:
	// Return the negation of this formula.
	virtual const Formula& negation() const;

public:
	// Return an equality of the two terms.
	static const Formula& make(const Term& term1, const Term& term2);

	// Return an equality of the two terms.
	static const Formula& make(const Term& term1, size_t id1,
		const Term& term2, size_t id2);

	// Return this formula subject to the given substitutions.
	virtual const Formula& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula.
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula.
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula.
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b) const;

	// Print this formula on the given stream with the given bindings.
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const;
};


// =================== Inequality ======================

// An inequality formula.
class Inequality :public BindingLiteral {

	// Construct an equality with assigned step ids.
	Inequality(const Variable& variable, size_t id1, const Term& term, size_t id2)
		: BindingLiteral(variable, id1, term, id2) {}

protected:
	// Return the negation of this formula.
	virtual const Formula& negation() const;

public:
	// Return an inequality of the two terms.
	static const Formula& make(const Term& term1, const Term& term2);

	// Return an inequality of the two terms.
	static const Formula& make(const Term& term1, size_t id1,
		const Term& term2, size_t id2);

	// Return this formula subject to the given substitutions.
	virtual const Formula& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula.
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula.
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula.
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b) const;

	// Print this formula on the given stream with the given bindings.
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const;
};


// =================== Conjunction ======================

// A conjunction.
class Conjunction :public Formula {
	// The conjuncts.
	FormulaList conjuncts;

protected:
	// Return the negation of this formula.
	virtual const Formula& negation() const;

public:
	// Construct an empty conjunction.
	Conjunction(){}

	// Destruct this conjunction.
	virtual ~Conjunction() {
		for (FormulaList::const_iterator fi = get_conjuncts().begin();
			fi != get_conjuncts().end(); fi++) {
			unregister_use(*fi);
		}
	}

	// Add a conjunct to this conjunction.
	void add_conjunct(const Formula& conjunct) {
		conjuncts.push_back(&conjunct);
		register_use(&conjunct);
	}

	// Return the conjuncts.
	const FormulaList& get_conjuncts() const { return conjuncts; }

	// Return a formula that separates the given effect from anything definitely asserted by this formula.
	virtual const Formula& get_separator(const Effect& effect,
		const Domain& domain) const;

	// Return this formula subject to the given substitutions.
	virtual const Formula& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula.
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula.
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula.
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b) const;

	// Print this formula on the given stream with the given bindings.
	virtual void print(std::ostream& os,
		size_t step_id, const Bindings& bindings) const;
};


// =================== Disjunction ======================

// A disjunction.
class Disjunction :public Formula {
	// The disjuncts.
	FormulaList disjuncts;

protected:
	// Return the negation of this formula.
	virtual const Formula& negation() const;

public:
	// Construct an empty disjunction.
	Disjunction() {}

	// Destruct this disjunction.
	virtual ~Disjunction() {
		for (FormulaList::const_iterator fi = get_disjuncts().begin();
			fi != get_disjuncts().end(); fi++) {
			unregister_use(*fi);
		}
	}

	// Add a disjunct to this conjunction.
	void add_disjunct(const Formula& disjunct) {
		disjuncts.push_back(&disjunct);
		register_use(&disjunct);
	}

	// Return the disjuncts.
	const FormulaList& get_disjuncts() const { return disjuncts; }

	// Return a formula that separates the given effect from anything definitely asserted by this formula.
	virtual const Formula& get_separator(const Effect& effect,
		const Domain& domain) const;

	// Return this formula subject to the given substitutions.
	virtual const Formula& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula.
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula.
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula.
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b) const;

	// Print this formula on the given stream with the given bindings.
	virtual void print(std::ostream& os,
		size_t step_id, const Bindings& bindings) const;
};


// =================== Quantification ======================

// An abstract quantified formula.
class Quantification :public Formula {

	// Quanitfied variables.
	VariableList parameters;
	// The quantified formula.
	const Formula* body;

protected:
	// Construct a quantified formula.
	explicit Quantification(const Formula& body)
		:body(&body) {
		register_use(this->body);
	}

public:
	// Destruct this quantified formula.
	virtual ~Quantification() {
		unregister_use(body);
	}

	// Add a quantified variable to this quantified formula.
	void add_parameter(Variable parameter) {
		parameters.push_back(parameter);
	}

	// Set the body of this quantified formula.
	void set_body(const Formula& body) {
		if (&body != this->body) {
			register_use(&body);
			unregister_use(this->body);
			this->body = &body;
		}
	}

	// Return the quanitfied variables.
	const VariableList& get_parameters() const { return parameters; }

	// Return the quantified formula.
	const Formula& get_body() const { return *body; }

	// Return a formula that separates the given effect from anything sdefinitely asserted by this formula.
	virtual const Formula& get_separator(const Effect& effect,
		const Domain& domain) const {
		return TRUE_FORMULA; // Conservative???
	}
};


// =================== Exists ======================

// Existential quantification.
class Exists :public Quantification {
protected:
	// Return the negation of this formula.
	virtual const Quantification& negation() const;

public:
	// Constructs an existentially quantified formula.
	Exists()
		:Quantification(FALSE_FORMULA) {
	}

	// Return this formula subject to the given substitutions.
	virtual const Formula& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula.
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula.
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula.
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b) const;

	// Print this formula on the given stream with the given bindings.
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const;
};


// =================== Forall ======================

// Universal quantification.
class Forall : public Quantification {

	// The cached universal base for this formula.
	mutable const Formula* universal_base;

protected:
	// Return the negation of this formula.
	virtual const Quantification& negation() const;

public:
	// Construct a universally quantified formula.
	Forall()
		:Quantification(TRUE_FORMULA), universal_base(0) {
	}

	// Return this formula subject to the given substitutions.
	virtual const Formula& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula.
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula.
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula.
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b) const;

	// Print this formula on the given stream with the given bindings.
	virtual void print(ostream& os,
		size_t step_id, const Bindings& bindings) const;
};


// A formula time.
typedef enum { AT_START_F, OVER_ALL_F, AT_END_F } FormulaTime;


// =================== TimedLiteral ======================

// A literal with a time stamp.
class TimedLiteral :public Formula {

	// Literal.
	const Literal* literal;
	// Time stamp.
	FormulaTime when;

	// Construct a timed literal.
	TimedLiteral(const Literal& literal, FormulaTime when)
		:literal(&literal), when(when) {
		register_use(this->literal);
	}

protected:
	// Return the negation of this formula.
	virtual const TimedLiteral& negation() const;

public:
	// Return a literal with the given time stamp.
	static const Formula& make(const Literal& literal, FormulaTime when);

	// Destruct this timed literal.
	~TimedLiteral() {
		unregister_use(literal);
	}

	// Return the literal.
	const Literal& get_literal() const { return *literal; }

	// Return the time stamp.
	FormulaTime get_when() const { return when; }

	// Return a formula that separates the given effect from anything definitely asserted by this formula.
	virtual const Formula& get_separator(const Effect& effect,
		const Domain& domain) const;

	// Return this formula subject to the given substitutions.
	virtual const TimedLiteral& get_substitution(const SubstitutionMap& subst) const;

	// Return an instantiation of this formula.
	virtual const Formula& get_instantiation(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the universal base of this formula.
	virtual const Formula& get_universal_base(const SubstitutionMap& subst,
		const Problem& problem) const;

	// Return the heuristic value of this formula (timed literal).
	virtual void get_heuristic_value(HeuristicValue& h, HeuristicValue& hs,
		const PlanningGraph& pg, size_t step_id,
		const Bindings* b) const;

	// Print this formula on the given stream with the given bindings.
	virtual void print(std::ostream& os,
		size_t step_id, const Bindings& bindings) const;
};