# AUL — A Universal Language

> A modernized Lua-like language. Less noise, safer defaults, same soul.

AUL is a programming language inspired by Lua, redesigned with modern sensibilities. It maintains Lua's simplicity and flexibility while introducing safer defaults, explicit syntax where it matters, and implicit behavior where it's safe.

## Philosophy

**Explicit where it matters. Implicit where it's safe.**

AUL has roughly the same keyword count as Lua but covers significantly more ground — removing genuine footguns while adding real features, and feeling immediately readable to anyone who has touched Python, Rust, Kotlin, or JavaScript.

## Key Features

- **Local by default**: Variables are local unless explicitly declared global.
- **Immutable variables**: Use `val` for immutable declarations.
- **Option types**: No `nil` — use option types for values that may be absent.
- **0-based indexing**: Consistent with most modern languages.
- **Modern syntax**: Uses `{}` instead of `end`, `func` instead of `function`.
- **Pattern matching**: With the `is` keyword.
- **Safe table lookups**: Missing keys return `none` instead of crashing.

## Quick Example

```aul
import math
from strings import upper

func factorial(n) {
    if n == 0 {
        1 ~
    }
    n * factorial(n - 1) ~
}

func greet(name: String?) {
    if name {
        "Hello, " .. upper(name) ~
    } else {
        "Hello, stranger" ~
    }
}

var result = factorial(5)
print(greet("alice"))  // Output: Hello, ALICE
```

## Getting Started

This repository contains the language specification for AUL. For more details, see [AUL_spec.md](AUL_spec.md).

### Building/Running AUL

```bash
mkdir build && cd build
cmake ..
make
./aul example.aul
```

The interpreter supports:
- **File mode**: `./aul example.aul` loads and executes an AUL file
- **Interactive REPL**: Running without arguments starts an interactive session
- **Built-in functions**: `print`, `range`, `entries`, `len`, `push`

## Contributing

Contributions are welcome! Please read the specification and feel free to open issues or pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

*AUL — because Lua deserved a second draft.*