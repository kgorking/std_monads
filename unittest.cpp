import monad;
import std;

constexpr auto is_even = [](int i) { return 0 == i % 2; };
constexpr auto lt_three = [](int i) { return i < 3; };
constexpr auto mul_two = [](int v) { return v * 2; };


// Test constructors
static_assert(
	[] -> bool {
		std::array v{ 1,2,3,4 };
		return monad(v)
			.equal(v);
	} (),
	"monad::monad"
);

//
// filter
static_assert(
	[] -> bool {
		return monad({ 1,2,3,4 })
			.filter(is_even)
			.equal({ 2,4 });
	} (),
	"monad::filter"
);

//
// transform
static_assert(
	[] -> bool {
		return monad({ 1,2,3,4 })
			.transform(is_even)
			.equal({ false, true, false, true });
	} (),
	"monad::transform"
);

//
// take
static_assert(
	[] -> bool {
		return monad({ 1,2,3,4 })
			.take(2)
			.equal({ 1,2 });
	} (),
	"monad::take"
);

//
// take_while
static_assert(
	[] -> bool {
		return monad({ 1,2,3,4 })
			.take_while(lt_three)
			.equal({ 1,2 });
	} (),
	"monad::take_while"
);

//
// drop
static_assert(
	[] -> bool {
		return monad({ 1,2,3,4 })
			.drop(2)
			.equal({ 3,4 });
	} (),
	"monad::drop"
);

//
// drop_while
static_assert(
	[] -> bool {
		return monad({ 1,2,3,4 })
			.drop_while(lt_three)
			.equal({ 3, 4 });
	} (),
	"monad::drop_while"
);

//
// join
static_assert(
	[] -> bool {
		std::array const v1{ 1,2     };
		std::array const v2{     3,4 };
		return monad({ v1, v2 })
			.join()
			.equal({ 1,2, 3,4 });
	} (),
	"monad::join"
);

//
// join_with
static_assert(
	[] -> bool {
		std::array const v1{ 1,2     };
		std::array const v2{     3,4 };
		return monad({ v1, v2 })
			.join_with(9)
			.equal({ 1,2, 9, 3,4 });
	} (),
	"monad::join_with"
);

//
// lazy_split
static_assert(
	[] -> bool {
		return monad({ 0,  1,  0,  2,3,  0,  4,5,6 })
			.lazy_split(0)
			.join()
			.equal({1, 2, 3, 4, 5, 6});
	} (),
	"monad::lazy_split"
);

#ifdef __cpp_lib_ranges_concat
//
// concat
static_assert(
	[] -> bool {
		return monad({ 1,2 })
			.concat({ 3,4 })
			.equal({1, 2, 3, 4});
	} (),
	"monad::concat"
);
#endif

//
// reverse
static_assert(
	[] -> bool {
		return monad({ 1,2,3 })
			.reverse()
			.equal({ 3,2,1 });
	} (),
	"monad::reverse"
);

//
// as_const
static_assert(
	[] -> bool {
		auto m = monad({ 1,2,3 }).as_const();
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
			m.elements<0>().equal({ 1, 2, 3, 4, 5 }) &&
			m.elements<1>().equal({ 'A', 'B', 'C', 'D', 'E' }) &&
			m.elements<2>().equal({ "a", "b", "c", "d", "e" });
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
			m.keys().equal({ 1, 2, 3, 4, 5 }) &&
			m.values().equal({ 'A', 'B', 'C', 'D', 'E' });
	} (),
	"monad::keys/values"
);

//
// enmumerate
static_assert(
	[] -> bool {
		return monad({ 'A', 'B', 'C', 'D', 'E' })
			.enumerate()
			.equal({
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
		return monad({ 0, 1, 2, 3 })
			.zip(std::array{ 'A', 'B', 'C', 'D', 'E' })
			.equal({
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

		return monad({ 0, 1, 2, 3 })
			.zip_transform(fn, y)
			.equal({'A', 'C', 'E', 'G'});
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
			.equal(std::array{ std::array{0, 1, 2}, std::array{1, 2, 3} });
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
			.equal({1, 3, 5});
	} (),
	"monad::adjacent_transform"
);

//
// chunk
static_assert(
	[] -> bool {
		auto v = monad({ 0, 1, 2, 3 })
			.chunk(2);
		auto it = v.begin();
		auto it2 = std::next(it);
		return
			std::ranges::equal(*it, std::array{0, 1}) &&
			std::ranges::equal(*it2, std::array{ 2, 3 });
	} (),
	"monad::chunk"
);

//
// slide
static_assert(
	[] -> bool {
		auto v = monad({ 0, 1, 2, 3 })
			.slide(3);
		auto it = v.begin();
		auto it2 = std::next(it);
		return
			std::ranges::equal(*it, std::array{0, 1, 2}) &&
			std::ranges::equal(*it2, std::array{ 1, 2, 3 });
	} (),
	"monad::slide"
);

//
// chunk_by
static_assert(
	[] -> bool {
		auto v = monad({ 0, 1, 0, 3 })
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
		return monad({ 0, 1, 2, 3 })
			.stride(2)
			.equal({ 0, 2 });
	} (),
	"monad::stride"
);

//
// cartesian_product
static_assert(
	[] -> bool {
		return 6 == monad({ 0, 1 })
			.cartesian_product(std::array{ 'A', 'B', 'C' })
			.distance();
	} (),
	"monad::cartesian_product"
);
