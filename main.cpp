//import monad;
import monad2;
import std;

using namespace std::literals;

auto is_odd = [](int i) { return i % 2 != 0; };
auto add_three = [](int v) { return v + 3; };
auto mul_two = [](int v) { return v * 2; };
auto putval = [](auto val) { std::print("{} ", val); };

std::optional<int> to_int(std::string_view sv) {
	int r{};
	auto const [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), r);
	if (ec == std::errc())
		return r;
	else
		return std::nullopt;
}

static constexpr std::array const ints = { 1, 2, 3, 4, 5 };
static constexpr std::array strings{ "This"sv, "is"sv, "a"sv, "test."sv };
static const     std::vector<std::optional<std::string>> opts{ "1234", "15 foo", "bar", std::nullopt, "42", "5000000000", " 5", std::nullopt, "-43" };

void test_one_value() {
	std::print("6.map(is_odd): ");
	monad2(6).map(is_odd).then(putval);
	std::println();

	std::print("5.map(is_odd): ");
	monad2(5).map(is_odd).then(putval);
	std::println();
}

void test_optionals() {
	std::print("optionals.map(to_int): ");
	monad2(opts).map(to_int).then(putval);
	std::println();
}

void test_map() {
	std::print("ints.map(is_odd): ");
	monad2(ints)
		.map(is_odd)
		.then(putval);
	std::println();
}

void test_concat_sum() {
	std::println("ints.concat(ints).sum(): {} ", monad2(ints)
		.concat(ints)
		.sum());
}

void test_join() {
	std::print("strings.join(): ");
	monad2(strings)
		.join()
		.then(&std::putchar);
	std::println();
}

void test_join_with() {
	std::print("strings.join_with(\" - \"): ");
	monad2(strings)
		.join_with(" - "sv)
		.then(&std::putchar);
	std::println();
}

int main() {
	std::println("ints: {}", ints);
	std::println("strings: {}", strings);
	std::print("optionals: [");
	monad2(opts)
		.value_or("nullopt"sv)
		.map([](auto const& v) { return std::format("{:?}", v); })
		.then(putval);
	std::println("]");

	std::println();

	test_optionals();
	test_one_value();
	test_map();
	test_concat_sum();
	test_join();
	test_join_with();

	return 0;
}
