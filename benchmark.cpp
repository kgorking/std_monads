import monad2;
import std;
#include "nanobench.h"

constexpr auto number_of_iterations = 1'000ull;

auto is_odd = [](int i) -> bool { return i & 1; };
auto add_three = [](int v) { return v + 3; };
auto mul_two = [](int v) { return v * 2; };

using namespace std::literals;
using namespace ankerl::nanobench;

static constexpr auto const ints = std::array{ 1, 2, 3, 4, 5 };
static constexpr auto const strings = std::array { "This"sv, "is"sv, "a"sv, "test."sv };
static           auto const opts = std::vector<std::optional<std::string_view>> { "1234", "15 foo", "bar", std::nullopt, "42", "5000000000", " 5", std::nullopt, "-43" };


void bench_map_filter_sum() {
    auto bench = Bench()
        .title("map-filter-sum")
        .relative(true)
        .minEpochIterations(number_of_iterations);

    bench.run("ranges", [&] {
        auto view = ints | std::views::transform(add_three) | std::views::filter(is_odd);
        int const sum = std::reduce(view.begin(), view.end());
		doNotOptimizeAway(sum);
        });

    bench.run("monad", [&] {
        int const sum = monad2(ints).map(add_three).filter(is_odd).sum<int>();
		doNotOptimizeAway(sum);
        });
}

void bench_join_split() {
    auto bench = Bench()
        .title("join-split")
        .relative(true)
        .minEpochIterations(number_of_iterations);

#if 1//def __cpp_lib_ranges_join_with
    bench.run("ranges", [&] {
        auto view = strings | std::views::join_with(',') | std::views::split(',');
        int const count = std::ranges::count_if(view, [](auto const&) { return true; });
        doNotOptimizeAway(count);
        });
#endif

    bench.run("monad", [&] {
        auto const count = monad2(strings).join_with(',').split(',').count();
        doNotOptimizeAway(count);
        });

    bench.run("monad - fast", [&] {
        auto const count = monad2(strings).join_with(',').split_fast<8>(',').count();
        doNotOptimizeAway(count);
        });
}

void bench_optional() {
    auto bench = Bench()
        .title("optional-map")
        .relative(true)
        .minEpochIterations(number_of_iterations);

    bench.run("ranges", [&] {
        auto view = opts
            | std::views::filter([](auto const& opt) { return opt.has_value(); })
            | std::views::transform([](auto const& opt) { return *opt; })
            | std::views::transform(&std::string_view::size);

        int const sum = std::reduce(view.begin(), view.end());
        doNotOptimizeAway(sum);
        });

    bench.run("monad", [&] {
        auto const sum = monad2(opts)
            .map(&std::string_view::size)
            .sum();
        doNotOptimizeAway(sum);
        });
}

void bench_join_to_string() {
    auto bench = Bench()
        .title("join-to-string")
        .relative(true)
        .minEpochIterations(number_of_iterations);

    bench.run("ranges", [&] {
        auto const result = strings | std::views::join | std::ranges::to<std::string>();
        doNotOptimizeAway(result);
        });

    bench.run("monad", [&] {
        auto const result = monad2(strings).join().to<std::string>();
        doNotOptimizeAway(result);
        });
}

void bench_map_size_sum() {
    auto bench = Bench()
        .title("map-size-sum")
        .relative(true)
        .minEpochIterations(number_of_iterations);

    bench.run("ranges", [&] {
        auto view = strings | std::views::transform(&std::string_view::size);
        auto const result = std::reduce(view.begin(), view.end());
        doNotOptimizeAway(result);
        });

    bench.run("monad", [&] {
        auto const result = monad2(strings).map(&std::string_view::size).sum();
        doNotOptimizeAway(result);
        });
}

int main() {
    bench_map_filter_sum();
	bench_join_split();
	bench_optional();
    bench_join_to_string();
    bench_map_size_sum();
}