#include "formulas.h"
#include "bindings.h"
#include "problems.h"
#include "domains.h"
#include <stack>


//=================== Formula ====================

// The true formula.
const Formula& Formula::TRUE_FORMULA = Constant::TRUE_CONSTANT;
// The false formula.
const Formula& Formula::FALSE_FORMULA = Constant::FALSE_CONSTANT;

// Construct a formula.
Formula::Formula() {
#ifdef DEBUG_MEMORY
	++created_formulas;
#endif // DEBUG_MEMORY
}

// Destruct the formula.
Formula::~Formula() {
#ifdef DEBUG_MEMORY
	++deleted_formulas;
#endif // DEBUG_MEMORY
}

// Negation operator for formulas.
const Formula& operator!(const Formula& f) {
	const Formula& neg = f.negation();
	Formula::register_use(&f);
	Formula::unregister_use(&f); //???
	return neg;
}

// Conjunction operator for formulas.
const Formula& operator&&(const Formula& f1, const Formula& f2) {
	if (f1.is_contradiction()) {
		Formula::register_use(&f2);
		Formula::unregister_use(&f2);
		return f1;
	}
	else if (f2.is_contradiction()) {
		Formula::register_use(&f1);
		Formula::unregister_use(&f1);
		return f2;
	}
	else if (f1.is_tautology()) {
		return f2;
	}
	else if (f2.is_tautology()) {
		return f1;
	}
	else if (&f1 == &f2) {
		return f1;
	}
	else {
		Conjunction& conjunction = *new Conjunction();
		const Conjunction* c1 = dynamic_cast<const Conjunction*>(&f1);
		if (c1 != NULL) {
			for (FormulaList::const_iterator fi = c1->get_conjuncts().begin();
				fi != c1->get_conjuncts().end(); fi++) {
				conjunction.add_conjunct(**fi);
			}
			Formula::register_use(c1);
			Formula::unregister_use(c1);
		}
		else {
			conjunction.add_conjunct(f1);
		}
		const Conjunction* c2 = dynamic_cast<const Conjunction*>(&f2);
		if (c2 != NULL) {
			for (FormulaList::const_iterator fi = c2->get_conjuncts().begin();
				fi != c2->get_conjuncts().end(); fi++) {
				conjunction.add_conjunct(**fi);
			}
			Formula::register_use(c2);
			Formula::unregister_use(c2);
		}
		else {
			conjunction.add_conjunct(f2);
		}
		return conjunction;
	}
}

// Disjunction operator for formulas.
const Formula& operator||(const Formula& f1, const Formula& f2) {
	if (f1.is_tautology()) {
		Formula::register_use(&f2);
		Formula::unregister_use(&f2);
		return f1;
	}
	else if (f2.is_tautology()) {
		Formula::register_use(&f1);
		Formula::unregister_use(&f1);
		return f2;
	}
	else if (f1.is_contradiction()) {
		return f2;
	}
	else if (f2.is_contradiction()) {
		return f1;
	}
	else if (&f1 == &f2) {
		return f1;
	}
	else {
		Disjunction& disjunction = *new Disjunction();
		const Disjunction* d1 = dynamic_cast<const Disjunction*>(&f1);
		if (d1 != NULL) {
			for (FormulaList::const_iterator fi = d1->get_disjuncts().begin();
				fi != d1->get_disjuncts().end(); fi++) {
				disjunction.add_disjunct(**fi);
			}
			Formula::register_use(d1);
			Formula::unregister_use(d1);
		}
		else {
			disjunction.add_disjunct(f1);
		}
		const Disjunction* d2 = dynamic_cast<const Disjunction*>(&f2);
		if (d2 != NULL) {
			for (FormulaList::const_iterator fi = d2->get_disjuncts().begin();
				fi != d2->get_disjuncts().end(); fi++) {
				disjunction.add_disjunct(**fi);
			}
			Formula::register_use(d2);
			Formula::unregister_use(d2);
		}
		else {
			disjunction.add_disjunct(f2);
		}
		return disjunction;
	}
}

// =================== Constant ======================

// Constant representing true.
const Constant Constant::TRUE_CONSTANT = Constant(true);
// Constant representing false.
const Constant Constant::FALSE_CONSTANT = Constant(false);

// Construct a constant formula.
Constant::Constant(bool value) :value(value) {
	register_use(this);
#ifdef DEBUG_MEMORY
	--created_formulas;
#endif
}

// Return a negation of the constant formula.
const Formula& Constant::negation() const {
	return value ? FALSE_FORMULA : TRUE_FORMULA;
}

// Return a formula that separates the given effect from anything definitely asserted by this formula.
const Formula& Constant::get_separator(const Effect& effect,
	const Domain& domain) const {
	return TRUE_FORMULA;
}

// Return this constant subject to the given substitutions.
const Formula& Constant::get_substitution(const SubstitutionMap& subst) const {
	return *this;
}

// Return an instantiation of this formula.
const Formula& Constant::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {
	return *this;
}

// Return the universal base of this formula.
const Formula& Constant::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	return *this;
}

// Print this formula on the given stream with the given bindings.
void Constant::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << (value ? "(and)" : "(or)");
}


// =================== Literal ======================

size_t Literal::next_id = 1;

// Assign an index to this literal.
void Literal::assign_id(bool ground) {
	if (ground) {
		id = next_id++;
	}
	else {
		id = 0;
	}
}

