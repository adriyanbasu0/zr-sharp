# ZR# Programming Language

ZR# is a modern programming language that combines the best features from Rust, C#, and C++. It focuses on memory safety, performance, and developer ergonomics.

## Features

- Strong static typing with type inference
- Memory safety checks (inspired by Rust)
- Modern syntax combining features from C#, Rust, and C++
- Basic arithmetic operations (+, -, *, /)
- Comparison operators (>, <, ==)
- Conditional statements (if-else)
- Print statements
- Block-scoped variables
- Single-line comments (//)

## Installation

### Prerequisites

- GCC compiler
- Bash shell (for Unix-like systems)
- Git (for cloning the repository)

### Steps

1. Clone the repository:
```bash
git clone https://github.com/adriyanbasu0/zr-sharp.git
cd zr-sharp
```

2. Make the compiler script executable:
```bash
chmod +x z
```

3. (Optional) Add ZR# to your PATH:
```bash
echo 'export PATH="$PATH:$(pwd)"' >> ~/.bashrc  # For Bash
# OR
echo 'export PATH="$PATH:$(pwd)"' >> ~/.zshrc   # For Zsh
```

## Usage

### Compiling a ZR# Program

```bash
./z source.zr [-o output]
```

Options:
- `-o output`: Specify the output file name (default: a.out)
- `-h, --help`: Show help message

### Example Program

Create a file named `example.zr`:

```zr
// Variable declaration
let x = 10;
let y = 5;

// Arithmetic operations
let sum = x + y;
print sum;

// Conditional statement
if (sum > 10) {
    let result = sum;
    print result;
} else {
    let result = x;
    print result;
};
```

Compile and run:
```bash
./z example.zr
// This cmd print out the output. We will add output files later   
```

## Language Syntax

### Variables
```zr
let x: int = 10;      // Integer
let y: float = 3.14;    // Float
let z = x + y;   // Type inference
```

### Arithmetic Operations
```zr
let a = x + y;   // Addition
let b = x - y;   // Subtraction
let c = x * y;   // Multiplication
let d = x / y;   // Division
```

### Conditional Statements
```zr
if (condition) {
    // code block
} else {
    // else block
};
```

### Comments
```zr
// This is a single-line comment
```

### Print Statement
```zr
print x;         // Print a value
```

## Development

The compiler is written in C and consists of several components:

- `lexer.c`: Tokenizes source code
- `parser.c`: Generates Abstract Syntax Tree (AST)
- `interpreter.c`: Executes parsed AST
- `main.c`: Main entry point
- `compiler.h`: Common header file
- `z`: Compiler wrapper script

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Known Limitations

- Limited language specification
- Basic error reporting
- No function definitions yet
- No complex data structures
- Limited type system

## Future Improvements

1. Enhanced type system
2. Function definitions
3. More robust error handling
4. Comprehensive test suite
5. Improved memory management
6. Support for complex data structures
7. Generics
8. Compile-time type checking

## Support

For issues, questions, or contributions, please:
1. Check the existing issues
2. Create a new issue if needed
3. Join our community discussions

## Acknowledgments

Special thanks to:
- The Rust programming language for memory safety inspiration
- C# for modern language features
- C++ for performance concepts
