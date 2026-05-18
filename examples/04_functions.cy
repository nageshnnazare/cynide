# Functions: parameters, return types, nested calls

fn double(x: int) -> int:
    return x * 2

fn add_three(a: int, b: int, c: int) -> int:
    return a + b + c

fn greet(tag: string) -> void:
    print(tag)

greet("functions..")

let v = double(5)
print(v)

let w = add_three(1, 2, 3)
print(w)

let y = double(double(2))
print(y)
