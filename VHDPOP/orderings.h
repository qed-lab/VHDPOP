#pragma once

#include "chain.h"
#include "formulas.h"

#include <map>
#include <vector>

class Effect;
class Step;


// =================== StepTime ====================

// A step time.
struct StepTime {
	typedef enum { START, END } StepPoint;
	typedef enum { BEFORE, AT, AFTER } StepRel;

	static const StepTime AT_START;
	static const StepTime AFTER_START;
	static const StepTime BEFORE_END;
	static const StepTime AT_END;

	StepPoint point;
	StepRel rel;

private:
	StepTime(StepPoint point, StepRel rel) : point(point), rel(rel) {}
};

inline bool operator==(const StepTime& st1, const StepTime& st2) {
	return st1.point == st2.point && st1.rel == st2.rel;
}

// diff here
inline bool operator<(const StepTime& st1, const StepTime& st2) {
	return (st1.point < st2.point
		|| (st1.point == st2.point && st1.rel < st2.rel));
}

inline bool operator<=(const StepTime& st1, const StepTime& st2) {
	return (st1.point <= st2.point
		|| (st1.point == st2.point && st1.rel <= st2.rel));
}

inline bool operator>(const StepTime& st1, const StepTime& st2) {
	return (st1.point > st2.point
		|| (st1.point == st2.point && st1.rel > st2.rel));
}

inline bool operator>=(const StepTime& st1, const StepTime& st2) {
	return (st1.point >= st2.point
		|| (st1.point == st2.point && st1.rel >= st2.rel));
}


// Return the step time corresponding to the end time of the given effect.
StepTime end_time(const Effect& e) {
	if (e.get_when() == EffectTime::AT_START) {
		return StepTime::AT_START;
	}
	else {
		return StepTime::AT_END;
	}
}

// Return the step time corresponding to the end time of the given formula time.
StepTime end_time(FormulaTime ft) {
	if (ft == FormulaTime::AT_START_F) {
		return StepTime::AT_START;
	}
	else if (ft == FormulaTime::AT_END_F) {
		return StepTime::AT_END;
	}
	else {
		return StepTime::BEFORE_END;
	}
}

// Return the step time corresponding to the start time of the given formula time.
StepTime start_time(FormulaTime ft) {
	if (ft == FormulaTime::AT_START_F) {
		return StepTime::AT_START;
	}
	else if (ft == FormulaTime::AT_END_F) {
		return StepTime::AT_END;
	}
	else {
		return StepTime::AFTER_START;
	}
}



// =================== Ordering ====================

// An ordering constraint between plan steps.
struct Ordering {
private:
	// Preceeding step id.
	size_t before_id;
	// Time point of preceeding step.
	StepTime before_time;
	// Succeeding step id.
	size_t after_id;
	// Time point of suceeding step.
	StepTime after_time;

public:
	// Construct an ordering constraint.
	Ordering(size_t before_id, StepTime before_time,
		size_t after_id, StepTime after_time)
		: before_id(before_id), before_time(before_time),
		after_id(after_id), after_time(after_time) {}

	// Return the preceeding step id.
	size_t get_before_id() const { return before_id; }

	// Return the time point of preceeding step.
	StepTime get_before_time() const { return before_time; }

	// Return the suceeding step id.
	size_t get_after_id() const { return after_id; }

	// Return the time point of the preceeding step.
	StepTime get_after_time() const { return after_time; }
};


// =================== Orderings ====================

// A collection of ordering constraints.
class Orderings :public RCObject {
	//// Reference counter. diff
	//mutable size_t ref_count_;

	friend ostream& operator<<(ostream& os, const Orderings& o);

protected:
	// Construct an empty ordering collection.
	Orderings() {
#ifdef DEBUG_MEMORY
		++created_orderings;
#endif
	}

	// Construct a copy of this ordering collection.
	Orderings(const Orderings& o) {
#ifdef DEBUG_MEMORY
		++created_orderings;
#endif
	}

	// Print this object on the given stream.
	virtual void print(ostream& os) const = 0;

public:
	// Minimum distance between two ordered steps.
	static float threshold;

	// Register use of this object.
	static void register_use(const Orderings* o) {
		ref(o);
	}

	// Unregister use of this object.
	static void unregister_use(const Orderings* o) {
		destructive_deref(o);
	}

	// Deletes this ordering collection.
	virtual ~Orderings() {
#ifdef DEBUG_MEMORY
		++deleted_orderings;
#endif
	}

