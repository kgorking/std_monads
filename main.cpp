import monad;
import std;

int main()
{
	std::vector<int> const v = { 1, 2, 3, 2, 1 };

	auto is_odd = [](int i) { return i % 2; };
	auto add_three = [](int v) { return v + 3; };
	auto mul_two = [](int v) { return v * 2; };

	std::println("monad: {} ", monad(v));

	{
		std::println("transform: {} ", monad(v)
			.transform(mul_two));
	}

	{
		std::println("filter: {} ", monad(v)
			.filter(is_odd));
	}

	{
		std::println("drop: {}", monad(v)
			.drop(2)
			.drop_while(is_odd)
			.transform(add_three));
	}

	{
		std::vector<std::string_view> v2 = { "he", "ll", "o", ", " };
		std::vector<std::string_view> v3 = { "mo", "na", "d" };

		std::println("join: {:s}", monad({ v2,v3 })
			.join()
			.join());
	}

	{
		std::println("splits: {}",
			monad({ '0',  '1',  '0',  '2','3',  '0',  '4','5','6' })
			.split('0'));
	}

	{
		std::println("enumerate: {}", monad({ 'A', 'B', 'C', 'D', 'E' })
			.enumerate());
	}

	{
		auto y = { 'A', 'B', 'C', 'D', 'E' };
		std::println("zip: {}", monad(y).zip(v));
	}

	{
		std::println("adjacent: {}", monad(v)
			.adjacent<3>());
	}

	{
		std::println("chunk_by: {}", monad(v)
			.chunk_by(std::ranges::less{}));
	}

	{
		std::println("cartesian_product: {}", monad({ 'A', 'B' })
			.cartesian_product(v));
	}

	return 0;
}
