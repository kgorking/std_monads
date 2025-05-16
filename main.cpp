import monads;
import std;


int main()
{
	std::vector<int> v = { 1, 2, 3, 4, 5 };

	auto add_three = [](int v) { return v + 3; };
	auto mul_two = [](int v) { return v * 2; };

	auto m = monad(v)
		.transform(add_three)
		.transform(mul_two)
		.view();
	std::println("{}", m);

	auto test{ m };

	auto m2 = monad(std::move(v))
		.transform(add_three)
		.transform(mul_two)
		;
	std::println("{}", m2.view());


	return 0;
}
