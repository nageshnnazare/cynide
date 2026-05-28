# while / for loops

let n = 5
let i = 0

while i < n:
    print("While tick = ", i)
    i = i + 1

print("for range(end)")
for i in range(10):
    print(i)

print("for range(start, end)")
for i in range(3, 10):
    print(i)

let t = 3
while t > 0:
    print(t)
    t = t - 1

print("Accumulate")
let limit = 5
let total = 0
let u = 1
while u <= limit:
    total = total + u
    u = u + 1
print("total = ", total)
