#pragma once

#include <functional>

class ScopeExit
{
public:
    template<typename T>
    explicit inline ScopeExit(T&& onScopeExitFunction) :
        m_onScopeExitFunction(std::forward<T>(onScopeExitFunction))
    {
    }

    inline ~ScopeExit()
    {
        m_onScopeExitFunction();
    }

private:
    std::function<void()> m_onScopeExitFunction;
};