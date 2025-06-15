export module monad2;
import std;

#ifdef __cpp_deleted_function
#define REASON(x) (x)
#else
#define REASON(x)
#endif

template<typename T> concept range_like = requires(T rng) { rng.begin(); rng.end(); };
template<typename T> concept optional_like = requires(T opt) { opt.has_value(); opt.value(); };
template<typename T> concept expected_like = optional_like<T> && requires(T exp) { exp.error(); };
template<typename Fn> concept function_like = requires(Fn fn) { fn([](auto) -> bool { return true; }); };


static constexpr auto unbox(auto const& opt) {
	if constexpr (optional_like<decltype(opt)>) {
		return opt.value();
	}
	else {
		return opt;
	}
}

static constexpr bool valid(auto const& v) {
	if constexpr (optional_like<decltype(v)>) {
		return v.has_value();
	}
	else {
		return true;
	}
}

static constexpr auto make_one(auto const& val) {
	return [val](auto dst) {
		return dst(val);
		};
}

static constexpr auto make_fn(range_like auto const& rng) {
	return [first = rng.begin(), last = rng.end()](auto dst) {
		auto it = first;
		bool cont = true;
		while(it != last && (cont = dst(*it)))
			++it;
		return cont;
		};
}

static constexpr auto make_fns(range_like auto const&... rng) {
	return [...fns = make_fn(rng)](auto dst) {
		return (fns(dst) && ...);
		};
}

export
template<function_like Fn>
class monad2 {
	Fn fn;

public:
	constexpr explicit monad2(Fn fn) : fn(fn) {}
	constexpr explicit monad2(range_like auto const& rng) : fn(make_fn(rng)) {}
	constexpr explicit monad2(auto const& val) : fn(make_one(val)) {}

	constexpr auto filter(auto pred) const {
		return ::monad2{
			[=, fn = fn](auto dst) {
				return fn([&](auto v) {
					if (pred(v)) {
						return dst(v);
					}
					return true;
				});
			}
		};
	}

	constexpr auto map(auto mf) const {
		return ::monad2{
			[=, fn = fn](auto dst) {
				return fn([&](auto v) {
					if (!valid(v))
						return true;
					return dst(mf(unbox(v)));
					});
			}
		};
	}

	constexpr auto take(int n) const {
		if (n <= 0)
			throw;

		return ::monad2{
			[=, fn = fn](auto dst) {
				int count = 0;
				return fn([&](auto v) {
					return count++ < n && dst(v);
				});
			}
		};
	}

	constexpr auto drop(int n) const {
		if (n <= 0)
			throw;

		return ::monad2{
			[=, fn = fn](auto dst) {
				int count = 0;
				return fn([&](auto v) {
					if (count++ >= n) {
						return dst(v);
					}
					return true;
				});
			}
		};
	}

	constexpr auto concat(range_like auto const&... rng) {
		return ::monad2{
			[fn = fn, ...fns = make_fn(rng)](auto dst) {
				return fn(dst) && (fns(dst) && ...);
			}
		};
	}

	constexpr auto join() {
		return ::monad2{
			[=, fn = fn](auto dst) {
				return fn([&](auto v) {
					static_assert(range_like<decltype(v)>, "Input must be a range");
					for(auto const& p : v) {
						if (!dst(p)) return false;
					}
					return true;
					});
			}
		};
	}

	template<typename T>
	constexpr auto join_with(T&& pattern) {
		return ::monad2{
			[=, fn = fn](auto dst) {
				return fn([&](auto v) {
					static_assert(range_like<decltype(v)>, "Input must be a range");
					for(auto const& p : v) {
						if (!dst(p)) return false;
					}
					
					if constexpr (range_like<T>) {
						for (auto const& p : pattern) {
							if (!dst(p)) return false;
						}
						return true;
					}
					else {
						return dst(pattern);
					}
					});
			}
		};
	}
	constexpr auto join_with(const char* pattern) = delete REASON("Don't use raw strings. Wrap it in a string_view.");

	constexpr auto value_or(auto other) const {
		return ::monad2{
			[=, fn = fn](auto dst) {
				return fn([&]<typename T>(T v) {
					static_assert(optional_like<T>, "Input must be an optional-like type");

					if (!valid(v))
						return dst(other);
					else
						return dst(unbox(v));
					});
			}
		};
	}

	constexpr monad2 then(auto user_fn) const {
		fn([&](auto v) {
			if (!valid(v)) return true;
			user_fn(unbox(v));
			return true;
			});
		return *this;
	}

	//
	// Terminal operations
	//

	constexpr auto sum() const {
		auto total = 0;
		fn([&](auto v) {
			if (!valid(v))
				return true;
			total += unbox(v);
			return true;
			});
		return total;
	}

	template<typename C>
	constexpr auto to() const {
		C c;
		using T = typename C::value_type;

		then([&](T v) {
			if constexpr (requires { c.emplace_back(std::declval<T>()); })
				c.emplace_back(std::forward<T>(v));
			else if constexpr (requires { c.push_back(std::declval<T>()); })
				c.push_back(std::forward<T>(v));
			else if constexpr (requires { c.emplace(std::declval<T>()); })
				c.emplace(std::forward<T>(v));
			else
				c.insert(c.end(), std::forward<T>(v));
			});

		return c;
	}
};

template<range_like T> monad2(T const& t) -> monad2<decltype(make_fn(t))>;

template<typename T>
	requires (!function_like<T>) && (!range_like<T>)
monad2(T const& val) -> monad2<decltype(make_one(val))>;