// Return a formula that separates the given effect from anything definitely asserted by this formula.
const Formula& Literal::get_separator(const Effect& effect,
	const Domain& domain) const {
	BindingList mgu;
	if (Bindings::is_unifiable(mgu, *this, 1, effect.get_literal(), 1)) {
		Disjunction* disj = NULL;
		const Formula* first_d = &FALSE_FORMULA;
		for (BindingList::const_iterator bi = mgu.begin(); bi != mgu.end(); bi++) {
			const Binding& b = *bi;
			if (b.get_var() != b.get_term()) {
				const Formula& d = Inequality::make(b.get_var(), b.get_term());
				if (first_d->is_contradiction()) {
					first_d = &d;
				}
				else if (disj == NULL) {
					disj = new Disjunction();
					disj->add_disjunct(*first_d);
				}
				if (disj != NULL) {
					disj->add_disjunct(d);
				}
			}
		}
		if (disj != NULL) {
			return *disj;
		}
		else {
			return *first_d;
		}
	}
	return TRUE_FORMULA;
}



// =================== Atom ======================

// Tests if the two atoms are unifiable, assuming the second atom is fully instantiated.
static bool unifiable_atoms(const Atom& a1, const Atom& a2) {
	if (a1.get_predicate() != a2.get_predicate()) {
		return false;
	}
	else {
		SubstitutionMap bind;
		size_t n = a1.get_arity();
		for (size_t i = 0; i < n; i++) {
			Term t1 = a1.get_term(i);
			if (t1.is_object()) {
				if (t1 != a2.get_term(i)) {
					return false;
				}
			}
			else {
				const Variable& v1 = t1.as_variable();
				SubstitutionMap::const_iterator b = bind.find(v1);
				if (b != bind.end()) {
					if ((*b).second != a2.get_term(i)) {
						return false;
					}
				}
				else {
					bind.insert(make_pair(v1, a2.get_term(i).as_object()));
				}
			}
		}
		return true;
	}
}

// Table of atomic formulas.
Atom::AtomTable Atom::atoms;

// Comparison function.
bool Atom::AtomLess::operator()(const Atom* a1, const Atom* a2) const {
	if (a1->get_predicate() < a2->get_predicate()) {
		return true;
	}
	else if (a2->get_predicate() < a1->get_predicate()) {
		return false;
	}
	else {
		for (size_t i = 0; i < a1->get_arity(); i++) {
			if (a1->get_term(i) < a2->get_term(i)) {
				return true;
			}
			else if (a2->get_term(i) < a1->get_term(i)) {
				return false;
			}
		}
		return false;
	}
}

// Destruct this atomic formula. 
Atom::~Atom() {
	AtomTable::const_iterator ai = atoms.find(this);
	if (*ai == this) {
		atoms.erase(ai);
	}
}

// Return the negation of this formula. 
const Literal& Atom::negation() const {
	return Negation::make(*this);
}

// Return an atomic formula with the given predicate and terms. 
const Atom& Atom::make(const Predicate& predicate, const TermList& terms) {
	Atom* atom = new Atom(predicate);
	bool ground = true;
	for (TermList::const_iterator ti = terms.begin(); ti != terms.end(); ti++) {
		atom->add_term(*ti);
		if (ground && (*ti).is_variable()) {
			ground = false;
		}
	}
	if (!ground) {
		atom->assign_id(ground);
		return *atom;
	}
	else {
		pair<AtomTable::const_iterator, bool> result = atoms.insert(atom);
		if (!result.second) {
			delete atom;
			return **result.first;
		}
		else {
			atom->assign_id(ground);
			return *atom;
		}
	}
}

// Return this formula subject to the given substitutions. 
const Atom& Atom::get_substitution(const SubstitutionMap& subst) const {
	if (get_id() > 0) {
		return *this;
	}
	else {
		TermList inst_terms;
		bool substituted = false;
		for (TermList::const_iterator ti = terms.begin();
			ti != terms.end(); ti++) {
			SubstitutionMap::const_iterator si =
				(*ti).is_variable() ? subst.find((*ti).as_variable()) : subst.end();
			if (si != subst.end()) {
				inst_terms.push_back((*si).second);
				substituted = true;
			}
			else {
				inst_terms.push_back(*ti);
			}
		}
		if (substituted) {
			// diff here
			return make(predicate, inst_terms);
		}
		else {
			return *this;
		}
	}
}

// Return an instantiation of this atom. 
const Formula& Atom::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {
	bool substituted = false;
	const Atom* inst_atom;
	if (get_id() > 0) {
		inst_atom = this;
	}
	else {
		TermList inst_terms;
		for (TermList::const_iterator ti = terms.begin();
			ti != terms.end(); ti++) {
			SubstitutionMap::const_iterator si =
				(*ti).is_variable() ? subst.find((*ti).as_variable()) : subst.end();
			if (si != subst.end()) {
				inst_terms.push_back((*si).second);
				substituted = true;
			}
			else {
				inst_terms.push_back(*ti);
			}
		}
		inst_atom = substituted ? &make(predicate, inst_terms) : this;
	}
	if (PredicateTable::is_static(predicate)) {
		if (inst_atom->get_id() > 0) {
			if (problem.get_init_atoms().find(inst_atom) != problem.get_init_atoms().end()) {
				register_use(inst_atom);
				unregister_use(inst_atom);
				return TRUE_FORMULA;
			}
			else {
				register_use(inst_atom);
				unregister_use(inst_atom);
				return FALSE_FORMULA;
			}
		}
		else {
			for (AtomSet::const_iterator ai = problem.get_init_atoms().begin();
				ai != problem.get_init_atoms().end(); ai++) {
				if (unifiable_atoms(*inst_atom, **ai)) {
					return *inst_atom;
				}
			}
			register_use(inst_atom);
			unregister_use(inst_atom);
			return FALSE_FORMULA;
		}
	}
	else {
		return *inst_atom;
	}
}

