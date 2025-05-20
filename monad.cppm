export module monad;
import std;

template<typename Fn, typename ...Args>
concept valid_transformer = std::is_invocable_v<Fn, Args...> && !std::same_as<void, std::invoke_result_t<Fn, Args...>>;

template<typename Fn, typename ...Args>
concept valid_passthrough = std::is_invocable_v<Fn, Args...> && std::same_as<void, std::invoke_result_t<Fn, Args...>>;



export template<typename V>
class monad : public std::ranges::view_interface<monad<V>> {
	V view_;
	using Base = std::iter_value_t<std::ranges::iterator_t<V>>;

public:
	monad() = delete;

	template<std::ranges::range Range>
	explicit constexpr monad(Range&& r) noexcept : view_{ std::forward<Range>(r) } {}
	template<std::ranges::view View>
	explicit constexpr monad(View&& v) noexcept : view_{ std::forward<View>(v) } {}

	constexpr auto begin() { return std::ranges::begin(view_); }
	constexpr auto end() { return std::ranges::end(view_); }

	constexpr auto as_rvalue() {
		return ::monad(
			std::ranges::views::as_rvalue(std::move(view_))
		);
	}

	template<std::predicate<Base const&> Fn>
	constexpr auto filter(Fn&& fn) {
		return ::monad(
			std::ranges::views::filter(std::move(view_), std::forward<Fn>(fn))
		);
	}

	// Filter using a member function pointer of 'Base'
	template<typename TypeHack = std::conditional_t<std::is_class_v<Base>, Base, std::nullopt_t>>
		requires std::is_class_v<Base>
	constexpr auto filter(bool (TypeHack::* fn)() const) {
		return ::monad(
			std::ranges::views::filter(std::move(view_), fn)
		);
	}

	template<typename Fn>
		requires valid_transformer<Fn, Base const&>
	constexpr auto transform(Fn&& fn) {
		return ::monad(
			std::ranges::views::transform(std::move(view_), std::forward<Fn>(fn))
		);
	}

	template<typename Fn>
		requires valid_passthrough<Fn, Base const&>
	constexpr auto passthrough(Fn&& fn) {
		return ::monad(
			std::ranges::views::transform(std::move(view_),
				[fn = std::forward<Fn>(fn)](auto const& v) { fn(v); return v; }
			)
		);
	}

	constexpr auto take(std::ranges::range_difference_t<V> count) {
		return ::monad(
			std::ranges::views::take(std::move(view_), count)
		);
	}

	template<std::predicate<Base const&> Fn>
	constexpr auto take_while(Fn&& fn) {
		return ::monad(
			std::ranges::views::take_while(std::move(view_), std::forward<Fn>(fn))
		);
	}

	constexpr auto drop(std::ranges::range_difference_t<V> count) {
		return ::monad(
			std::ranges::views::drop(std::move(view_), count)
		);
	}

	template<std::predicate<Base const&> Fn>
	constexpr auto drop_while(Fn&& fn) {
		return ::monad(
			std::ranges::views::drop_while(std::move(view_), std::forward<Fn>(fn))
		);
	}

	constexpr auto join() requires std::ranges::range<Base> {
		return ::monad(
			// Use a 'views::join' to be able to do a double join
			std::ranges::views::join(std::move(view_))
		);
	}

	constexpr auto join_with(auto&& pattern) requires std::ranges::range<Base> {
		return ::monad(
			std::ranges::views::join_with(std::move(view_), std::move(pattern))
		);
	}

	constexpr auto lazy_split(auto&& pattern) {
		return ::monad(
			std::ranges::views::lazy_split(std::move(view_), std::move(pattern))
		);
	}

	constexpr auto split(auto&& pattern) {
		return ::monad(
			std::ranges::views::split(std::move(view_), std::move(pattern))
		);
	}

#ifdef __cpp_lib_ranges_concat
	constexpr auto concat(auto&& ...stuff) {
		return ::monad(
			std::ranges::views::concat(view_, stuff...)
		);
	}
#endif

	constexpr auto reverse() {
		return ::monad(
			std::ranges::views::reverse(std::move(view_))
		);
	}

	constexpr auto as_const() {
		return ::monad(
			std::ranges::views::as_const(std::move(view_))
		);
	}

	template<int I>
		requires (I >= 0 && I < std::tuple_size_v<Base>)
	constexpr auto elements() {
		return ::monad(
			std::ranges::views::elements<I>(std::move(view_))
		);
	}

	constexpr auto keys()
		requires (std::tuple_size_v<Base> > 0) {
		return ::monad(
			std::ranges::views::keys(std::move(view_))
		);
	}

