#pragma once

#include "refcount.h"

// Template chain class.
template<class T>
class Chain :public RCObject {
public:
	// The head (data) of the chain.
	T head;
	// The tail (pointer to the rest) of the chain.
	const Chain<T>* tail;

	// Construct a chain with the given head and tail.
	Chain<T>(const T& head, const Chain<T>* tail)
	:head(head), tail(tail){
		ref(tail);
	}

	// Destruct the chain.
	Chain<T>() {
		destructive_deref(tail);
	}

	// Check if the chain contains the given element.
	bool contains(const T& h) const {
		for (const Chain<T>* ci = this; ci != 0; ci = ci->tail) {
			if (h == ci->head) {
				return true;
			}
		}
		return false;
	}

	// Return a chain (pointer) with the first occurance of the given element removed.
	const Chain<T>* remove(const T& h) const {
		if (h == head) {
			return tail;
		}
		else if (tail != 0) {
			Chain<T>* prev = new Chain<T>(head, 0);
			const Chain<T>* top = prev;
			for (const Chain<T>* ci = tail; ci != 0; ci = ci->tail) {
				if (h == ci->head) {
					prev->tail = ci->tail;
					ref(ci->tail);
					break;
				}
				else {
					Chain<T>* tmp = new Chain<T>(ci->head, 0);
					prev->tail = tmp;
					ref(tmp);
					prev = tmp;
				}
			}
			return top;
		}
		else {
			return this;
		}
	}

	// Return the size of this chain.
	int size() const {
		int result = 0;
		for (const Chain<T>* ci = this; ci != 0; ci = ci->tail) {
			++result;
		}
		return result;
	}

	const Chain<T>* get_tail() const {
		return tail;
	}

	T get_head() const {
		return head;
	}
};