#include "orderings.h"

#include "domains.h"
#include "expressions.h"
#include "heuristics.h"
#include "plans.h"

#include <algorithm>

// =================== Orderings ====================

// Minimum distance between two ordered steps.
float Orderings::threshold = 0.01f;

// Output operator for orderings.
ostream& operator<<(ostream& os, const Orderings& o) {
	o.print(os);
	return os;
}


// =================== BinaryOrderings ====================

// Schedule the given instruction with the given constraints.
float BinaryOrderings::schedule(map<size_t, float>& start_times,
	map<size_t, float>& end_times, size_t step_id) const {
	map<size_t, float>::const_iterator d = start_times.find(step_id);
	if (d != start_times.end()) {
		return (*d).second;
	}
	else {
		float sd = 1.0f;
		size_t n = before.size() + 1;
		for (size_t j = 1; j <= n; j++) {
			if (step_id != j && is_before(j, step_id)) {
				float ed = 1.0f + schedule(start_times, end_times, j);
				if (ed > sd) {
					sd = ed;
				}
			}
		}
		start_times.insert(make_pair(step_id, sd));
		end_times.insert(make_pair(step_id, sd));
		return sd;
	}
}

// Schedule the given instruction with the given constraints.
float BinaryOrderings::schedule(map<size_t, float>& start_times,
	map<size_t, float>& end_times, size_t step_id,
	const map<pair<size_t,
	StepTime::StepPoint>, float>& min_times) const {
	map<size_t, float>::const_iterator d = start_times.find(step_id);
	if (d != start_times.end()) {
		return (*d).second;
	}
	else {
		float sd = threshold;
		size_t n = before.size() + 1;
		for (size_t j = 1; j <= n; j++) {
			if (step_id != j && is_before(j, step_id)) {
				float ed = threshold + schedule(start_times, end_times, j, min_times);
				if (ed > sd) {
					sd = ed;
				}
			}
		}
		map<pair<size_t, StepTime::StepPoint>, float>::const_iterator
			md = min_times.find(make_pair(step_id, StepTime::START));
		if (md == min_times.end()) {
			md = min_times.find(make_pair(step_id, StepTime::END));
		}
		if (md != min_times.end()) {
			if ((*md).second > sd) {
				sd = (*md).second;
			}
		}
		start_times.insert(make_pair(step_id, sd));
		end_times.insert(make_pair(step_id, sd));
		return sd;
	}
}

// Return true if the first step is ordered before the second step.
bool BinaryOrderings::is_before(size_t id1, size_t id2) const {
	if (id1 == id2) {
		return false;
	}
	else if (id1 < id2) {
		return (*before[id2 - 2])[id1 - 1];
	}
	else {
		return (*before[id1 - 2])[2 * id1 - 2 - id2];
	}
}

// Order the first step before the second step.
void BinaryOrderings::set_before(map<size_t, BoolVector*>& own_data,
	size_t id1, size_t id2) {
	if (id1 != id2) {
		size_t i = max(id1, id2) - 2;
		BoolVector* bv;
		map<size_t, BoolVector*>::const_iterator vi = own_data.find(i);
		if (vi != own_data.end()) {
			bv = (*vi).second;
		}
		else {
			const BoolVector* old_bv = before[i];
			bv = new BoolVector(*old_bv);
			BoolVector::register_use(bv);
			BoolVector::unregister_use(old_bv);
			before[i] = bv;
			own_data.insert(make_pair(i, bv));
		}
		if (id1 < id2) {
			(*bv)[id1 - 1] = true;
		}
		else {
			(*bv)[2 * id1 - 2 - id2] = true;
		}
	}
}

// Update the transitive closure given a new ordering constraint.
void BinaryOrderings::fill_transitive(map<size_t, BoolVector*>& own_data,
	const Ordering& ordering) {
	size_t i = ordering.get_before_id();
	size_t j = ordering.get_after_id();
	if (!is_before(i, j)) {
		// Reordering
		size_t n = before.size() + 1;
		for (size_t k = 1; k <= n; k++) {
			if ((k == i || is_before(k, i)) && !is_before(k, j)) {
				for (size_t l = 1; l <= n; l++) {
					if ((j == l || is_before(j, l)) && !is_before(k, l)) {
						set_before(own_data, k, l);
					}
				}
			}
		}
	}
}