// Return the universal base of this formula. 
const Formula& Atom::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	return get_instantiation(subst, problem);
}

// Print this formula on the given stream with the given bindings. 
void Atom::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {

}



// =================== Negation ======================

bool Negation::NegationLess::operator()(const Negation* n1,
	const Negation* n2) const {
	return &n1->get_atom() < &n2->get_atom();
}

// Construct a negated atom. 
Negation::Negation(const Atom& atom)
	:atom(&atom) {
	register_use(this->atom);
}

// Return the negation of this formula. 
const Literal& Negation::negation() const {
	return get_atom();
}

// Return a negation of the given atom. 
const Negation& Negation::make(const Atom& atom) {
	Negation* negation = new Negation(atom);
	bool ground = atom.get_id() > 0;
	if (!ground) {
		negation->assign_id(ground);
		return *negation;
	}
	else {
		pair<NegationTable::const_iterator, bool> result =
			negations.insert(negation);
		if (!result.second) {
			delete negation;
			return **result.first;
		}
		else {
			negation->assign_id(ground);
			return *negation;
		}
	}
}

// Destruct this negated atom. 
Negation::~Negation() {
	unregister_use(atom);
	NegationTable::const_iterator ni = negations.find(this);
	if (*ni == this) {
		negations.erase(ni);
	}
}

// Return this formula subject to the given substitutions. 
const Negation& Negation::get_substitution(const SubstitutionMap& subst) const {
	if (get_id() > 0) {
		return *this;
	}
	else {
		const Atom& f = get_atom().get_substitution(subst);
		if (&f == atom) {
			return *this;
		}
		else {
			return make(f);
		}
	}
}

// Return an instantiation of this formula. 
const Formula& Negation::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {
	const Formula& f = get_atom().get_instantiation(subst, problem);
	if (&f == atom) {
		return *this;
	}
	else {
		return !f;
	}
}

// Return the universal base of this formula. 
const Formula& Negation::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	return get_instantiation(subst, problem);
}


// Print this formula on the given stream with the given bindings. 
void Negation::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << "(not ";
	get_atom().print(os, step_id, bindings);
	os << ")";
}


// =================== BindingLiteral ======================


// =================== Equality ======================

// Return the negation of this formula.
const Formula& Equality::negation() const {
	return Inequality::make(get_variable(), step_id1(0), get_term(), step_id2(0));
}

// Return an equality of the two terms.
const Formula& Equality::make(const Term& term1, const Term& term2) {
	return make(term1, 0, term2, 0);
}

// Return an equality of the two terms.
const Formula& Equality::make(const Term& term1, size_t id1,
	const Term& term2, size_t id2) {
	if (term1 == term2 && id1 == id2) {
		return TRUE_FORMULA;
	}
	else if (term1.is_variable()) {
		const Type& t1 = TermTable::type(term1);
		const Type& t2 = TermTable::type(term2);
		if ((term2.is_variable() && TypeTable::is_compatible(t1, t2))
			|| (term2.is_object() && TypeTable::is_subtype(t2, t1))) {
			return *new Equality(term1.as_variable(), id1, term2, id2);
		}
		else {
			// The terms have incompatible types.
			return FALSE_FORMULA;
		}
	}
	else if (term2.is_variable()) {
		if (TypeTable::is_subtype(TermTable::type(term1), TermTable::type(term2))) {
			return *new Equality(term2.as_variable(), id1, term1, id2);
		}
		else {
			// The terms have incompatible types.
			return FALSE_FORMULA;
		}
	}
	else {
		// The two terms are different objects.
		return FALSE_FORMULA;
	}
}

// Return this formula subject to the given substitutions.
const Formula& Equality::get_substitution(const SubstitutionMap& subst) const {
	SubstitutionMap::const_iterator si = subst.find(get_variable());
	const Term& term1 = (si != subst.end()) ? (*si).second : Term(get_variable());
	si = get_term().is_variable() ? subst.find(get_term().as_variable()) : subst.end();
	const Term& term2 = (si != subst.end()) ? (*si).second : get_term();
	if (term1 == get_variable() && term2 == get_term()) {
		return *this;
	}
	else {
		return make(term1, step_id1(0), term2, step_id2(0));
	}
}

// Return an instantiation of this formula.
const Formula& Equality::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {
	return get_substitution(subst);
}

// Return the universal base of this formula.
const Formula& Equality::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	return get_substitution(subst);
}


// Print this formula on the given stream with the given bindings.
void Equality::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << "(= ";
	bindings.print_term(os, get_variable(), step_id);
	os << ' ';
	bindings.print_term(os, get_term(), step_id);
	os << ")";
}


// =================== Inequality ======================

