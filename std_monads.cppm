export module monads;
import std;


export
template<typename V>
struct monad {
	monad() = delete;
	template<typename T> monad(monad<T> const&) = delete;

	explicit monad(std::ranges::view auto const& view) noexcept : view_(view) {}
	template<std::ranges::range R> explicit monad(R const& cont) noexcept : view_(cont) {}
	template<std::ranges::range R> explicit monad(R     && cont) noexcept : view_(std::forward<R>(cont)) {}


	template<typename Fn>
	auto transform(Fn&& fn) {
		auto xform = std::ranges::transform_view(std::move(view_), std::forward<Fn>(fn));
		return monad<decltype(xform)>(std::move(xform));
	}

	template<template <typename...> typename To>
	auto to() const {
		return std::ranges::to<To>(view_);
	}

	template<typename To>
	auto to() const {
		return std::ranges::to<To>(view_);
	}

	// can not call view() on temporaries
	decltype(auto) view() && = delete;// ("can not call view() on temporaries");

	decltype(auto) view() & {
		return std::ranges::ref_view(view_);
	}

private:
	V view_;
};

template<std::ranges::range R> monad(R const& r) -> monad<std::ranges::ref_view<R>>;
template<std::ranges::range R> monad(R     && r) -> monad<std::ranges::owning_view<R>>;