// Check if the first step could be ordered before the second step.
bool BinaryOrderings::possibly_before(size_t id1, StepTime t1,
	size_t id2, StepTime t2) const {
	if (id1 == id2) {
		return false;
	}
	else if (id1 == 0 || id2 == Plan::GOAL_ID) {
		return true;
	}
	else if (id1 == Plan::GOAL_ID || id2 == 0) {
		return false;
	}
	else {
		return !is_before(id2, id1);
	}
}

// Check if the first step could be ordered after or at the same time as the second step.
bool BinaryOrderings::possibly_not_before(size_t id1, StepTime t1,
	size_t id2, StepTime t2) const {
	return possibly_after(id1, t1, id2, t2);
}

// Check if the first step could be ordered after the second step.
bool BinaryOrderings::possibly_after(size_t id1, StepTime t1,
	size_t id2, StepTime t2) const {
	if (id1 == id2) {
		return false;
	}
	else if (id1 == 0 || id2 == Plan::GOAL_ID) {
		return false;
	}
	else if (id1 == Plan::GOAL_ID || id2 == 0) {
		return true;
	}
	else {
		return !is_before(id1, id2);
	}
}

// Check if the first step could be ordered before or at the same time as the second step.
bool BinaryOrderings::possibly_not_after(size_t id1, StepTime t1,
	size_t id2, StepTime t2) const {
	return possibly_before(id1, t1, id2, t2);
}

// Check if the two steps are possibly concurrent.
bool BinaryOrderings::possibly_concurrent(size_t id1, size_t id2,
	bool& ss, bool& se, bool& es, bool& ee) const {
	if (id1 == id2 || id1 == 0 || id1 == Plan::GOAL_ID
		|| id2 == 0 || id2 == Plan::GOAL_ID) {
		return false;
	}
	else {
		return ss = se = es = ee = !is_before(id1, id2) && !is_before(id2, id1);
	}
}

// Return the ordering collection with the given addition.
const BinaryOrderings* BinaryOrderings::refine(const Ordering& new_ordering) const {
	if (new_ordering.get_before_id() != 0
		&& new_ordering.get_after_id() != Plan::GOAL_ID
		&& possibly_not_before(new_ordering.get_before_id(),
			new_ordering.get_before_time(),
			new_ordering.get_after_id(),
			new_ordering.get_after_time())) {
		BinaryOrderings& orderings = *new BinaryOrderings(*this);
		map<size_t, BoolVector*> own_data;
		orderings.fill_transitive(own_data, new_ordering);
		return &orderings;
	}
	else {
		return this;
	}
}

// Return the ordering collection with the given additions.
const BinaryOrderings* BinaryOrderings::refine(const Ordering& new_ordering,
	const Step& new_step, const PlanningGraph* pg, const Bindings* bindings) const {
	if (new_step.get_id() != 0 && new_step.get_id() != Plan::GOAL_ID) {
		BinaryOrderings& orderings = *new BinaryOrderings(*this);
		map<size_t, BoolVector*> own_data;
		if (new_step.get_id() > before.size() + 1) {
			if (new_step.get_id() > 1) {
				BoolVector* bv = new BoolVector(2 * new_step.get_id() - 2, false);
				own_data.insert(make_pair(orderings.before.size(), bv));
				orderings.before.push_back(bv);
				BoolVector::register_use(bv);
			}
		}
		if (new_ordering.get_before_id() != 0
			&& new_ordering.get_after_id() != Plan::GOAL_ID) {
			orderings.fill_transitive(own_data, new_ordering);
		}
		return &orderings;
	}
	else {
		return this;
	}
}

