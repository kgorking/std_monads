import monad;
import std;

using namespace std::literals;

auto is_odd = [](int i) { return i % 2 != 0; };
auto add_three = [](int v) { return v + 3; };
auto mul_two = [](int v) { return v * 2; };
auto putval = [](auto val) { std::print("{} ", val); };
auto puterr = [](std::errc val) { std::print("*{}* ", std::make_error_condition(val).message()); };

static std::expected<int, std::errc> to_int(std::string_view sv) {
	int r{};
	auto const [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), r);
	if (ec == std::errc())
		return r;
	else
		return std::unexpected(ec);
}

static constexpr auto ints = std::array{ 1, 2, 3, 4, 5 };
static constexpr auto strings = std::array{ "This"sv, "is"sv, "a"sv, "test."sv };
static constexpr auto opts = std::to_array<std::optional<std::string_view>>({ "1234", "15 foo", "bar", std::nullopt, "42", "5000000000", " 5", std::nullopt, "-43" });


static void test_filter() {
	std::println("\n== Filter ==");

	std::println("  ints.filter(is_odd)     : {}", as_monad(ints).join().filter(is_odd).to<std::vector>());
	std::println("  strings.filter(size > 2): {}", as_monad(strings).join().filter([](std::string_view s) { return s.size() > 2; }).to<std::vector>());
}

static void test_count() {
	std::println("\n== Count ==");

	std::println("  strings.filter(size > 2).count(): {}", as_monad(strings)
		.join()
		.map(&std::string_view::size)
		.filter(std::bind_back(std::greater{}, 2))
		.count());
}

static void test_split() {
	std::println("\n== Split ==");

	std::println("  ints.split(1): {}", as_monad(ints).join().split(1).to<std::vector>());
	std::println("  ints.split(3): {}", as_monad(ints).join().split(3).to<std::vector>());
	std::println("  ints.split(5): {}", as_monad(ints).join().split(5).to<std::vector>());
}

static void test_split_fast() {
	std::println("\n== Split Fast(8) ==");

	// split_fast produces std::span, so a conversion is needed to std::vector
	std::println("  ints.split_fast(1): {}", as_monad(ints).join().split_fast<8>(1).as<std::vector<int>>().to<std::vector>());
	std::println("  ints.split_fast(3): {}", as_monad(ints).join().split_fast<8>(3).as<std::vector<int>>().to<std::vector>());
	std::println("  ints.split_fast(5): {}", as_monad(ints).join().split_fast<8>(5).as<std::vector<int>>().to<std::vector>());
}

static void test_one_value() {
	std::println("\n== Monad of single value ==");

	std::print("  6.map(is_odd): ");
	as_monad(6).map(is_odd).then(putval);
	std::println();

	std::print("  5.map(is_odd): ");
	as_monad(5).map(is_odd).then(putval);
	std::println();
}

static void test_optionals() {
	std::println("\n== Map over optionals ==");

	std::print("  optionals.map(to_int): ");
	as_monad(opts).join().map(to_int).then(putval);
	std::println();

	std::print("  optionals.map(to_int).unexpected(): ");
	as_monad(opts).join().map(to_int).unexpected(puterr).then(putval);
	std::println();
}

static void test_map() {
	std::println("\n== Map ==");

	std::println("  ints.map(is_odd): {}", as_monad(ints).join().map(is_odd).to<std::vector<int>>());

	auto transform_fn = [](int i) { std::putchar('x'); return i; };

	std::print("  monad       map.filter.sum call count: ");
	int sum1 = as_monad(ints).join().map(transform_fn).filter(is_odd).sum();
	std::println();

	std::print("  std::ranges map.filter.sum call count: ");
	int sum2 = std::ranges::fold_left(ints | std::views::transform(transform_fn) | std::views::filter(is_odd), 0, std::plus{});
	std::println();

	std::println("  sums match: {}", sum1 == sum2);
}

static void test_concat_sum() {
	std::println("\n== Concat and Sum ==");

	std::println("  ints.sum(): {} ", as_monad(ints).join().sum());
	std::println("  ints.concat(ints).sum(): {} ", as_monad(ints).concat(ints).join().sum());
}

static void test_link_monad() {
	std::println("\n== Link monads ==");
	
	auto m = as_monad(opts).join().map(to_int).unbox();
	std::println("  m = opts.map(to_int): {} ", m.to<std::vector>());
	auto v = as_monad(ints).join().link(m).to<std::vector>();
	std::println("  ints.link(m): {} ", v);
}

static void test_join() {
	std::println("\n== Join ==");

	std::println("  strings  .join(): {}", as_monad(strings).join().join().to<std::string>());
	std::println("  optionals.join(): {}", as_monad(opts).join().join().to<std::string>());
}

static void test_join_with() {
	std::println("\n== Join-with ==");

	std::print("  strings.join_with(\" - \"): ");
	as_monad(strings).join_with(" - "sv).join().then(&std::putchar);
	std::println();

	std::print("  strings.join_with(\"|\"): ");
	as_monad(strings).join_with("|"sv).join().then(&std::putchar);
	std::println();

	std::print("  ints.join_with(0): ");
	as_monad(ints).join_with(0).then(putval);
	std::println();
}

static void test_join_split() {
	std::println("\n== Join/Split ==");

	auto joined = as_monad(strings).join_with(","sv).join().to<std::string>();
	std::println("  strings.join_with(,): {:?}", joined);

	auto split = as_monad(joined).join().split(',').to<std::vector>();
	std::println("  {:?}.split(,): {}", joined, split);
}

static void test_take() {
	std::println("\n== Take ==");

	std::print("  ints.take(3): ");
	as_monad(ints).join().take(3).then(putval);
	std::println();
}

static void test_drop() {
	std::println("\n== Drop ==");

	std::print("  ints.drop(3): ");
	as_monad(ints).join().drop(3).then(putval);
	std::println();
}

static void test_to() {
	std::println("\n== To ==");

	std::println("  ints.join().to<std::set>(): {}", as_monad(ints).join().to<std::set>());
	std::println("  ints.concat(ints).join().to<std::set>(): {}", as_monad(ints).concat(ints).join().to<std::set>());
	std::println("  ints.concat(ints).join().to<std::vector>(): {}", as_monad(ints).concat(ints).join().to<std::vector>());
}

static void test_async() {
	std::println("\n== Async ==");

	std::print("  strings.async.print: ");
	as_monad(strings).join().join().async().then(&std::putchar);
	std::println();

	std::atomic<std::size_t> sum = 0;
	as_monad(opts).join().async().map(to_int).then([&sum](int c) { sum += c; });
	std::println("  opts.async.map(to_int).sum: {}", sum.load());
	std::println("  opts.      map(to_int).sum: {}", as_monad(opts).join().map(to_int).sum());
}

int main() {
	std::println("ints: {}", ints);
	std::println("strings: {}", strings);
	std::println("optionals: {}", as_monad(opts).join().value_or("<>"sv).to<std::vector>());

	std::println();

	test_filter();
	test_count();
	test_split();
	test_split_fast();
	test_map();
	test_optionals();
	test_one_value();
	test_concat_sum();
	test_link_monad();
	test_join();
	test_join_with();
	test_join_split();
	test_take();
	test_drop();
	test_to();
	test_async();

	return 0;
}