	constexpr auto values()
		requires (std::tuple_size_v<Base> > 1) {
		return ::monad(
			std::ranges::views::values(std::move(view_))
		);
	}

	constexpr auto enumerate() {
		return ::monad(
			std::ranges::views::enumerate(std::move(view_))
		);
	}

	template<std::ranges::viewable_range... Rs >
	constexpr auto zip(Rs&& ...rs) {
		return ::monad(
			std::ranges::views::zip(std::move(view_), std::forward<Rs>(rs)...)
		);
	}

	template<typename F, std::ranges::viewable_range... Rs>
	constexpr auto zip_transform(F&& f, Rs&& ...rs) {
		return ::monad(
			std::ranges::views::zip_transform(std::forward<F>(f), std::move(view_), std::forward<Rs>(rs)...)
		);
	}

	template<int I>
		requires (I > 0)
	constexpr auto adjacent() {
		return ::monad(
			std::ranges::views::adjacent<I>(std::move(view_))
		);
	}

	template<int I, typename F>
		requires (I > 0)
	constexpr auto adjacent_transform(F&& f) {
		return ::monad(
			std::ranges::views::adjacent_transform<I>(std::move(view_), std::forward<F>(f))
		);
	}

	constexpr auto chunk(int n) {
		return ::monad(
			std::ranges::views::chunk(std::move(view_), n)
		);
	}

	constexpr auto slide(int n) {
		return ::monad(
			std::ranges::views::slide(std::move(view_), n)
		);
	}

	template<std::indirect_binary_predicate<std::ranges::iterator_t<V>, std::ranges::iterator_t<V>> Pred>
	constexpr auto chunk_by(Pred&& pred) {
		return ::monad(
			std::ranges::views::chunk_by(std::move(view_), std::forward<Pred>(pred))
		);
	}

	constexpr auto stride(int n) {
		return ::monad(
			std::ranges::views::stride(std::move(view_), n)
		);
	}

	template<std::ranges::viewable_range... Rs >
	constexpr auto cartesian_product(Rs&& ...rs) {
		return ::monad(
			std::ranges::views::cartesian_product(std::move(view_), std::forward<Rs>(rs)...)
		);
	}

#ifdef __cpp_lib_ranges_cache_latest
	constexpr auto cache_latest() {
		return ::monad(
			std::ranges::views::cache_latest(std::move(view_))
		);
	}
#endif

#ifdef __cpp_lib_ranges_to_input
	constexpr auto to_input() {
		return ::monad(
			std::ranges::views::to_input(std::move(view_))
		);
	}
#endif

	//
	// My own stuff
	// 

	// Pick out the elements of the tuple at the given indices
	template<int ...Is>
	constexpr auto select() {
		return ::monad(
			std::ranges::views::zip(std::ranges::views::elements<Is>(view_)...)
		);
	}

	// Apply projections to the incoming value and return them as a tuple
	template<typename ...Projs>
	constexpr auto as_tuple(Projs&& ...projs) {
		auto converter = [=](auto&& v) { return std::tuple{ std::invoke(projs, v)... }; };

		return ::monad(std::ranges::views::transform(view_, converter));
	}

	// Like transform, but for tuple-like values
	template<typename Fn>
	constexpr auto apply(Fn&& fn) {
		auto converter = [=](auto&& v) { return std::apply(fn, v); };
		return ::monad(std::ranges::views::transform(view_, converter));
	}

	template<typename Fn>
	constexpr auto pair_with(Fn&& fn) {
		return ::monad(
			std::ranges::views::zip(
				std::ranges::views::transform(view_, std::forward<Fn>(fn)),
				view_
			)
		);
	}

	// Convert to T
	template<typename T>
	constexpr auto as() {
		return ::monad(std::ranges::views::transform(view_, +[](Base&& v) { return T{ v }; }));
	}

	//
	// Wrap some ranges stuff 
	//

	constexpr auto distance() {
		return std::ranges::distance(view_);
	}

	template<typename T>
	constexpr bool equal(std::initializer_list<T> other) {
		return std::ranges::equal(view_, other);
	}

	constexpr bool equal(std::ranges::input_range auto&& other) {
		return std::ranges::equal(view_, std::forward<decltype(other)>(other));
	}

	template<template <typename...> typename To>
	constexpr auto to() {
		return std::ranges::to<To>(std::move(view_));
	}

	template<typename To>
	constexpr auto to() {
		return std::ranges::to<To>(std::move(view_));
	}
};


template<std::ranges::range R>
monad(R&&) -> monad<std::ranges::views::all_t<R>>;

template<std::ranges::view V>
monad(V&&) -> monad<V>;
