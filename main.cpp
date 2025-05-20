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
		std::println("transform x2 +3: {} ",
			monad(v).transform(mul_two).transform(add_three)
		);
	}

	{
		std::println("filter odd: {} ",
			monad(v).filter(is_odd));
	}

	{
		std::println("drop 2: {}",
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
		std::println("join {} and {}: {:s}", v2, v3, joiner);
	}

	{
		std::println("splits: {}",
			monad(std::array{ '0',  '1',  '0',  '2','3',  '0',  '4','5','6' }).split('0'));
	}

	{
		std::println("enumerate: {}",
			monad(std::array{ 'A', 'B', 'C', 'D', 'E' }).enumerate());
	}

	{
		std::println("zip: {}",
			monad(std::array{ 'A', 'B', 'C', 'D', 'E' }).zip(v));
	}

	{
		std::println("adjacent: {}",
			monad(v).adjacent<3>());
	}

	{
		std::println("chunk_by: {}",
			monad(v).chunk_by(std::ranges::less{}));
	}

	{
		std::println("cartesian_product: {}",
			monad(std::array{ 'A', 'B' }).cartesian_product(v));
	}

	return 0;
}
