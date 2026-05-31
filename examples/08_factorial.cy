# Factorial: iterative loop vs recursive definition

fn factorial_loop(n: int) -> int:
    let acc = 1
    let i = 1
    while i <= n:
        acc = acc * i
        i = i + 1
    return acc

fn factorial_rec(n: int) -> int:
    if n <= 1:
        return 1
    return n * factorial_rec(n - 1)

let x = 7
print("iterative", factorial_loop(x))
print("recursive", factorial_rec(x))