	// Check if the first step could be ordered before the second step.
	virtual bool possibly_before(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const = 0;

	// Check if the first step could be ordered after or at the same time as the second step.
	virtual bool possibly_not_before(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const = 0;

	// Check if the first step could be ordered after the second step.
	virtual bool possibly_after(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const = 0;

	// Check if the first step could be ordered before or at the same time as the second step.
	virtual bool possibly_not_after(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const = 0;

	// Check if the two steps are possibly concurrent.
	virtual bool possibly_concurrent(size_t id1, size_t id2, bool& ss, bool& se,
		bool& es, bool& ee) const = 0;

	// Return the ordering collection with the given addition.
	virtual const Orderings* refine(const Ordering& new_ordering) const = 0;

	// Return the ordering collection with the given additions.
	virtual const Orderings* refine(const Ordering& new_ordering,
		const Step& new_step,
		const PlanningGraph* pg,
		const Bindings* bindings) const = 0;

	// Fill the given tables with distances for each step from the start step, and returns the greatest distance.
	virtual float schedule(map<size_t, float>& start_times,
		map<size_t, float>& end_times) const = 0;

	// Return the makespan of this ordering collection.
	virtual float makespan(const map<pair<size_t,
		StepTime::StepPoint>, float>& min_times) const = 0;
};



// =================== BoolVector ====================

// A collectible bool vector.
class BoolVector :public vector<bool>, public RCObject {
public:
	// Register use of the given vector. diff
	static void register_use(const BoolVector* v) {
		ref(v);
	}

	// Unregister use of the given vector.
	static void unregister_use(const BoolVector* v) {
		destructive_deref(v);
	}

	// Construct a vector with n copies of b.
	BoolVector(size_t n, bool b)
		:vector<bool>(n, b) {
#ifdef DEBUG_MEMORY
		++created_bool_vectors;
#endif //DEBUG_MEMORY
	}

	// Construct a copy of the given vector.
	BoolVector(const BoolVector& v)
		:vector<bool>(v) {
#ifdef DEBUG_MEMORY
		++created_bool_vectors;
#endif //DEBUG_MEMORY
	}

#ifdef DEBUG_MEMORY
	// Destruct this vector. ???
	~BoolVector() {
		++deleted_bool_vectors;
	}
#endif //DEBUG_MEMORY
};


// =================== BinaryOrderings ====================

// A collection of binary ordering constraints.
class BinaryOrderings :public Orderings {
	// Matrix representing the transitive closure of the ordering constraints.
	vector<const BoolVector*> before;

	// Construct a copy of this ordering collection.
	BinaryOrderings(const BinaryOrderings& o) {
		size_t n = before.size();
		for (size_t i = 0; i < n; i++) {
			BoolVector::register_use(before[i]);
		}
	}

	// Schedule the given instruction with the given constraints.
	float schedule(map<size_t, float>& start_times,
		map<size_t, float>& end_times, size_t step_id) const;

	// Schedule the given instruction with the given constraints.
	float schedule(map<size_t, float>& start_times,
		map<size_t, float>& end_times, size_t step_id,
		const map<pair<size_t,
		StepTime::StepPoint>, float>& min_times) const;

	// Return true if the first step is ordered before the second step.
	bool is_before(size_t id1, size_t id2) const;

	// Order the first step before the second step.
	void set_before(map<size_t, BoolVector*>& own_data,
		size_t id1, size_t id2);

	// Update the transitive closure given a new ordering constraint.
	void fill_transitive(map<size_t, BoolVector*>& own_data,
		const Ordering& ordering);

protected:
	// Print this object on the given stream.
	virtual void print(ostream& os) const;

public:
	// Construct an empty ordering collection.
	BinaryOrderings() {}

	// Destruct this ordering collection.
	virtual ~BinaryOrderings() {
		size_t n = before.size();
		for (size_t i = 0; i < n; i++) {
			BoolVector::unregister_use(before[i]);
		}
	}

	// Check if the first step could be ordered before the second step.
	virtual bool possibly_before(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const;

	// Check if the first step could be ordered after or at the same time as the second step.
	virtual bool possibly_not_before(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const;

	// Check if the first step could be ordered after the second step.
	virtual bool possibly_after(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const;

	// Check if the first step could be ordered before or at the same time as the second step.
	virtual bool possibly_not_after(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const;

	// Check if the two steps are possibly concurrent.
	virtual bool possibly_concurrent(size_t id1, size_t id2, bool& ss, bool& se,
		bool& es, bool& ee) const;

	// Return the ordering collection with the given addition.
	virtual const BinaryOrderings* refine(const Ordering& new_ordering) const;

	// Return the ordering collection with the given additions.
	virtual const BinaryOrderings* refine(const Ordering& new_ordering,
		const Step& new_step,
		const PlanningGraph* pg,
		const Bindings* bindings) const;

	// Fill the given tables with distances for each step from the start step, and returns the greatest distance.
	virtual float schedule(map<size_t, float>& start_times,
		map<size_t, float>& end_times) const;

	// Return the makespan of this ordering collection.
	virtual float makespan(const map<pair<size_t,
		StepTime::StepPoint>, float>& min_times) const;
};


// =================== IntVector ====================

// A collectible bool vector.
class IntVector :public vector<int>, public RCObject {
public:
	// diff here

	// Register use of the given vector.
	static void register_use(const IntVector* v) {
		ref(v);
	}

	// Unregister use of the given vector.
	static void unregister_use(const IntVector* v) {
		destructive_deref(v);
	}

	// Construct a vector with n copies of b.
	IntVector(size_t n, int f)
		:vector<int>(n, f) {
#ifdef DEBUG_MEMORY
		++created_float_vectors;
#endif
	}

	// Construct a copy of the given vector.
	IntVector(const IntVector& v)
		:vector<int>(v) {
#ifdef DEBUG_MEMORY
		++created_float_vectors;
#endif
	}

#ifdef DEBUG_MEMORY
	// Destruct this vector.
	~IntVector() {
		++deleted_float_vectors;
	}
#endif //DEBUG_MEMORY
};


// =================== TemporalOrderings ====================

// A collection of temporal ordering constraints.
class TemporalOrderings :public Orderings {

	// Matrix representing the minimal network for the ordering constraints.
	vector<const IntVector*> distance;
	// Steps that are linked to the goal.
	const Chain<size_t>* goal_achievers;

	// Construct a copy of this ordering collection.
	TemporalOrderings(const TemporalOrderings& o)
		:Orderings(o), distance(o.distance), goal_achievers(o.goal_achievers) {
		size_t n = distance.size();
		for (size_t i = 0; i < n; i++) {
			IntVector::register_use(distance[i]);
		}
		RCObject::ref(goal_achievers);
	}

	// Return the time node for the given step.
	size_t time_node(size_t id, StepTime t) const {
		return (t.point == StepTime::START) ? 2 * id - 1 : 2 * id;
	}

	// Return the maximum distance from the first and the second time node.
	int get_distance(size_t t1, size_t t2) const;

	// Set the maximum distance from the first and the second time node.
	void set_distance(map<size_t, IntVector*>& own_data,
		size_t t1, size_t t2, int d);

	// Update the transitive closure given a new ordering constraint.
	bool fill_transitive(map<size_t, IntVector*>& own_data,
		size_t i, size_t j, int dist);

protected:
	// Print this opbject on the given stream.
	virtual void print(ostream& os) const;

public:
	// Construct an empty ordering collection.
	TemporalOrderings()
		:goal_achievers(0) {}

	// Destruct this ordering collection.
	virtual ~TemporalOrderings() {
		size_t n = distance.size();
		for (size_t i = 0; i < n; i++) {
			IntVector::unregister_use(distance[i]);
		}
		RCObject::destructive_deref(goal_achievers);
	}

	// Check if the first step could be ordered before the second step.
	virtual bool possibly_before(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const;

	// Check if the first step could be ordered after or at the same time as the second step.
	virtual bool possibly_not_before(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const;

	// Check if the first step could be ordered after the second step.
	virtual bool possibly_after(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const;

	// Check if the first step could be ordered before or at the same time as the second step.
	virtual bool possibly_not_after(size_t id1, StepTime t1,
		size_t id2, StepTime t2) const;

	// Check if the two steps are possibly concurrent.
	virtual bool possibly_concurrent(size_t id1, size_t id2, bool& ss, bool& se,
		bool& es, bool& ee) const;

	// Return the ordering collection with the given additions.
	const TemporalOrderings* refine(size_t step_id,
		float min_start, float min_end) const;

	// Return the ordering collection with the given additions.
	const TemporalOrderings* refine(float time, const Step& new_step) const;

	// Return the ordering collection with the given addition.
	virtual const TemporalOrderings* refine(const Ordering& new_ordering) const;

	// Return the ordering collection with the given additions.
	virtual const TemporalOrderings* refine(const Ordering& new_ordering,
		const Step& new_step,
		const PlanningGraph* pg,
		const Bindings* bindings) const;

	// Fill the given tables with distances for each step from the start step, and returns the greatest distance.
	virtual float schedule(map<size_t, float>& start_times,
		map<size_t, float>& end_times) const;

	// Return the makespan of this ordering collection.
	virtual float makespan(const map<pair<size_t,
		StepTime::StepPoint>, float>& min_times) const;
};