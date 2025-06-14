export module range;
import std;

#ifdef __cpp_deleted_function
#define REASON(x) (x)
#else
#define REASON(x)
#endif

template<typename T>
concept range_like = requires(T rng) { std::begin(rng); std::end(rng); };

static constexpr auto make_fn(range_like auto const& rng) {
	return [first = rng.begin(), last = rng.end()](auto dst) {
		auto it = first;
		bool cont = true;
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
template<typename Fn>
class range {
	Fn fn;

public:
	constexpr explicit range(Fn fn) requires (!range_like<Fn>) : fn(fn) {}

	template<range_like ...Ts>
	constexpr explicit range(Ts const&... rng) : fn(make_fns(rng...)) {}

	constexpr auto filter(auto pred) const {
		return ::range{
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

	constexpr auto transform(auto xform) const {
		return ::range{
			[=, fn = fn](auto dst) {
				return fn([&](auto v) {
					return dst(xform(v));
					});
			}
		};
	}

	constexpr auto take(int n) const {
		if (n <= 0)
			throw;

		return ::range{
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

		return ::range{
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
		return ::range{
			[fn = fn, ...fns = make_fn(rng)](auto dst) {
				return fn(dst) && (fns(dst) && ...);
			}
		};
	}

	constexpr auto join() {
		return ::range{
			[=, fn = fn](auto dst) {
				return fn([&](auto v) {
					static_assert(std::ranges::forward_range<decltype(v)>, "Input must be a range");
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
		return ::range{
			[=, fn = fn](auto dst) {
				return fn([&](auto v) {
					for(auto const& p : v) {
						if (!dst(p)) return false;
					}
					
					if constexpr (std::ranges::forward_range<T>) {
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

	constexpr range for_each(auto user_fn) const {
		fn([&](auto v) {
			user_fn(v);
			return true;
			});
		return *this;
	}

	constexpr auto sum() const {
		auto total = 0;
		fn([&](auto v) { total += v; return true; });
		return total;
	}
};

template<range_like ...Ts>
range(Ts const& ... t) -> range<decltype(make_fns(t...))>;
