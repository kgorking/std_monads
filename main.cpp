//import monad;
import monad2;
import std;

using namespace std::literals;

auto is_odd = [](int i) { return i % 2 != 0; };
auto add_three = [](int v) { return v + 3; };
auto mul_two = [](int v) { return v * 2; };
auto putval = [](auto val) { std::print("{} ", val); };
auto puterr = [](std::errc val) { std::print("*{}* ", std::make_error_condition(val).message()); };

std::expected<int, std::errc> to_int(std::string_view sv) {
	int r{};
	auto const [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), r);
	if (ec == std::errc())
		return r;
	else
		return std::unexpected(ec);
}

static constexpr std::array const ints = { 1, 2, 3, 4, 5 };
static constexpr std::array strings{ "This"sv, "is"sv, "a"sv, "test."sv };
static const     std::vector<std::optional<std::string_view>> opts{ "1234", "15 foo", "bar", std::nullopt, "42", "5000000000", " 5", std::nullopt, "-43" };


void test_filter() {
	std::println("\n== Filter ==");

	std::println("  ints.filter(is_odd): {}",
		monad2(ints).filter(is_odd).to<std::vector>());

	std::println("  strings.filter(size > 2): {}",
		monad2(strings).filter([](std::string_view s) { return s.size() > 2; }).to<std::vector>());
}

void test_count() {
	std::println("\n== Count ==");

	std::println("  strings.filter(size > 2).count(): {}", monad2(strings)
		.map(&std::string_view::size)
		.filter(std::bind_back(std::greater{}, 2))
		.count());
}

void test_split() {
	std::println("\n== Split ==");

	std::println("  ints.split(1): {}", monad2(ints).split(1).to<std::vector>());
	std::println("  ints.split(3): {}", monad2(ints).split(3).to<std::vector>());
	std::println("  ints.split(5): {}", monad2(ints).split(5).to<std::vector>());
}

void test_split_fast() {
	std::println("\n== Split Fast(8) ==");

	// split_fast produces std::span, so a conversion is needed to std::vector
	std::println("  ints.split_fast(1): {}", monad2(ints).split_fast<8>(1).as<std::vector<int>>().to<std::vector>());
	std::println("  ints.split_fast(3): {}", monad2(ints).split_fast<8>(3).as<std::vector<int>>().to<std::vector>());
	std::println("  ints.split_fast(5): {}", monad2(ints).split_fast<8>(5).as<std::vector<int>>().to<std::vector>());
}

void test_join_split() {
	std::println("\n== Join/Split ==");

	auto joined = monad2(strings).join_with(',').to<std::string>();
	std::println("  strings.join_with(,): {:?}", joined);

	auto split = monad2(joined).split(',').to<std::vector>();
	std::println("  {:?}.split(,): {}", joined, split);

	std::println("  strings.join_with(,).split(,): {}", monad2(strings).join_with(',').split(',').to<std::vector>());
}

void test_one_value() {
	std::println("\n== Monad of single value ==");

	std::print("  6.map(is_odd): ");
	monad2(6).map(is_odd).then(putval);
	std::println();

	std::print("  5.map(is_odd): ");
	monad2(5).map(is_odd).then(putval);
	std::println();
}

void test_optionals() {
	std::println("\n== Map over optionals ==");

	std::print("  optionals.map(to_int): ");
	monad2(opts).map(to_int).then(putval);
	std::println();

	std::print("  optionals.map(to_int).on_unexpected(): ");
	monad2(opts).map(to_int).unexpected(puterr).then(putval);
	std::println();
}

void test_map() {
	std::println("\n== Map ==");

	std::println("  ints.map(is_odd): {}", monad2(ints).map(is_odd).to<std::vector<int>>());

	auto transform_fn = [](int i) { std::putchar('x'); return i; };

	std::print("  monad       map.filter.sum call count: ");
	int sum1 = monad2(ints).map(transform_fn).filter(is_odd).sum();
	std::println();

	std::print("  std::ranges map.filter.sum call count: ");
	int sum2 = std::ranges::fold_left(ints | std::views::transform(transform_fn) | std::views::filter(is_odd), 0, std::plus{});
	std::println();

	std::println("  sums match: {}", sum1 == sum2);
}

void test_concat_sum() {
	std::println("\n== Concat and Sum ==");

	std::println("  ints.sum(): {} ", monad2(ints).sum());
	std::println("  ints.concat(ints).sum(): {} ", monad2(ints).concat(ints).sum());
}

void test_join() {
	std::println("\n== Join ==");

	std::println("  strings  .join(): {}", monad2(strings).join().to<std::string>());
	std::println("  optionals.join(): {}", monad2(opts).join().to<std::string>());
}

void test_join_with() {
	std::println("\n== Join-with ==");

	std::print("  strings.join_with(\" - \"): ");
	monad2(strings).join_with(" - "sv).then(&std::putchar);
	std::println();

	std::print("  strings.join_with(|): ");
	monad2(strings).join_with('|').then(&std::putchar);
	std::println();
}

void test_take() {
	std::println("\n== Take ==");

	std::print("  ints.take(3): ");
	monad2(ints).take(3).then(putval);
	std::println();
}

void test_drop() {
	std::println("\n== Drop ==");

	std::print("  ints.drop(3): ");
	monad2(ints).drop(3).then(putval);
	std::println();
}

void test_to() {
	std::println("\n== To ==");

	std::println("  ints.to<std::set>(): {}", monad2(ints).to<std::set>());
	std::println("  ints.concat(ints).to<std::set>(): {}", monad2(ints).concat(ints).to<std::set>());
	std::println("  ints.concat(ints).to<std::vector>(): {}", monad2(ints).concat(ints).to<std::vector>());
}

int main() {
	std::println("ints: {}", ints);
	std::println("strings: {}", strings);
	std::println("optionals: {}", monad2(opts).value_or("<nullopt>"sv).to<std::vector>()
	);

	std::println();

	test_filter();
	test_count();
	test_split();
	test_split_fast();
	test_join_split();
	test_map();
	test_optionals();
	test_one_value();
	test_concat_sum();
	test_join();
	test_join_with();
	test_take();
	test_drop();
	test_to();

	return 0;
}
