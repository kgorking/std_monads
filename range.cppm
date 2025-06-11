export module range;

template<typename T>
concept range_like = requires(T rng) { rng.begin(); rng.end(); };

template<typename T>
static constexpr auto make_fn(T const& rng) {
	return [first = rng.begin(), last = rng.end()](auto dst) {
		auto it = first;
		for (; it != last; ++it) {
			dst(*it);
		}
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
						dst(v);
					}
				});
			}
		};
	}

	auto transform(auto xform) const {
		return ::range{
			[=](auto dst) {
				return fn([&](auto v) {
					dst(xform(v));
				});
			}
		};
	}

	auto take(int n) const {
		return ::range{
			[=](auto dst) {
				int count = 0;
				return fn([&](auto v) {
					if (count < n) {
						dst(v);
						++count;
					}
				});
			}
		};
	}

	auto for_each(auto dst) const {
		fn(dst);
		return *this;
	}

	auto sum() const {
		auto total = 0;
		fn([&](auto v) { total += v; });
		return total;
	}
};

template<range_like T>
range(T const& t) -> range<decltype(make_fn(t))>;
