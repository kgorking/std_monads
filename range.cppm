export module range;

template<typename T>
concept range_like = requires(T rng) { rng.begin(); rng.end(); };

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

	struct func_t {};
public:
	explicit range(Fn fn) requires (!range_like<Fn>) : fn(fn) {}

	template<range_like ...Ts>
	explicit range(Ts const&... rng) : fn(make_fns(rng...)) {}

	auto filter(auto pred) const {
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

	auto transform(auto xform) const {
		return ::range{
			[=, fn = fn](auto dst) {
				return fn([&](auto v) {
					return dst(xform(v));
					});
			}
		};
	}

	auto take(int n) const {
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

	auto drop(int n) const {
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

	constexpr auto join(range_like auto const&... rng) {
		return ::range{
			[fn = fn, ...fns = make_fn(rng)](auto dst) {
				return fn(dst) && (fns(dst) && ...);
			}
		};
	}

	range for_each(auto dst) const {
		fn([&](auto v) {
			dst(v);
			return true;
			});
		return *this;
	}

	auto sum() const {
		auto total = 0;
		fn([&](auto v) { total += v; return true; });
		return total;
	}
};

template<range_like ...Ts>
range(Ts const& ... t) -> range<decltype(make_fns(t...))>;
