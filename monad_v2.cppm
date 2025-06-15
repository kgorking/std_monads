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

template<typename Fn>
static constexpr auto defer(Fn&& fn) {
	struct D {
		Fn fn;
		constexpr ~D() noexcept(noexcept(fn())) { fn(); }
	} defer{ std::forward<Fn>(fn) };
	return defer;
}

static constexpr auto unbox(auto&& opt) {
	if constexpr (optional_like<decltype(opt)>) {
		return opt.value();
	}
	else {
		return opt;
	}
}

static constexpr bool valid(auto&& v) {
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
		// TODO unroll
		while (it != last && (cont = dst(*it)))
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
template<typename T, function_like Fn>
class monad2 {
	using value_type = T;
	Fn fn;

public:
	constexpr explicit monad2(Fn fn) : fn(fn) {}
	constexpr explicit monad2(range_like auto const& rng) : fn(make_fn(rng)) {}
	constexpr explicit monad2(auto const& val) : fn(make_one(val)) {}

	constexpr auto filter(auto pred) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](auto v) {
				if (pred(v)) {
					return dst(v);
				}
				return true;
				});
			};
		return ::monad2<T, decltype(f)>{f};
	}

	constexpr auto map(auto mf) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](auto v) {
				if (!valid(v))
					return true;
				return dst(mf(unbox(v)));
				});
			};
		return ::monad2 < std::invoke_result_t<decltype(mf), decltype(unbox(value_type{})) > , decltype(f) > {f};
	}

	constexpr auto take(int n) const {
		if (n <= 0)
			throw;

		auto f = [=, fn = fn](auto dst) {
			int count = 0;
			return fn([&](auto v) {
				return count++ < n && dst(v);
				});
			};
		return ::monad2<value_type, decltype(f)>{f};
	}

	constexpr auto drop(int n) const {
		if (n <= 0)
			throw;

		auto f = [=, fn = fn](auto dst) {
			int count = 0;
			return fn([&](auto v) {
				if (count++ >= n) {
					return dst(v);
				}
				return true;
				});
			};
		return ::monad2<value_type, decltype(f)>{f};
	}

	constexpr auto concat(range_like auto const&... rng) const {
		auto f = [fn = fn, ...fns = make_fn(rng)](auto dst) {
			return fn(dst) && (fns(dst) && ...);
			};
		return ::monad2<value_type, decltype(f)>{f};
	}

	constexpr auto join() const requires range_like<value_type> {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](auto v) {
				for (auto const& p : v) {
					if (!dst(p)) return false;
				}
				return true;
				});
			};
		return ::monad2<typename value_type::value_type, decltype(f)>{f};
	}

	template<typename P>
	constexpr auto join_with(P&& pattern) const requires range_like<value_type> {
		auto f = [=, fn = fn](auto dst) {
			bool retval = true;
			value_type const* last = nullptr;

			auto def_ = defer([&] {
				if (last) {
					for (auto const& p : *last) {
						retval = dst(p);
						if (!retval)
							return false;
					}
				}
				return retval;
				});

			return fn([&](auto const& v) {
				if (last) {
					for (auto const& p : *last) {
						if (!dst(p)) {
							retval = false;
							return false;
						}
					}

					if constexpr (range_like<decltype(pattern)>) {
						for (auto const& p : pattern) {
							if (!dst(p)) {
								retval = false;
								return false;
							}
						}
					}
					else {
						retval = dst(pattern);
					}
				}

				last = &v;
				return retval;
				});
			};
		return ::monad2<typename value_type::value_type, decltype(f)>{f};
	}
	constexpr auto join_with(const char* pattern) = delete REASON("Don't use raw strings. Wrap it in a string_view.");

	constexpr auto value_or(auto other) const requires optional_like<value_type> {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](auto v) {
				if (!valid(v))
					return dst(other);
				else
					return dst(unbox(v));
			});
			};
		return ::monad2<typename value_type::value_type, decltype(f)>{f};
	}

	constexpr auto split(auto delimiter) const {
		auto f = [=, fn = fn]<typename Dst>(Dst dst) {
			std::vector<value_type> part;
			bool retval = true;

			auto def_ = defer([&] {
				if constexpr (requires { std::string_view{ part }; }) {
					retval = retval && dst(std::string_view{ part });
				}
				else {
					retval = retval && dst(part);
				}
				});

			return fn([&](auto v) {
				if (!valid(v))
					return true;

				auto const uv = unbox(v);
				if (uv == delimiter) {
					if constexpr (requires { std::string_view{ part }; }) {
						if (!dst(std::string_view{ part })) {
							retval = false;
							return false;
						}
					}
					else {
						if (!dst(part)) {
							retval = false;
							return false;
						}
					}
					part.clear();
				}
				else {
					part.push_back(uv);
				}

				return retval;
				});
		};
		return ::monad2<std::span<T>, decltype(f)>{f};
	}

	constexpr auto and_then(auto user_fn) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](auto v) {
				if (!valid(v))
					return true;
				auto const ub = unbox(v);
				user_fn(ub);
				return dst(ub);
				});
			};
		return ::monad2<value_type, decltype(f)>{f};
	}

	//
	// Terminal operations
	//

	constexpr void then(auto user_fn) const {
		fn([&](auto v) {
			if (!valid(v)) return true;
			user_fn(unbox(v));
			return true;
			});
	}

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
		using CT = typename C::value_type;

		then([&]<typename VT>(VT&& v) {
			static_assert(std::convertible_to<VT, CT>, "Value type is not convertible to container type.");

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

	template<template<class...> typename C>
	constexpr auto to() const {
		return to<C<value_type>>();
	}
};

template<range_like T>
monad2(T const& t) -> monad2<typename T::value_type, decltype(make_fn(t))>;

template<typename T>
	requires (!function_like<T>) && (!range_like<T>)
monad2(T const& val)->monad2<T, decltype(make_one(val))>;
