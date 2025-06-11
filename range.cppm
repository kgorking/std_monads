export module range;

template<typename T>
concept range_like = requires(T rng) { rng.begin(); rng.end(); };

template<typename T>
static constexpr auto make_fn(T const& rng) {
	return [first = rng.begin(), last = rng.end()](auto dst) {
		auto it = first;
		while (it != last && dst(*it))
			++it;
	};
}

export
template<typename Fn>
class range {
	Fn fn;

public:
	explicit range(Fn fn) : fn(fn) {}
	explicit range(range_like auto const& rng) : fn(make_fn(rng)) {}

	auto filter(auto pred) const {
		return ::range{
			[=](auto dst) {
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
			[=](auto dst) {
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
			[=](auto dst) {
				int count = 0;
				return fn([&](auto v) {
					return count++ < n && dst(v);
				});
			}
		};
	}

	auto for_each(auto dst) const {
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

template<range_like T>
range(T const& t) -> range<decltype(make_fn(t))>;
