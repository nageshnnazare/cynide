# Cynide language design & reference

Cynide is an indentation-based language that resembles Python, but is compiled **ahead-of-time** to native code via llvm.

## Lexical structure 

### Indentation rules
- Cynide uses indentation to mark blocks.
- **Only spaces** can be used for indentation at the beginning of logical lines. **Tabs** are not yet supported.
- **Blank line** are generally ignored for structure.
- Lines that contain only spaces and / or comments are treated as blank lines.

### Comments

Cynide **line comments** start with **#**.

#### Basic comment
```python
# This line is ignored by compiler
let x: int = 1
```

#### Comment with code 
```python
let pi = 3.145 # approximately
```

#### Comments inside blocks
```python
fn call(n: int) -> int:
    # local vars
    let a = 10
    # return stmt
    return a + 1
```

#### Comment-only lines
```python
let x = 10
# assign y = x
let y = x
```

### Types

#### **int** - integers
```python
let zero = 0
let result: int = 10
let neg = -5
```

#### **float** - floating point
```python
let pi: float = 3.14
let x = 6.9 
```

#### **bool** - boolean
```python
let result: bool = true
let fail = false
```

#### **string** - text string
```python
let empty = ""
let name: string = "cynide"
```

#### **void** 
```python
fn no_return() -> void:
    print("abc")
```

### Literals

#### Integer literals
```python
let dec = 255
let inc = 3
```

#### Floating point literals
```python
let pi:float = 3.14159
let a = 0.1
let b = 1.4
```

#### String literals
```python
let s = "chars inside double quotes."
```

### Variables
Cynide uses `let` statements to declare variables. If the type is omitted, it is inferred, or it can be explicitly annotated using a colon `:` and the type name.
```python
let x = 10                  # Inferred type (int)
let pi: float = 3.14159     # Explicitly typed float
let name: string = "cynide" # Explicitly typed string
let active: bool = true     # Explicitly typed boolean
```

### Assignment
Variables are mutable and can be reassigned with new values using the `=` operator. Reassignment must match the variable's initial or annotated type.
```python
let x = 10
x = 20                     # Reassignment of x
```

### Operators & expressions
Cynide supports standard binary operators for arithmetic, comparison, and logic, as well as unary operators.
* **Arithmetic**: `+`, `-`, `*`, `/`, `%`
* **Comparison**: `==`, `!=`, `<`, `>`, `<=`, `>=`
* **Logical**: `and`, `or`, `not`
* **Unary**: `-` (negation), `not` (logical inversion)
* **Parentheses**: `(expr)` can be used to override default operator precedence.

```python
let i_sum = 17 + 5
let i_mul = 6 * 9
let neg = -(3 + 2)
let is_valid = x > 10 and x < 20
```

### Functions
Functions are declared using the `fn` keyword. Parameters and return types must be explicitly annotated. The `void` type is used for functions that do not return a value.
The body of the function must be indented relative to the signature.
```python
fn double(x: int) -> int:
    return x * 2

fn greet(tag: string) -> void:
    print(tag)
```

### Control flow
Cynide supports conditional execution with `if`, `elif`, and `else` statements.
```python
if score >= 90:
    print("grade A")
elif score >= 80:
    print("grade B")
else:
    print("grade F")
```

### Loops
Cynide supports both `while` loops and `for` loops.
* **while loops**: Executes the block as long as the condition evaluates to `true`.
* **for loops**: Iterate over a range. The `range(end)` function iterates from `0` up to (but excluding) `end`. The `range(start, end)` function iterates from `start` up to (but excluding) `end`.

```python
# while loop
let i = 0
while i < 5:
    print(i)
    i = i + 1

# for loop with range(end)
for i in range(10):
    print(i)

# for loop with range(start, end)
for i in range(3, 10):
    print(i)
```

### Print
The built-in `print()` function takes a comma-separated list of expressions and outputs their values.
```python
print("total = ", total)
```

### Misc
* Indentation structure defines the code blocks (no curly braces `{}` or semicolons `;` required).
* Spaces are strictly enforced for indentation; tabs are not permitted.