// Return an inequality of the two terms.
const Formula& Inequality::make(const Term& term1, const Term& term2) {
	return make(term1, 0, term2, 0);
}

// Return an inequality of the two terms.
const Formula& Inequality::make(const Term& term1, size_t id1,
	const Term& term2, size_t id2) {
	if (term1 == term2 && id1 == id2) {
		return FALSE_FORMULA;
	}
	else if (term1.is_variable()) {
		const Type& t1 = TermTable::type(term1);
		const Type& t2 = TermTable::type(term2);
		if ((term2.is_variable() && TypeTable::is_compatible(t1, t2))
			|| (term2.is_object() && TypeTable::is_subtype(t2, t1))) {
			return *new Inequality(term1.as_variable(), id1, term2, id2);
		}
		else {
			// The terms have incompatible types.
			return TRUE_FORMULA;
		}
	}
	else if (term2.is_variable()) {
		if (TypeTable::is_subtype(TermTable::type(term1), TermTable::type(term2))) {
			return *new Inequality(term2.as_variable(), id1, term1, id2);
		}
		else {
			// The terms have incompatible types.
			return TRUE_FORMULA;
		}
	}
	else {
		// The two terms are different objects.
		return TRUE_FORMULA;
	}
}

// Return this formula subject to the given substitutions.
const Formula& Inequality::get_substitution(const SubstitutionMap& subst) const {
	SubstitutionMap::const_iterator si = subst.find(get_variable());
	const Term& term1 = (si != subst.end()) ? (*si).second : Term(get_variable());
	si = get_term().is_variable() ? subst.find(get_term().as_variable()) : subst.end();
	const Term& term2 = (si != subst.end()) ? (*si).second : get_term();
	if (term1 == get_variable() && term2 == get_term()) {
		return *this;
	}
	else {
		return make(term1, step_id1(0), term2, step_id2(0));
	}
}

// Return an instantiation of this formula.
const Formula& Inequality::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {
	return get_substitution(subst);
}

// Return the universal base of this formula.
const Formula& Inequality::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	return get_substitution(subst);
}

// Print this formula on the given stream with the given bindings.
void Inequality::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << "(not (= ";
	bindings.print_term(os, get_variable(), step_id);
	os << ' ';
	bindings.print_term(os, get_term(), step_id);
	os << "))";
}

// Return the negation of this formula.
const Formula& Inequality::negation() const {
	return Equality::make(get_variable(), step_id1(0), get_term(), step_id2(0));
}


// =================== Conjunction ======================

// Return a formula that separates the given effect from anything definitely asserted by this formula.
const Formula& Conjunction::get_separator(const Effect& effect,
	const Domain& domain) const {
	Conjunction* conj = NULL;
	const Formula* first_c = &TRUE_FORMULA;
	for (FormulaList::const_iterator fi = get_conjuncts().begin();
		fi != get_conjuncts().end(); fi++) {
		const Formula& c = (*fi)->get_separator(effect, domain);
		if (c.is_contradiction()) {
			if (conj == NULL) {
				register_use(first_c);
				unregister_use(first_c);
			}
			else {
				register_use(conj);
				unregister_use(conj);
			}
			return FALSE_FORMULA;
		}
		else if (!c.is_tautology()) {
			if (first_c->is_tautology()) {
				first_c = &c;
			}
			else if (conj == NULL) {
				conj = new Conjunction();
				conj->add_conjunct(*first_c);
			}
			if (conj != NULL) {
				conj->add_conjunct(c);
			}
		}
	}
	if (conj != NULL) {
		return *conj;
	}
	else {
		return *first_c;
	}
}

// Return this formula subject to the given substitutions.
const Formula& Conjunction::get_substitution(const SubstitutionMap& subst) const {
	Conjunction* conj = NULL;
	const Formula* first_c = &TRUE_FORMULA;
	bool changed = false;
	for (FormulaList::const_iterator fi = get_conjuncts().begin();
		fi != get_conjuncts().end(); fi++) {
		const Formula& c = (*fi)->get_substitution(subst);
		if (&c != *fi) {
			changed = true;
		}
		if (c.is_contradiction()) {
			if (conj == NULL) {
				register_use(first_c);
				unregister_use(first_c);
			}
			else {
				register_use(conj);
				unregister_use(conj);
			}
			return FALSE_FORMULA;
		}
		else if (!c.is_tautology()) {
			if (first_c->is_tautology()) {
				first_c = &c;
			}
			else if (conj == NULL) {
				conj = new Conjunction();
				conj->add_conjunct(*first_c);
			}
			if (conj != NULL) {
				conj->add_conjunct(c);
			}
		}
	}
	if (!changed) {
		if (conj == NULL) {
			register_use(first_c);
			unregister_use(first_c);
		}
		else {
			register_use(conj);
			unregister_use(conj);
		}
		return *this;
	}
	else if (conj != NULL) {
		return *conj;
	}
	else {
		return *first_c;
	}
}

