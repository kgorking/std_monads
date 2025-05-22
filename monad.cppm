﻿export module monad;
import std;

template<typename Fn, typename ...Args>
concept valid_transformer = std::is_invocable_v<Fn, Args...> && !std::same_as<void, std::invoke_result_t<Fn, Args...>>;

template<typename Fn, typename ...Args>
concept valid_passthrough = std::is_invocable_v<Fn, Args...> && std::same_as<void, std::invoke_result_t<Fn, Args...>>;


// TODO intercept concept failures and replace them with 'static_assert's


export template<typename V>
class monad : public std::ranges::view_interface<monad<V>> {
	V view_;
	using Base = std::iter_value_t<std::ranges::iterator_t<V>>;

public:
	monad() = delete;

	template<std::ranges::range Range> explicit constexpr monad(Range&& r) noexcept : view_{ std::forward<Range>(r) } {}
	template<std::ranges::view View>   explicit constexpr monad(View&& v) noexcept : view_{ std::forward<View>(v) } {}

	constexpr auto begin() { return std::ranges::begin(view_); }
	constexpr auto end() { return std::ranges::end(view_); }

	constexpr auto as_rvalue() {
		return ::monad(
			std::ranges::views::as_rvalue(std::move(view_))
		);
	}

	template<std::predicate<Base const&> Fn>
	constexpr auto filter(Fn&& fn) {
		return ::monad(std::ranges::views::filter(std::move(view_), std::forward<Fn>(fn)));
	}

	template<std::predicate<Base const&> Fn>
	constexpr auto filter_not(Fn&& fn) {
		return ::monad(std::ranges::views::filter(std::move(view_), std::not_fn(std::forward<Fn>(fn))));
	}

	// Filter using a member function pointer of 'Base'
	template<typename TypeHack = std::conditional_t<std::is_class_v<Base>, Base, std::nullopt_t>>
		requires std::is_class_v<Base>
	constexpr auto filter(bool (TypeHack::* fn)() const) {
		return ::monad(
			std::ranges::views::filter(std::move(view_), fn)
		);
	}

	template<typename TypeHack = std::conditional_t<std::is_class_v<Base>, Base, std::nullopt_t>>
		requires std::is_class_v<Base>
	constexpr auto filter_not(bool (TypeHack::* fn)() const) {
		return ::monad(std::ranges::views::filter(std::move(view_), std::not_fn(fn)));
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
		requires requires(Base const& b) { std::get<0>(b); } // 'Base' must be a tuple-like type
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
		return ::monad(std::ranges::views::transform(view_, +[](Base&& v) { return T{ std::forward<Base>(v) }; }));
	}

	//
	// Non-modifying sequence operations 
	//
	template<class Proj = std::identity,
		std::indirect_unary_predicate<
		std::projected<std::ranges::iterator_t<V>, Proj>> Pred >
	constexpr bool any_of(Pred pred, Proj proj = {}) {
		return std::ranges::any_of(view_, std::move(pred), std::move(proj));
	}

	template<class Proj = std::identity,
		std::indirect_unary_predicate<
		std::projected<std::ranges::iterator_t<V>, Proj>> Pred >
	constexpr bool all_of(Pred pred, Proj proj = {}) {
		return std::ranges::all_of(view_, std::move(pred), std::move(proj));
	}

	template<class Proj = std::identity,
		std::indirect_unary_predicate<
		std::projected<std::ranges::iterator_t<V>, Proj>> Pred >
	constexpr bool none_of(Pred pred, Proj proj = {}) {
		return std::ranges::none_of(view_, std::move(pred), std::move(proj));
	}

	template< class T, class Proj = std::identity >
		requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<V>, Proj>, const T*>
	constexpr auto count(const T& value, Proj proj = {}) -> std::ranges::range_difference_t<V> {
		return std::ranges::count(std::move(view_), value, std::move(proj));
	}

