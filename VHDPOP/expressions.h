#pragma once

#include "refcount.h"
#include "functions.h"
#include "terms.h"

class ValueMap;


// ================== Expression ================

// An abstract expression.
class Expression :public RCObject {
protected:
	// Print the object on the given stream. ???
	virtual void print(ostream& os) const = 0;

	friend ostream& operator<<(ostream& os, const Expression& e);

public:
	// Return the value of this expression in the given state.
	virtual float get_value(const ValueMap& values) const = 0;

	// Return an instantiation of this expression.
	virtual const Expression& get_instantiation(const SubstitutionMap& subst,
		const ValueMap& values) const = 0;
};


// ================= Value ===================

// A constant value.
class Value :public Expression {
	// The value.
	float value;

protected:
	// Print the constant value on the given stream.
	virtual void print(ostream& os) const;

public:
	// Construct a constant value.
	explicit Value(float value) :value(value) {}
	
	// Return the value of this constant value.
	float get_value() const { return value; }

	// Return the value of this constant value in the given state.
	virtual float get_value(const ValueMap& values) const;
	
	// Return an instantiation of this expression.
	virtual const Value& get_instantiation(const SubstitutionMap& subst,
		const ValueMap& values) const;
};


// ================= Fluent ===================

// A fluent.
class Fluent :public Expression {

	// Less than comparision function object for fluents.
	class FluentLess
		:public binary_function<const Fluent*, const Fluent*, bool>{
	public:
		// Comparison operator "()" for fluents. 
		bool operator()(const Fluent* f1, const Fluent* f2) const;
	};

	// A table of fluents. (Set of 1. Fluent pointer and 2. FluentLess)
	class FluentTable :public set<const Fluent*, FluentLess> {
	};

	// Table of fluents.
	static FluentTable fluents;

	// Next index for ground fluents.
	static size_t next_id;

	// Unique id for a ground fluents (0 if lifted).
	size_t id;

	// Function of this fluent.
	Function function;

	// Terms of this fluent.
	TermList terms;

	// Add a term to this fluent.
	void add_term(Term term) { terms.push_back(term); }

	// Construct a fluent with the given function.
	explicit Fluent(const Function& function) :function(function) {}

protected:
	// Assign an index to this fluent.
	void assign_id(bool ground);

	// Print this fluent on the given stream.
	virtual void print(ostream& os) const;

public:
	// Destruct this fluent.
	virtual ~Fluent();

	// Return a fluent with the given function and terms.
	static const Fluent& make(const Function& function, const TermList& terms);

	// Return the id for this fluent (0 if lifted).
	size_t get_id() const { return id; }

	// Return the function of this fluent.
	const Function& get_function() const { return function; }

	// Return the terms of this fluent.
	const TermList& get_terms() const { return terms; }

	// Return the value of this expression in the given state.
	virtual float get_value(const ValueMap& values) const;

	// Return an instantiation of this expression.
	virtual const Expression& get_instantiation(const SubstitutionMap& subst,
		const ValueMap& values) const;

	// Return this fluent subject to the given substitution.
	const Fluent& get_substitution(const SubstitutionMap& subst) const;
};

// Less-than specification for fluent pointers.
namespace std {
	template<>
	struct less<const Fluent*>
		: public binary_function<const Fluent*, const Fluent*, bool> {
		// Comparison function operator
		bool operator()(const Fluent* f1, const Fluent* f2) const {
			return f1->get_id() < f2->get_id();
		}
	};
}

//Mapping from (pointers to) fluents to values.
class ValueMap :public map<const Fluent*, float> {
};

// ================= Computation ===================

// A computation expression.
class Computation :public Expression {
	// The first operand (pointer) of this computation
	const Expression* operand1;
	// The second operand (pointer) of this computation
	const Expression* operand2;

protected:
	// Construct a computation.
	Computation(const Expression& op1, const Expression& op2);

public:
	// Destruct a computation.
	virtual ~Computation();

	// Return the first operand of this computation.
	const Expression& get_op1() const { return *operand1; }

	// Return the second operand of this computation.
	const Expression& get_op2() const { return *operand2; }
};


// ================= Addition ===================

