#include <iostream>
#include <iomanip>     // To use 'std::boolalpha'.
#include <type_traits>

// See #[02-11-18]
// Detector idiom implementation and test cases

namespace detail
{
template <class Default, class AlwaysVoid,
          template<class...> class Op, class... Args>
struct detector
{
    using value_t = std::false_type;
    using type = Default;
};

template <class Default, template<class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
{
    // Note that std::void_t is a C++17 feature
    using value_t = std::true_type;
    using type = Op<Args...>;
};

struct nonesuch
{
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const&) = delete;
    void operator=(nonesuch const&) = delete;
};

} // namespace detail

template <template<class...> class Op, class... Args>
using is_detected = typename detail::detector<detail::nonesuch, void, Op, Args...>::value_t;

template <template<class...> class Op, class... Args>
using detected_t = typename detail::detector<detail::nonesuch, void, Op, Args...>::type;

template <class Default, template<class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

#define GENERATE_HAS_MEMBER( FN )                                                \
template <typename T, typename... Args>                                          \
using FN##_detector = decltype(std::declval<T&>( ).FN(std::declval<Args>()...)); \
template <typename T, typename... Args>                                          \
using has_##FN = is_detected<FN##_detector, T, Args...>;                         \
template <typename T, typename... Args>                                          \
constexpr bool has_##FN##_v = has_##FN<T, Args...>::value


// Test code

GENERATE_HAS_MEMBER(process);
GENERATE_HAS_MEMBER(update);
GENERATE_HAS_MEMBER(nop);
GENERATE_HAS_MEMBER(age_of_the_captain);
GENERATE_HAS_MEMBER(ambiguous);
GENERATE_HAS_MEMBER(vfunc);
GENERATE_HAS_MEMBER(vfunc2);
GENERATE_HAS_MEMBER(vfunc3);

struct Foo
{
    // ...
};

struct A
{
    void nop();
    void update(float);
    bool process(int, int);

    void ambiguous(bool);

    virtual void vfunc(int);
    virtual void vfunc2();
    virtual void vfunc3() = 0;

    int foo;
    float bar;
};

struct B
{
    void age_of_the_captain(float, const Foo&);

    void ambiguous(bool);

    int baz;
    double qux;
};

struct C : A, B
{
    virtual void vfunc(int) override;
    virtual void vfunc3() final;
};

// Update behavior using SFINAE

struct CmpBase
{
    CmpBase(char name): name_(name){}
    char name_;
};

struct CmpNotUpdatable: public CmpBase
{
    CmpNotUpdatable(char name): CmpBase(name){}
};

struct CmpUpdatable: public CmpBase
{
    CmpUpdatable(char name): CmpBase(name){}

    void update(float dt)
    {
        t_ += dt;
        std::cout << "t_= " << t_;
    }

    float t_ = 0.f;
};

template <typename Component = CmpBase>
struct RigSFINAE
{
    template<class K = Component>
    typename std::enable_if<has_update_v<K, float>, bool>::type update(K& cmp)
    {
        std::cout << "Updating component '" << cmp.name_ << "': ";
        cmp.update(0.1f);
        std::cout << std::endl;
        return true;
    }

    template<class K = Component>
    typename std::enable_if<!has_update_v<K, float>, bool>::type update(K& cmp)
    {
        std::cout << "Component '" << cmp.name_ << "' has no update(float) function and will not be updated." << std::endl;
        return false;
    }
};

// Update behavior using tag dispatching
namespace detail
{
template <typename Component>
void update_dispatch(Component& component, std::false_type)
{
    std::cout << "Component '" << component.name_ << "' has no update(float) function and will not be updated." << std::endl;
}

template <typename Component>
void update_dispatch(Component& component, std::true_type)
{
    std::cout << "Updating component '" << component.name_ << "': ";
    component.update(0.1f);
    std::cout << std::endl;
}
} // namespace detail

template <typename Component>
void Update(Component& component)
{
    detail::update_dispatch(component, has_update<Component, float>());
}

int main(int argc, char const *argv[])
{
    std::cout << "* Displaying detector values for various members." << std::endl;

    std::cout << std::boolalpha
              << "struct 'A' has 'nop()': "
              << has_nop_v<A>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'A' has 'update(float)': "
              << has_update_v<A, float>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'A' has 'process(int, int)': "
              << has_process_v<A, int, int>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'A' has 'vfunc(int)': "
              << has_vfunc_v<A, int>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'A' has 'vfunc2()': "
              << has_vfunc2_v<A>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'A' has 'vfunc3()': "
              << has_vfunc3_v<A>
              << std::endl;


    std::cout << std::boolalpha
              << "struct 'B' has 'age_of_the_captain(float, const Foo&)': "
              << has_age_of_the_captain_v<B, float, const Foo&>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'B' has 'update(float)': "
              << has_update_v<B, float>
              << std::endl;


    std::cout << std::boolalpha
              << "struct 'C' has 'nop()': "
              << has_nop_v<C>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'C' has 'update(float)': "
              << has_update_v<C, float>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'C' has 'age_of_the_captain(float, const Foo&)': "
              << has_age_of_the_captain_v<C, float, const Foo&>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'C' has 'ambiguous(bool)': "
              << has_ambiguous_v<C, bool>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'C' has 'vfunc2()': "
              << has_vfunc2_v<C>
              << std::endl;

    std::cout << std::boolalpha
              << "struct 'C' has 'vfunc3()': "
              << has_vfunc3_v<C>
              << std::endl;

    std::cout << std::endl;


    // Use SFINAE and enable_if to select behavior depending on the presence
    // of an update(float) function
    std::cout << "* Unsing SFINAE to select correct behavior depending on update(float) existence." << std::endl;

    RigSFINAE<> component_updater;
    CmpNotUpdatable a('a');
    CmpUpdatable b('b');

    for(int ii=0; ii<3; ++ii)
    {
        component_updater.update(a);
        component_updater.update(b);
    }

    std::cout << std::endl;
    std::cout << "* Unsing Tag Dispatching to select correct behavior depending on update(float) existence." << std::endl;

    // Use tag dispatching instead
    for(int ii=0; ii<3; ++ii)
    {
        Update(a);
        Update(b);
    }

    return 0;
}
