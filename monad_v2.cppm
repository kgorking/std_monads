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

template<typename VT>
constexpr void add_to_container(auto& c, VT&& v) {
	if constexpr (requires { c.emplace_back(std::declval<VT>()); })
		c.emplace_back(std::forward<VT>(v));
	else if constexpr (requires { c.push_back(std::declval<VT>()); })
		c.push_back(std::forward<VT>(v));
	else if constexpr (requires { c.emplace(std::declval<VT>()); })
		c.emplace(std::forward<VT>(v));
	else if constexpr (requires { c.insert_range(c.end(), std::declval<VT>()); })
		c.insert_range(c.end(), std::forward<VT>(v));
	else
		c.insert(c.end(), std::forward<VT>(v));
}

template<typename T>
constexpr auto unbox(T const& opt) {
	if constexpr (optional_like<T>) {
		return opt.value();
	}
	else {
		return opt;
	}
}

template<typename T>
using unboxed_t = decltype(unbox(std::declval<T>()));

constexpr bool valid(auto const& v) {
	if constexpr (optional_like<decltype(v)>) {
		return v.has_value();
	}
	else {
		return true;
	}
}

constexpr auto make_one(auto const& val) {
	return [val](auto dst) {
		return dst(val);
		};
}

constexpr auto make_fn(range_like auto const& rng) {
	return [first = rng.begin(), last = rng.end()](auto dst) {
		auto it = first;
		bool cont = true;
		// TODO unroll
		while (it != last && (cont = dst(*it)))
			++it;
		return cont;
		};
}

constexpr auto make_fns(range_like auto const&... rng) {
	return [...fns = make_fn(rng)](auto dst) {
		return (fns(dst) && ...);
		};
}

// TODO return std::bool_constant
export
template<typename T, function_like Fn>
class monad2 {
	Fn fn;

public:
	constexpr explicit monad2(Fn&& fn) : fn(std::forward<Fn>(fn)) {}
	constexpr explicit monad2(range_like auto const& rng) : fn(make_fn(rng)) {}
	constexpr explicit monad2(auto const& val) : fn(make_one(val)) {}

	constexpr auto filter(std::predicate<T> auto pred) const {
		auto f = [=, fn = std::move(fn)](auto dst) {
			return fn([&](auto v) {
				if (pred(v))
					return dst(v);
				return true;
				});
			};
		return ::monad2<T, decltype(f)>{std::move(f)};
	}

	constexpr auto map(std::invocable<unboxed_t<T>> auto mf) const {
		auto f = [=, fn = std::move(fn)](auto dst) {
			return fn([&](auto v) {
				if (valid(v))
					return dst(std::invoke(mf, unbox(v)));
				return true;
				});
			};
		return ::monad2<std::invoke_result_t<decltype(mf), unboxed_t<T>> , decltype(f) > {std::move(f)};
	}

	constexpr auto take(std::signed_integral auto n) const {
		if (n <= 0) throw;

		auto f = [=, fn = std::move(fn)](auto dst) {
			decltype(n) count = 0;
			return fn([&](auto v) {
				return count++ < n && dst(v);
				});
			};
		return ::monad2<T, decltype(f)>{std::move(f)};
	}

	constexpr auto drop(std::signed_integral auto n) const {
		if (n <= 0) throw;

		auto f = [=, fn = std::move(fn)](auto dst) {
			decltype(n) count = 0;
			return fn([&](auto v) {
				return count++ < n || dst(v);
				});
			};
		return ::monad2<T, decltype(f)>{std::move(f)};
	}

	constexpr auto concat(range_like auto const&... rng) const {
		auto f = [fn = std::move(fn), ...fns = make_fn(rng)](auto dst) {
			return fn(dst) && (fns(dst) && ...);
			};
		return ::monad2<T, decltype(f)>{std::move(f)};
	}