// Fill the given tables with distances for each step from the start step, and returns the greatest distance.
float BinaryOrderings::schedule(map<size_t, float>& start_times,
	map<size_t, float>& end_times) const {
	float max_dist = 0.0f;
	size_t n = before.size() + 1;
	for (size_t i = 1; i <= n; i++) {
		float ed = schedule(start_times, end_times, i);
		if (ed > max_dist) {
			max_dist = ed;
		}
	}
	return max_dist;
}

// Return the makespan of this ordering collection.
float BinaryOrderings::makespan(const map<pair<size_t,
	StepTime::StepPoint>, float>& min_times) const {
	map<size_t, float> start_times, end_times;
	float max_dist = 0.0f;
	size_t n = before.size() + 1;
	for (size_t i = 1; i <= n; i++) {
		float ed = schedule(start_times, end_times, i, min_times);
		if (ed > max_dist) {
			max_dist = ed;
		}
	}
	map<pair<size_t, StepTime::StepPoint>, float>::const_iterator md =
		min_times.find(make_pair(Plan::GOAL_ID, StepTime::START));
	if (md != min_times.end()) {
		if ((*md).second > max_dist) {
			max_dist = (*md).second;
		}
	}
	return max_dist;
}

// Print this object on the given stream.
void BinaryOrderings::print(ostream& os) const {
	os << "{";
	size_t n = before.size() + 1;
	for (size_t i = 1; i <= n; i++) {
		for (size_t j = 1; j <= n; j++) {
			if (is_before(i, j)) {
				os << ' ' << i << '<' << j;
			}
		}
	}
	os << " }";
}


// Return the maximum distance from the first and the second time node.
int TemporalOrderings::get_distance(size_t t1, size_t t2) const {
	if (t1 == t2) {
		return 0;
	}
	else if (t1 < t2) {
		return (*distance[t2 - 1])[t1];
	}
	else {
		return (*distance[t1 - 1])[2 * t1 - 1 - t2];
	}
}

// Set the maximum distance from the first and the second time node.
void TemporalOrderings::set_distance(map<size_t, IntVector*>& own_data,
	size_t t1, size_t t2, int d) {
	if (t1 != t2) {
		size_t i = max(t1, t2) - 1;
		IntVector* fv;
		map<size_t, IntVector*>::const_iterator vi = own_data.find(i);
		if (vi != own_data.end()) {
			fv = (*vi).second;
		}
		else {
			const IntVector* old_fv = distance[i];
			fv = new IntVector(*old_fv);
			IntVector::register_use(fv);
			IntVector::unregister_use(old_fv);
			distance[i] = fv;
			own_data.insert(make_pair(i, fv));
		}
		if (t1 < t2) {
			(*fv)[t1] = d;
		}
		else {
			(*fv)[2 * t1 - 1 - t2] = d;
		}
	}
}

// Update the transitive closure given a new ordering constraint.
bool TemporalOrderings::fill_transitive(map<size_t, IntVector*>& own_data,
	size_t i, size_t j, int dist) {
	if (get_distance(j, i) > -dist) {
		// Update the temporal constraints.
		size_t n = distance.size();
		for (size_t k = 0; k <= n; k++) {
			int d_ik = get_distance(i, k);
			if (d_ik < INT_MAX && get_distance(j, k) > d_ik - dist) {
				for (size_t l = 0; l <= n; l++) {
					int d_lj = get_distance(l, j);
					int new_d = d_ik + d_lj - dist;
					if (d_lj < INT_MAX && get_distance(l, k) > new_d) {
						set_distance(own_data, l, k, new_d);
						if (-get_distance(k, l) > new_d) {
							return false;
						}
					}
				}
			}
		}
	}
	return true;
}

// Check if the first step could be ordered before the second step.
bool TemporalOrderings::possibly_before(size_t id1, StepTime t1,
	size_t id2, StepTime t2) const {
	if (id1 == id2 && t1 >= t2) {
		return false;
	}
	else if (id1 == 0 || id2 == Plan::GOAL_ID) {
		return true;
	}
	else if (id1 == Plan::GOAL_ID || id2 == 0) {
		return false;
	}
	else {
		int dist = get_distance(time_node(id1, t1), time_node(id2, t2));
		return dist > 0 || (dist == 0 && t1.rel < t2.rel);
	}
}

