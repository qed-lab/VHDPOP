#include "actions.h"
#include "bindings.h"
#include "problems.h"
#include <stack>

// =================== Action ======================

// Next action id.
size_t Action::next_id = 0;

// Construct an action with the given name.
Action::Action(const string& name, bool durative)
	:id(next_id++), name(name), condition(&Formula::TRUE_FORMULA),
	durative(durative), min_duration(new Value(0.0f)),
	max_duration(new Value(durative ?
		numeric_limits<float>::infinity() : 0.0f)) {
	Formula::register_use(this->condition);
	RCObject::ref(this->min_duration);
	RCObject::ref(this->max_duration);
}


// Destruct this action.
Action::~Action() {
	Formula::unregister_use(condition);
	RCObject::destructive_deref(min_duration);
	RCObject::destructive_deref(max_duration);
	for (EffectList::const_iterator ei = effects.begin();
		ei != effects.end(); ei++) {
		delete *ei;
	}
}

// Set the condition for this action.
void Action::set_condition(const Formula& condition) {
	if (this->condition != &condition) {
		Formula::register_use(&condition);
		Formula::unregister_use(this->condition);
		this->condition = &condition;
	}
}

// Add an effect to this action.
void Action::add_effect(const Effect& effect) {
	effects.push_back(&effect);
}

// Set the minimum duration for this action.
void Action::set_min_duration(const Expression& min_duration) {
	const Expression& md = Maximum::make(*this->min_duration, min_duration);
	if (&md != this->min_duration) {
		RCObject::ref(&md);
		RCObject::destructive_deref(this->min_duration);
		this->min_duration = &md;
	}
}

// Set the maximum duration for this action.
void Action::set_max_duration(const Expression& max_duration) {
	const Expression& md = Minimum::make(*this->max_duration, max_duration);
	if (&md != this->max_duration) {
		RCObject::ref(&md);
		RCObject::destructive_deref(this->max_duration);
		this->max_duration = &md;
	}
}

// Set the duration for this action.
void Action::set_duration(const Expression& duration) {
	set_min_duration(duration);
	set_max_duration(duration);
}

// "Strengthen" the effects of this action.
void Action::strengthen_effects(const Domain& domain) {
	// Separate negative effects from positive effects occuring at the same time.
	for (size_t i = 0; i < effects.size(); i++) {
		const Effect& ei = *effects[i];
		if (typeid(ei.get_literal()) == typeid(Negation)) {
			const Negation& neg = dynamic_cast<const Negation&>(ei.get_literal());
			const Formula* cond = &Formula::TRUE_FORMULA;
			for (EffectList::const_iterator j = effects.begin();
				j != effects.end() && !cond->is_contradiction(); j++) {
				const Effect& ej = **j;
				if (ei.get_when() == ej.get_when() && typeid(ej.get_literal()) == typeid(Atom)) {
					bool diff_param = ei.get_arity() != ej.get_arity();
					for (size_t pi = 0; pi < ei.get_arity() && !diff_param; pi++) {
						if (ei.get_parameter(pi) != ej.get_parameter(pi)) {
							diff_param = true;
						}
					}
					if (!diff_param) {
						// Only separate two effects with same universally quantified variables.
						BindingList mgu;
						if (Bindings::is_unifiable(mgu, neg.get_atom(), 1, ej.get_literal(), 1)) {
							const Formula* sep = &Formula::FALSE_FORMULA;
							for (BindingList::const_iterator si = mgu.begin();
								si != mgu.end(); si++) {
								const Binding& subst = *si;
								sep = &(*sep || Inequality::make(subst.get_var(), subst.get_term()));
							}
							cond = &(*cond && (*sep || !ej.get_condition()));
						}
					}
				}
			}
			if (!cond->is_tautology()) {
				ei.set_link_condition(*cond);
			}
		}
	}

	// Separate effects from conditions asserted at the same time.
	for (size_t i = 0; i < effects.size(); i++) {
		const Effect& ei = *effects[i];
		ei.set_link_condition(ei.get_link_condition()
			&& get_condition().get_separator(ei, domain));
	}
}



// =================== ActionSchema ======================