	template<class Proj = std::identity, std::indirect_unary_predicate<std::projected<std::ranges::iterator_t<V>, Proj>> Pred >
	constexpr auto count_if(Pred pred, Proj proj = {}) -> std::ranges::range_difference_t<V> {
		return std::ranges::count_if(std::move(view_), std::move(pred), std::move(proj));
	}

	// mismatch ?

	template<typename T>
	constexpr bool equal_to(std::initializer_list<T> other) {
		return std::ranges::equal(std::move(view_), other);
	}

	template<std::ranges::input_range R, class Pred = std::ranges::equal_to>
		requires std::indirectly_comparable<std::ranges::iterator_t<V>, std::ranges::iterator_t<R>, Pred>
	constexpr bool compare(R&& r, Pred pred = {}) {
		return std::ranges::equal(std::move(view_), std::forward<R>(r), std::move(pred));
	}

	template<std::ranges::input_range R>
	constexpr bool equal_to(R&& r) {
		return std::ranges::equal(std::move(view_), std::forward<R>(r), std::ranges::equal_to{});
	}

	template<std::ranges::input_range R>
	constexpr bool not_equal_to(R&& r) {
		return !equal_to(std::forward<R>(r));
	}

	template<std::ranges::input_range R>
	constexpr bool greater(R&& r) {
		return std::ranges::equal(std::move(view_), std::forward<R>(r), std::ranges::greater{});
	}

	template<std::ranges::input_range R>
	constexpr bool greater_equal(R&& r) {
		return std::ranges::equal(std::move(view_), std::forward<R>(r), std::ranges::greater_equal{});
	}

	template<std::ranges::input_range R>
	constexpr bool less(R&& r) {
		return std::ranges::equal(std::move(view_), std::forward<R>(r), std::ranges::less{});
	}

	template<std::ranges::input_range R>
	constexpr bool less_equal(R&& r) {
		return std::ranges::equal(std::move(view_), std::forward<R>(r), std::ranges::less_equal{});
	}

	template<std::ranges::input_range R, class Proj = std::identity,
		std::indirect_strict_weak_order<
			std::ranges::iterator_t<V>,
			std::projected<std::ranges::iterator_t<R>, Proj>> Comp = std::ranges::less >
	constexpr bool lexicographical_compare(R&& r, Comp comp = {}, Proj proj = {}) {
		return std::ranges::lexicographical_compare(std::move(view_), std::forward<R>(r), std::move(comp), std::identity{}, std::move(proj));
	}

	template<class T, class Proj = std::identity>
		requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<V>, Proj>, const T*>
	constexpr bool contains(const T& value, Proj proj = {}) {
		return std::ranges::contains(std::move(view_), value, std::move(proj));
	}

	template<std::ranges::forward_range R, class Pred = std::ranges::equal_to, class Proj = std::identity>
		requires std::indirectly_comparable<std::ranges::iterator_t<V>, std::ranges::iterator_t<R>, Pred, std::identity, Proj>
	constexpr bool contains_subrange(R&& r, Pred pred = {}, Proj proj = {}) {
		return std::ranges::contains_subrange(std::move(view_), std::forward<R>(r), std::move(pred), std::identity{}, std::move(proj));
	}

	template<std::ranges::input_range R, class Pred = std::ranges::equal_to, class Proj = std::identity>
		requires std::indirectly_comparable<std::ranges::iterator_t<V>, std::ranges::iterator_t<R>, Pred, std::identity, Proj>
	constexpr bool starts_with(R&& r, Pred pred = {}, Proj proj = {}) {
		return std::ranges::starts_with(std::move(view_), std::forward<R>(r), std::move(pred), std::identity{}, std::move(proj));
	}

	template<std::ranges::input_range R, class Pred = std::ranges::equal_to, class Proj = std::identity>
		requires (std::ranges::forward_range<V> || std::ranges::sized_range<V>) && (std::ranges::forward_range<R> || std::ranges::sized_range<R>) &&
			std::indirectly_comparable<std::ranges::iterator_t<V>, std::ranges::iterator_t<R>, Pred, std::identity, Proj>
	constexpr bool ends_with(R&& r, Pred pred = {}, Proj proj = {}) {
		return std::ranges::ends_with(std::move(view_), std::forward<R>(r), std::move(pred), std::identity{}, std::move(proj));
	}