// Check if the first step could be ordered after or at the same time as the second step.
bool TemporalOrderings::possibly_not_before(size_t id1, StepTime t1,
	size_t id2, StepTime t2) const {
	if (id1 == id2 && t1 < t2) {
		return false;
	}
	else if (id1 == 0 || id2 == Plan::GOAL_ID) {
		return false;
	}
	else if (id1 == Plan::GOAL_ID || id2 == 0) {
		return true;
	}
	else {
		int dist = get_distance(time_node(id2, t2), time_node(id1, t1));
		return dist > 0 || (dist == 0 && t2.rel <= t1.rel);
	}
}

// ???

// Check if the first step could be ordered after the second step.
bool TemporalOrderings::possibly_after(size_t id1, StepTime t1,
	size_t id2, StepTime t2) const {
	if (id1 == id2 && t1 <= t2) {
		return false;
	}
	else if (id1 == 0 || id2 == Plan::GOAL_ID) {
		return false;
	}
	else if (id1 == Plan::GOAL_ID || id2 == 0) {
		return true;
	}
	else {
		int dist = get_distance(time_node(id2, t2), time_node(id1, t1));
		return dist > 0 || (dist == 0 && t2.rel < t1.rel);
	}
}

// Check if the first step could be ordered before or at the same time as the second step.
bool TemporalOrderings::possibly_not_after(size_t id1, StepTime t1,
	size_t id2, StepTime t2) const {
	if (id1 == id2 && t1 > t2) {
		return false;
	}
	else if (id1 == 0 || id2 == Plan::GOAL_ID) {
		return true;
	}
	else if (id1 == Plan::GOAL_ID || id2 == 0) {
		return false;
	}
	else {
		int dist = get_distance(time_node(id1, t1), time_node(id2, t2));
		return dist > 0 || (dist == 0 && t1.rel <= t2.rel);
	}
}

// Check if the two steps are possibly concurrent.
bool TemporalOrderings::possibly_concurrent(size_t id1, size_t id2, bool& ss, bool& se,
	bool& es, bool& ee) const {
	if (id1 == id2 || id1 == 0 || id1 == Plan::GOAL_ID
		|| id2 == 0 || id2 == Plan::GOAL_ID) {
		return false;
	}
	else {
		size_t t1s = time_node(id1, StepTime::AT_START);
		size_t t1e = time_node(id1, StepTime::AT_END);
		size_t t2s = time_node(id2, StepTime::AT_START);
		size_t t2e = time_node(id2, StepTime::AT_END);
		ss = get_distance(t1s, t2s) >= 0 && get_distance(t2s, t1s) >= 0;
		se = get_distance(t1s, t2e) >= 0 && get_distance(t2e, t1s) >= 0;
		es = get_distance(t1e, t2s) >= 0 && get_distance(t2s, t1e) >= 0;
		ee = get_distance(t1e, t2e) >= 0 && get_distance(t2e, t1e) >= 0;
		return ss || se || es || ee;
	}
}

// Return the ordering collection with the given additions.
const TemporalOrderings* TemporalOrderings::refine(size_t step_id,
	float min_start, float min_end) const {
	if (step_id != 0 && step_id != Plan::GOAL_ID) {
		size_t i = time_node(step_id, StepTime::AT_START);
		size_t j = time_node(step_id, StepTime::AT_END);
		int start = int(min_start / threshold + 0.5);
		int end = int(min_end / threshold + 0.5);
		if (-get_distance(i, 0) >= start && -get_distance(j, 0) >= end) {
			return this;
		}
		else if (get_distance(0, i) < start || get_distance(0, j) < end) {
			return NULL;
		}
		else {
			TemporalOrderings& orderings = *new TemporalOrderings(*this);
			map<size_t, IntVector*> own_data;
			if (orderings.fill_transitive(own_data, 0, i, start)
				&& orderings.fill_transitive(own_data, 0, j, end)) {
				return &orderings;
			}
			else {
				delete &orderings;
				return NULL;
			}
		}
	}
	else {
		return this;
	}
}

