#pragma once
#include <cstddef>
#include <generator>
#include <optional>
#include <string_view>
#include <variant>
#include <list>
#include <expected>

enum class ruler_type {
    no_argument,
    required_argument,
    optional_argument
};

struct ruler{
    std::string_view short_name;
    std::string_view long_name;
    ruler_type type;

    consteval ruler(std::string_view ln, ruler_type t, std::string_view sn = "") : short_name(sn), long_name(ln), type(t) {
        #define throw_if(condition, message) \
            if (condition)  \
                std::terminate(), message; \
                // throw message; // if exceptions were allowed

        if (!sn.empty()){
            throw_if( !sn.starts_with("-"), "Short option must start with '-'");
        }
        throw_if(ln.empty(), "Long option must not be empty");
        throw_if(!ln.starts_with("--"), "Long option must start with '--'");
    }
};

struct no_argument {
    std::string_view short_name;
    std::string_view long_name;
};

struct required_argument {
    std::string_view short_name;
    std::string_view long_name;
    std::string_view arg;
};


struct optional_argument {
    std::string_view short_name;
    std::string_view long_name;
    std::optional<std::string_view> arg;
};

struct positional_argument {
    std::list<std::string_view> args;
};

template <size_t N>
class opts{
public:
    using item = std::variant<
        no_argument,
        required_argument,
        optional_argument,
        positional_argument
    >;

    ruler rs[N];
    std::generator<std::expected<item, std::string>> parse(int argc, char** argv) {
        std::list<std::string_view> args{argv + 1, argv + argc};

        for (const ruler& r : rs) {
            switch (r.type) {
                case ruler_type::no_argument:{
                    auto it = std::ranges::find_if(args, [&](const std::string_view& arg) {
                        return arg == r.short_name || arg == r.long_name;
                    });
                    if (it != args.end()) {
                        args.erase(it);
                        co_yield no_argument{r.short_name, r.long_name};
                    }
                }
                break;
                case ruler_type::required_argument:{
                    auto it = std::ranges::find_if(args, [&](const std::string_view& arg) {
                        return arg == r.short_name || arg == r.long_name;
                    });
                    if (it != args.end()){
                        if (std::next(it) != args.end() && !(*std::next(it)).starts_with('-')) {
                            std::string_view arg_value = *std::next(it);
                            args.erase(it, std::next(it, 2));
                            co_yield required_argument{r.short_name, r.long_name, arg_value};
                        } else {
                            co_yield std::unexpected{"Required argument missing for " + std::string(r.short_name)};
                        }
                    }
                        
                }
                break;
                case ruler_type::optional_argument:{
                    auto it = std::ranges::find_if(args, [&](const std::string_view& arg) {
                        return arg == r.short_name || arg == r.long_name;
                    });
                    if (it != args.end()) {
                        std::optional<std::string_view> arg_value;
                        if (std::next(it) != args.end() && !(*std::next(it)).starts_with('-')) {
                            arg_value = *std::next(it);
                            args.erase(it, std::next(it, 2));
                        } else {
                            args.erase(it);
                        }
                        co_yield optional_argument{r.short_name, r.long_name, arg_value};
                    }
                }
                break;
            }
        }

        co_yield positional_argument{std::move(args)};
    }
};