	//
	// Modifying sequence operations 
	//

	//
	// Partitioning operations 
	//

	//
	// Sorting operations 
	//

	//
	// Binary search operations (on sorted ranges) 
	//

	//
	// Set operations (on sorted ranges)
	//

	//
	// Heap operations
	//

	//
	// Minimum/maximum operations
	//

	//
	// Fold operations
	//

	//
	// ??? uninitialized memory algorithms
	//

	constexpr auto sort() requires std::sortable<V> {
		std::ranges::sort(view_);
		return ::monad(std::move(view_));
	}

	constexpr auto sort_copy() {
		auto v = to<std::vector>();
		std::ranges::sort(v);
		return ::monad(std::move(v));
	}

	constexpr auto distance() {
		return std::ranges::distance(view_);
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


constexpr auto is_even = [](int i) { return 0 == i % 2; };
constexpr auto lt_three = [](int i) { return i < 3; };
constexpr auto mul_two = [](int v) { return v * 2; };


// Test constructors
static_assert(
	[] -> bool {
		std::array v{ 1,2,3,4 };
		return monad(v)
			.equal_to(v);
	} (),
		"monad::monad"
		);

//
// filter
static_assert(
	[] -> bool {
		auto m_even = monad(std::array{ 1,2,3,4 }).filter(is_even);
		auto m_odd = monad(std::array{ 1,2,3,4 }).filter_not(is_even);
		return m_even.equal_to({ 2, 4 }) && m_odd.equal_to({ 1, 3 });
	} (),
		"monad::filter"
		);
static_assert(
	[] -> bool {
		struct test {
			int i;
			constexpr bool is_even() const { return 0 == i % 2; }
			constexpr operator int() const { return i; }
		};
		auto m_even = monad(std::array<test,4>{ 1,2,3,4 }).filter(&test::is_even);
		auto m_odd =  monad(std::array<test,4>{ 1,2,3,4 }).filter_not(&test::is_even);
		return m_even.equal_to({ 2, 4 }) && m_odd.equal_to({ 1, 3 });
	} (),
		"monad::filter member function"
		);

//
// transform
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4 })
			.transform(is_even)
			.equal_to({ false, true, false, true });
	} (),
		"monad::transform"
		);

//
// take
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4 })
			.take(2)
			.equal_to({ 1,2 });
	} (),
		"monad::take"
		);

//
// take_while
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4 })
			.take_while(lt_three)
			.equal_to({ 1,2 });
	} (),
		"monad::take_while"
		);

//
// drop
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4 })
			.drop(2)
			.equal_to({ 3,4 });
	} (),
		"monad::drop"
		);

//
// drop_while
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4 })
			.drop_while(lt_three)
			.equal_to({ 3, 4 });
	} (),
		"monad::drop_while"
		);

//
// join
static_assert(
	[] -> bool {
		std::array const v1{ 1,2 };
		std::array const v2{ 3,4 };
		return monad(std::array{ v1, v2 })
			.join()
			.equal_to({ 1,2, 3,4 });
	} (),
		"monad::join"
		);

//
// join_with
static_assert(
	[] -> bool {
		std::array const v1{ 1,2 };
		std::array const v2{ 3,4 };
		return monad(std::array{ v1, v2 })
			.join_with(9)
			.equal_to({ 1,2, 9, 3,4 });
	} (),
		"monad::join_with"
		);

//
// lazy_split
static_assert(
	[] -> bool {
		return monad(std::array{ 0,  1,  0,  2,3,  0,  4,5,6 })
			.lazy_split(0)
			.join()
			.equal_to({ 1, 2, 3, 4, 5, 6 });
	} (),
		"monad::lazy_split"
		);

#ifdef __cpp_lib_ranges_concat
//
// concat
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2 })
			.concat({ 3,4 })
			.equal_to({ 1, 2, 3, 4 });
	} (),
		"monad::concat"
		);
