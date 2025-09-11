module;
#include <version>
export module monad;
import std;

#ifdef __cpp_deleted_function
#define REASON(x) (x)
#else
#define REASON(x)
#endif

template<typename T> concept range_like    = requires(T rng) { std::ranges::begin(rng); std::ranges::end(rng); };
template<typename T> concept optional_like = requires(T opt) { opt.has_value(); opt.value(); };
template<typename T> concept expected_like = optional_like<T> && requires(T exp) { exp.error(); };
template<typename F> concept function_like = std::invocable<F, decltype([]<typename T>(T const&) { return true; }) > ;

template<typename T>
using in = std::conditional_t<std::is_trivially_copyable_v<T> && sizeof(T) <= 2 * sizeof(void*), T const, T const&>;

#pragma warning(disable : 4324)
template<typename T>
struct alignas(std::hardware_destructive_interference_size) task {
	std::future<void> future;
	T data;
	std::size_t id = 0;
	bool return_value = true;
	std::binary_semaphore sema{ 0 };
	char _pad[std::hardware_destructive_interference_size - sizeof(std::size_t) - sizeof(bool) - sizeof(std::binary_semaphore) - sizeof(std::future<void>)]{};
};

template<typename Container, typename T>
	requires requires { typename Container::value_type;  }
static constexpr void add_to_container(Container& c, T const& v) {
	if constexpr (std::constructible_from<typename Container::value_type, T>) {
		if constexpr (requires { c.emplace_back(v); })
			c.emplace_back(v);
		else if constexpr (requires { c.push_back(v); })
			c.push_back(v);
		else if constexpr (requires { c.insert(v); })
			c.insert(v);
		else if constexpr (requires { c.insert(c.end(), v); })
			c.insert(c.end(), v);
		else if constexpr (requires { c.insert_range(v); })
			c.insert_range(v);
		else if constexpr (requires { c.emplace(v); })
			c.emplace(v);
		else
			static_assert(false, "Container does not support adding elements.");
	}
	else if constexpr (range_like<T> && std::same_as<typename Container::value_type, typename T::value_type>) {
		if constexpr (requires { c.append_range(v); })
			c.append_range(v);
		else if constexpr (requires { c.insert_range(v); })
			c.insert_range(v);
		else if constexpr (requires { c.insert_range(v.end(), v); })
			c.insert_range(v.end(), v);
		else if constexpr (requires { c.insert_range_after(v.end(), v); })
			c.insert_range_after(v.end(), v);
		else
			static_assert(false, "Container does not support adding ranges, or I forgot to add code that can.");
	}
	else {
		static_assert(false, "Not sure how to insert this type T into the container. Do the types match?");
	}
}

template<typename T>
static constexpr bool has_value(T const& v) noexcept {
	if constexpr (optional_like<T>)
		return v.has_value();
	else
		return true;
}
template<typename T>
static constexpr decltype(auto) unwrap(T&& v) noexcept {
	if constexpr (optional_like<T>)
		return std::forward<T>(v).value();
	else
		return std::forward<T>(v);
}

template<typename      T>	struct unwrapped    { using type = T; };
template<optional_like T>	struct unwrapped<T> { using type = typename T::value_type; };
template<typename T>		using  unwrapped_t = typename unwrapped<T>::type;

template<typename UserFn, typename ...Args>
concept must_return_void = std::is_same_v<void, std::invoke_result_t<UserFn, Args...>>;


export template<typename T, function_like Fn>
class monad {
	Fn fn;

	// Allow acces to private constructor
	template<typename, function_like>
	friend class monad;

	// Allow acces to 'as_monad' function
	template<typename T>
	friend constexpr auto as_monad(T const&);