// An addition of two expressions.
class Addition :public Computation {
	// Construct an addition.
	Addition(const Expression& exp1, const Expression& exp2)
		:Computation(exp1, exp2) {}

protected:
	// Print this object on the given stream.
	virtual void print(ostream& os) const;

public:
	// Return an addition of two given expressions.
	static const Expression& make(const Expression& exp1,
		const Expression& exp2);

	// Return the value of this expression (addition) in the given state.
	virtual float get_value(const ValueMap& values) const;

	// Return an instantiation of this expression (addition).
	virtual const Expression& get_instantiation(const SubstitutionMap& subst,
		const ValueMap& values) const;
};


// ================= Subtraction ===================

// A subtraction.
class Subtraction :public Computation {
	// Construct a subtraction.
	Subtraction(const Expression& exp1, const Expression& exp2)
		:Computation(exp1, exp2) {}

protected:
	// Print this subtraction on the given stream.
	virtual void print(ostream& os) const;

public:
	// Return a subtraction of two given expressions.
	static const Expression& make(const Expression& exp1,
		const Expression& exp2);

	// Return the value of this expression (subtraction) in the given state.
	virtual float get_value(const ValueMap& values) const;

	// Return an instantiation of this expression (subtraction).
	virtual const Expression& get_instantiation(const SubstitutionMap& subst,
		const ValueMap& values) const;
};


// ================= Multiplication ===================

// A Multiplication.
class Multiplication :public Computation {
	// Construct a multiplication.
	Multiplication(const Expression& exp1, const Expression& exp2)
		:Computation(exp1, exp2) {}

protected:
	// Print this multiplication on the given stream.
	virtual void print(ostream& os) const;

public:
	// Return a multiplication of two given expressions.
	static const Expression& make(const Expression& exp1,
		const Expression& exp2);

	// Return the value of this expression (multiplication) in the given state.
	virtual float get_value(const ValueMap& values) const;

	// Return an instantiation of this expression (multiplication).
	virtual const Expression& get_instantiation(const SubstitutionMap& subst,
		const ValueMap& values) const;
};


// ================= Division ===================

// A division.
class Division :public Computation {
	// Construct a division.
	Division(const Expression& exp1, const Expression& exp2)
		:Computation(exp1, exp2) {}

protected:
	// Print this division on the given stream.
	virtual void print(ostream& os) const;

public:
	// Return a division of two given expressions.
	static const Expression& make(const Expression& exp1,
		const Expression& exp2);

	// Return the value of this expression (division) in the given state.
	virtual float get_value(const ValueMap& values) const;

	// Return an instantiation of this expression (division).
	virtual const Expression& get_instantiation(const SubstitutionMap& subst,
		const ValueMap& values) const;
};


// ================= Minimum ===================

// The minimum of two expressions.
class Minimum :public Computation {
	// Construct a minimum.
	Minimum(const Expression& exp1, const Expression& exp2)
		:Computation(exp1, exp2) {}

protected:
	// Print this minimum on the given stream.
	virtual void print(ostream& os) const;

public:
	// Return the minimum of two given expressions.
	static const Expression& make(const Expression& exp1,
		const Expression& exp2);

	// Return the value of this expression (minimum) in the given state.
	virtual float get_value(const ValueMap& values) const;

	// Return an instantiation of this expression (minimum).
	virtual const Expression& get_instantiation(const SubstitutionMap& subst,
		const ValueMap& values) const;
};


// ================= Maximum ===================

// The maximum of two expressions.
class Maximum :public Computation {
	// Construct a maximum.
	Maximum(const Expression& exp1, const Expression& exp2)
		:Computation(exp1, exp2) {}

protected:
	// Print this maximum on the given stream.
	virtual void print(ostream& os) const;

public:
	// Return the maximum of two given expressions.
	static const Expression& make(const Expression& exp1,
		const Expression& exp2);

	// Return the value of this expression (maximum) in the given state.
	virtual float get_value(const ValueMap& values) const;

	// Return an instantiation of this expression (maximum).
	virtual const Expression& get_instantiation(const SubstitutionMap& subst,
		const ValueMap& values) const;
};