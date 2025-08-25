# Gemini Interpreter

![C](https://img.shields.io/badge/language-C-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

The Gemini Interpreter is a lightweight interpreter for the Gemini programming language, written in C. This project demonstrates the fundamentals of building an interpreter, including a lexer, parser, and virtual machine (VM) for executing code. Gemini supports variables, arithmetic operations, control flow (if, while, for), and basic function declarations.

## Features
- **Lexer**: Converts Gemini source code into tokens.
- **Parser**: Builds an Abstract Syntax Tree (AST) from tokens.
- **Virtual Machine**: Executes the AST with support for variables, expressions, and control flow.
- **Data Types**: Supports integers, floats, strings, and booleans.
- **Control Structures**: Includes if-else, while, and for loops.
- **Example Programs**: Includes simple (`hello.gemini`) and feature-rich (`showcase.gemini`) examples.

## Prerequisites
- **GCC**: A C compiler (or another C11-compatible compiler).
- **Make**: For building the project using the provided Makefile.
- A POSIX-compliant operating system (for functions like `strndup`).

## Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/<username>/gemini-interpreter.git
   cd gemini-interpreter
   ```
2. Build the project:
   ```bash
   make
   ```
3. Run the interpreter with an example file:
   ```bash
   make run
   ```

## Usage
Run the interpreter with a Gemini source file:
```bash
./bin/gemini <file.gemini>
```

Example running `hello.gemini`:
```bash
./bin/gemini examples/hello.gemini
```

Output:
```
Hello, Gemini!
8
```

Example running `showcase.gemini`:
```bash
./bin/gemini examples/showcase.gemini
```

Output:
```
0
1
2
3
4
Done!
0
1
2
```

## Project Structure
```
gemini-interpreter/
├── examples/           # Example Gemini programs
│   ├── hello.gemini
│   └── showcase.gemini
├── include/            # Header files
│   ├── common.h
│   ├── lexer.h
│   ├── parser.h
│   └── vm.h
├── src/                # Source code
│   ├── lexer.c
│   ├── parser.c
│   ├── vm.c
│   └── main.c
├── Makefile            # Build script
└── README.md           # Project documentation
```

## Gemini Syntax
Gemini is a simple C-like language. Here's an example from `showcase.gemini`:
```gemini
var i = 0;
while (i < 5) {
    print(i);
    i = i + 1;
}

if (i == 5) {
    print("Done!");
} else {
    print("Error");
}

for (var j = 0; j < 3; j = j + 1) {
    print(j);
}
```

### Supported Features
- **Variables**: Declare with `var` (e.g., `var x = 10;`).
- **Data Types**: Integers (`10`), floats (`3.14`), strings (`"Hello"`), booleans (from comparisons).
- **Operators**: Arithmetic (`+`, `-`, `*`, `/`, `%`), comparison (`==`, `!=`, `>`, `>=`, `<`, `<=`).
- **Control Flow**: `if`, `while`, `for`.
- **Print**: Outputs values to the console with `print`.

## Development Notes
- **Functions**: Function declarations are supported, but execution is not fully implemented.
- **Memory Management**: The AST and VM are properly cleaned up to prevent memory leaks.
- **Future Extensions**: Potential support for function calls, string concatenation, and complex expressions.

## Contributing
1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/your-feature`).
3. Commit changes with semantic messages (`feat:`, `fix:`, `docs:`, etc.).
4. Push to the branch (`git push origin feature/your-feature`).
5. Open a Pull Request.

## License
Licensed under the [MIT License](LICENSE).


---
Built with passion by @gtkrshnaaa.