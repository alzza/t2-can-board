---
sidebar_position: 2
---

# Contributing

Tesla Open CAN Mod is an open-source project and contributions are welcome.

## Getting Started

1. Fork the repository on GitLab
2. Clone your fork locally
3. Install [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html)
4. Run the tests to make sure everything works: `pio test -e native`

## Development Workflow

1. Create a branch for your changes
2. Make your changes
3. Run all tests:
   ```bash
   pio test -e native
   pio test -e native_force_fsd
   pio test -e native_log_buffer
   ```
4. Ensure your code passes the linter:
   ```bash
   git ls-files '*.cpp' '*.h' '*.hpp' '*.ino' | xargs clang-format --dry-run --Werror --style=file
   ```
5. Submit a merge request

## Code Style

The project uses `clang-format` for consistent code formatting. The style configuration is in the repository root. Run the formatter before committing:

```bash
git ls-files '*.cpp' '*.h' '*.hpp' '*.ino' | xargs clang-format -i --style=file
```

## Adding a New Board

To add support for a new board:

1. Create a new driver header in `include/drivers/` that implements the `CanDriver` interface
2. Add a new `DRIVER_*` define in `sketch_config.h`
3. Add a new PlatformIO environment in `platformio.ini`
4. Add tests that cover the new driver
5. Update the documentation

## Adding a New Feature

1. Implement the feature in the appropriate handler(s)
2. Add a `#define` flag in `sketch_config.h` if the feature should be optional
3. Add unit tests
4. Update the documentation

## Community

Join the [Discord](https://discord.gg/ZTQKAUTd2F) to discuss development, ask questions, or report issues.
