#include "expressions.h"
#include <stdexcept>
#include <algorithm>

// Output operator for expressions.
ostream& operator<<(ostream& os, const Expression& e) {
	e.print(os);
	return os;
}

// ================= Value ===================

// Print the constant value on the given stream.
void Value::print(ostream& os) const {
	os << value;
}

// Return the value of this constant value in the given state.
float Value::get_value(const ValueMap& values) const {
	return value;
}

// Return an instantiation of this expression.
const Value& Value::get_instantiation(const SubstitutionMap& subst,
	const ValueMap& values) const {
	return *this;
}

// ================= Fluent ===================

// Table of fluents.
Fluent::FluentTable Fluent::fluents;

// Next index for ground fluents.
size_t Fluent::next_id = 1;

// Comparison operator "()" for fluents. 
bool Fluent::FluentLess::operator()(const Fluent* f1, const Fluent* f2) const {
	if (f1->get_function() < f2->get_function()) {
		return true;
	}
	else if (f2->get_function() < f1->get_function()) {
		return false;
	}
	else {
		return f1->get_terms() < f2->get_terms();
	}
}

// Assign an index to this fluent.
void Fluent::assign_id(bool ground) {
	if (ground) {
		id = next_id++;
	}
	else {
		id = 0;
	}
}

// Print this fluent on the given stream.
void Fluent::print(ostream& os) const {
	// diff here, original: get_function/get terms
	os << '(' << function;
	for (TermList::const_iterator ti = terms.begin();
		ti != terms.end(); ++ti) {
		os << ' ' << *ti;
	}
	os << ')';
}

// Destruct this fluent.
Fluent::~Fluent() {
	FluentTable::const_iterator fi = fluents.find(this);
	if (*fi == this) {
		fluents.erase(fi);
	}
}

// Return a fluent with the given function and terms.
const Fluent& Fluent::make(const Function& function, const TermList& terms) {
	Fluent* fluent = new Fluent(function);
	bool ground = true;
	for (TermList::const_iterator ti = terms.begin();
		ti != terms.end(); ++ti) {
		fluent->add_term(*ti);
		if (ground && (*ti).is_variable()) {
			ground = false;
		}
	}

	if (!ground) {
		fluent->assign_id(ground);
		return *fluent;
	}

	else {
		pair<FluentTable::const_iterator, bool> result =
			fluents.insert(fluent);
		if (!result.second) {
			// If the fluent already exists, no need to assign a new id.
			delete fluent;
			return **result.first;
		}
		else {
			fluent->assign_id(ground);
			return *fluent;
		}
	}

}

// Return the value of this expression in the given state.
float Fluent::get_value(const ValueMap& values) const {
	ValueMap::const_iterator vi = values.find(this);
	if (vi != values.end()) {
		return (*vi).second;
	}
	else {
		throw logic_error("Value of fluent is undefined.");
	}
}

// Return an instantiation of this expression (this fluent).
const Expression& Fluent::get_instantiation(const SubstitutionMap& subst,
	const ValueMap& values) const {
	if (terms.empty()) {
		// diff here, get_function()
		if (FunctionTable::is_static(function)) {
			ValueMap::const_iterator vi = values.find(this);
			if (vi != values.end()) {
				// If the function is static and no terms, return the value.
				return *new Value((*vi).second);
			}
			else {
				throw logic_error("Value of static fluent is undefined.");
			}
		}
		else {
			//  If the function is not static and no terms, return the fluent.
			return *this;
		}
	}
	else {
		TermList inst_terms;
		bool substituted = false;
		size_t objects = 0;
		for (TermList::const_iterator ti = terms.begin();
			ti != terms.end(); ++ti) {
			SubstitutionMap::const_iterator si =
				(*ti).is_variable() ? subst.find((*ti).as_variable()) : subst.end();
			if (si != subst.end()) {
				inst_terms.push_back((*si).second);
				substituted = true;
				objects++;
			}
			else {
				inst_terms.push_back(*ti);
				if ((*ti).is_object()) {
					objects++;
				}
			}
		}
		if (substituted) {
			// diff here, original: get_function()
			const Fluent& inst_fluent = make(function, inst_terms);
			if (FunctionTable::is_static(function)
				&& objects == inst_terms.size()) {
				ValueMap::const_iterator vi = values.find(&inst_fluent);
				if (vi != values.end()) {
					return *new Value((*vi).second);
				}
				else {
					throw logic_error("Value of static fluent is undefined.");
				}
			}
			else {
				return inst_fluent;
			}
		}
		else {
			return *this;
		}
	}
}

