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
├── bin/                # Contains the compiled executable
├── obj/                # Stores intermediate object files
├── src/                # C source files (.c)
│   ├── lexer.c
│   ├── parser.c
│   ├── vm.c
│   └── main.c
├── include/            # Header files (.h)
│   ├── common.h
│   ├── lexer.h
│   ├── parser.h
│   └── vm.h
├── examples/           # Gemini language script examples
│   ├── test.gemini
│   └── modularity/
├── Makefile            # Build script
└── README.md           # Project documentation
```


## Complex Code Gemini Language Interpreter Can Run

`examples/test.gemini`

```c

// ===============================================
// GEMINI LANGUAGE COMPLETE FEATURE DEMONSTRATION
// File: test.gemini
// ===============================================

print("=== GEMINI LANGUAGE FEATURE DEMO ===");
print("");

// ===============================================
// 1. VARIABLE DECLARATIONS AND ASSIGNMENTS
// ===============================================
print("1. Variables and Data Types:");

// Integer variables
var age = 25;
var score = 100;
print("Integer: age = " + age + ", score = " + score);

// Float variables (if supported)
var pi = 3.14159;
var temperature = 98.6;
print("Float: pi = " + pi + ", temperature = " + temperature);

// String variables
var name = "Gemini User";
var greeting = "Hello";
var language = "Gemini";
print("String: name = " + name);

// Boolean operations (through comparisons)
var isAdult = age >= 18;
print("Boolean expression: isAdult = " + isAdult);

// Variable reassignment
age = age + 1;
print("After increment: age = " + age);

print("");

// ===============================================
// 2. ARITHMETIC OPERATIONS
// ===============================================
print("2. Arithmetic Operations:");

var a = 15;
var b = 4;

print("a = " + a + ", b = " + b);
print("Addition: a + b = " + (a + b));
print("Subtraction: a - b = " + (a - b));
print("Multiplication: a * b = " + (a * b));
print("Division: a / b = " + (a / b));
print("Modulo: a % b = " + (a % b));

// Unary operations
print("Negation: -a = " + (-a));

print("");

// ===============================================
// 3. COMPARISON OPERATIONS
// ===============================================
print("3. Comparison Operations:");

var x = 10;
var y = 20;

print("x = " + x + ", y = " + y);
print("Equal: x == y is " + (x == y));
print("Not equal: x != y is " + (x != y));
print("Greater than: x > y is " + (x > y));
print("Greater or equal: x >= y is " + (x >= y));
print("Less than: x < y is " + (x < y));
print("Less or equal: x <= y is " + (x <= y));

print("");

// ===============================================
// 4. STRING OPERATIONS
// ===============================================
print("4. String Operations:");

var firstName = "John";
var lastName = "Doe";
var fullName = firstName + " " + lastName;
print("String concatenation: " + fullName);

var message = "Welcome to " + language + " programming!";
print("Complex concatenation: " + message);

// String with numbers
var info = "User " + firstName + " is " + age + " years old";
print("Mixed concatenation: " + info);

print("");

// ===============================================
// 5. CONDITIONAL STATEMENTS (IF-ELSE)
// ===============================================
print("5. Conditional Statements:");

var number = 42;
print("Testing number: " + number);

if (number > 50) {
    print("Number is greater than 50");
} else {
    print("Number is not greater than 50");
}

// Nested conditions
var grade = 85;
print("Grade: " + grade);

if (grade >= 90) {
    print("Grade A - Excellent!");
} else {
    if (grade >= 80) {
        print("Grade B - Good job!");
    } else {
        if (grade >= 70) {
            print("Grade C - Passing");
        } else {
            print("Grade F - Need improvement");
        }
    }
}

// Multiple conditions
var weather = 75;
if (weather >= 80) {
    print("It's hot outside!");
} else {
    if (weather >= 60) {
        print("It's nice weather!");
    } else {
        print("It's cold outside!");
    }
}

print("");

// ===============================================
// 6. LOOP STATEMENTS
// ===============================================
print("6. Loop Statements:");

// While loop
print("While loop (counting 1 to 5):");
var counter = 1;
while (counter <= 5) {
    print("Count: " + counter);
    counter = counter + 1;
}

// For loop
print("For loop (counting 0 to 4):");
for (var i = 0; i < 5; i = i + 1) {
    print("Index: " + i);
}

