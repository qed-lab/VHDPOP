#pragma once
class RCObject
{
	mutable unsigned long ref_count;	// Reference counter.

protected:
	// Construct an object with a reference counter.
	RCObject() :ref_count(0) {}

	// Copy constructor.
	RCObject(const RCObject& o) :ref_count(0) {}

public:
	// Destructor.
	virtual ~RCObject() {};

	// Increase the reference count for the given object.
	static void ref(const RCObject* o) {
		if (o != 0) {
			++ o->ref_count;
		}
	}

	// Decrease the reference count for the given object
	static void deref(const RCObject* o) {
		if (o != 0) {
			-- o->ref_count;
		}
	}

	// Decrease the reference count for the given object and delete it if the reference count becomes 0.
	static void destructive_deref(const RCObject* o) {
		if (o != 0) {
			-- o->ref_count;
			if (o->ref_count == 0) {
				delete o;
			}
		}
	}

};

