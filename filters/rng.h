#ifndef RNG_H
#define RNG_H

#include <stdint.h>
#include <limits>

namespace rng {
    class LCG
    {
        uint32_t x_;
        enum { a = 0x41c64e6d, b = 0x6073 };
    public:
        typedef uint32_t result_type;

        static result_type (min)() { return 0; }
        static result_type (max)()
        {
            return std::numeric_limits<result_type>::max();
        }
        LCG() { seed(); }
        void seed() { x_ = 0; }
        void seed(const result_type &n) { x_ = n; }
        result_type operator()() { return x_ = a * x_ + b; }
    };

    class Xor128
    {
        uint32_t x_[4];
        enum { a = 11, b = 8, c = 19 };
    public:
        typedef uint32_t result_type;
      
        static result_type (min)() { return 0; }
        static result_type (max)()
        {
            return std::numeric_limits<result_type>::max();
        }
        Xor128() { seed(); }
        void seed()
        {
            x_[0] = 123456789;
            x_[1] = 362436069;
            x_[2] = 521288629;
            x_[3] = 88675123;
        }
        void seed(const result_type &n)
        {
            result_type x = n;
            for (int i = 0; i < 4; ++i)
                x_[i] = x = 1812433253 * (x ^ (x >> 30)) + i;
        }
        result_type operator()()
        {
            result_type t = x_[0] ^ x_[0] << a;
            x_[0] = x_[1];
            x_[1] = x_[2];
            x_[2] = x_[3];
            return x_[3] ^=  x_[3] >> c ^ t ^ t >> b;
        }
    };
}
#endif