// Return an instantiation of this formula.
const Formula& Conjunction::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {

	Conjunction* conj = NULL;
	const Formula* first_c = &TRUE_FORMULA;
	bool changed = false;
	for (FormulaList::const_iterator fi = get_conjuncts().begin();
		fi != get_conjuncts().end(); fi++) {
		const Formula& c = (*fi)->get_instantiation(subst, problem);
		if (&c != *fi) {
			changed = true;
		}
		if (c.is_contradiction()) {
			if (conj == NULL) {
				register_use(first_c);
				unregister_use(first_c);
			}
			else {
				register_use(conj);
				unregister_use(conj);
			}
			return FALSE_FORMULA;
		}
		else if (!c.is_tautology()) {
			if (first_c->is_tautology()) {
				first_c = &c;
			}
			else if (conj == NULL) {
				conj = new Conjunction();
				conj->add_conjunct(*first_c);
			}
			if (conj != NULL) {
				conj->add_conjunct(c);
			}
		}
	}
	if (!changed) {
		if (conj == NULL) {
			register_use(first_c);
			unregister_use(first_c);
		}
		else {
			register_use(conj);
			unregister_use(conj);
		}
		return *this;
	}
	else if (conj != NULL) {
		return *conj;
	}
	else {
		return *first_c;
	}
}

// Return the universal base of this formula.
const Formula& Conjunction::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	Conjunction* conj = NULL;
	const Formula* first_c = &TRUE_FORMULA;
	bool changed = false;
	for (FormulaList::const_iterator fi = get_conjuncts().begin();
		fi != get_conjuncts().end(); fi++) {
		const Formula& c = (*fi)->get_universal_base(subst, problem);
		if (&c != *fi) {
			changed = true;
		}
		if (c.is_contradiction()) {
			if (conj == NULL) {
				register_use(first_c);
				unregister_use(first_c);
			}
			else {
				register_use(conj);
				unregister_use(conj);
			}
			return FALSE_FORMULA;
		}
		else if (!c.is_tautology()) {
			if (first_c->is_tautology()) {
				first_c = &c;
			}
			else if (conj == NULL) {
				conj = new Conjunction();
				conj->add_conjunct(*first_c);
			}
			if (conj != NULL) {
				conj->add_conjunct(c);
			}
		}
	}
	if (!changed) {
		if (conj == NULL) {
			register_use(first_c);
			unregister_use(first_c);
		}
		else {
			register_use(conj);
			unregister_use(conj);
		}
		return *this;
	}
	else if (conj != NULL) {
		return *conj;
	}
	else {
		return *first_c;
	}
}

// Return the negation of this formula.
const Formula& Conjunction::negation() const {
	Disjunction* disj = NULL;
	const Formula* first_d = &FALSE_FORMULA;
	for (FormulaList::const_iterator fi = get_conjuncts().begin();
		fi != get_conjuncts().end(); fi++) {
		const Formula& d = !**fi;
		if (d.is_tautology()) {
			if (disj == NULL) {
				register_use(first_d);
				unregister_use(first_d);
			}
			else {
				register_use(disj);
				unregister_use(disj);
			}
			return TRUE_FORMULA;
		}
		else if (!d.is_contradiction()) {
			if (first_d->is_contradiction()) {
				first_d = &d;
			}
			else if (disj == NULL) {
				disj = new Disjunction();
				disj->add_disjunct(*first_d);
			}
			if (disj != NULL) {
				disj->add_disjunct(d);
			}
		}
	}
	if (disj != NULL) {
		return *disj;
	}
	else {
		return *first_d;
	}
}

// Print this formula on the given stream with the given bindings.
void Conjunction::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << "(and";
	for (FormulaList::const_iterator fi = get_conjuncts().begin();
		fi != get_conjuncts().end(); fi++) {
		os << ' ';
		(*fi)->print(os, step_id, bindings);
	}
	os << ")";
}


// =================== Disjunction ======================

// Return the negation of this formula.
const Formula& Disjunction::negation() const {
	Conjunction* conj = NULL;
	const Formula* first_c = &TRUE_FORMULA;
	for (FormulaList::const_iterator fi = get_disjuncts().begin();
		fi != get_disjuncts().end(); fi++) {
		const Formula& c = !**fi;
		if (c.is_contradiction()) {
			if (conj == NULL) {
				register_use(first_c);
				unregister_use(first_c);
			}
			else {
				register_use(conj);
				unregister_use(conj);
			}
			return FALSE_FORMULA;
		}
		else if (!c.is_tautology()) {
			if (first_c->is_tautology()) {
				first_c = &c;
			}
			else if (conj == NULL) {
				conj = new Conjunction();
				conj->add_conjunct(*first_c);
			}
			if (conj != NULL) {
				conj->add_conjunct(c);
			}
		}
	}
	if (conj != NULL) {
		return *conj;
	}
	else {
		return *first_c;
	}
}

// Return a formula that separates the given effect from anything definitely asserted by this formula.
const Formula& Disjunction::get_separator(const Effect& effect,
	const Domain& domain) const {
	Conjunction* conj = NULL;
	const Formula* first_c = &TRUE_FORMULA;
	for (FormulaList::const_iterator fi = get_disjuncts().begin();
		fi != get_disjuncts().end(); fi++) {
		const Formula& d = **fi;
		const Formula& c = !d && d.get_separator(effect, domain);
		if (c.is_contradiction()) {
			if (conj == NULL) {
				register_use(first_c);
				unregister_use(first_c);
			}
			else {
				register_use(conj);
				unregister_use(conj);
			}
			return FALSE_FORMULA;
		}
		else if (!c.is_tautology()) {
			if (first_c->is_tautology()) {
				first_c = &c;
			}
			else if (conj == NULL) {
				conj = new Conjunction();
				conj->add_conjunct(*first_c);
			}
			if (conj != NULL) {
				conj->add_conjunct(c);
			}
		}
	}
	if (conj != NULL) {
		return *conj;
	}
	else {
		return *first_c;
	}
}