// Return an instantiation of this action schema.
const GroundAction* ActionSchema::get_instantiation(const SubstitutionMap& args,
	const Problem& problem, const Formula& condition) const {
	EffectList inst_effects;
	size_t useful = 0;
	for (EffectList::const_iterator ei = get_effects().begin();
		ei != get_effects().end(); ei++) {
		(*ei)->instantiations(inst_effects, useful, args, problem);
	}
	if (useful > 0) {
		GroundAction& ga = *new GroundAction(get_name(), is_durative());
		size_t n = get_parameters().size();
		for (size_t i = 0; i < n; i++) {
			SubstitutionMap::const_iterator si = args.find(get_parameters()[i]);
			ga.add_argument((*si).second.as_object());
		}
		ga.set_condition(condition);
		for (EffectList::const_iterator ei = inst_effects.begin();
			ei != inst_effects.end(); ei++) {
			ga.add_effect(**ei);
		}
		ga.set_min_duration(get_min_duration().get_instantiation(args,
			problem.get_init_values()));
		ga.set_max_duration(get_max_duration().get_instantiation(args,
			problem.get_init_values()));
		const Value* v1 = dynamic_cast<const Value*>(&ga.get_min_duration());
		if (v1 != NULL) {
			const Value* v2 = dynamic_cast<const Value*>(&ga.get_max_duration());
			if (v2 != NULL) {
				if (v1->get_value() > v2->get_value()) {
					delete &ga;
					return NULL;
				}
			}
		}
		return &ga;
	}
	else {
		for (EffectList::const_iterator ei = inst_effects.begin();
			ei != inst_effects.end(); ei++) {
			delete *ei;
		}
		return NULL;
	}
}

// Fill the provided action list with all instantiations of this action schema.
void ActionSchema::instantiations(GroundActionList& actions, const Problem& problem) const {
	size_t n = get_parameters().size();
	if (n == 0) {
		const GroundAction* inst_action =
			get_instantiation(SubstitutionMap(), problem, get_condition());
		if (inst_action != NULL) {
			actions.push_back(inst_action);
		}
	}
	else {
		SubstitutionMap args;
		vector<const ObjectList*> arguments(n);
		vector<ObjectList::const_iterator> next_arg;
		for (size_t i = 0; i < n; i++) {
			const Type& t = TermTable::type(get_parameters()[i]);
			arguments[i] = &problem.get_terms().compatible_objects(t);
			if (arguments[i]->empty()) {
				return;
			}
			next_arg.push_back(arguments[i]->begin());
		}
		stack<const Formula*> conds;
		conds.push(&get_condition());
		Formula::register_use(conds.top());
		for (size_t i = 0; i < n; ) {
			args.insert(make_pair(get_parameters()[i], *next_arg[i]));
			SubstitutionMap pargs;
			pargs.insert(make_pair(get_parameters()[i], *next_arg[i]));
			const Formula& inst_cond = conds.top()->get_instantiation(pargs, problem);
			conds.push(&inst_cond);
			Formula::register_use(conds.top());
			if (i + 1 == n || inst_cond.is_contradiction()) {
				if (!inst_cond.is_contradiction()) {
					const GroundAction* inst_action =
						get_instantiation(args, problem, inst_cond);
					if (inst_action != NULL) {
						actions.push_back(inst_action);
					}
				}
				for (int j = i; j >= 0; j--) {
					Formula::unregister_use(conds.top());
					conds.pop();
					args.erase(get_parameters()[j]);
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

// Print this action on the given stream.
void ActionSchema::print(ostream& os) const {
	os << "  " << get_name();
	os << endl << "    parameters:";
	for (VariableList::const_iterator vi = get_parameters().begin();
		vi != get_parameters().end(); vi++) {
		os << ' ' << *vi;
	}
	if (is_durative()) {
		os << endl << "    duration: [" << get_min_duration() << ','
			<< get_max_duration() << "]";
	}
	os << endl << "    condition: ";
	get_condition().print(os, 0, Bindings::EMPTY);
	os << endl << "    effect: (and";
	for (EffectList::const_iterator ei = get_effects().begin();
		ei != get_effects().end(); ei++) {
		os << ' ';
		(*ei)->print(os);
	}
	os << ")";
}

// Print this action on the given stream with the given bindings.
void ActionSchema::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << '(' << get_name();
	for (VariableList::const_iterator ti = get_parameters().begin();
		ti != get_parameters().end(); ti++) {
		os << ' ';
		bindings.print_term(os, *ti, step_id);
	}
	os << ')';
}




// Print this ground action on the given stream with the given bindings.
void GroundAction::print(ostream& os,
	size_t step_id, const Bindings& bindings) const {
	os << '(' << get_name();
	for (ObjectList::const_iterator ni = get_arguments().begin();
		ni != get_arguments().end(); ni++) {
		os << ' ' << *ni;
	}
	os << ')';
}