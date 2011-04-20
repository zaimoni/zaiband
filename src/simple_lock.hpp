// simple_lock.hpp

#ifndef ZAIMONI_STL_SIMPLE_LOCK_HPP
#define ZAIMONI_STL_SIMPLE_LOCK_HPP 1

namespace zaimoni {

template<class T>
class simple_lock
{
private:
	T& _lock_counter;

	// copy is not meaningful
	simple_lock(const simple_lock& src);
	void operator=(const simple_lock& src);
public:
	simple_lock(T& _lock_this) : _lock_counter(_lock_this) {++_lock_counter;};
	~simple_lock() {--_lock_counter;};
};

}

#endif
