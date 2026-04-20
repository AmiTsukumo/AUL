# AUL — A Universal Language
> A modernized Lua-like language. Less noise, safer defaults, same soul.

---

## Philosophy

AUL is Lua, redesigned with one guiding principle:

> **Explicit where it matters. Implicit where it's safe.**

It has roughly the same keyword count as Lua but covers significantly more ground — removing genuine footguns while adding real features, and feeling immediately readable to anyone who has touched Python, Rust, Kotlin, or JavaScript.

---

## Keywords (23 total)

| Keyword | Purpose |
|---|---|
| `var` | Declare a mutable variable |
| `val` | Declare an immutable variable |
| `global` | Opt a variable into global scope |
| `func` | Declare a function |
| `if` | Conditional |
| `elif` | Else-if branch (shorthand — `else if` also works) |
| `else` | Else branch |
| `for` | For loop |
| `while` | While loop |
| `break` | Break out of a loop |
| `continue` | Skip to the next loop iteration |
| `in` | Iterator keyword (used in for-in loops) |
| `and` | Logical and |
| `or` | Logical or |
| `not` | Logical not |
| `true` | Boolean true |
| `false` | Boolean false |
| `none` | Absence of a value (replaces nil) |
| `import` | Import a module |
| `from` | Import specific symbols from a module |
| `block` | Create an explicit scoped block |
| `return` | Return a value from a function (alternative to `~`) |
| `is` | Pattern match / type check |

---

## Variables

Variables are **local by default**. Use `global` to opt into global scope.

```aul
var x = 10         // mutable, local
val y = 20         // immutable, local

global var count = 0    // mutable global
global val MAX = 100    // immutable global
```

Attempting to reassign a `val` is a compile-time error.

---

## Functions

```aul
func add(a, b) {
    a + b ~
}

func greet(name) {
    "Hello, " .. name ~
}
```

The `~` operator at the **end** of an expression returns it from the function. `return` is also valid and does the same thing — use whichever reads better to you.

```aul
func add(a, b) {
    return a + b   // also valid
}
```

For functions that return nothing, simply omit `~`:

```aul
func logMessage(msg) {
    print(msg)
}
```

---

## Control Flow

### if / elif / else

```aul
if x > 10 {
    print("big")
} elif x == 5 {
    print("five")
} else {
    print("small")
}
```

### Inline unwrap with option types

If a variable is an option type, an `if` block automatically unwraps it:

```aul
if name {
    print(name)   // name is guaranteed non-none here
} else {
    print("no name")
}
```

---

## Loops

### while

```aul
while condition {
    // ...
}
```

### for (numeric)

0-based. The range `0, 10` iterates `0` through `9`.

```aul
for i = 0, 10 {
    print(i)
}

// with step
for i = 0, 10, 2 {
    print(i)   // 0, 2, 4, 6, 8
}
```

Or using `range()` like Python:

```aul
for i in range(10) {        // 0 through 9
    print(i)
}

for i in range(2, 10) {     // 2 through 9
    print(i)
}

for i in range(0, 10, 2) {  // with step
    print(i)
}
```

### for-in (iterator)

Use `entries()` to iterate a table with both key and value, or just iterate values directly:

```aul
for k, v in entries(t) {    // key-value pairs (replaces pairs)
    print(k, v)
}

for v in t {                // values only
    print(v)
}
```

### break and continue

```aul
for i = 0, 10 {
    if i == 3 { continue }
    if i == 7 { break }
    print(i)
}
```

---

## Option Types

AUL has no `nil`. Instead, values that may be absent use **option types**.

```aul
var name: String? = "Alice"   // has a value
var empty: String? = none     // no value
```

The `?` suffix on a type marks it as optional. A non-optional type is **always guaranteed to have a value** — no null checks needed.

### Checking for none

```aul
if name {
    print(name)
} else {
    print("nothing here")
}
```

### Pattern matching with is

```aul
if name is String(val n) {
    "Name is: " .. n ~
} else {
    "none" ~
}
```

### Table lookups

Missing keys in a table return `none` automatically instead of crashing:

```aul
var t = { foo = "bar" }
var x = t["missing"]   // x is none, not a crash
```

---

## Scoped Blocks

Use `block` to create an explicit scope for temporary variables:

```aul
block {
    var temp = computeSomething()
    print(temp)
    // temp is gone after this closing brace
}
```

---

## Imports

```aul
import math
import os.path

from strings import upper, lower
from mymodule import MyThing
```

---

## Indexing

AUL uses **0-based indexing**.

```aul
var arr = {10, 20, 30}
arr[0]   // 10
arr[1]   // 20
arr[2]   // 30
```

---

## Operators

### Arithmetic

| Operator | Meaning |
|---|---|
| `+` | Addition |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |
| `%` | Modulo |
| `^` | Exponentiation |
| `..` | String concatenation |

### Comparison

| Operator | Meaning |
|---|---|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |

### Logical / Bitwise

Both words and symbols work — use whichever fits. They do the same thing on both booleans and integers.

| Word | Symbol | Meaning |
|---|---|---|
| `and` | `&` | AND |
| `or` | `\|\|` | OR |
| `not` | `!` | NOT |

```aul
true and false   // false  (boolean)
5 & 3            // 1      (bitwise AND)
true || false    // true   (boolean)
5 or 3           // 7      (bitwise OR)
!true            // false  (boolean)
not 5            // -6     (bitwise NOT)
```

Mix and match freely — `x & y`, `x and y`, `!x`, `not x` are all valid.

---

## Comments

```aul
// single line comment

/*
    multi
    line
    comment
*/
```

---

## What Was Removed from Lua

| Removed | Reason |
|---|---|
| `nil` | Replaced by option types — absence is always explicit |
| `end` / `then` | Replaced by `{}` |
| `do` in loops | Redundant with `{}` |
| `repeat` / `until` | Use `while` instead |
| `elseif` | Replaced by `elif` |
| `function` | Replaced by `func` |
| `local` | Locality is now the default |
| `require` | Replaced by `import` |
| `goto` / `::labels::` | Removed entirely, no replacement |

---

## Full Example

```aul
import math
from strings import upper

global val VERSION = "1.0.0"

func factorial(n) {
    if n == 0 {
        1 ~
    }
    n * factorial(n - 1) ~
}

func greetAll(names: String?[]) {
    for i, name in entries(names) {
        if name {
            print("Hello, " .. upper(name))
        } else {
            print("Hello, stranger")
        }
    }
}

var results = {}
for i = 0, 10 {
    if i % 2 == 0 { continue }
    results[i] = factorial(i)
}
```

---

*AUL — because Lua deserved a second draft.*
