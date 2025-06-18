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

template<typename T>
using in = std::conditional_t<sizeof(T) <= 2 * sizeof(void*), T, T const&>;

template<typename VT>
constexpr void add_to_container(auto& c, in<VT> v) {
	if constexpr (requires { c.insert_range(c.end(), v); })
		c.insert_range(c.end(), v);
	else if constexpr (requires { c.emplace_back(v); })
		c.emplace_back(v);
	else if constexpr (requires { c.push_back(v); })
		c.push_back(v);
	else if constexpr (requires { c.emplace(v); })
		c.emplace(v);
	else if constexpr (requires { c.insert(c.end(), v); })
		c.insert(c.end(), v);
	else
		static_assert(false, "Container does not support adding elements.");
}

template<typename T>
constexpr bool valid(T const& v) {
	if constexpr (optional_like<decltype(v)>) {
		return v.has_value();
	}
	else {
		return true;
	}
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

template<auto OrValue, typename T>
constexpr auto unbox_or(T const& opt) {
	if constexpr (optional_like<T>) {
		return opt.value_or(OrValue);
	}
	else {
		return opt;
	}
}

template<typename T>
using unboxed_t = decltype(unbox(std::declval<T>()));

template<typename T>
constexpr auto make_one(T const& val) {
	return [val](auto dst) {
		in<T> v = val;
		return dst(v);
		};
}

#if 1
template<range_like T>
constexpr auto make_fn(T const& rng) {
	return [&](auto dst) {
		for (in<typename T::value_type> v : rng) {
			if (!dst(v))
				return false;
		}

		return true;
		};
}
#else
template<range_like T>
constexpr auto make_fn(T const& rng) {
	return [first = rng.begin(), last = rng.end()](auto dst) {
		auto it = first;

		while (it != last) {
			if (!dst(*it))
				return false;
			++it;
		}

		return true;
		};
}
#endif

constexpr auto make_fns(range_like auto const&... rng) {
	return [...fns = make_fn(rng)](auto dst) {
		return (fns(dst) && ...);
		};
}

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
			return fn([&](in<T> v) {
				if (pred(v))
					return dst(v);
				return true;
				});
			};
		return ::monad2<T, decltype(f)>{std::move(f)};
	}

	constexpr auto map(std::invocable<unboxed_t<T>> auto mf) const {
		auto f = [=, fn = std::move(fn)](auto dst) {
			return fn([&](in<T> v) {
				if (valid(v))
					return dst(std::invoke(mf, unbox(v)));
				return true;
				});
			};
		return ::monad2<std::remove_cvref_t<std::invoke_result_t<decltype(mf), unboxed_t<T>>> , decltype(f) > {std::move(f)};
	}

	constexpr auto take(std::signed_integral auto n) const {
		if (n <= 0) throw;

		auto f = [=, fn = std::move(fn)](auto dst) {
			decltype(n) count = 0;
			return fn([&](in<T> v) {
				return count++ < n && dst(v);
				});
			};
		return ::monad2<T, decltype(f)>{std::move(f)};
	}

	constexpr auto drop(std::signed_integral auto n) const {
		if (n <= 0) throw;

		auto f = [=, fn = std::move(fn)](auto dst) {
			decltype(n) count = 0;
			return fn([&](in<T> v) {
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
			return fn([&](in<T> v) {
				using in_t_val = in<typename T::value_type>;
				for (in_t_val p : v) {
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
			auto send_to_dst = [&]<typename DstT = T>(in<DstT> l) {
				if constexpr (range_like<DstT>) {
					using in_dst_t = in<typename DstT::value_type>;
					for (in_dst_t p : l) {
						if (!dst(p))
							return false;
					}
					return true;
				}
				else {
					return dst(l);
				}
				};

			std::byte last[sizeof(in<T>)];

			bool first = true;
			bool const retval = fn([&](in<T> v) {
				if (first) {
					first = false;
					std::memcpy(&last, &v, sizeof(v));
					return true;
				}
				else {
					bool const cont = (send_to_dst(reinterpret_cast<in<T>&>(last)) && send_to_dst.template operator() < P > (pattern));
					std::memcpy(&last, &v, sizeof(v));
					return cont;
				}
				});

			if (retval) {
				return send_to_dst(reinterpret_cast<in<T>&>(last));
			}
			return true;
			};
		return ::monad2<typename T::value_type, decltype(f)>{std::move(f)};
	}
	constexpr auto join_with(const char* pattern) = delete REASON("Don't use raw strings. Wrap it in a string_view.");

	template<typename Other>
	constexpr auto value_or(Other&& other) const requires optional_like<T> {
		auto f = [=, fn = std::move(fn)](auto dst) {
			return fn([&](in<T> v) {
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

			bool const retval = fn([&](in<T> v) {
				if (valid(v)) {
					in<unboxed_t<T>> uv = unbox(v);
					if (uv == delimiter) {
						if (!dst(part)) {
							return false;
						}
						part.clear();
					}
					else {
						add_to_container<unboxed_t<T>>(part, uv);
					}
				}

				return true;
				});

			return retval && dst(part);
		};
		return ::monad2<Container, decltype(f)>{std::move(f)};
	}

	template<int MaxSplitSize>
		requires (MaxSplitSize > 0)
	constexpr auto split_fast(auto delimiter) const {
		constexpr bool is_string_type = std::same_as<T, char>;
		using View = std::conditional_t<is_string_type, std::string_view, std::span<T>>;
		using Container = std::array<T, MaxSplitSize>;

		auto f = [&](auto dst) {
			Container part;
			auto data = part.data();
			std::size_t i = 0;

			bool const retval = fn([&](in<T> v) {
				if (valid(v)) {
					in<unboxed_t<T>> uv = unbox(v);
					if (uv == delimiter) {
						if (!dst(View{ part.data(), i })) {
							return false;
						}
						data = part.data();
						i = 0;
					}
					else {
						if (data != &part.back()) {
							data[i++] = std::move(uv);
						}
					}
				}

				return true;
				});

			return retval && dst(View{ part.data(), i });
			};
		return ::monad2<View, decltype(f)>{std::move(f)};
	}

	// TODO use bloom filter
	//constexpr auto split(range_like auto delimiter) const {

	template<typename Cast>
		requires std::constructible_from<Cast, unboxed_t<T>>
				|| std::constructible_from<Cast, std::from_range_t, unboxed_t<T>>
	constexpr auto as() const {
		auto f = [=, fn = std::move(fn)](auto dst) {
			return fn([&](in<T> v) {
				if (valid(v)) {
					if constexpr (std::constructible_from<Cast, unboxed_t<T>>) {
						return dst(Cast{ unbox(v) });
					}
					else {
						return dst(Cast{ std::from_range, unbox(v) });
					}
					//return dst(Cast{ unbox(v) });
				}
				return true;
				});
			};
		return ::monad2<Cast, decltype(f)>{std::move(f)};
	}

	constexpr auto and_then(auto user_fn) const {
		auto f = [=, fn = std::move(fn)](auto dst) {
			return fn([&](in<T> v) {
				if (valid(v)) {
					in<unboxed_t<T>> const ub = unbox(v);
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
		fn([&](in<T> v) {
			if (valid(v))
				user_fn(unbox(v));
			return true;
			});
	}

	template<typename I = std::int64_t>
	constexpr I sum(I init = 0) const {
		fn([&](in<T> v) {
			init += unbox_or<0>(v);
			return true;
			});
		return init;
	}

	constexpr std::int64_t count() const {
		std::int64_t c{ 0 };
		fn([&](in<T> v) {
			c += valid(v);
			return true;
			});
		return c;
	}

	template<typename C>
	constexpr auto to() const {
		C c;

		then([&](in<T> v) {
			add_to_container<T>(c, v);
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
