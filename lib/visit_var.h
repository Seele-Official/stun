#pragma once
#include <tuple>
#include <type_traits>
#include <variant>
#include <utility>
template <typename T>
struct function_traits;

template<typename R, typename C, typename... A>
struct function_traits<auto (C::*)(A...) -> R> {
    using class_t = C;
    using return_t = R;
    using args_t = std::tuple<A...>;
};

template<typename R, typename C, typename... A>
struct function_traits<auto (C::*)(A...) const -> R> {
    using class_t = C;
    using return_t = R;
    using args_t = std::tuple<A...>;
};

template<typename R, typename C, typename... A>
struct function_traits<auto (C::*)(A...) noexcept -> R> {
    using class_t = C;
    using return_t = R;
    using args_t = std::tuple<A...>;
};

template<typename R, typename C, typename... A>
struct function_traits<auto (C::*)(A...) const noexcept -> R> {
    using class_t = C;
    using return_t = R;
    using args_t = std::tuple<A...>;
};

template<typename R, typename... A>
struct function_traits<auto (*)(A...) -> R> {
    using class_t = void; 
    using return_t = R;
    using args_t = std::tuple<A...>;
};


template<typename R, typename... A>
struct function_traits<auto (*)(A...) noexcept -> R> {
    using class_t = void; 
    using return_t = R;
    using args_t = std::tuple<A...>;
};
template<typename F>
struct function_traits : function_traits<decltype(&F::operator())> {};

template <typename T, typename lambda_t>
concept visitable = 
    requires (T&& t, lambda_t&& lambda) {
        lambda(std::forward<T>(t));
    } 
    && std::is_same_v<T, std::tuple_element_t<0, typename function_traits<lambda_t>::args_t>>;

template <typename T, typename... lambda_t>
concept visitable_from = (visitable<T, lambda_t> || ...);


template <typename variant_t, typename... lambdas_t>
constexpr bool var_visitable_from = false;


template <typename... args_t, typename... lambdas_t>
constexpr bool var_visitable_from<std::variant<args_t...>&, lambdas_t...> = (visitable_from<args_t&, lambdas_t...> && ...);

template <typename... args_t, typename... lambdas_t>
constexpr bool var_visitable_from<std::variant<args_t...>&&, lambdas_t...> = (visitable_from<args_t&&, lambdas_t...> && ...);

template <typename... args_t, typename... lambdas_t>
constexpr bool var_visitable_from<const std::variant<args_t...>&, lambdas_t...> = (visitable_from<const args_t&, lambdas_t...> && ...);

template <typename T, typename lambda_t, typename... lambdas_t>
void visit_from(T&& t, lambda_t&& lambda, lambdas_t&&... lambdas) {
    if constexpr (visitable<T&&, lambda_t>) {
        return lambda(std::forward<T>(t));
    } else if constexpr(sizeof...(lambdas) == 0) {
        static_assert(true, "No valid lambda provided");
    } else {
        return visit_from(std::forward<T>(t), std::forward<lambdas_t>(lambdas)...);
    }

}


template <typename variant_t, typename... lambdas_t>
void visit_var(variant_t&& var, lambdas_t&&... lambdas) {
    static_assert(
        var_visitable_from<variant_t&&, lambdas_t...>,      
        "Each variant alternative must be visitable by at least one of the provided lambdas. "
        "Check that lambda argument types are compatible with the value categories "
        "(e.g., T&, const T&, T&&) of the variant's alternatives."
    );

    std::visit(
        [&](auto&& arg) {
            visit_from(std::forward<decltype(arg)>(arg), std::forward<lambdas_t>(lambdas)...);
        },
        std::forward<variant_t>(var)
    );
        
}