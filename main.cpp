import monads;
import std;

int main()
{
	std::vector<int> v = { 1, 2, 3, 4, 5 };

	auto add_three = [](int v) { return v + 3; };
	auto mul_two = [](int v) { return v * 2; };

	std::println("{}",
		monad(v)
		.transform(add_three)
		.transform(mul_two)
		.to<std::vector<int>>());

	//auto v2 = monad(v)
	//	.transform([](int v) { return v + 3; })
	//	.to<std::vector>();

	return 0;
}