// Return the ordering collection with the given additions.
const TemporalOrderings* TemporalOrderings::refine(float time, const Step& new_step) const {
	if (new_step.get_id() != 0 && new_step.get_id() != Plan::GOAL_ID
		&& new_step.get_id() > distance.size() / 2) {
		int itime = int(time / threshold + 0.5);
		TemporalOrderings& orderings = *new TemporalOrderings(*this);
		IntVector* fv = new IntVector(4 * new_step.get_id() - 2, INT_MAX);
		/* Time for start of new step. */
		(*fv)[0] = itime;
		(*fv)[4 * new_step.get_id() - 3] = -itime;
		for (size_t id = 1; id < new_step.get_id(); id++) {
			int t = itime - (*distance[2 * id - 1])[0];
			(*fv)[2 * id - 1] = (*fv)[2 * id] = t;
			(*fv)[4 * new_step.get_id() - 2 * id - 2] = -t;
			(*fv)[4 * new_step.get_id() - 2 * id - 3] = -t;
			// ???
		}
		orderings.distance.push_back(fv);
		IntVector::register_use(fv);
		fv = new IntVector(4 * new_step.get_id(), INT_MAX);
		/* Time for end of new step. */
		(*fv)[0] = itime;
		(*fv)[4 * new_step.get_id() - 1] = -itime;
		for (size_t id = 1; id < new_step.get_id(); id++) {
			int t = itime - (*distance[2 * id - 1])[0];
			(*fv)[2 * id - 1] = (*fv)[2 * id] = t;
			(*fv)[4 * new_step.get_id() - 2 * id] = -t;
			(*fv)[4 * new_step.get_id() - 2 * id - 1] = -t;
		}
		(*fv)[2 * new_step.get_id() - 1] = (*fv)[2 * new_step.get_id()] = 0;
		orderings.distance.push_back(fv);
		IntVector::register_use(fv);
		return &orderings;
	}
	else {
		return this;
	}
}

// Return the ordering collection with the given addition.
const TemporalOrderings* TemporalOrderings::refine(const Ordering& new_ordering) const {
	if (new_ordering.get_before_id() != 0
		&& new_ordering.get_after_id() != Plan::GOAL_ID
		&& possibly_not_before(new_ordering.get_before_id(),
			new_ordering.get_before_time(),
			new_ordering.get_after_id(),
			new_ordering.get_after_time())) {
		TemporalOrderings& orderings = *new TemporalOrderings(*this);
		map<size_t, IntVector*> own_data;
		size_t i = time_node(new_ordering.get_before_id(), new_ordering.get_before_time());
		size_t j = time_node(new_ordering.get_after_id(), new_ordering.get_after_time());
		int dist;
		if (new_ordering.get_before_time().rel < new_ordering.get_after_time().rel) {
			dist = 0;
		}
		else {
			dist = 1;
		}
		if (orderings.fill_transitive(own_data, i, j, dist)) {
			return &orderings;
		}
		else {
			delete &orderings;
			return NULL;
		}
	}
	else {
		return this;
	}
}

