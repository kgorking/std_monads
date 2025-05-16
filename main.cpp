import monads;
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
	std::println("{} ", m);

	auto m2 = monad(std::move(v))
		.drop(2)
		.drop_while(is_even)
		.transform(add_three)
		;
	std::println("{}", m2);


	//std::vector<char> v2 = { 'h', 'e', 'l', 'l', 'o' };
	std::vector<std::string> v2 = { "h", "e", "l", "l", "o" };
	std::vector<std::string> v3 = { "w", "o", "r", "l", "d" };
	auto str = monad({ v2,v3 })
		.join()
		.join();
	std::println("{:s}", str);

	return 0;
}
