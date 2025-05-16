export module monads;
import std;

export
template<typename T, std::ranges::viewable_range V = decltype(std::views::all)>
struct monad {
	explicit monad(T& cont, V view = {}) noexcept : cont_(&cont), view_(view) {}

	auto transform(auto&& fn) const {
		return monad(
			(*cont_) | std::views::transform(std::forward<decltype(fn)>(fn))
		);
	}

	template<typename To>
	auto to() const {
		return std::ranges::to<To>(*cont_);
	}

private:
	T* cont_;
	V view_;
};
