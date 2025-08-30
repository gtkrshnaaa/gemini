# Gemini Language Interpreter

![C](https://img.shields.io/badge/language-C-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)


**A simple, dynamic, C-style scripting language interpreter built from scratch in C.**

### Table of Contents

  * [About The Project](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#about-the-project)
  * [Features](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#features)
  * [Getting Started](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#getting-started)
      * [Prerequisites](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#prerequisites)
      * [Build Instructions](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#build-instructions)
  * [Usage](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#usage)
  * [Language Syntax Showcase](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#language-syntax-showcase)
  * [Interpreter Architecture](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#interpreter-architecture)
  * [Project Structure](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#project-structure)
  * [Complex Code Demonstration](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#complex-code-gemini-language-interpreter-can-run)
  * [Author](https://github.com/gtkrshnaaa/gemini?tab=readme-ov-file#author)

-----

## About The Project

Gemini is a tree-walk interpreter for a dynamic, C-style scripting language, written entirely in C with zero external dependencies. The project was developed as a comprehensive exercise in understanding the fundamentals of how programming languages are designed and implemented.

Every component, from the lexical analyzer (Lexer) to the Abstract Syntax Tree (AST) parser and the Virtual Machine (VM) that executes the code, has been built from the ground up. This provides a clear and concise case study of the entire interpretation pipeline.

## Features

This section details the core features of the Gemini programming language.

**General**

  - **C-style Syntax:** An intuitive and familiar syntax for developers experienced with C, Java, JavaScript, or similar languages.
  - **Dynamic Typing:** Variables are not bound to a specific type. The interpreter handles type management at runtime for Integers, Floats, Strings, and Booleans (derived from comparisons).
  - **Single-Line Comments:** Use `//` to add comments to your code, which will be ignored by the interpreter.

**Variables and Scope**

  - **Variable Declaration:** Declare variables using the `var` keyword (e.g., `var x = 10;`).
  - **Reassignment:** Variables can be reassigned to new values of any type after declaration.
  - **Lexical Scoping:** The language supports both global scope and local (function-level) scope. Variables declared inside a function are not accessible from the outside.
  - **Block Statements:** Use curly braces `{ ... }` to group multiple statements into a single block, creating a new scope context for control flow structures.

**Operators**

  - **Full Arithmetic Operators:** Includes addition (`+`), subtraction (`-`), multiplication (`*`), division (`/`), and modulo (`%`).
  - **Unary Negation:** Supports the unary minus operator to negate numeric values (e.g., `-x`).
  - **Complete Comparison Operators:** Provides `==` (equal), `!=` (not equal), `>` (greater than), `<` (less than), `>=` (greater or equal), and `<=` (less or equal).
  - **String Concatenation:** The `+` operator is overloaded to concatenate strings.
  - **Automatic Type Coercion:** Non-string types are automatically converted to strings during concatenation operations (e.g., `"Age: " + 25`).

**Functions**

  - **Function Definition:** Declare functions using the `function` keyword, followed by a name, parameters, and a body.
  - **Parameters and Arguments:** Functions can accept multiple arguments, which are passed by value.
  - **Return Values:** Functions can return a value using the `return` statement. If no value is returned, a default of `0` (integer) is provided.
  - **Recursion:** The call stack architecture fully supports recursive function calls, allowing for elegant solutions to problems like factorial or Fibonacci sequences.

**Modules and Imports**

  - **Module Import:** Import modules using `import <name> as <alias>;`.
  - **Module Cache:** Repeated imports of the same logical module are cached to avoid re-parsing.
  - **GEMINI_PATH Resolution:** Set `GEMINI_PATH` (colon-separated list of directories) to prioritize module lookup before falling back to the project root.
  - **Examples:** See `examples/modularity/` and run `make modularity-run`.

**Arrays and Maps**

  - **Dynamic Arrays:** Create with `array()`, index with `arr[i]`, and assign with `arr[i] = value`.
  - **Hash Maps:** Create with `map()`, key access via `m[key]` and `m[key] = value` for set/update.
  - **Built-ins:**
    - Arrays: `push(arr, value)`, `pop(arr)`, `length(arr)`
    - Maps: `has(map, key)`, `delete(map, key)`, `keys(map)`, `length(map)`
  - **Truthiness:** Arrays/Maps are truthy when non-empty (length > 0).
  - **Equality:** `==`/`!=` use identity (pointer), not deep equality.
  - **String Concatenation:** Concatenation renders compact descriptors, e.g., array as `[array length=N]` and map as `{map size=N}`.

## Getting Started

Follow these instructions to get a local copy up and running.

### Prerequisites

Ensure you have a standard C development environment. This project relies on:

  * `gcc` (GNU Compiler Collection)
  * `make`
  * `git`

The project has been tested on Linux-based systems.

### Build Instructions

1.  **Clone the repository:**

    ```sh
    git clone https://github.com/gtkrshnaaa/gemini.git
    ```

2.  **Navigate to the project directory:**

    ```sh
    cd gemini
    ```

3.  **Compile the project using the Makefile:**

    ```sh
    make all
    ```

    This command will compile all source files, placing object files in the `obj/` directory and the final `gemini` executable in the `bin/` directory.

## Usage

The interpreter is a command-line application that takes the path to a Gemini script file as an argument.

**General Syntax:**

```sh
./bin/gemini [path_to_script.gemini]
```

**Example:**
To run the comprehensive demonstration script included in the repository, you can use the `run` target in the Makefile for convenience:

```sh
make run
```

This is equivalent to running:

```sh
./bin/gemini examples/test.gemini
```

**Modularity Example:**

Run the modularity demo:

```sh
make modularity-run
```

Optionally, specify additional module search paths via `GEMINI_PATH` (colon-separated):

```sh
export GEMINI_PATH=/path/to/modules:/another/path
make modularity-run

**Arrays/Maps Example:**

Run the arrays/maps demo:

```sh
make arrays-maps-run
```
Script: `examples/arrays_maps.gemini`
```

## Language Syntax Showcase

Here are some code snippets demonstrating key Gemini language features.

#### Variables and Expressions

```c
// Declare a variable and print its value.
var message = "Hello from Gemini!";
print(message);

// Variables can be reassigned.
var score = 90;
score = score + 10; // score is now 100
print("Final score: " + score); // Automatic type coercion
```

#### Functions and Recursion

```c
// A function to calculate the factorial of a number.
function factorial(n) {
    // Base case for the recursion.
    if (n <= 1) {
        return 1;
    }
    // Recursive step.
    return n * factorial(n - 1);
}

var result = factorial(6);
print("Factorial of 6 is: " + result); // Prints: 720
```

#### Scope Management

```c
var globalVar = "I am global.";

function testScope() {
    var localVar = "I am local.";
    print("Inside function: " + globalVar); // Can access global
    print("Inside function: " + localVar);
}

testScope();
print("Outside function: " + globalVar);
// The following line would cause an error, as localVar is out of scope.
// print(localVar); 
```

## Interpreter Architecture

The Gemini interpreter follows a classic three-stage pipeline:

`Source Code (.gemini)` -> `[ Lexer ]` -> `Tokens` -> `[ Parser ]` -> `AST` -> `[ Interpreter ]` -> `Output`

1.  **Lexer (Scanner) - `lexer.c`**
    This stage performs lexical analysis, breaking down the raw source code into a stream of fundamental units called **tokens**.

2.  **Parser - `parser.c`**
    The parser consumes the token stream and constructs a tree-like data structure known as an **Abstract Syntax Tree (AST)**. The AST represents the hierarchical structure and logical flow of the code.

3.  **Interpreter (VM) - `vm.c`**
    This is the execution engine. It is a **tree-walking interpreter** that recursively traverses the AST. At each node, it evaluates expressions, executes statements, manages variable environments (scopes), and handles the function call stack.

## Project Structure

```
.
├── bin/                                # Compiled executable(s)
├── obj/                                # Intermediate object files
├── src/                                # C source files (.c)
│   ├── lexer.c
│   ├── parser.c
│   ├── vm.c
│   └── main.c
├── include/                            # Header files (.h)
│   ├── common.h
│   ├── lexer.h
│   ├── parser.h
│   └── vm.h
├── examples/                           # Gemini language script examples
│   ├── test.gemini
│   ├── arrays_maps.gemini
│   └── modularity/
│       ├── main.gemini
│       └── utils/
│           ├── geometry.gemini
│           ├── math.gemini
│           └── stats.gemini
├── Makefile                            # Build and run targets
├── LICENSE                             # MIT License
└── README.md                           # Project documentation
```


## Complex Code Gemini Language Interpreter Can Run

### Project Codebase

`examples/project_test/main.gemini`

```c
// Real-world style demo project for Gemini
// This project showcases: variables, control flow, functions, recursion,
// arrays, maps, and modularity via imports.

import inventory as inv;
import report as rpt;
import math as m;

print("=== PROJECT TEST: STORE INVENTORY DASHBOARD ===");
print("");

// Basic variables and expressions
var storeName = "Gemini Mart";
var openHour = 9;
var closeHour = 21;
print("Store: " + storeName);
print("Open hours: " + openHour + " to " + closeHour);
print("");

// Build initial inventory using a map
var stock = inv.createInventory();
inv.addItem(stock, "apple", 10);
inv.addItem(stock, "banana", 5);
inv.addItem(stock, "chocolate", 12);

// Arrays: incoming shipments
var shipments = array();
push(shipments, 3); // apples
push(shipments, 5); // bananas
push(shipments, 2); // chocolates

// Update stock from shipments using a loop
var items = inv.itemKeys(); // ["apple", "banana", "chocolate"]
var idx = 0;
while (idx < length(items)) {
    var name = items[idx];
    var qty = shipments[idx];
    inv.addItem(stock, name, qty);
    idx = idx + 1;
}
print("");

// Use functions and conditionals
var total = inv.totalItems(stock);
if (total >= 30) {
    print("Stock level: healthy");
} else {
    print("Stock level: low");
}
print("");

// Use math module utilities
var dailySales = array();
push(dailySales, 7);
push(dailySales, 11);
push(dailySales, 5);
push(dailySales, 9);

print("Sales sum: " + m.sum(dailySales));
print("Sales avg: " + m.avg(dailySales));
print("");

// Recursion demo
print("factorial(5) = " + m.factorial(5));
print("");

// Reporting via separate module
rpt.printSummary(storeName, stock);
print("");

// Small analytics: build a frequency map from a sales array
print("-- Sales frequency (units sold per value) --");
var freq = inv.frequency(dailySales);
var fkeys = keys(freq);
var i = 0;
while (i < length(fkeys)) {
    var k = fkeys[i];
    print(k + " -> " + freq[k]);
    i = i + 1;
}
print("");

print("=== END OF PROJECT TEST ===");

```

`examples/project_test/utils/inventory.gemini`

```c
// Inventory utilities module

function createInventory() {
    return map();
}

function addItem(inv, name, qty) {
    if (has(inv, name)) {
        inv[name] = inv[name] + qty;
    } else {
        inv[name] = qty;
    }
    return inv[name];
}

function totalItems(inv) {
    var ks = keys(inv);
    var i = 0;
    var total = 0;
    while (i < length(ks)) {
        var k = ks[i];
        total = total + inv[k];
        i = i + 1;
    }
    return total;
}

// Predefined item keys to align with shipments in project demo
function itemKeys() {
    var arr = array();
    push(arr, "apple");
    push(arr, "banana");
    push(arr, "chocolate");
    return arr;
}

// Build frequency map from a numeric array
function frequency(arr) {
    var freq = map();
    var i = 0;
    while (i < length(arr)) {
        var v = arr[i];
        // Store keys as strings to align with keys(map) return values
        var key = "" + v;
        if (has(freq, key)) {
            freq[key] = freq[key] + 1;
        } else {
            freq[key] = 1;
        }
        i = i + 1;
    }
    return freq;
}

```

`examples/project_test/utils/math.gemini`

```c
// Math utilities module

function sum(arr) {
    var s = 0;
    var i = 0;
    while (i < length(arr)) {
        s = s + arr[i];
        i = i + 1;
    }
    return s;
}

function avg(arr) {
    var n = length(arr);
    if (n == 0) {
        return 0;
    }
    return sum(arr) / n;
}

function factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

```

`examples/project_test/utils/report.gemini`

```c
// Reporting utilities module

import inventory as inv;

function printSummary(storeName, stock) {
    print("=== REPORT: " + storeName + " ===");
    print("");
    var total = inv.totalItems(stock);
    print("Total items in stock: " + total);
    print("");

    var ks = keys(stock);
    print("Item breakdown (name -> qty):");
    var i = 0;
    while (i < length(ks)) {
        var k = ks[i];
        print("- " + k + " -> " + stock[k]);
        i = i + 1;
    }
    print("");
}

```

### Project Output

```sh
./bin/gemini examples/project_test/main.gemini
Tokenized 403 tokens successfully.
=== PROJECT TEST: STORE INVENTORY DASHBOARD ===

Store: Gemini Mart
Open hours: 9 to 21


Stock level: healthy

Sales sum: 32
Sales avg: 8

factorial(5) = 120

=== REPORT: Gemini Mart ===

Total items in stock: 37

Item breakdown (name -> qty):
- banana -> 10
- chocolate -> 14
- apple -> 13


-- Sales frequency (units sold per value) --
9 -> 1
5 -> 1
11 -> 1
7 -> 1

=== END OF PROJECT TEST ===
```

---

## Author

**Gtkrshnaaa**