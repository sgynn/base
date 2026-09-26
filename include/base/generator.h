#pragma once

// c++20 generator co-routine 

#if __cplusplus < 202002L
#error "Coroutines require c++20
#else

#include <coroutine>

template<typename T>
struct Generator {
	// This type and all its members must be named thus
	struct promise_type {
		T value; // cached next item
		Generator get_return_object() { return Generator(*this); }
		std::suspend_never initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
		template<std::convertible_to<T> From>
		std::suspend_always yield_value(From&& val) { value = std::move(val); return {}; }
		void unhandled_exception() {}
		void return_void() {}
	};

	using Handle = std::coroutine_handle<promise_type>;
	Handle handle;

	struct Sentinel {};
	struct Iterator {
		Generator& generator;
		bool operator==(Sentinel) const { return !generator.handle || generator.handle.done(); }
		Iterator& operator++() { generator.handle(); return *this; }
		T& operator*() const { return generator.handle.promise().value; }
		T& operator->() const { return **this; }
	};

	Generator() = default;
	Generator(promise_type& h) : handle(Handle::from_promise(h)) {}
	Generator(Generator&& move) : handle(std::move(move.handle)) { move.handle = nullptr; }
	~Generator() { if(handle) handle.destroy(); }
	bool empty() { return !handle || handle.done(); }
	Iterator begin() { return Iterator{*this}; }
	Sentinel end() { return Sentinel(); }
};
#endif




