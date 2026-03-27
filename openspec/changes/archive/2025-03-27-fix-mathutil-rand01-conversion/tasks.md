## 1. Fix rand01() Global Function

- [x] 1.1 Add static_cast<float>() to Fixed16::fromRaw() return at line 245 in MathUtil.h

## 2. Fix Random::rand01() Member Function

- [x] 2.1 Add static_cast<float>() to Fixed16::fromRaw() return at line 333 in MathUtil.h

## 3. Verify Fix

- [x] 3.1 Run pio test -e native_test to confirm compilation succeeds
- [x] 3.2 Verify all 530 test cases pass
