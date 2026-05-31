# Comprehensive demo: combines variables, functions, control flow, and loops

fn clamp_positive(x: int) -> int:
    if x < 0:
        return 0
    return x

fn average_three(a: int, b: int, c: int) -> float:
    let total = a + b + c
    return total / 3.0

print("=== Blaze comprehensive demo ===")

let score_a = 72
let score_b = 81
let score_c = 69

let avg = average_three(score_a, score_b, score_c)
print("average score", avg)

if avg >= 80.0:
    print("team doing great")
elif avg >= 70.0:
    print("solid progress")
else:
    print("keep practicing")

print("sanitized deltas:")
for raw in range(-2, 4):
    let fixed = clamp_positive(raw)
    print(raw, fixed)

let power = 1
let exp = 6
let count = 0
while count < exp:
    power = power * 2
    count = count + 1
print("2^6 via loop", power)

print("=== done ===")
