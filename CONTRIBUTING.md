# Contributing to OSynaptic-FX

Thank you for your interest in contributing to OSynaptic-FX! This document provides guidelines and instructions for contributing to the project.

## Getting Started

### Prerequisites

- Knowledge of C99 programming language
- Arduino IDE 2.x or Arduino CLI (for library integration validation)
- Familiarity with embedded systems development
- Understanding of the project's specification documents

### Development Environment Setup

1. **Clone the repository:**
   ```powershell
   git clone https://github.com/REPLACE_WITH_REAL_ORG/OSynaptic-FX.git
   cd OSynaptic-FX
   ```

2. **Review project documentation:**
   - Start with `README.md` for project overview
   - Read `DATA_FORMATS_SPEC.md` for protocol specification
   - Check `docs/SUMMARY.md` for comprehensive documentation

3. **Set up local build:**
   ```powershell
   # Using the build script
   powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Compiler auto

   # Validate Arduino example build (adjust FQBN to your target)
   arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicEncode
   ```

4. **Run tests locally:**
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1 -Compiler auto
   ```

## Contribution Workflow

### Suggested Workflow for Contributors

1. **Start with the specification first**
   - Read `DATA_FORMATS_SPEC.md` for protocol/format details
   - Review send API documentation (`SEND_API_REFERENCE.md`)
   - Check related implementation docs in `docs/`

2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   # or
   git checkout -b fix/issue-number
   ```

3. **Implement changes**
   - Update C99 code in `src/` and headers in `include/`
   - Follow existing code style and conventions
   - Add or update comments for complex logic
   - Keep changes focused and atomic

4. **Write or update tests**
   - Add unit tests to `tests/` for new functionality
   - Ensure all existing tests continue to pass
   - Aim for reasonable code coverage

5. **Run quality checks**
   ```powershell
   # Run unit tests
   powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1 -Compiler auto
   
   # Run full matrix testing for protocol-sensitive changes
   powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1 -Matrix
   
   # Run benchmarks if performance-sensitive
   powershell -ExecutionPolicy Bypass -File .\scripts\bench.ps1 -Compiler auto
   ```

6. **Update documentation**
   - Update docstrings and comments in code
   - Update `docs/` for implementation workflow details
   - Update root spec files for protocol changes
   - Keep examples current in quick-start guides

7. **Commit and push**
   ```bash
   git add .
   git commit -m "type: brief description

   - Detailed explanation if needed
   - Reference any related issues: Fixes #123"
   git push origin feature/your-feature-name
   ```

8. **Submit a pull request**
   - Provide a clear title and description
   - Reference any related issues
   - Include test results
   - Be responsive to review feedback

## Coding Standards

### C99 Guidelines

- Follow C99 standard strictly
- Keep functions focused and single-purpose
- Use meaningful variable and function names
- Add header comments for public functions
- Document complex algorithms or non-obvious logic

### Style Conventions

- Use 4-space indentation (not tabs)
- Maximum line length: 100 characters (soft limit)
- Use standard C-style comments: `/* ... */` and `// ...`
- Maintain consistent naming: `snake_case` for functions/variables, `SCREAMING_SNAKE_CASE` for macros

### Header Files

- Include guards: `#ifndef OSFX_MODULE_NAME_H`
- Group related declarations
- Document public APIs with doxygen-style comments
- Keep implementation details out of headers

## Testing Requirements

### Mandatory for All Contributions

1. **Unit tests** - Add tests for new functionality
2. **Existing tests** - Ensure all existing tests pass
3. **Manual testing** - Verify changes work as expected

### For Protocol/Runtime-Sensitive Changes

1. **Matrix testing** - Use `scripts/test.ps1 -Matrix`
2. **Benchmark comparison** - Run before/after benchmarks
3. **Cross-platform validation** - Test on supported platforms

## Documentation

### Documentation Ownership

- **Root `README.md`**: Project-level portal and overview
- **`docs/` folder**: Implementation workflows and module details
- **Root spec files**: Protocol/API normative references
  - `DATA_FORMATS_SPEC.md`
  - `SEND_API_INDEX.md`
  - `SEND_API_REFERENCE.md`
  - `QUICK_START_SEND_EXAMPLES.md`

### Documentation Updates

- Keep documentation synchronized with code changes
- Use clear, concise language
- Include examples where helpful
- Update related documentation at all applicable levels
- Ensure links and references remain current

## Pull Request Process

1. **Before opening:**
   - Rebase on latest main branch
   - Run all tests locally and verify they pass
   - Format and lint your code
   - Review your own changes first

2. **When opening:**
   - Use a descriptive title
   - Provide clear context and motivation
   - Link to related issues
   - Include test results or reproduction steps

3. **During review:**
   - Respond to feedback promptly and courteously
   - Make requested changes in follow-up commits
   - Explain decisions if you disagree with feedback
   - Keep discussions professional and respectful

4. **Before merging:**
   - Ensure all CI checks pass
   - Obtain approval from maintainers
   - Resolve any remaining discussions
   - Clean up unnecessary commits if needed

## Reporting Bugs

When reporting bugs, include:

1. **Environment details**
   - OS and version
   - Compiler and version
   - Arduino IDE/CLI version
   - Board core package and FQBN
   - Any relevant configuration

2. **Reproduction steps**
   - Minimal steps to reproduce
   - Expected behavior
   - Actual behavior

3. **Supporting information**
   - Relevant code snippets
   - Error messages and stack traces
   - Build logs
   - Test outputs

## Feature Requests

When proposing new features:

1. Check if a similar feature already exists
2. Describe the use case and benefits
3. Propose implementation approach if possible
4. Consider impacts on protocol compatibility
5. Discuss performance and size implications

## Release Process

- Version numbering follows semantic versioning
- Major releases may include breaking changes
- Minor releases add backward-compatible features
- Patch releases contain bug fixes only
- Release notes are maintained in `RELEASE_NOTES_v*.*.*.md`

## Questions or Need Help?

- Check existing issues and discussions
- Review documentation in `docs/`
- Ask in project discussions
- Reach out to maintainers if needed

## Code of Conduct

This project adheres to the Code of Conduct. By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

Thank you for contributing to OSynaptic-FX!