#endif

//
// reverse
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3 })
			.reverse()
			.equal_to({ 3,2,1 });
	} (),
		"monad::reverse"
		);

//
// as_const
static_assert(
	[] -> bool {
		auto m = monad(std::array{ 1,2,3 }).as_const();
		if constexpr (!requires { m.front()++; })
			return true;
		else
			return false;
	} (),
		"monad::as_const"
		);

//
// elements
static_assert(
	[] -> bool {
		const std::vector<std::tuple<int, char, std::string_view>> vt
		{
			{1, 'A', "a"},
			{2, 'B', "b"},
			{3, 'C', "c"},
			{4, 'D', "d"},
			{5, 'E', "e"},
		};
		auto m = monad(vt);

		return
			m.elements<0>().equal_to({ 1, 2, 3, 4, 5 }) &&
			m.elements<1>().equal_to({ 'A', 'B', 'C', 'D', 'E' }) &&
			m.elements<2>().equal_to({ "a", "b", "c", "d", "e" });
	} (),
		"monad::elements"
		);

//
// keys/values
static_assert(
	[] -> bool {
		const std::vector<std::pair<int, char>> vt
		{
			{1, 'A'},
			{2, 'B'},
			{3, 'C'},
			{4, 'D'},
			{5, 'E'},
		};
		auto m = monad(vt);

		return
			m.keys().equal_to({ 1, 2, 3, 4, 5 }) &&
			m.values().equal_to({ 'A', 'B', 'C', 'D', 'E' });
	} (),
		"monad::keys/values"
		);

//
// enmumerate
static_assert(
	[] -> bool {
		return monad(std::array{ 'A', 'B', 'C', 'D', 'E' })
			.enumerate()
			.equal_to({
				std::make_pair(0, 'A'),
				std::make_pair(1, 'B'),
				std::make_pair(2, 'C'),
				std::make_pair(3, 'D'),
				std::make_pair(4, 'E')
				});
	} (),
		"monad::enmumerate"
		);

//
// zip
static_assert(
	[] -> bool {
		return monad(std::array{ 0, 1, 2, 3 })
			.zip(std::array{ 'A', 'B', 'C', 'D', 'E' })
			.equal_to({
				std::make_pair(0, 'A'),
				std::make_pair(1, 'B'),
				std::make_pair(2, 'C'),
				std::make_pair(3, 'D')
				});
	} (),
		"monad::zip"
		);

//
// zip_transform
static_assert(
	[] -> bool {
		auto fn = [](int i, char c) { return char(c + i); };
		auto y = { 'A', 'B', 'C', 'D', 'E' };

		return monad(std::array{ 0, 1, 2, 3 })
			.zip_transform(fn, y)
			.equal_to({ 'A', 'C', 'E', 'G' });
	} (),
		"monad::zip_transform"
		);

//
// adjacent (compiler bug)
static_assert(
	[] -> bool {
		constexpr auto v = std::array{ 0, 1, 2, 3 };
		return monad(v)
			.adjacent<3>()
			.equal_to(std::array{ std::array{0, 1, 2}, std::array{1, 2, 3} });
	} (),
		"monad::adjacent"
		);

//
// adjacent_transform (compiler bug)
static_assert(
	[] -> bool {
		constexpr auto v = std::array{ 0, 1, 2, 3 };
		return monad(v)
			.adjacent_transform<2>(std::plus{})
			.equal_to({ 1, 3, 5 });
	} (),
		"monad::adjacent_transform"
		);

//
// chunk
static_assert(
	[] -> bool {
		auto v = monad(std::array{ 0, 1, 2, 3 })
			.chunk(2);
		auto it = v.begin();
		auto it2 = std::next(it);
		return
			std::ranges::equal(*it, std::array{ 0, 1 }) &&
			std::ranges::equal(*it2, std::array{ 2, 3 });
	} (),
		"monad::chunk"
		);