// Return the ordering collection with the given additions.
const TemporalOrderings* TemporalOrderings::refine(const Ordering& new_ordering,
	const Step& new_step, const PlanningGraph* pg, const Bindings* bindings) const {
	if (new_step.get_id() != 0 && new_step.get_id() != Plan::GOAL_ID) {
		TemporalOrderings& orderings = *new TemporalOrderings(*this);
		map<size_t, IntVector*> own_data;
		if (new_step.get_id() > distance.size() / 2) {
			const Value* min_v =
				dynamic_cast<const Value*>(&new_step.get_action().get_min_duration());
			if (min_v == NULL) {
				throw runtime_error("non-constant minimum duration");
			}
			const Value* max_v =
				dynamic_cast<const Value*>(&new_step.get_action().get_max_duration());
			if (max_v == NULL) {
				throw runtime_error("non-constant maximum duration");
			}
			float start_time = threshold;
			float end_time;
			if (pg != NULL) {
				HeuristicValue h, hs;
				new_step.get_action().get_condition().get_heuristic_value(h, hs, *pg,
					new_step.get_id(), bindings);
				if (hs.get_makespan() > start_time) {
					start_time = hs.get_makespan();
				}
				end_time = start_time + min_v->get_value();
				if (h.get_makespan() > end_time) {
					end_time = h.get_makespan();
				}
			}
			else {
				end_time = threshold + min_v->get_value();
			}
			IntVector* fv = new IntVector(4 * new_step.get_id() - 2, INT_MAX);
			/* Earliest time for start of new step. */
			(*fv)[4 * new_step.get_id() - 3] = -int(start_time / threshold + 0.5);
			own_data.insert(make_pair(orderings.distance.size(), fv));
			orderings.distance.push_back(fv);
			IntVector::register_use(fv);
			fv = new IntVector(4 * new_step.get_id(), INT_MAX);
			/* Earliest time for end of new step. */
			(*fv)[4 * new_step.get_id() - 1] = -int(end_time / threshold + 0.5);
			if (max_v->get_value() != numeric_limits<float>::infinity()) {
				(*fv)[2 * new_step.get_id() - 1] = int(max_v->get_value() / threshold + 0.5);
			}
			(*fv)[2 * new_step.get_id()] = -int(min_v->get_value() / threshold + 0.5);
			own_data.insert(make_pair(orderings.distance.size(), fv));
			orderings.distance.push_back(fv);
			IntVector::register_use(fv);
		}
		if (new_ordering.get_before_id() != 0) {
			if (new_ordering.get_after_id() != Plan::GOAL_ID) {
				size_t i = time_node(new_ordering.get_before_id(),
					new_ordering.get_before_time());
				size_t j = time_node(new_ordering.get_after_id(),
					new_ordering.get_after_time());
				int dist;
				if (new_ordering.get_before_time().rel < new_ordering.get_after_time().rel) {
					dist = 0;
				}
				else {
					dist = 1;
				}
				if (orderings.fill_transitive(own_data, i, j, dist)) {
					return &orderings;
				}
				else {
					delete &orderings;
					return NULL;
				}
			}
			else {
				orderings.goal_achievers =
					new Chain<size_t>(new_ordering.get_before_id(),
						orderings.goal_achievers);
				RCObject::ref(orderings.goal_achievers);
			}
		}
		return &orderings;
	}
	else {
		return this;
	}
}

// Fill the given tables with distances for each step from the start step, and returns the greatest distance.
float TemporalOrderings::schedule(map<size_t, float>& start_times,
	map<size_t, float>& end_times) const {
	float max_dist = 0.0f;
	size_t n = distance.size() / 2;
	for (size_t i = 1; i <= n; i++) {
		float sd = -get_distance(time_node(i, StepTime::AT_START), 0)*threshold;
		start_times.insert(make_pair(i, sd));
		float ed = -get_distance(time_node(i, StepTime::AT_END), 0)*threshold;
		end_times.insert(make_pair(i, ed));
		if (ed > max_dist
			&& goal_achievers != NULL && goal_achievers->contains(i)) {
			max_dist = ed;
		}
	}
	return max_dist;
}

// Return the makespan of this ordering collection.
float TemporalOrderings::makespan(const map<pair<size_t,
	StepTime::StepPoint>, float>& min_times) const {
	float max_dist = 0.0f;
	size_t n = distance.size() / 2;
	for (size_t i = 1; i <= n; i++) {
		float ed = -get_distance(time_node(i, StepTime::AT_END), 0)*threshold;
		if (ed > max_dist
			&& goal_achievers != NULL && goal_achievers->contains(i)) {
			max_dist = ed;
		}
	}
	return max_dist;
}

// Print this opbject on the given stream.
void TemporalOrderings::print(ostream& os) const {
	size_t n = distance.size();
	for (size_t r = 0; r <= n; r++) {
		os << endl;
		for (size_t c = 0; c <= n; c++) {
			os.width(7);
			int d = get_distance(r, c);
			if (d < INT_MAX) {
				os << d;
			}
			else {
				os << "inf";
			}
		}
	}
}