// Return this fluent subject to the given substitution.
const Fluent& Fluent::get_substitution(const SubstitutionMap& subst) const {
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
		// diff here, get_function()
		return make(function, inst_terms);
	}
	else {
		return *this;
	}
}


// ================= Computation ===================

Computation::Computation(const Expression& op1, const Expression& op2)
	:operand1(&op1), operand2(&op2) {
	ref(operand1);
	ref(operand2);
}

// Destruct a computation.
Computation::~Computation() {
	destructive_deref(operand1);
	destructive_deref(operand2);
}


// ================= Addition ===================

// Print this addition on the given stream.
void Addition::print(ostream& os) const {
	os << "(+ " << get_op1() << ' ' << get_op2() << ")";
}

// Return an addition of two given expressions.
const Expression& Addition::make(const Expression& exp1,
	const Expression& exp2) {

	const Value* v1 = dynamic_cast<const Value*>(&exp1);
	if (v1 != 0) {
		const Value* v2 = dynamic_cast<const Value*>(&exp2);
		if (v2 != 0) {
			const Value& value = *new Value(v1->get_value() + v2->get_value());
			// why like this?
			ref(v1);
			ref(v2);
			destructive_deref(v1);
			destructive_deref(v2);
			return value;
		}
	}
	return *new Addition(exp1, exp2);
}

// Return the value of this expression (addition) in the given state.
float Addition::get_value(const ValueMap& values) const {
	return get_op1().get_value(values) + get_op2().get_value(values);
}

// Return an instantiation of this expression (addition).
const Expression& Addition::get_instantiation(const SubstitutionMap& subst,
	const ValueMap& values) const {
	const Expression& inst_op1 = get_op1().get_instantiation(subst, values);
	const Expression& inst_op2 = get_op2().get_instantiation(subst, values);
	if (&inst_op1 == &get_op1() && &inst_op2 == &get_op2()) {
		return *this;
	}
	else {
		return make(inst_op1, inst_op2);
	}
}


// ================= Subtraction ===================

// Print this subtraction on the given stream.
void Subtraction::print(ostream& os) const {
	os << "(- " << get_op1() << ' ' << get_op2() << ")";
}

// Return an subtraction of two given expressions.
const Expression& Subtraction::make(const Expression& exp1,
	const Expression& exp2) {

	const Value* v1 = dynamic_cast<const Value*>(&exp1);
	if (v1 != 0) {
		const Value* v2 = dynamic_cast<const Value*>(&exp2);
		if (v2 != 0) {
			const Value& value = *new Value(v1->get_value() - v2->get_value());
			// why like this?
			ref(v1);
			ref(v2);
			destructive_deref(v1);
			destructive_deref(v2);
			return value;
		}
	}
	return *new Subtraction(exp1, exp2);
}

// Return the value of this expression (subtraction) in the given state.
float Subtraction::get_value(const ValueMap& values) const {
	return get_op1().get_value(values) - get_op2().get_value(values);
}

// Return an instantiation of this expression (subtraction).
const Expression& Subtraction::get_instantiation(const SubstitutionMap& subst,
	const ValueMap& values) const {
	const Expression& inst_op1 = get_op1().get_instantiation(subst, values);
	const Expression& inst_op2 = get_op2().get_instantiation(subst, values);
	if (&inst_op1 == &get_op1() && &inst_op2 == &get_op2()) {
		return *this;
	}
	else {
		return make(inst_op1, inst_op2);
	}
}


// ================= Multiplication ===================

// Print this multiplication on the given stream.
void Multiplication::print(ostream& os) const {
	os << "(// " << get_op1() << ' ' << get_op2() << ")";
}

// Return an multiplication of two given expressions.
const Expression& Multiplication::make(const Expression& exp1,
	const Expression& exp2) {

	const Value* v1 = dynamic_cast<const Value*>(&exp1);
	if (v1 != 0) {
		const Value* v2 = dynamic_cast<const Value*>(&exp2);
		if (v2 != 0) {
			const Value& value = *new Value(v1->get_value() * v2->get_value());
			// why like this?
			ref(v1);
			ref(v2);
			destructive_deref(v1);
			destructive_deref(v2);
			return value;
		}
	}
	return *new Multiplication(exp1, exp2);
}

// Return the value of this expression (multiplication) in the given state.
float Multiplication::get_value(const ValueMap& values) const {
	return get_op1().get_value(values) * get_op2().get_value(values);
}

// Return an instantiation of this expression (Multiplication).
const Expression& Multiplication::get_instantiation(const SubstitutionMap& subst,
	const ValueMap& values) const {
	const Expression& inst_op1 = get_op1().get_instantiation(subst, values);
	const Expression& inst_op2 = get_op2().get_instantiation(subst, values);
	if (&inst_op1 == &get_op1() && &inst_op2 == &get_op2()) {
		return *this;
	}
	else {
		return make(inst_op1, inst_op2);
	}
}