//
// slide
static_assert(
	[] -> bool {
		auto v = monad(std::array{ 0, 1, 2, 3 })
			.slide(3);
		auto it = v.begin();
		auto it2 = std::next(it);
		return
			std::ranges::equal(*it, std::array{ 0, 1, 2 }) &&
			std::ranges::equal(*it2, std::array{ 1, 2, 3 });
	} (),
		"monad::slide"
		);

//
// chunk_by
static_assert(
	[] -> bool {
		auto v = monad(std::array{ 0, 1, 0, 3 })
			.chunk_by(std::ranges::less{});
		auto it = v.begin();
		auto it2 = std::next(it);
		return
			std::ranges::equal(*it, std::array{ 0, 1 }) &&
			std::ranges::equal(*it2, std::array{ 0, 3 });
	} (),
		"monad::chunk_by"
		);

//
// stride
static_assert(
	[] -> bool {
		return monad(std::array{ 0, 1, 2, 3 })
			.stride(2)
			.equal_to({ 0, 2 });
	} (),
		"monad::stride"
		);

//
// cartesian_product
static_assert(
	[] -> bool {
		return 6 == monad(std::array{ 0, 1 })
			.cartesian_product(std::array{ 'A', 'B', 'C' })
			.distance();
	} (),
		"monad::cartesian_product"
		);

//
// any_of
static_assert(
	[] -> bool {
		return monad(std::array{ 2,4,5,6,8 }).any_of(is_even);
	} (),
		"monad::any_of"
		);

//
// all_of
static_assert(
	[] -> bool {
		return !monad(std::array{ 2,4,5,6,8 }).all_of(is_even);
	} (),
		"monad::all_of"
		);

//
// none_of
static_assert(
	[] -> bool {
		return monad(std::array{ 1,3,5,7 }).none_of(is_even);
	} (),
		"monad::none_of"
		);

//
// count
static_assert(
	[] -> bool {
		return 1 == monad(std::array{ 1,3,5,7 }).count(5);
	} (),
		"monad::count"
		);

//
// count_if
static_assert(
	[] -> bool {
		return 3 == monad(std::array{ 1,2,3,4,5,6,7 }).count_if(is_even);
	} (),
		"monad::count_if"
		);

//
// equal_to
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4,5,6,7 }).filter(is_even).equal_to({2,4,6});
	} (),
		"monad::equal_to"
		);

//
// not_equal_to
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4,5,6,7 })
			.filter(is_even)
			.not_equal_to(std::array{ 1,3,5,7 });
	} (),
		"monad::not_equal_to"
		);

//
// greater
static_assert(
	[] -> bool {
		return monad(std::array{ 2,3,4 }).greater(std::array{ 1,2,3 });
	} (),
		"monad::greater"
		);

//
// greater_equal
static_assert(
	[] -> bool {
		return monad(std::array{ 2,3,4 }).greater_equal(std::array{ 1,3,4 });
	} (),
		"monad::greater_equal"
		);

//
// less
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3 }).less(std::array{ 2,3,4 });
	} (),
		"monad::less"
		);

//
// less_equal
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3 }).less_equal(std::array{ 2,2,3 });
	} (),
		"monad::less_equal"
		);

//
// lexicographical_compare
static_assert(
	[] -> bool {
		return monad(std::string_view{ "abcd" }).lexicographical_compare(std::string_view{ "bacd" });
	} (),
		"monad::lexicographical_compare"
		);

//
// contains
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4,5 }).contains(4);
	} (),
		"monad::contains"
		);

//
// contains_subrange
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4,5 }).contains_subrange(std::array{ 2,3 });
	} (),
		"monad::contains_subrange"
		);

//
// starts_with
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4,5 }).starts_with(std::array{ 1,2,3 });
	} (),
		"monad::starts_with"
		);

//
// ends_with
static_assert(
	[] -> bool {
		return monad(std::array{ 1,2,3,4,5 }).ends_with(std::array{ 3,4,5 });
	} (),
		"monad::ends_with"
		);