	// This function does nothing. Good for reference.
	constexpr auto identity() const {
		auto f = [=, fn = fn](auto dst) {
			bool const retval = fn([=](in<T> v) {
				if (has_value(v)) {
					return dst(unwrap(v));
				}
				return true;
				});

			return retval;
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	constexpr explicit monad(Fn&& fn) : fn(std::forward<Fn>(fn)) {}
public:
	// Disable construction and assignment
	monad() = delete REASON("Use 'as_monad()'");
	monad(auto const& val) = delete REASON("Use 'as_monad()'");
	monad(monad const&) = delete REASON("No");
	monad(monad&&) = delete REASON("No");
	void operator=(monad const&) = delete REASON("No");
	void operator=(monad&&) = delete REASON("No");

	constexpr auto filter(std::predicate<unwrapped_t<T>> auto pred) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v)) {
					in<unwrapped_t<T>> uv = unwrap(v);
					if (std::invoke(pred, uv))
						return dst(uv);
				}
				return true;
				});
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	template<typename TypeHack = std::conditional_t<std::is_class_v<T>, T, std::nullopt_t>>
		requires std::is_class_v<T>
	constexpr auto filter(bool (TypeHack::* pred)() const) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v)) {
					in<unwrapped_t<T>> uv = unwrap(v);
					if (std::invoke(pred, uv))
						return dst(uv);
				}
				return true;
				});
			};
		using F = decltype(f);
		return ::monad<unwrapped_t<T>, F>{std::move(f)};
	}

	template<typename MapFn, typename ...Args>
		requires std::invocable<MapFn, unwrapped_t<T>, Args...>
	constexpr auto map(MapFn mf, Args&& ...args) const {
		auto f = [=, fn = fn, ...args = std::forward<Args>(args)](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v))
					return dst(std::invoke(mf, unwrap(v), args...));
				return true;
				});
			};
		using F = decltype(f);
		return monad<std::invoke_result_t<MapFn, unwrapped_t<T>, Args...>, F> {std::move(f)};
	}

	constexpr auto take(std::signed_integral auto n) const {
		auto f = [=, fn = fn](auto dst) {
			if (n <= 0)
				return true;

			//decltype(n) count = 0;
			return fn([=, count = 0](in<T> v) mutable {
				if (has_value(v))
					return count++ < n && dst(unwrap(v));
				return true;
				});
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	constexpr auto take_while(bool& b) const {
		auto f = [=, fn = fn, &b](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v))
					return b && dst(unwrap(v));
				return true;
				});
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	constexpr auto take_while(std::predicate auto pred) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v))
					return pred(unwrap(v)) && dst(unwrap(v));
				return true;
				});
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	constexpr auto drop(std::signed_integral auto n) const {
		auto f = [=, fn = fn](auto dst) {
			if (n <= 0)
				return true;

			//decltype(n) count = 0;
			return fn([=, count = 0](in<T> v) mutable {
				if (has_value(v))
					return count++ < n || dst(unwrap(v));
				return true;
				});
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	template<int N>
		requires (N >= 0 && N < std::tuple_size_v<unwrapped_t<T>>)
	constexpr auto element() const {
		auto f = [=, fn = fn](auto dst) {
			return fn([&](in<T> v) {
				if (has_value(v)) {
					return dst(std::get<N>(unwrap(v)));
				}
				return true;
				});
			};
		using F = decltype(f);
		using ElementT = std::tuple_element_t<N, unwrapped_t<T>>;
		return monad<ElementT, F>{std::move(f)};
	}

	constexpr auto keys() const requires (std::tuple_size_v<unwrapped_t<T>> >= 2) {
		return element<0>();
	}

	constexpr auto values() const requires (std::tuple_size_v<unwrapped_t<T>> >= 2) {
		return element<1>();
	}

	template<typename ...Ts>
		//requires (std::same_as<unwrapped_t<T>, unwrapped_t<Ts>> && ...)
	constexpr auto concat(Ts const&... ts) const {
		auto make_fn = [](auto const& v) {
			return [&v](auto dst) {
				if (has_value(v))
					if (!dst(unwrap(v)))
						return false;
				return true;
				};
		};

		auto f = [fn = fn, ...fns = make_fn(ts)](auto dst) {
			return fn(dst) && (fns(dst) && ...);
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	template<typename OtherT, typename OtherFn>
	constexpr auto link(monad<OtherT, OtherFn> const& m) const {
		auto f = [fn = fn, &m](auto dst) {
			return fn(dst) && m.fn(dst);
			};
		using F = decltype(f);
		return monad<T, F>{std::move(f)};
	}

	constexpr auto join() const requires range_like<unwrapped_t<T>> {
		using VT = typename unwrapped_t<T>::value_type;
		auto f = [=, fn = fn](auto dst) {
			return fn([=](auto const& v) mutable {
				if (has_value(v)) {
					for (auto const& p : unwrap(v)) {
						if (!dst(p))
							return false;
					}
				}
				return true;
				});
			};
		using F = decltype(f);
		using MonadJoin = monad<VT, F>;
		return MonadJoin{std::move(f)};
	}

	template<typename P>
		requires range_like<unwrapped_t<T>> && std::is_same_v<P, typename unwrapped_t<T>::value_type>
	constexpr auto join_with(P&& pattern) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](in<T> v) mutable {
				if (has_value(v)) {
					auto const& uv = unwrap(v);
					if (uv.empty())
						return true;
					for (auto it = std::begin(uv); it != std::prev(std::end(uv)); ++it) {
						if (!dst(*it) || !dst(pattern))
							return false;
					}
					return dst(uv.back());
				}
				return true;
				});
			};

		using F = decltype(f);
		using VT = typename unwrapped_t<T>::value_type;
		using MonadJoinWith = monad<VT, F>;
		return MonadJoinWith{std::move(f)};
	}

	template<int S>
	constexpr auto join_with(const char(&pattern)[S]) const {
		return join_with(std::string_view{ pattern });
	}

	template<typename D>
	constexpr auto split(D const delimiter) const {
		static_assert(std::is_same<unwrapped_t<T>, D>::value, "Input type 'T' and delimiter type 'D' are not comparable; maybe call 'join()' before this function?");

		constexpr bool use_string_as_container = std::same_as<T, char>;
		using Container = std::conditional_t<use_string_as_container, std::basic_string<T>, std::vector<T>>;

		auto f = [=, fn = fn](auto dst) {
			Container part{};

			bool const retval = fn([=, &part](in<T> v) mutable {
				if (has_value(v)) {
					in<unwrapped_t<T>> uv = unwrap(v);

					if (uv == delimiter) {
						if (!dst(part)) {
							return false;
						}
						part.clear();
					}
					else {
						add_to_container(part, uv);
					}
				}

				return true;
				});

			return retval && dst(part);
			};
		using F = decltype(f);
		return monad<Container, F>{std::move(f)};
	}

	template<int MaxSplitSize, typename D>
		requires (MaxSplitSize > 0)
	constexpr auto split_fast(D const delimiter) const {
		static_assert(std::is_same_v<unwrapped_t<T>, D>, "Input type 'T' and delimiter type 'D' are not comparable; maybe call 'join()' before this function?");

		constexpr bool is_string_type = std::same_as<unwrapped_t<T>, char>;
		using View = std::conditional_t<is_string_type, std::string_view, std::span<unwrapped_t<T>>>;
		using Container = std::array<T, MaxSplitSize>;

		auto f = [delimiter, fn = fn](auto dst) {
			Container part{};
			std::size_t i = 0;

			bool const retval = fn([=, &part, &i](in<T> v) {
				if (has_value(v)) {
					in<unwrapped_t<T>> uv = unwrap(v);
					if (uv == delimiter) {
						bool const retval = dst(View{ part.data(), i });
						i = 0;
						return retval;
					}
					else {
						part[i++] = uv;
						return i < MaxSplitSize;
					}
				}
				return true;
				});

			return retval && dst(View{ part.data(), i });
			};
		using F = decltype(f);
		using MonadSplitFast = monad<View, F>;
		return MonadSplitFast{std::move(f)};
	}

	// TODO use bloom filter
	//constexpr auto split(range_like auto delimiter) const {

	template<typename Cast>
		requires std::constructible_from<Cast, unwrapped_t<T>>
			  || std::constructible_from<Cast, std::from_range_t, unwrapped_t<T>>
	constexpr auto as() const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v)) {
					if constexpr (std::constructible_from<Cast, unwrapped_t<T>>) {
						return dst(Cast{ unwrap(v) });
					}
					else {
						return dst(Cast{ std::from_range, unwrap(v) });
					}
				}
				return true;
				});
			};
		using F = decltype(f);
		return monad<Cast, F>{std::move(f)};
	}

	template<typename ...Projs>
	constexpr auto project(Projs const ...projs) const {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v)) {
					return dst(std::tuple{ std::invoke(projs, unwrap(v))... });
				}
				return true;
				});
			};

		using Tuple = std::tuple<std::invoke_result_t<Projs, unwrapped_t<T>>...>;
		using F = decltype(f);
		return monad<Tuple, F>{std::move(f)};
	}

	template<typename Other>
	constexpr auto value_or(Other const& other) const requires optional_like<T> {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v))
					return dst(unwrap(v));
				else
					return dst(other);
				});
			};
		using F = decltype(f);
		return monad<typename T::value_type, F>{std::move(f)};
	}

	constexpr auto unexpected(auto err_handler) const requires expected_like<T> {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v))
					return dst(unwrap(v));
				else
					err_handler(v.error());
				return true;
				});
			};
		using F = decltype(f);
		return monad<typename T::value_type, F>{std::move(f)};
	}

	template<typename UserFn, typename ...Args>
		requires must_return_void<UserFn, unwrapped_t<T>, Args...>
	constexpr auto and_then(UserFn&& user_fn, Args&& ...args) const {
		auto f = [user_fn = std::forward<UserFn>(user_fn), &...args = std::forward<Args>(args), fn = fn](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v)) {
					in<unwrapped_t<T>> ub = unwrap(v);
					std::invoke(user_fn, ub, std::forward<Args>(args)...);
					return dst(ub);
				}
				return true;
				});
			};
		using F = decltype(f);
		return monad<T, F>{std::move(f)};
	}

	constexpr auto unbox() const requires optional_like<T> {
		auto f = [=, fn = fn](auto dst) {
			return fn([=](in<T> v) {
				if (has_value(v)) {
					if (!dst(unwrap(v)))
						return false;
				}
				return true;
				});
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	// Requires the exception handler to be callable with an exception
	template<typename Exception = std::exception, typename ExceptionHandler>
		requires std::invocable<ExceptionHandler, Exception const&>
	constexpr auto guard(ExceptionHandler&& exception_handler) const {
		auto f = [=, fn = fn, eh = std::forward<ExceptionHandler>(exception_handler)](auto dst) {
			bool const retval = fn([=](in<T> v) {
				try {
					return dst(v);
				}
				catch (Exception const& e) {
					std::invoke(eh, e);
					return true;
				}
				catch (...) {
					std::print(std::cerr, "monad::guard - unhandled exception:\n{}", std::stacktrace::current(0));
					std::terminate();
				}
				});

			return retval;
			};
		using F = decltype(f);
		return monad<T, F>{std::move(f)};
	}

	// This requires data that takes a while to process, in order to be worthwile.
	auto async(std::size_t num_threads = std::thread::hardware_concurrency()) const {
		if (num_threads > std::thread::hardware_concurrency())
			num_threads = std::thread::hardware_concurrency();

		auto f = [=, fn = fn](auto dst) {
			auto tasks = std::vector<task<T>>(num_threads);
			auto task_bitset = std::atomic_size_t{ std::numeric_limits<std::size_t>::max() };
			auto producer_completed = bool{ false };

			auto receiver = [&](task<T>* task) {
				// Enable the task slot
				task_bitset ^= (1ull << task->id);

				// Wait for an initial signal. All threads park here until they are fed data.
				task->sema.acquire();

				// Process data while the producer is not done
				while (!producer_completed) {
					// Do the work
					task->return_value = dst(unwrap(task->data));

					// Re-enable the task slot
					task_bitset ^= (1ull << task->id);

					// Wait for signal
					task->sema.acquire();
				}
				};

			// Start all the threads.
			// The task slot is initially disabled until thread setup is done
			for (std::size_t task_counter = 0; task<T>& task : tasks) {
				task_bitset ^= (1ull << task_counter);
				task.id = task_counter;
				task.future = std::async(std::launch::async, receiver, &task);
				task_counter += 1;
			}

			// Process the data.
			// Each element is fed to an available task slot on its own thread.
			bool const retval = fn([&](in<T> v) {
				if (has_value(v)) {
					// Find available task slot
					std::size_t id = 0;
					do {
						id = static_cast<std::size_t>(std::countr_zero(task_bitset.load()));
					} while (id >= num_threads);

					// Check last return value
					auto* task = &tasks.at(id);
					if (!task->return_value)
						return false;

					// Disable the task slot
					task_bitset ^= (1ull << id);

					// Copy the data to the task slot
					task->data = v;

					// Signal the task to process the data
					task->sema.release();
				}
				return true;
				});

			// Wait for tasks to finish
			while (task_bitset.load() != std::numeric_limits<std::size_t>::max())
				;

			// Mark the producer as completed and stop all tasks
			producer_completed = true;
			for (auto& task : tasks) {
				task.sema.release();
			}
			for (auto& task : tasks) {
				task.future.get();
			}

			return retval;
			};
		using F = decltype(f);
		return monad<unwrapped_t<T>, F>{std::move(f)};
	}

	//
	// Terminal operations
	//

	template<typename UserFn, typename ...Args>
		requires std::invocable<UserFn&&, unwrapped_t<T>, Args...>
	constexpr void then(UserFn&& user_fn, Args&& ...args) const {
		fn([&](const auto& v) {
			if (has_value(v))
				std::forward<UserFn>(user_fn)(unwrap(v), std::forward<Args>(args)...);
			return true;
			});
	}

	template<typename I = unwrapped_t<T>>
	constexpr I sum(I init = {}) const {
		fn([&](auto const& v) {
			if (has_value(v))
				init += unwrap(v);
			return true;
			});
		return init;
	}

	constexpr std::int64_t count() const {
		std::int64_t c{ 0 };
		fn([&](auto const& v) {
			c += has_value(v);
			return true;
			});
		return c;
	}

	template<typename C>
	constexpr void dest(C& c) const {
		fn([&](auto const& v) {
			static_assert(requires { add_to_container(c, unwrap(v)); }, "Unsupported type can not be added to container.");
			if (has_value(v))
				add_to_container(c, unwrap(v));
			return true;
			});
	}

	template<typename C>
	constexpr auto to_dest(void (*user_fn)(C&, unwrapped_t<T> const&)) const {
		C c{};

		fn([&](in<T> v) {
			if (has_value(v))
				user_fn(c, unwrap(v));
			return true;
			});

		return c;
	}

	template<typename C>
		requires requires (C c, unwrapped_t<T> t) { add_to_container(c, t); }
	constexpr auto to_dest(C& c) const {
		fn([&](T&& v) {
			if (has_value(v)) {
				add_to_container(c, unwrap(v));
			}
			return true;
			});

		return c;
	}

	template<typename C>
		requires requires (C c, unwrapped_t<T> t) { add_to_container(c, t); }
	constexpr auto to() const {
		C c{};

		fn([&](in<T> v) {
			if (has_value(v))
				add_to_container(c, unwrap(v));
			return true;
			});

		return c;
	}

	template<template<class...> typename C>
	constexpr auto to() const {
		return to<C<unwrapped_t<T>>>();
	}

	template<template<class...> typename C, typename ...Projs>
		requires (sizeof...(Projs) > 0 && requires { C<std::remove_cvref_t<std::invoke_result_t<Projs, T>>...>{}; })
	constexpr auto to(Projs ...projs) const {
		C<std::remove_cvref_t<std::invoke_result_t<Projs, T>>...> c;

		fn([&](in<T> v) {
			if (has_value(v))
				add_to_container(c, std::tuple{ std::invoke(projs, unwrap(v))... });
			return true;
			});

		return c;
	}
};


export template<typename T>
constexpr auto as_monad(T const& val) {
	auto f = [&val](auto dst) {
		if (has_value(val))
			return dst(unwrap(val));
		return true;
		};

	using F = decltype(f);
	return monad<T, F>(std::move(f));
}
