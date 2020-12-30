#pragma once

namespace uc
{
    namespace util
    {
        namespace noncopyable_  // protection from unintended ADL
        {
            class noncopyable
            {
                protected:
                noncopyable() = default;
                ~noncopyable() = default;

                private:  // emphasize the following members are private
                noncopyable(const noncopyable&) = delete;
                const noncopyable& operator=(const noncopyable&) = delete;
            };
        }

        using  noncopyable = noncopyable_::noncopyable;
    }
}
