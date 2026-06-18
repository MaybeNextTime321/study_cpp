# Calculator

A simple command-line calculator that supports basic arithmetic operations.

## Dependencies

- CMake 3.8+
- GCC 13+ or Clang 18+
- clang-format
- clang-tidy
- Git (for fetching dependencies)

## Build & Install

```bash
cmake -B build
cmake --build build
sudo cmake --build build --target install
```

## Usage

```bash
calculation '<JSON string>'
```

### JSON Fields

| Field | Type | Description |
|-------|------|-------------|
| `firstNumber` | integer | First operand |
| `secondNumber` | integer | Second operand |
| `operation` | string | Operator symbol |

### Supported Operations

| Operation | Symbol |
|-----------|--------|
| Addition | `+` |
| Subtraction | `-` |
| Multiplication | `*` |
| Division | `/` |
| Power | `^` |
| Factorial | `!` |

> **Note:** wrap JSON in single quotes to prevent shell expansion

## Examples

```bash
# Addition
calculation '{"firstNumber": 5, "secondNumber": 3, "operation": "+"}'

# Subtraction
calculation '{"firstNumber": 10, "secondNumber": 4, "operation": "-"}'

# Multiplication
calculation '{"firstNumber": 6, "secondNumber": 7, "operation": "*"}'

# Division
calculation '{"firstNumber": 10, "secondNumber": 3, "operation": "/"}'

# Power
calculation '{"firstNumber": 2, "secondNumber": 10, "operation": "^"}'

# Factorial (secondNumber is ignored)
calculation '{"firstNumber": 5, "secondNumber": 0, "operation": "!"}'
```

## Error Handling

The calculator detects and reports the following errors:

- Division by zero
- Integer overflow
- Factorial of a negative number
- Invalid JSON input
- Invalid input arguments

## Help

```bash
calculation --help
```