// Nested loops
print("Nested loops (multiplication table 2x3):");
for (var row = 1; row <= 2; row = row + 1) {
    for (var col = 1; col <= 3; col = col + 1) {
        var product = row * col;
        print("Row " + row + " x Col " + col + " = " + product);
    }
}

print("");

// ===============================================
// 7. FUNCTION DEFINITIONS AND CALLS
// ===============================================
print("7. Functions:");

// Simple function with no parameters
function sayHello() {
    print("Hello from a function!");
}

print("Calling sayHello():");
sayHello();

// Function with parameters
function greetPerson(personName, personAge) {
    print("Hello, " + personName + "! You are " + personAge + " years old.");
}

print("Calling greetPerson():");
greetPerson("Alice", 30);
greetPerson("Bob", 25);

// Function with return value
function add(num1, num2) {
    return num1 + num2;
}

print("Function with return value:");
var sum1 = add(5, 3);
var sum2 = add(10, 20);
print("add(5, 3) = " + sum1);
print("add(10, 20) = " + sum2);

// Function calling another function
function multiply(a, b) {
    return a * b;
}

function calculateArea(width, height) {
    var area = multiply(width, height);
    return area;
}

var rectangleArea = calculateArea(5, 8);
print("Rectangle area (5 x 8) = " + rectangleArea);

print("");

// ===============================================
// 8. RECURSIVE FUNCTIONS
// ===============================================
print("8. Recursive Functions:");

// Factorial function
function factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

print("Factorial calculations:");
print("factorial(5) = " + factorial(5));
print("factorial(6) = " + factorial(6));
print("factorial(0) = " + factorial(0));

