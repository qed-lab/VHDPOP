#include "effects.h"
#include "bindings.h"
#include "problems.h"
#include "formulas.h"
#include <algorithm>
#include <stack>


// Return an instantiation of this effect.
const Effect* Effect::get_instantiation(const SubstitutionMap& args,
	const Problem& problem, const Formula& condition) const {
	Effect* inst_eff = new Effect(get_literal().get_substitution(args), get_when());
	inst_eff->set_condition(condition);
	inst_eff->set_link_condition(get_link_condition().get_instantiation(args, problem));
	return inst_eff;
}

// Construct an effect.
Effect::Effect(const Literal& literal, EffectTime when)
	:condition(&Formula::TRUE_FORMULA), link_condition(&Formula::TRUE_FORMULA),
	literal(&literal), when(when) {
	Formula::register_use(this->condition);
	Formula::register_use(this->link_condition);
	Formula::register_use(this->literal);
}

// Destruct an effect.
Effect::~Effect() {
	Formula::unregister_use(this->condition);
	Formula::unregister_use(this->link_condition);
	Formula::unregister_use(this->literal);
}

// Add a universally quantified variable to this effect.
void Effect::add_parameter(Variable parameter) {
	parameters.push_back(parameter);
}

// Set the condition of this effect.
void Effect::set_condition(const Formula& condition) {
	if (this->condition != &condition) {
		Formula::register_use(&condition);
		Formula::unregister_use(this->condition);
		this->condition = &condition;
	}
}

// Set the link condition of this effect.
void Effect::set_link_condition(const Formula& link_condition) const {
	if (this->link_condition != &link_condition) {
		Formula::register_use(&link_condition);
		Formula::unregister_use(this->link_condition);
		this->link_condition = &link_condition;
	}
}

// Test if this effect quantifies the given variable.
bool Effect::quantifies(Variable variable) const {
	return (find(parameters.begin(), parameters.end(), variable)
		!= parameters.end());
}

// Fill the provided list with instantiations of this effect.
void Effect::instantiations(EffectList& effects, size_t& useful,
	const SubstitutionMap& subst,
	const Problem& problem) const {
	size_t n = get_arity();
	if (n == 0) {
		// ???
		const Formula& inst_cond = get_condition().get_instantiation(subst, problem);
		if (!inst_cond.is_contradiction()) {
			const Effect* inst_effect = get_instantiation(subst, problem, inst_cond);
			effects.push_back(inst_effect);
			if (!inst_effect->get_link_condition().is_contradiction()) {
				useful++;
			}
		}
	}
	else {
		SubstitutionMap args(subst);
		vector<const ObjectList*> arguments(n);
		vector<ObjectList::const_iterator> next_arg;
		for (size_t i = 0; i < n; i++) {
			const Type& t = TermTable::type(get_parameter(i));
			arguments[i] = &problem.get_terms().compatible_objects(t);
			if (arguments[i]->empty()) {
				return;
			}
			next_arg.push_back(arguments[i]->begin());
		}
		stack<const Formula*> conds;
		conds.push(&get_condition().get_instantiation(args, problem));
		Formula::register_use(conds.top());
		for (size_t i = 0; i < n; ) {
			args.insert(make_pair(get_parameter(i), *next_arg[i]));
			SubstitutionMap pargs;
			pargs.insert(make_pair(get_parameter(i), *next_arg[i]));
			const Formula& inst_cond = conds.top()->get_instantiation(pargs, problem);
			conds.push(&inst_cond);
			Formula::register_use(conds.top());
			if (i + 1 == n || inst_cond.is_contradiction()) {
				if (!inst_cond.is_contradiction()) {
					const Effect* inst_effect = get_instantiation(args, problem, inst_cond);
					effects.push_back(inst_effect);
					if (!inst_effect->get_link_condition().is_contradiction()) {
						useful++;
					}
				}
				for (int j = i; j >= 0; j--) {
					Formula::unregister_use(conds.top());
					conds.pop();
					args.erase(get_parameter(j));
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
				i++;
			}
		}
		while (!conds.empty()) {
			Formula::unregister_use(conds.top());
			conds.pop();
		}
	}
}

// Print this effect on the given stream.
void Effect::print(ostream& os) const {
	os << '(';
	for (VariableList::const_iterator vi = parameters.begin();
		vi != parameters.end(); vi++) {
		os << *vi << ' ';
	}
	switch (when) {
	case EffectTime::AT_START:
		os << "at start ";
		break;
	case EffectTime::AT_END:
		os << "at end ";
		break;
	}
	os << '[';
	get_condition().print(os, 0, Bindings::EMPTY);
	os << ',';
	get_link_condition().print(os, 0, Bindings::EMPTY);
	os << "->";
	get_literal().print(os, 0, Bindings::EMPTY); //???
	os << ']' << ')';
}