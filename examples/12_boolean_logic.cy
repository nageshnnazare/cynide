# Boolean logic: and, or, not and relational operators

let p = true
let q = false

print(p and q)
print(p or q)
print(not p)
print(not q)

let a = 10
let b = 20

print(a < b and b > 15)
print(a == 10 or b == 0)
print(not (a > b))

# Combining comparisons mirrors readable prose constraints
let age = 30
let ok = age >= 18 and age <= 65
print(ok)