// Fibonacci function
function fibonacci(n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

print("Fibonacci sequence:");
for (var fib = 0; fib <= 8; fib = fib + 1) {
    print("fibonacci(" + fib + ") = " + fibonacci(fib));
}

// Recursive countdown
function countdown(num) {
    print("Countdown: " + num);
    if (num > 0) {
        countdown(num - 1);
    }
    return num;
}

print("Recursive countdown from 5:");
countdown(5);

print("");

// ===============================================
// 9. COMPLEX EXPRESSIONS AND CALCULATIONS
// ===============================================
print("9. Complex Expressions:");

// Mathematical expressions
var result1 = (5 + 3) * 2 - 4;
print("(5 + 3) * 2 - 4 = " + result1);

var result2 = 10 / 2 + 3 * 4;
print("10 / 2 + 3 * 4 = " + result2);

// Boolean expressions
var complexCondition = (age >= 18) == (score > 50);
print("Complex boolean: (age >= 18) == (score > 50) is " + complexCondition);

// String and number combination
var mixedResult = "Result: " + (100 - 25) + " points";
print("Mixed expression: " + mixedResult);

print("");

// ===============================================
// 10. PRACTICAL EXAMPLES
// ===============================================
print("10. Practical Examples:");

// Grade calculator function
function calculateGrade(points, total) {
    var percentage = (points * 100) / total;
    if (percentage >= 90) {
        return "A";
    } else {
        if (percentage >= 80) {
            return "B";
        } else {
            if (percentage >= 70) {
                return "C";
            } else {
                if (percentage >= 60) {
                    return "D";
                } else {
                    return "F";
                }
            }
        }
    }
}

print("Grade Calculator:");
print("85/100 points = Grade " + calculateGrade(85, 100));
print("92/100 points = Grade " + calculateGrade(92, 100));
print("67/100 points = Grade " + calculateGrade(67, 100));

// Simple interest calculator
function simpleInterest(principal, rate, time) {
    return (principal * rate * time) / 100;
}

print("Simple Interest Calculator:");
var principal = 1000;
var rate = 5;
var time = 3;
var interest = simpleInterest(principal, rate, time);
print("Principal: $" + principal + ", Rate: " + rate + "%, Time: " + time + " years");
print("Simple Interest: $" + interest);
print("Total Amount: $" + (principal + interest));

// Number guessing game logic
function checkGuess(guess, target) {
    if (guess == target) {
        return "Correct! You guessed it!";
    } else {
        if (guess < target) {
            return "Too low! Try higher.";
        } else {
            return "Too high! Try lower.";
        }
    }
}

print("Number Guessing Game Simulation:");
var targetNumber = 7;
print("Target number is: " + targetNumber);
print("Guess 5: " + checkGuess(5, targetNumber));
print("Guess 9: " + checkGuess(9, targetNumber));
print("Guess 7: " + checkGuess(7, targetNumber));

print("");

// ===============================================
// 11. ADVANCED FUNCTION PATTERNS
// ===============================================
print("11. Advanced Function Patterns:");

// Function that returns different types
function processNumber(num) {
    if (num < 0) {
        return "Negative number";
    } else {
        if (num == 0) {
            return "Zero";
        } else {
            return num * 2;
        }
    }
}

print("Process Number Function:");
print("processNumber(-5) = " + processNumber(-5));
print("processNumber(0) = " + processNumber(0));
print("processNumber(3) = " + processNumber(3));

// Utility functions
function isEven(number) {
    return (number % 2) == 0;
}

function isOdd(number) {
    return (number % 2) != 0;
}

print("Even/Odd Checker:");
for (var check = 1; check <= 6; check = check + 1) {
    if (isEven(check)) {
        print(check + " is even");
    } else {
        print(check + " is odd");
    }
}

// Maximum of three numbers
function max3(a, b, c) {
    var maxVal = a;
    if (b > maxVal) {
        maxVal = b;
    }
    if (c > maxVal) {
        maxVal = c;
    }
    return maxVal;
}

print("Maximum of three numbers:");
print("max3(5, 12, 8) = " + max3(5, 12, 8));
print("max3(15, 3, 9) = " + max3(15, 3, 9));

print("");

// ===============================================
// 12. SCOPE AND VARIABLE MANAGEMENT
// ===============================================
print("12. Variable Scope Examples:");

var globalVar = "I'm global";

function testScope(param) {
    var localVar = "I'm local";
    print("Inside function: " + globalVar);
    print("Inside function: " + localVar);
    print("Parameter: " + param);
    return localVar + " " + param;
}

print("Testing variable scope:");
var scopeResult = testScope("parameter");
print("Function returned: " + scopeResult);
print("Outside function: " + globalVar);

// Variable shadowing example
var shadowed = "original";
function testShadowing() {
    var shadowed = "shadowed";
    print("Inside function: " + shadowed);
    return shadowed;
}

print("Variable shadowing:");
print("Before function: " + shadowed);
testShadowing();
print("After function: " + shadowed);

print("");

// ===============================================
// 13. ERROR CONDITIONS AND EDGE CASES
// ===============================================
print("13. Edge Cases:");

// Division by zero (should be handled by VM)
print("Testing edge cases:");
var divResult = 10 / 1;
print("10 / 1 = " + divResult);

// Large numbers
var largeNum1 = 999999;
var largeNum2 = 1000001;
print("Large number arithmetic: " + largeNum1 + " + " + largeNum2 + " = " + (largeNum1 + largeNum2));

// Empty strings
var emptyStr = "";
var nonEmptyStr = "Hello";
var combined = emptyStr + nonEmptyStr + emptyStr;
print("String with empty: '" + combined + "'");

print("");

// ===============================================
// 14. PERFORMANCE AND COMPLEXITY TESTS
// ===============================================
print("14. Performance Tests:");

// Iterative vs recursive comparison
function factorialIterative(n) {
    var result = 1;
    for (var i = 1; i <= n; i = i + 1) {
        result = result * i;
    }
    return result;
}

print("Factorial comparison:");
var n = 7;
print("Recursive factorial(" + n + ") = " + factorial(n));
print("Iterative factorial(" + n + ") = " + factorialIterative(n));

// Loop performance
function sumNumbers(limit) {
    var total = 0;
    for (var i = 1; i <= limit; i = i + 1) {
        total = total + i;
    }
    return total;
}

print("Sum of numbers 1 to 100: " + sumNumbers(100));

print("");

// ===============================================
// FINAL MESSAGE
// ===============================================
print("=== END OF GEMINI FEATURE DEMONSTRATION ===");
print("All major features have been tested:");
print("✓ Variables and data types");
print("✓ Arithmetic operations");
print("✓ Comparison operations"); 
print("✓ String operations");
print("✓ Conditional statements");
print("✓ Loop statements");
print("✓ Function definitions");
print("✓ Recursive functions");
print("✓ Complex expressions");
print("✓ Practical examples");
print("✓ Advanced patterns");
print("✓ Variable scope");
print("✓ Edge cases");
print("✓ Performance tests");
print("");
print("Gemini language interpreter working successfully!");

```

### Output

By running `make run` command.

```
./bin/gemini examples/test.gemini
Tokenized 2118 tokens successfully.
=== GEMINI LANGUAGE FEATURE DEMO ===

1. Variables and Data Types:
Integer: age = 25, score = 100
Float: pi = 3.14159, temperature = 98.6
String: name = Gemini User
Boolean expression: isAdult = true
After increment: age = 26

2. Arithmetic Operations:
a = 15, b = 4
Addition: a + b = 19
Subtraction: a - b = 11
Multiplication: a * b = 60
Division: a / b = 3
Modulo: a % b = 3
Negation: -a = -15

3. Comparison Operations:
x = 10, y = 20
Equal: x == y is false
Not equal: x != y is true
Greater than: x > y is false
Greater or equal: x >= y is false
Less than: x < y is true
Less or equal: x <= y is true

4. String Operations:
String concatenation: John Doe
Complex concatenation: Welcome to Gemini programming!
Mixed concatenation: User John is 26 years old

5. Conditional Statements:
Testing number: 42
Number is not greater than 50
Grade: 85
Grade B - Good job!
It's nice weather!

6. Loop Statements:
While loop (counting 1 to 5):
Count: 1
Count: 2
Count: 3
Count: 4
Count: 5
For loop (counting 0 to 4):
Index: 0
Index: 1
Index: 2
Index: 3
Index: 4
Nested loops (multiplication table 2x3):
Row 1 x Col 1 = 1
Row 1 x Col 2 = 2
Row 1 x Col 3 = 3
Row 2 x Col 1 = 2
Row 2 x Col 2 = 4
Row 2 x Col 3 = 6

7. Functions:
Calling sayHello():
Hello from a function!
Calling greetPerson():
Hello, Alice! You are 30 years old.
Hello, Bob! You are 25 years old.
Function with return value:
add(5, 3) = 8
add(10, 20) = 30
Rectangle area (5 x 8) = 40

8. Recursive Functions:
Factorial calculations:
factorial(5) = 120
factorial(6) = 720
factorial(0) = 1
Fibonacci sequence:
fibonacci(0) = 0
fibonacci(1) = 1
fibonacci(2) = 1
fibonacci(3) = 2
fibonacci(4) = 3
fibonacci(5) = 5
fibonacci(6) = 8
fibonacci(7) = 13
fibonacci(8) = 21
Recursive countdown from 5:
Countdown: 5
Countdown: 4
Countdown: 3
Countdown: 2
Countdown: 1
Countdown: 0

9. Complex Expressions:
(5 + 3) * 2 - 4 = 12
10 / 2 + 3 * 4 = 17
Complex boolean: (age >= 18) == (score > 50) is true
Mixed expression: Result: 75 points

10. Practical Examples:
Grade Calculator:
85/100 points = Grade B
92/100 points = Grade A
67/100 points = Grade D
Simple Interest Calculator:
Principal: $1000, Rate: 5%, Time: 3 years
Simple Interest: $150
Total Amount: $1150
Number Guessing Game Simulation:
Target number is: 7
Guess 5: Too low! Try higher.
Guess 9: Too high! Try lower.
Guess 7: Correct! You guessed it!

11. Advanced Function Patterns:
Process Number Function:
processNumber(-5) = Negative number
processNumber(0) = Zero
processNumber(3) = 6
Even/Odd Checker:
1 is odd
2 is even
3 is odd
4 is even
5 is odd
6 is even
Maximum of three numbers:
max3(5, 12, 8) = 12
max3(15, 3, 9) = 15

12. Variable Scope Examples:
Testing variable scope:
Inside function: I'm global
Inside function: I'm local
Parameter: parameter
Function returned: I'm local parameter
Outside function: I'm global
Variable shadowing:
Before function: original
Inside function: shadowed
After function: original

13. Edge Cases:
Testing edge cases:
10 / 1 = 10
Large number arithmetic: 999999 + 1000001 = 2000000
String with empty: 'Hello'

14. Performance Tests:
Factorial comparison:
Recursive factorial(7) = 5040
Iterative factorial(7) = 5040
Sum of numbers 1 to 100: 5050

=== END OF GEMINI FEATURE DEMONSTRATION ===
All major features have been tested:
✓ Variables and data types
✓ Arithmetic operations
✓ Comparison operations
✓ String operations
✓ Conditional statements
✓ Loop statements
✓ Function definitions
✓ Recursive functions
✓ Complex expressions
✓ Practical examples
✓ Advanced patterns
✓ Variable scope
✓ Edge cases
✓ Performance tests

Gemini language interpreter working successfully!


```

## Author

**Gtkrshnaaa**