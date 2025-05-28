#pragma once
#include <type_traits>
#include <variant>
#include <utility>
template <typename T>
struct function_traits;

template<typename R, typename C, typename Arg>
struct function_traits<R(C::*)(Arg) const> {
  using arg0_t = Arg;
};

template<typename F>
struct function_traits : function_traits<decltype(&F::operator())> {};


template <typename T, typename lambda_t>
concept visitable = requires (T t, lambda_t lambda) {lambda(t);} && 
    std::is_same_v<T, typename function_traits<lambda_t>::arg0_t>;

template <typename T, typename... lambda_t>
concept visitable_from = (visitable<T, lambda_t> || ...);

template <typename T, typename lambda_t, typename... lambdas_t>
void visit_from(T&& t, lambda_t&& lambda, lambdas_t&&... lambdas) {
    if constexpr (visitable<T, lambda_t>) {
        return lambda(std::forward<T>(t));
    } else if constexpr(sizeof...(lambdas) == 0) {
        static_assert(true, "No valid lambda provided");
    } else {
        return visit_from(std::forward<T>(t), std::forward<lambdas_t>(lambdas)...);
    }

}


template <typename... args_t, typename... lambdas_t>
void visit_var(const std::variant<args_t...>& var, lambdas_t&&... lambdas) {
    static_assert(
        (visitable_from<const args_t&, lambdas_t...> && ...),
        "All variant types must be invocable by at least one lambda"
    );

    std::visit(
        [&](auto&& arg) {
            visit_from(std::forward<decltype(arg)>(arg), std::forward<lambdas_t>(lambdas)...);
        },
        var
    );
        
}