// Return this formula subject to the given substitutions.
const Formula& Disjunction::get_substitution(const SubstitutionMap& subst) const {
	Disjunction* disj = NULL;
	const Formula* first_d = &FALSE_FORMULA;
	bool changed = false;
	for (FormulaList::const_iterator fi = get_disjuncts().begin();
		fi != get_disjuncts().end(); fi++) {
		const Formula& d = (*fi)->get_substitution(subst);
		if (&d != *fi) {
			changed = true;
		}
		if (d.is_tautology()) {
			if (disj == NULL) {
				register_use(first_d);
				unregister_use(first_d);
			}
			else {
				register_use(disj);
				unregister_use(disj);
			}
			return TRUE_FORMULA;
		}
		else if (!d.is_contradiction()) {
			if (first_d->is_contradiction()) {
				first_d = &d;
			}
			else if (disj == NULL) {
				disj = new Disjunction();
				disj->add_disjunct(*first_d);
			}
			if (disj != NULL) {
				disj->add_disjunct(d);
			}
		}
	}
	if (!changed) {
		if (disj == NULL) {
			register_use(first_d);
			unregister_use(first_d);
		}
		else {
			register_use(disj);
			unregister_use(disj);
		}
		return *this;
	}
	else if (disj != NULL) {
		return *disj;
	}
	else {
		return *first_d;
	}
}

// Return an instantiation of this formula.
const Formula& Disjunction::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {
	Disjunction* disj = NULL;
	const Formula* first_d = &FALSE_FORMULA;
	bool changed = false;
	for (FormulaList::const_iterator fi = get_disjuncts().begin();
		fi != get_disjuncts().end(); fi++) {
		const Formula& d = (*fi)->get_instantiation(subst, problem);
		if (&d != *fi) {
			changed = true;
		}
		if (d.is_tautology()) {
			if (disj == NULL) {
				register_use(first_d);
				unregister_use(first_d);
			}
			else {
				register_use(disj);
				unregister_use(disj);
			}
			return TRUE_FORMULA;
		}
		else if (!d.is_contradiction()) {
			if (first_d->is_contradiction()) {
				first_d = &d;
			}
			else if (disj == NULL) {
				disj = new Disjunction();
				disj->add_disjunct(*first_d);
			}
			if (disj != NULL) {
				disj->add_disjunct(d);
			}
		}
	}
	if (!changed) {
		if (disj == NULL) {
			register_use(first_d);
			unregister_use(first_d);
		}
		else {
			register_use(disj);
			unregister_use(disj);
		}
		return *this;
	}
	else if (disj != NULL) {
		return *disj;
	}
	else {
		return *first_d;
	}
}

// Return the universal base of this formula.
const Formula& Disjunction::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	Disjunction* disj = NULL;
	const Formula* first_d = &FALSE_FORMULA;
	bool changed = false;
	for (FormulaList::const_iterator fi = get_disjuncts().begin();
		fi != get_disjuncts().end(); fi++) {
		const Formula& d = (*fi)->get_universal_base(subst, problem);
		if (&d != *fi) {
			changed = true;
		}
		if (d.is_tautology()) {
			if (disj == NULL) {
				register_use(first_d);
				unregister_use(first_d);
			}
			else {
				register_use(disj);
				unregister_use(disj);
			}
			return TRUE_FORMULA;
		}
		else if (!d.is_contradiction()) {
			if (first_d->is_contradiction()) {
				first_d = &d;
			}
			else if (disj == NULL) {
				disj = new Disjunction();
				disj->add_disjunct(*first_d);
			}
			if (disj != NULL) {
				disj->add_disjunct(d);
			}
		}
	}
	if (!changed) {
		if (disj == NULL) {
			register_use(first_d);
			unregister_use(first_d);
		}
		else {
			register_use(disj);
			unregister_use(disj);
		}
		return *this;
	}
	else if (disj != NULL) {
		return *disj;
	}
	else {
		return *first_d;
	}
}

// Print this formula on the given stream with the given bindings.
void Disjunction::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << "(or";
	for (FormulaList::const_iterator fi = get_disjuncts().begin();
		fi != get_disjuncts().end(); fi++) {
		os << ' ';
		(*fi)->print(os, step_id, bindings);
	}
	os << ")";
}


// =================== Quantification ======================

// =================== Exists ======================

// Return the negation of this formula.
const Quantification& Exists::negation() const {
	Forall& forall = *new Forall();
	for (VariableList::const_iterator vi = get_parameters().begin();
		vi != get_parameters().end(); vi++) {
		forall.add_parameter(*vi);
	}
	forall.set_body(!get_body());
	return forall;
}

// Return this formula subject to the given substitutions.
const Formula& Exists::get_substitution(const SubstitutionMap& subst) const {
	const Formula& b = get_body().get_substitution(subst);
	if (&b == &get_body()) {
		return *this;
	}
	else if (b.is_tautology() || b.is_contradiction()) {
		return b;
	}
	else {
		Exists& exists = *new Exists();
		for (VariableList::const_iterator vi = get_parameters().begin();
			vi != get_parameters().end(); vi++) {
			exists.add_parameter(*vi);
		}
		exists.set_body(b);
		return exists;
	}
}

