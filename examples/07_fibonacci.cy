# Fibonacci numbers with a while loop
# Prints the first N Fibonacci values starting from F0 = 0, F1 = 1.

let N = 10
let a = 0
let b = 1
let step = 0

while step < N:
    print(a)
    let temp = a + b
    a = b
    b = temp
    step = step + 1