// ================= Division ===================

// Print this division on the given stream.
void Division::print(ostream& os) const {
	os << "(/ " << get_op1() << ' ' << get_op2() << ")";
}

// Return an division of two given expressions.
const Expression& Division::make(const Expression& exp1,
	const Expression& exp2) {

	const Value* v1 = dynamic_cast<const Value*>(&exp1);
	if (v1 != 0) {
		const Value* v2 = dynamic_cast<const Value*>(&exp2);
		if (v2 != 0) {
			if (v2->get_value() == 0) {
				throw logic_error("Division by 0!");
			}
			const Value& value = *new Value(v1->get_value() / v2->get_value());
			// why like this?
			ref(v1);
			ref(v2);
			destructive_deref(v1);
			destructive_deref(v2);
			return value;
		}
	}
	return *new Division(exp1, exp2);
}

// Return the value of this expression (division) in the given state.
float Division::get_value(const ValueMap& values) const {
	float v2 = get_op2().get_value(values);
	if (v2 == 0) {
		throw logic_error("Division by 0!");
	}
	return get_op1().get_value(values) / v2;
}

// Return an instantiation of this expression (division).
const Expression& Division::get_instantiation(const SubstitutionMap& subst,
	const ValueMap& values) const {
	const Expression& inst_op1 = get_op1().get_instantiation(subst, values);
	const Expression& inst_op2 = get_op2().get_instantiation(subst, values);
	if (&inst_op1 == &get_op1() && &inst_op2 == &get_op2()) {
		return *this;
	}
	else {
		return make(inst_op1, inst_op2);
	}
}


// ================= Minimum ===================

// Print this minimum on the given stream.
void Minimum::print(ostream& os) const {
	os << "(min " << get_op1() << ' ' << get_op2() << ")";
}

// Return the minimum of two given expressions.
const Expression& Minimum::make(const Expression& exp1,
	const Expression& exp2) {

	const Value* v1 = dynamic_cast<const Value*>(&exp1);
	if (v1 != 0) {
		const Value* v2 = dynamic_cast<const Value*>(&exp2);
		if (v2 != 0) {
			if (v1->get_value() < v2->get_value()) {
				ref(v2);
				destructive_deref(v2);
				return *v1;
			}
			else {
				ref(v1);
				destructive_deref(v1);
				return *v2;
			}
		}
	}
	return *new Minimum(exp1, exp2);
}

// Return the value of this expression (minimum) in the given state.
float Minimum::get_value(const ValueMap& values) const {
	return min(get_op1().get_value(values), get_op2().get_value(values));
}

// Return an instantiation of this expression (Minimum).
const Expression& Minimum::get_instantiation(const SubstitutionMap& subst,
	const ValueMap& values) const {
	const Expression& inst_op1 = get_op1().get_instantiation(subst, values);
	const Expression& inst_op2 = get_op2().get_instantiation(subst, values);
	if (&inst_op1 == &get_op1() && &inst_op2 == &get_op2()) {
		return *this;
	}
	else {
		return make(inst_op1, inst_op2);
	}
}


// ================= Maximum ===================

// Print this maximum on the given stream.
void Maximum::print(ostream& os) const {
	os << "(max " << get_op1() << ' ' << get_op2() << ")";
}

// Return the maximum of two given expressions.
const Expression& Maximum::make(const Expression& exp1,
	const Expression& exp2) {

	const Value* v1 = dynamic_cast<const Value*>(&exp1);
	if (v1 != 0) {
		const Value* v2 = dynamic_cast<const Value*>(&exp2);
		if (v2 != 0) {
			if (v1->get_value() > v2->get_value()) {
				ref(v2);
				destructive_deref(v2);
				return *v1;
			}
			else {
				ref(v1);
				destructive_deref(v1);
				return *v2;
			}
		}
	}
	return *new Maximum(exp1, exp2);
}

// Return the value of this expression (maximum) in the given state.
float Maximum::get_value(const ValueMap& values) const {
	return max(get_op1().get_value(values), get_op2().get_value(values));
}

// Return an instantiation of this expression (maximum).
const Expression& Maximum::get_instantiation(const SubstitutionMap& subst,
	const ValueMap& values) const {
	const Expression& inst_op1 = get_op1().get_instantiation(subst, values);
	const Expression& inst_op2 = get_op2().get_instantiation(subst, values);
	if (&inst_op1 == &get_op1() && &inst_op2 == &get_op2()) {
		return *this;
	}
	else {
		return make(inst_op1, inst_op2);
	}
}