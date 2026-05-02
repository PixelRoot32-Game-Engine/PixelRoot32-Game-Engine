# Testing

Engine tests use the **Unity** framework on the **`native_test`** PlatformIO environment, with optional coverage reports (`scripts/coverage_*.py`).

The **complete** testing documentation (suite layout under `test/unit/`, mocks, `test_config.h`, coverage flags, CI notes) is maintained in one canonical page:

- **[Testing Guide](../reference/testing-guide.md)**

## Quick commands

```bash
pio test -e native_test
pio test -e native_test --verbose
pio test -e native_test -f test_physics_actor
```

```bash
python scripts/coverage_win.py --report
python scripts/coverage_linux.py --report
```
