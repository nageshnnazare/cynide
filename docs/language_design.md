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

### Assignment

### Operators & expressions

### Functions

### Control flow

### Loops

### Print

### Misc