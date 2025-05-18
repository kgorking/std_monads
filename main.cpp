import monad;
import std;

int main()
{
	std::vector<int> v = { 1, 2, 3, 4, 5 };

	auto is_even = [](int i) { return i % 2; };
	auto add_three = [](int v) { return v + 3; };
	auto mul_two = [](int v) { return v * 2; };

	std::println("{} ", monad(v));

	auto m = monad(v)
		.transform(mul_two);
	if(std::ranges::equal(m, m))
		std::println("{} ", m);

	auto m2 = monad(std::move(v))
		.drop(2)
		.drop_while(is_even)
		.transform(add_three);
	std::println("{}", m2);

	//std::vector<char> v2 = { 'h', 'e', 'l', 'l', 'o' };
	std::vector<std::string_view> v2 = { "he", "ll", "o", ", " };
	std::vector<std::string_view> v3 = { "mo", "na", "d" };
	auto str = monad({ v2,v3 })
		.join()
		.join();

	std::println("{:s}", str);


	std::array const v4{ '0',  '1',  '0',  '2','3',  '0',  '4','5','6' };
	auto splits = monad(v4).lazy_split('0');
	std::println("splits: {:s}", splits.join());


	const auto vt = { 'A', 'B', 'C', 'D', 'E' };
	auto m3 = monad(vt).enumerate();
	std::println("{}", m3);

	{
		auto x = { 1, 2, 3, 4 };
		auto y = { 'A', 'B', 'C', 'D', 'E' };

		auto m = monad(x).zip(y);
		std::println("{}", m);
	}

	std::println("{}", monad({ 0, 1, 2, 3 })
		.adjacent<3>());

	{
		auto v = monad({ 0, 1, 0, 3 })
			.chunk_by(std::ranges::less{});
		std::println("chunk: {}", v);
	}

	{
		const auto x = std::array{ 'A', 'B' };
		const auto y = std::vector{ 1, 2, 3 };
		std::println("{}", monad(x).cartesian_product(y));
	}

	return 0;
}