	constexpr auto join() const requires range_like<T> {
		auto f = [=, fn = std::move(fn)](auto dst) {
			return fn([&](auto const& v) {
				for (auto p : v) {
					if (!dst(p)) return false;
				}
				return true;
				});
			};
		return ::monad2<typename T::value_type, decltype(f)>{std::move(f)};
	}

	template<typename P>
	constexpr auto join_with(P&& pattern) const requires range_like<T> {
		auto f = [=, fn = std::move(fn)](auto dst) {
			auto send_to_dst = [&](auto const& l) {
				if constexpr (range_like<decltype(l)>) {
					for (auto p : l) {
						if (!dst(p))
							return false;
					}
					return true;
				}
				else {
					return dst(l);
				}
				};

			std::optional<T> last;
			bool const retval = fn([&](auto const& v) {
				auto const l = last;
				last = v;

				if (l)
					if (!(send_to_dst(*l) && send_to_dst(pattern)))
						return false;
				return true;
				});

			return last ? send_to_dst(*last) : true;
			};
		return ::monad2<typename T::value_type, decltype(f)>{std::move(f)};
	}
	constexpr auto join_with(const char* pattern) = delete REASON("Don't use raw strings. Wrap it in a string_view.");

	template<typename Other>
	constexpr auto value_or(Other&& other) const requires optional_like<T> {
		auto f = [=, fn = std::move(fn)](auto dst) {
			return fn([&](auto v) {
				if (!valid(v))
					return dst(other);
				else
					return dst(unbox(v));
				});
			};
		return ::monad2<typename T::value_type, decltype(f)>{std::move(f)};
	}

	// TODO detect immutable streams and use iterators instead of copying
	constexpr auto split(auto delimiter) const {
		constexpr bool use_string_as_container = std::same_as<T, char>;
		using Container = std::conditional_t<use_string_as_container, std::basic_string<T>, std::vector<T>>;

		auto f = [=, fn = std::move(fn)](auto dst) {
			Container part;

			bool const retval = fn([&](auto const& v) {
				if (valid(v)) {
					auto const uv = unbox(v);
					if (uv == delimiter) {
						if (!dst(part)) {
							return false;
						}
						part.clear();
					}
					else {
						add_to_container(part, std::move(uv));
					}
				}

				return true;
				});

			return retval && dst(part);
		};
		return ::monad2<Container, decltype(f)>{std::move(f)};
	}

	// TODO use bloom filter
	//constexpr auto split(range_like auto delimiter) const {

	constexpr auto and_then(auto user_fn) const {
		auto f = [=, fn = std::move(fn)](auto dst) {
			return fn([&](auto v) {
				if (valid(v)) {
					auto const ub = unbox(v);
					user_fn(ub);
					return dst(ub);
				}
				return true;
				});
			};
		return ::monad2<T, decltype(f)>{std::move(f)};
	}

	//
	// Terminal operations
	//

	template<typename UserFn>
	constexpr void then(UserFn&& user_fn) const {
		fn([&](auto const& v) {
			if (valid(v))
				user_fn(unbox(v));
			return true;
			});
	}

	template<typename I = std::int64_t>
	constexpr I sum(I init = 0) const {
		fn([&](auto const& v) {
			if (valid(v))
				init += unbox(v);
			return true;
			});
		return init;
	}

	constexpr std::int64_t count() const {
		std::int64_t c{ 0 };
		fn([&](auto const& v) {
			c += valid(v);
			return true;
			});
		return c;
	}

	template<typename C>
	constexpr auto to() const {
		C c;

		then([&](auto v) {
			add_to_container(c, v);
			});

		return c;
	}

	template<template<class...> typename C>
	constexpr auto to() const {
		return to<C<T>>();
	}
};


template<range_like T>
monad2(T const& t) -> monad2<typename T::value_type, decltype(make_fn(t))>;

template<typename T>
	requires (!function_like<T>) && (!range_like<T>)
monad2(T const& val)->monad2<T, decltype(make_one(val))>;
