# Contributing to LibreSprite

First off, **thank you** for considering contributing! I truly believe in open source and the power of community collaboration. Unlike many repositories, I actively welcome contributions of all kinds - from bug fixes to new features.

## My Promise to Contributors

- **I will respond to every PR and issue** - I guarantee feedback on all contributions
- **Bug fixes are obvious accepts** - If it fixes a bug, it's getting merged
- **New features are welcome** - I'm genuinely open to new ideas and enhancements
- **Direct line of communication** - If I'm not responding to a PR or issue, email me directly at johnvondrashek@gmail.com

## Code of Conduct

This project follows the [Rule of St. Benedict](CODE_OF_CONDUCT.md) as its code of conduct.

## Getting Started

### Set Up Your Repository

1. Fork [LibreSprite](https://github.com/JohnVonDrashek/LibreSprite) so you have a personal repository to push to.
2. Clone your personal repository with `git clone --recursive https://github.com/YOUR-USERNAME/LibreSprite`. The `--recursive` flag downloads the third party libraries needed for building.
3. Move into the directory: `cd LibreSprite`
4. Follow the [installation instructions](INSTALL.md) to build and verify your local copy works.
5. Add an upstream remote to get updates: `git remote add upstream https://github.com/JohnVonDrashek/LibreSprite`

### Quick Build

```bash
make deps      # Install dependencies (brew/apt/dnf)
make build     # Build with CMake + Ninja
make run       # Run the app
```

Run `make help` for all available commands.

## Making Changes

### Create a Feature Branch

Always work on a feature branch, not directly on master:

```bash
git checkout master
git checkout -b name-of-your-feature
```

### Commit Guidelines

- Make meaningful, atomic commits that can be easily reviewed and reverted if needed
- Write clear commit messages that describe *what* changed and *why*
- Example: `"Convert 'bool' members in ui::Widget to flags"`

### Keeping Your Fork Updated

```bash
git fetch upstream
git checkout master
git merge upstream/master
git push origin master
```

### Submitting Your Changes

1. Update your fork (see above)
2. Switch to your feature branch: `git checkout name-of-your-feature`
3. Rebase onto master: `git rebase master`
4. Rebuild and verify everything works
5. Push your branch: `git push origin name-of-your-feature`
6. Create a Pull Request on GitHub

## What Makes a Good Contribution?

- **Bug fixes** - Always welcome, always merged
- **New features** - Open an issue first to discuss, then implement
- **Documentation improvements** - Helping others understand the codebase is valuable
- **Code cleanup/refactoring** - Improves maintainability
- **Tests** - More test coverage is always appreciated

## Project Structure

LibreSprite is organized into layered modules:

| Directory | Purpose |
|-----------|---------|
| `src/app/commands/` | User commands (Open, Save, Undo, etc.) |
| `src/app/tools/` | Drawing tools (pencil, brush, fill, selection) |
| `src/app/ui/` | Application UI widgets |
| `src/doc/` | Document model (Sprite, Layer, Frame, Image) |
| `src/she/` | Platform backends (SDL2, Win32, Cocoa) |
| `data/` | Runtime assets (skins, palettes, scripts) |

See the README files in each `src/` subdirectory for detailed documentation.

## Questions?

- Open an issue on GitHub
- Join our [Discord](https://discord.gg/95gbyU5) or [Matrix](https://matrix.to/#/%23libresprite:matrix.org)
- Email: johnvondrashek@gmail.com
