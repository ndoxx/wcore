#ifndef SINGLETON_HPP_INCLUDED
#define SINGLETON_HPP_INCLUDED

#include <utility>

namespace wcore
{

template <class T> class Singleton
{
    public:
        static T&   Instance();
        static void Kill();
        virtual ~Singleton(){};

    protected:
        static T*   instance_;
    private:
        T& operator= (const T&) {};
};

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

template <class T> T* Singleton<T>::instance_ = nullptr;

// Here's the meat
template <class T> T& Singleton<T>::Instance()
{
    if(instance_ == nullptr)
        instance_ = new T();
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
            if(instance_ == nullptr)
                instance_ = new T(std::forward<Args>(args)...);
        }

        static void Kill();
        virtual ~SingletonNDI(){};

    protected:
        static T* instance_;
    private:
        T& operator= (const T&) {};
};

template <class T> T* SingletonNDI<T>::instance_ = nullptr;

template <class T> void SingletonNDI<T>::Kill()
{
    delete instance_;
    instance_ = nullptr;
}

}
#endif // SINGLETON_HPP_INCLUDED
