import monad;
import std;

int main()
{
	std::vector<int> const v = { 1, 2, 3, 2, 1 };

	auto is_odd = [](int i) { return i % 2; };
	auto add_three = [](int v) { return v + 3; };
	auto mul_two = [](int v) { return v * 2; };
	auto no_return = [](int v) { std::println("no return: {}", v); };

	std::println("monad: {} ", monad(v));

	{
		auto letters = std::array{ 'e', 'd', 'c', 'b', 'a' };
		auto m = monad(letters)
			.filter(is_odd)
			.sort_copy()
			.enumerate();
		std::println("\nsort and enum {}: {}", letters, m);
	}

	{
		std::println("\ntransform x2: {} ",
			monad(v).transform(mul_two)
		);
	}

	{
		auto const a2 = std::array{ 2,4,6 };
		auto m = monad(std::array{ 1,2,3,4,5,6,7 }).filter(is_odd);
		std::println("\n{} != {}: {}", m, a2, m.not_equal_to(a2));
	}

	{
		std::println("\nfilter odd: {} ",
			monad(v).filter(is_odd));
	}

	{
		std::println("\nfilter not odd: {} ",
			monad(v).filter_not(is_odd));
	}

	{
		std::println("\ndrop 2: {}",
			monad(v).drop(2));
	}

	{
		std::vector<std::string_view> v2 = { "he", "ll", "o", ", " };
		std::vector<std::string_view> v3 = { "mo", "na", "d" };
		auto joiner = monad(std::array{ v2,v3 })
			.join() // join string_view
			.join() // join vector
			//.join()
		;
		std::println("\njoin {} and {}: {:?s}", v2, v3, joiner);
	}

	{
		auto const arr = std::string_view{ "hello, monad" };
		std::println("\nsplit {:?} on 'o': {}", arr, monad(arr).split('o'));
	}

	{
		auto const arr = std::array{ 'A', 'B', 'C', 'D', 'E' };
		std::println("\nenumerate on {}: {}", arr, monad(arr).enumerate());
	}

	{
		auto const arr = std::array{ 'A', 'B', 'C', 'D', 'E' };
		std::println("\nzip {} and {}: {}", arr, v, monad(arr).zip(v));
	}

	{
		std::println("\nadjacent: {}",
			monad(v).adjacent<3>());
	}

	{
		std::println("\nchunk_by: {}",
			monad(v).chunk_by(std::ranges::less{}));
	}

	{
		std::println("\ncartesian_product: {}",
			monad(std::array{ 'A', 'B' }).cartesian_product(v));
	}

	return 0;
}
