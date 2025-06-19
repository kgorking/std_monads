import monad2;
import std;

using namespace std::literals;

int main() { return 0; }

auto is_odd = [](int i) { return i % 2 != 0; };
auto add_three = [](int v) { return v + 3; };
auto not_empty = [](std::string_view s) { return !s.empty(); };
auto size_over_4 = [](std::string_view s) { return s.size() > 4; };

static constexpr auto ints = std::array{ 1, 2, 3, 4, 5 };
static constexpr auto strings = std::array{ "This"sv, "is"sv, "a"sv, "test."sv };
static constexpr auto opt_ints = std::to_array<std::optional<int>>({
	1, 2, 3, std::nullopt, 5, 6, std::nullopt, 8
	});
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
	auto m1 = monad2(ints);
	auto m2 = monad2(strings);
	auto m3 = monad2(opts);
	auto m4 = monad2(432);
	auto m5 = monad2(std::optional{"test"sv});
	auto m6 = monad2(std::optional<int>{});
	return true;
	}());

// Test filter
static_assert([] {
	return 3 == monad2(ints).filter(is_odd).count() &&
		   7 == monad2(opts).filter(not_empty).count();
	}());

// Test map
static_assert([] {
	return 30 == monad2(ints).map(add_three).sum() &&
		    2 == monad2(opts).map(size_over_4).sum();
	}());

// Test take
static_assert([] {
	return 3 == monad2(ints).take(3).count() &&
		   5 == monad2(opts).take(5).count();
	}());

// Test drop
static_assert([] {
	return 2 == monad2(ints).drop(3).count() &&
		   2 == monad2(opts).drop(5).count();
	}());

// Test concat
static_assert([] {
	return 30 == monad2(ints).concat(ints).sum() &&
		   40 == monad2(ints).concat(opt_ints).sum();
	}());