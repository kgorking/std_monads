export module monads;
import std;

export template<typename V>
class monad : std::ranges::view_interface<monad<V>> {
	V view_;
	using Base = decltype(view_.front());

public:
	monad() = delete;

	monad(monad const& m) : view_(m.view_) {}
	monad(monad     && m) : view_(std::forward<V>(m.view_)) {}

	template<typename T>
	monad(std::initializer_list<T> const& il) : view_(std::vector<T>(il.begin(), il.end())) {}

	//explicit monad(std::ranges::view auto const& view) noexcept : view_(view) {}
	template<std::ranges::range R> requires !requires(R r) { r.view_; }
	explicit monad(R const& cont) noexcept : view_(cont) {}
	
	template<std::ranges::range R> requires !requires(R r) { r.view_; }
	explicit monad(R     && cont) noexcept : view_(std::forward<R>(cont)) {}

	auto as_rvalue() {
		return ::monad(
			std::ranges::as_rvalue_view(std::move(view_))
		);
	}

	template<typename Fn>
	auto filter(Fn&& fn) {
		return ::monad(
			std::ranges::filter_view(std::move(view_), std::forward<Fn>(fn))
		);
	}

	template<typename Fn>
	auto transform(Fn&& fn) {
		return ::monad(
			std::ranges::transform_view(std::move(view_), std::forward<Fn>(fn))
		);
	}

	auto take(std::integral auto i) {
		return ::monad(
			std::ranges::take_view(std::move(view_), i)
		);
	}

	template<typename Fn>
	auto take_while(Fn&& fn) {
		return ::monad(
			std::ranges::take_while_view(std::move(view_), std::forward<Fn>(fn))
		);
	}

	auto drop(std::integral auto i) {
		return ::monad(
			std::ranges::drop_view(std::move(view_), i)
		);
	}

	template<typename Fn>
	auto drop_while(Fn&& fn) {
		return ::monad(
			std::ranges::drop_while_view(std::move(view_), std::forward<Fn>(fn))
		);
	}

	auto join() {
		static_assert(std::ranges::input_range<Base>, "Input to 'join' must be a view of ranges");
		return ::monad(
			std::ranges::join_view(std::move(view_))
		);
	}

	auto join_with(auto&& pattern) {
		return ::monad(
			std::ranges::join_with_view(std::move(view_), std::move(pattern))
		);
	}

	template<template <typename...> typename To>
	auto to() const {
		return std::ranges::to<To>(view_);
	}

	template<typename To>
	auto to() const {
		return std::ranges::to<To>(view_);
	}

	auto begin() { return view_.begin(); }
	auto end() { return view_.end(); }
};

template<typename T> monad(std::initializer_list<T> const&) -> monad<std::ranges::owning_view<std::vector<T>>>;
template<std::ranges::range R> monad(R const&) -> monad<std::ranges::ref_view<R>>;
template<std::ranges::range R> monad(R     &&) -> monad<std::ranges::owning_view<R>>;
