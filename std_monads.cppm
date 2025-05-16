export module monads;
import std;

export
template<typename T, std::ranges::viewable_range V>
struct monad {
	explicit monad(T& cont) noexcept : cont_(&cont) {}

	auto transform(auto&& fn) const {
		auto xform = (*cont_) | std::views::transform(std::forward<decltype(fn)>(fn));
		return monad<decltype(xform)>(xform);
	}

	template<typename To>
	auto to() const {
		return std::ranges::to<To>(*cont_);
	}

private:
	T* cont_;
};
