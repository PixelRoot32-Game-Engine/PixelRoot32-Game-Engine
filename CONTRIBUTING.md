# Contributing to PixelRoot32 Game Engine

First off, thanks for taking the time to contribute!

The following is a set of guidelines for contributing to pixelroot32 Game Engine. These are mostly guidelines, not rules. Use your best judgment, and feel free to propose changes to this document in a pull request.

## Code of Conduct

This project and everyone participating in it is governed by a Code of Conduct. By participating, you are expected to uphold this code.

## How Can I Contribute?

### Reporting Bugs

This section guides you through submitting a bug report for PixelRoot32. Following these guidelines helps maintainers and the community understand your report, reproduce the behavior, and find related reports.

- **Use a clear and descriptive title** for the issue to identify the problem.
- **Describe the exact steps to reproduce the problem** in as many details as possible.
- **Provide specific examples to demonstrate the steps**. Include copy/pasteable snippets, which you use in those examples.
- **Describe the behavior you observed after following the steps** and point out what exactly is the problem with that behavior.
- **Explain which behavior you expected to see instead and why.**

### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for PixelRoot32, including completely new features and minor improvements to existing functionality.

- **Use a clear and descriptive title** for the issue to identify the suggestion.
- **Provide a step-by-step description of the suggested enhancement** in as many details as possible.
- **Provide specific examples to demonstrate the steps**.
- **Describe the current behavior** and **explain which behavior you expected to see instead** and why.

### Pull Requests

The process described here has several goals:

- Maintain PixelRoot32's quality
- Fix problems that are important to users
- Engage the community in working toward the best possible PixelRoot32

Please follow these steps to have your contribution considered by the maintainers:

1. Follow all instructions in the template
2. Follow the [Style Guide](docs/guide/index.md#standards-&-compatibility)
3. After you submit your pull request, verify that all status checks are passing

### Adding an example

`examples/` holds **five** projects, and that number is deliberate. It was
thirteen; most of those were complete games, and a catalogue nobody could read
in one sitting is a catalogue nobody reads. Complete games and per-topic demos
now live in
[**PixelRoot32-Demo-Projects**](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects),
and that is where a new project almost always belongs.

An addition here has to earn its place, so open an issue first. An example
qualifies only if it teaches **one** engine capability that none of the five
already covers, and it is not a game. If it needs a paragraph to explain what
it demonstrates, it belongs in Demo-Projects.

Every example must:

- Build `native` **and** at least one hardware environment.
- Ship `README.md`, `lib/platformio.ini` and `src/main.cpp`.
- Have a row in [`examples/README.md`](examples/README.md) — CI fails the build
  if the catalogue and the disk disagree in either direction.
- Turn **off** the capabilities it does not use. The flag list is part of the
  lesson: it tells the reader what the feature costs.

Note that examples depend on the engine through `lib_deps = symlink://../../`,
which is correct here and banned in Demo-Projects. An example inside this
repository must build against the working tree; a demo outside it must build
against a published release.

## Coding Standards

PixelRoot32 follows a specific coding style to ensure consistency and maintainability.

- Please read [Style Guide](docs/guide/index.md#standards-&-compatibility) before starting any development.
- Ensure your code matches the existing style of the project.
- Use descriptive variable and function names.
- Comment your code where necessary, especially for complex logic.

## Development Environment Setup

Please refer to the [README.md](README.md) for detailed instructions on how to set up the development environment for both ESP32 and Native PC (SDL2) platforms.

## License

By contributing, you agree that your contributions will be licensed under its MIT License.
