#ifndef SINGLETON_HPP_INCLUDED
#define SINGLETON_HPP_INCLUDED

#include <utility>
#include <mutex>

namespace wcore
{

//usage :
/*
class AAA : public Singleton<AAA>
{
public:
    friend AAA& Singleton<AAA>::Instance();
    friend void Singleton<AAA>::Kill();
private:
    AAA (const AAA&){};
    AAA();
   ~AAA();
};
*/

template <class T> class Singleton
{
public:
    static T& Instance();
    static void Kill();

    Singleton() = default;
    virtual ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    static T* instance_;

private:
    static std::once_flag init_instance_flag_;
};

template <class T> T* Singleton<T>::instance_ = nullptr;
template <class T> std::once_flag Singleton<T>::init_instance_flag_;

// Here's the meat
template <class T> T& Singleton<T>::Instance()
{
    // Ensure initialization is performed only once, even in a
    // multithreaded context
    std::call_once(init_instance_flag_, []()
    {
        instance_ = new T();
    });
    return *instance_;
}

template <class T> void Singleton<T>::Kill()
{
    delete instance_;
    instance_ = nullptr;
}

// Singleton with Non Default Initialization
template <class T> class SingletonNDI
{
public:
    static T& Instance()
    {
        return *instance_;
    }
    template <typename... Args>
    static void Init(Args&&... args)
    {
        std::call_once(init_instance_flag_, [&args...]() // may not perfect forward?
        {
            instance_ = new T(std::forward<Args>(args)...);
        });
    }

    static void Kill();
    virtual ~SingletonNDI(){};

    SingletonNDI(const SingletonNDI&) = delete;
    SingletonNDI& operator=(const SingletonNDI&) = delete;

protected:
    static T* instance_;

private:
    static std::once_flag init_instance_flag_;
};

template <class T> T* SingletonNDI<T>::instance_ = nullptr;
template <class T> std::once_flag SingletonNDI<T>::init_instance_flag_;

template <class T> void SingletonNDI<T>::Kill()
{
    delete instance_;
    instance_ = nullptr;
}

}
#endif // SINGLETON_HPP_INCLUDED