// Return an instantiation of this formula.
const Formula& Exists::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {
	int n = get_parameters().size();
	if (n == 0) {
		return get_body().get_instantiation(subst, problem);
	}
	else {
		SubstitutionMap args(subst);
		vector<const ObjectList*> arguments(n);
		vector<ObjectList::const_iterator> next_arg;
		for (int i = 0; i < n; i++) {
			const Type& t = TermTable::type(get_parameters()[i]);
			arguments[i] = &problem.get_terms().compatible_objects(t);
			if (arguments[i]->empty()) {
				return FALSE_FORMULA;
			}
			next_arg.push_back(arguments[i]->begin());
		}
		const Formula* result = &FALSE_FORMULA;
		stack<const Formula*> disjuncts;
		disjuncts.push(&get_body().get_instantiation(args, problem));
		register_use(disjuncts.top());
		for (int i = 0; i < n; ) {
			SubstitutionMap pargs;
			pargs.insert(make_pair(get_parameters()[i], *next_arg[i]));
			const Formula& disjunct = disjuncts.top()->get_instantiation(pargs, problem);
			disjuncts.push(&disjunct);
			if (i + 1 == n) {
				result = &(*result || disjunct);
				if (result->is_tautology()) {
					break;
				}
				for (int j = i; j >= 0; j--) {
					if (j < i) {
						unregister_use(disjuncts.top());
					}
					disjuncts.pop();
					next_arg[j]++;
					if (next_arg[j] == arguments[j]->end()) {
						if (j == 0) {
							i = n;
							break;
						}
						else {
							next_arg[j] = arguments[j]->begin();
						}
					}
					else {
						i = j;
						break;
					}
				}
			}
			else {
				register_use(disjuncts.top());
				i++;
			}
		}
		while (!disjuncts.empty()) {
			unregister_use(disjuncts.top());
			disjuncts.pop();
		}
		return *result;
	}
}

// Return the universal base of this formula.
const Formula& Exists::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	const Formula& b = get_body().get_universal_base(subst, problem);
	if (&b == &get_body()) {
		return *this;
	}
	else if (b.is_tautology() || b.is_contradiction()) {
		return b;
	}
	else {
		Exists& exists = *new Exists();
		for (VariableList::const_iterator vi = get_parameters().begin();
			vi != get_parameters().end(); vi++) {
			exists.add_parameter(*vi);
		}
		exists.set_body(b);
		return exists;
	}
}



// Print this formula on the given stream with the given bindings.
void Exists::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << "(exists (";
	for (VariableList::const_iterator vi = get_parameters().begin();
		vi != get_parameters().end(); vi++) {
		if (vi != get_parameters().begin()) {
			os << ' ';
		}
		bindings.print_term(os, *vi, step_id);
	}
	os << ") ";
	get_body().print(os, step_id, bindings);
	os << ")";
}


// =================== Forall ======================

// Return the negation of this formula.
const Quantification& Forall::negation() const {
	Exists& exists = *new Exists();
	for (VariableList::const_iterator vi = get_parameters().begin();
		vi != get_parameters().end(); vi++) {
		exists.add_parameter(*vi);
	}
	exists.set_body(!get_body());
	return exists;
}

// Return this formula subject to the given substitutions.
const Formula& Forall::get_substitution(const SubstitutionMap& subst) const {
	const Formula& b = get_body().get_substitution(subst);
	if (&b == &get_body()) {
		return *this;
	}
	else if (b.is_tautology() || b.is_contradiction()) {
		return b;
	}
	else {
		Forall& forall = *new Forall();
		for (VariableList::const_iterator vi = get_parameters().begin();
			vi != get_parameters().end(); vi++) {
			forall.add_parameter(*vi);
		}
		forall.set_body(b);
		return forall;
	}
}

// Return an instantiation of this formula.
const Formula& Forall::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {
	int n = get_parameters().size();
	if (n == 0) {
		return get_body().get_instantiation(subst, problem);
	}
	else {
		SubstitutionMap args(subst);
		vector<const ObjectList*> arguments(n);
		vector<ObjectList::const_iterator> next_arg;
		for (int i = 0; i < n; i++) {
			const Type& t = TermTable::type(get_parameters()[i]);
			arguments[i] = &problem.get_terms().compatible_objects(t);
			if (arguments[i]->empty()) {
				return TRUE_FORMULA;
			}
			next_arg.push_back(arguments[i]->begin());
		}
		const Formula* result = &TRUE_FORMULA;
		stack<const Formula*> conjuncts;
		conjuncts.push(&get_body().get_instantiation(args, problem));
		register_use(conjuncts.top());
		for (int i = 0; i < n; ) {
			SubstitutionMap pargs;
			pargs.insert(make_pair(get_parameters()[i], *next_arg[i]));
			const Formula& conjunct = conjuncts.top()->get_instantiation(pargs, problem);
			conjuncts.push(&conjunct);
			if (i + 1 == n) {
				result = &(*result && conjunct);
				if (result->is_contradiction()) {
					break;
				}
				for (int j = i; j >= 0; j--) {
					if (j < i) {
						unregister_use(conjuncts.top());
					}
					conjuncts.pop();
					next_arg[j]++;
					if (next_arg[j] == arguments[j]->end()) {
						if (j == 0) {
							i = n;
							break;
						}
						else {
							next_arg[j] = arguments[j]->begin();
						}
					}
					else {
						i = j;
						break;
					}
				}
			}
			else {
				register_use(conjuncts.top());
				i++;
			}
		}
		while (!conjuncts.empty()) {
			unregister_use(conjuncts.top());
			conjuncts.pop();
		}
		return *result;
	}
}

