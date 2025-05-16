export module monads;
import std;

export
template<typename V>
struct monad : std::ranges::view_interface<monad<V>> {
	monad() = delete;

	monad(monad const& m) : view_(m.view_) {}
	monad(monad     && m) : view_(std::forward<V>(m.view_)) {}

	//explicit monad(std::ranges::view auto const& view) noexcept : view_(view) {}
	template<std::ranges::range R> requires !requires(R r) { r.view_; }
	explicit monad(R const& cont) noexcept : view_(cont) {}
	
	template<std::ranges::range R> requires !requires(R r) { r.view_; }
	explicit monad(R     && cont) noexcept : view_(std::forward<R>(cont)) {}


	template<typename Fn>
	auto transform(Fn&& fn) {
		auto xform = std::ranges::transform_view(std::move(view_), std::forward<Fn>(fn));
		return monad<decltype(xform)>(std::move(xform));
	}

	template<template <typename...> typename To>
	auto to() const {
		return std::ranges::to<To>(view_);
	}

	//template<typename To>
	//auto to() const {
	//	return std::ranges::to<To>(view_);
	//}

	auto begin() const { return view_.begin(); }
	auto end() const { return view_.end(); }

	// can not call view() on temporaries
	//decltype(auto) view() && = delete;// ("can not call view() on temporaries");
	//
	//decltype(auto) view() & {
	//	return std::ranges::ref_view(view_);
	//}

private:
	V view_;
};

template<std::ranges::range R> monad(R const&) -> monad<std::ranges::ref_view<R>>;
template<std::ranges::range R> monad(R     &&) -> monad<std::ranges::owning_view<R>>;
