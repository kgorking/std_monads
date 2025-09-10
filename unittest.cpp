import monad;
import std;

using namespace std::literals;

int main() { return 0; }

auto is_odd = [](int i) { return i % 2 != 0; };
auto add_three = [](int v) { return v + 3; };
auto not_empty = [](std::string_view s) { return !s.empty(); };
auto size_over_4 = [](std::string_view s) { return s.size() > 4; };

static constexpr auto ints = std::array{ 1, 2, 3, 4, 5 };
static constexpr auto strings = std::array{ "This"sv, "is"sv, "a"sv, "test."sv };
static constexpr auto splittable = "This*is*a*test."sv;
static constexpr auto opt_ints = std::to_array<std::optional<int>>({1, 2, 3, std::nullopt, 5, 6, std::nullopt, 8});
static constexpr auto opts = std::to_array<std::optional<std::string_view>>({
	"1234",
	"15 foo",
	"bar",
	std::nullopt,
	"42",
	"5000000000",
	" 5",
	std::nullopt,
	"-43"
	});

// Test constructors
static_assert([] {
	auto m1 = as_monad(ints);
	auto m2 = as_monad(strings);
	auto m3 = as_monad(opts);
	auto m4 = as_monad(432);
	auto m5 = as_monad(std::optional{"test"sv});
	auto m6 = as_monad(std::optional<int>{});
	return true;
	}());

// Test filter
static_assert([] {
	return 3 == as_monad(ints).join().filter(is_odd).count() &&
		   7 == as_monad(opts).join().filter(not_empty).count();
	}());

// Test map
static_assert([] {
	return 30 == as_monad(ints).join().map(add_three).sum() &&
		    2 == as_monad(opts).join().map(size_over_4).sum<int>();
	}());

// Test take
static_assert([] {
	return 3 == as_monad(ints).join().take(3).count() &&
		   5 == as_monad(opts).join().take(5).count() &&
		   0 == as_monad(opts).join().take(-3).count();
	}());

// Test drop
static_assert([] {
	return 2 == as_monad(ints).join().drop(3).count() &&
		   2 == as_monad(opts).join().drop(5).count() &&
		   0 == as_monad(opts).join().drop(-3).count();
	}());

// Test concat
static_assert([] {
	return 
		30 == as_monad(ints).concat(ints).join().sum() &&
		40 == as_monad(ints).concat(opt_ints).join().sum();
	}());

// Test concat monad
static_assert([] {
	auto m1 = as_monad(ints).join();
	auto m2 = as_monad(opt_ints).join().link(m1);
	return (1+2+3+4+5) + (1+2+3+5+6+8) == m2.sum();
	}());

// Test join
static_assert([] {
	return
		"Thisisatest."sv == as_monad(strings).join().join().to<std::string>() &&
		"Thisisatest."sv == as_monad(std::optional{ strings }).join().join().to<std::string>() &&
		"123415 foobar425000000000 5-43"sv == as_monad(opts).join().join().to<std::string>();
	}());

// Test join with
static_assert([] {
	return
		"This - is - a - test."sv == as_monad(strings).join_with(" - ").join().to<std::string>() &&
		"This - is - a - test."sv == as_monad(std::optional{ strings }).join_with(" - ").join().to<std::string>() &&
		"This|is|a|test."sv == as_monad(strings).join_with("|"sv).join().to<std::string>() &&
		std::vector{ 1,0,2,0,3,0,4,0,5 } == as_monad(ints).join_with(0).to<std::vector>();
	}());

// Test split
static_assert([] {
	auto const parts = as_monad(splittable).join().split('*').to<std::vector>();
	auto const oparts = as_monad(std::optional{ splittable }).join().split('*').to<std::vector>();
	if (!std::ranges::equal(parts, strings))
		return false;
	if (!std::ranges::equal(oparts, strings))
		return false;

	auto const actual1 = as_monad(ints).join().split(0).to<std::vector>();
	auto const actual2 = as_monad(ints).join().split(3).to<std::vector>();
	auto const actual3 = as_monad(ints).join().split(5).to<std::vector>();

	return
		(actual1.size() == 1 && actual1[0].size() == 5) &&
		(actual2.size() == 2 && actual2[0].size() == 2 && actual2[1].size() == 2) &&
		(actual3.size() == 2 && actual3[0].size() == 4 && actual3[1].empty());
	}());

// Test split fast
static_assert([] {
	auto const  parts = as_monad(splittable)                 .join().split_fast<8>('*').to<std::vector<std::string>>();
	auto const oparts = as_monad(std::optional{ splittable }).join().split_fast<8>('*').to<std::vector<std::string>>();
	if (parts != oparts)
		return false;

	auto const actual1 = as_monad(ints).join().split_fast<8>(0).to<std::vector>();
	auto const actual2 = as_monad(ints).join().split_fast<8>(3).to<std::vector>();
	auto const actual3 = as_monad(ints).join().split_fast<8>(5).to<std::vector>();
	
	return
		(actual1.size() == 1 && actual1[0].size() == 5) &&
		(actual2.size() == 2 && actual2[0].size() == 2 && actual2[1].size() == 2) &&
		(actual3.size() == 2 && actual3[0].size() == 4 && actual3[1].empty());
	}());

// Test large object, copying it should not be allowed
struct LargeObject {
	char data[32];

	LargeObject() = default;
	LargeObject(LargeObject const&) = delete;
	LargeObject(LargeObject&&) = default;
	LargeObject& operator=(LargeObject const&) = delete;
};

static_assert([] {
	std::vector<LargeObject> large_objects;
	large_objects.emplace_back();
	large_objects.push_back({});

	as_monad(large_objects).join()
		.filter		([](LargeObject const&) { return true; })
		.map		([](LargeObject const&) { return LargeObject{}; })
		.and_then	([](LargeObject const&) { })
		.then		([](LargeObject const&) { return 42; });
	return true;
	}());

// Test terminal then
static_assert([] {
	int sum = 0;
	as_monad(ints).concat(opt_ints).join().then([&sum](int v) { sum += v; });
	return sum == 40;
	}());

// Test terminal sum
static_assert([] {
	return 40 == as_monad(ints).concat(opt_ints).join().sum();
	}());

// Test terminal count
static_assert([] {
	return 11 == as_monad(ints).concat(opt_ints).join().count();
	}());

// Test terminal dest
static_assert([] {
	std::vector<int> v;
	as_monad(ints).concat(opt_ints).join().dest(v);
	return v.size() == 11;
	}());