// Return the universal base of this formula.
const Formula& Forall::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	if (universal_base != NULL) {
		return *universal_base;
	}
	int n = get_parameters().size();
	if (n == 0) {
		universal_base = &get_body().get_universal_base(subst, problem);
	}
	else {
		SubstitutionMap args(subst);
		vector<const ObjectList*> arguments(n);
		vector<ObjectList::const_iterator> next_arg;
		for (int i = 0; i < n; i++) {
			const Type& t = TermTable::type(get_parameters()[i]);
			arguments[i] = &problem.get_terms().compatible_objects(t);
			if (arguments[i]->empty()) {
				universal_base = &TRUE_FORMULA;
				return TRUE_FORMULA;
			}
			next_arg.push_back(arguments[i]->begin());
		}
		universal_base = &TRUE_FORMULA;
		stack<const Formula*> conjuncts;
		conjuncts.push(&get_body().get_universal_base(args, problem));
		register_use(conjuncts.top());
		for (int i = 0; i < n; ) {
			SubstitutionMap pargs;
			pargs.insert(make_pair(get_parameters()[i], *next_arg[i]));
			const Formula& conjunct =
				conjuncts.top()->get_universal_base(pargs, problem);
			conjuncts.push(&conjunct);
			if (i + 1 == n) {
				universal_base = &(*universal_base && conjunct);
				if (universal_base->is_contradiction()) {
					break;
				}
				for (int j = i; j >= 0; j--) {
					if (j < i) {
						unregister_use(conjuncts.top());
					}
					conjuncts.pop();
					next_arg[j]++;
					if (next_arg[j] == arguments[j]->end()) {
						if (j == 0) {
							i = n;
							break;
						}
						else {
							next_arg[j] = arguments[j]->begin();
						}
					}
					else {
						i = j;
						break;
					}
				}
			}
			else {
				register_use(conjuncts.top());
				i++;
			}
		}
		while (!conjuncts.empty()) {
			unregister_use(conjuncts.top());
			conjuncts.pop();
		}
	}
	return *universal_base;
}

// Print this formula on the given stream with the given bindings.
void Forall::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << "(forall (";
	for (VariableList::const_iterator vi = get_parameters().begin();
		vi != get_parameters().end(); vi++) {
		if (vi != get_parameters().begin()) {
			os << ' ';
		}
		bindings.print_term(os, *vi, step_id);
	}
	os << ") ";
	get_body().print(os, step_id, bindings);
	os << ")";
}


// =================== TimedLiteral ======================

// Return the negation of this formula.
const TimedLiteral& TimedLiteral::negation() const {
	const Literal& neg_literal = dynamic_cast<const Literal&>(!get_literal());
	return *new TimedLiteral(neg_literal, get_when());
}

// Return a literal with the given time stamp.
const Formula& TimedLiteral::make(const Literal& literal, FormulaTime when) {
	if (when == AT_START) {
		return literal;
	}
	else {
		return *new TimedLiteral(literal, when);
	}
}

// Return a formula that separates the given effect from anything definitely asserted by this formula.
const Formula& TimedLiteral::get_separator(const Effect& effect,
	const Domain& domain) const {
	if ((when == AT_END && effect.get_when() == EffectTime::AT_END)
		|| (when != AT_END && effect.get_when() != EffectTime::AT_END)) {
		return get_literal().get_separator(effect, domain);
	}
	else {
		return TRUE_FORMULA;
	}
}

// Return this formula subject to the given substitutions.
const TimedLiteral& TimedLiteral::get_substitution(const SubstitutionMap& subst) const {
	const Literal& subst_literal = get_literal().get_substitution(subst);
	if (&subst_literal != &get_literal()) {
		return *new TimedLiteral(subst_literal, get_when());
	}
	else {
		return *this;
	}
}

// Return an instantiation of this formula.
const Formula& TimedLiteral::get_instantiation(const SubstitutionMap& subst,
	const Problem& problem) const {
	const Formula& inst_literal = get_literal().get_instantiation(subst, problem);
	if (&inst_literal != &get_literal()) {
		const Literal* l = dynamic_cast<const Literal*>(&inst_literal);
		if (l != NULL) {
			return *new TimedLiteral(*l, get_when());
		}
		else {
			return inst_literal;
		}
	}
	else {
		return *this;
	}
}

// Return the universal base of this formula.
const Formula& TimedLiteral::get_universal_base(const SubstitutionMap& subst,
	const Problem& problem) const {
	return get_instantiation(subst, problem);
}

// Print this formula on the given stream with the given bindings.
void TimedLiteral::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << '(';
	switch (get_when()) {
	case FormulaTime::AT_START_F:
	default:
		os << "at start ";
		break;
	case FormulaTime::OVER_ALL_F:
		os << "over all ";
		break;
	case FormulaTime::AT_END_F:
		os << "at end ";
		break;
	}
	get_literal().print(os, step_id, bindings);
	os << ')';
}