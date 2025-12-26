#pragma once

// -----------------------------------------------------------------------------

#ifndef __cplusplus
    #error C++ compiler required.
#endif // !__cplusplus

// SimpleFunctionRef implements a lightweight version of the std::function.
template<typename R, typename... Args>
class SimpleFunctionRef
{
private:
    void* obj;
    R (*callback)(void*, Args...);

public:
    SimpleFunctionRef() : obj(nullptr), callback(nullptr) {}
    SimpleFunctionRef(const SimpleFunctionRef&) = default;
    SimpleFunctionRef(SimpleFunctionRef&&) = default;
    SimpleFunctionRef& operator=(const SimpleFunctionRef&) = default;
    SimpleFunctionRef& operator=(SimpleFunctionRef&&) = default;

    template<typename Lambda>
    SimpleFunctionRef(Lambda& lambda)
        {
            obj = &lambda;
            callback = [](void* o, Args... args) -> R {
                return (*static_cast<Lambda*>(o))(args...);
            };
        };

    R operator()(Args... args) const
        {
            return callback(obj, args...);
        };

    explicit operator bool() const
        {
            return callback != nullptr;
        };
};
