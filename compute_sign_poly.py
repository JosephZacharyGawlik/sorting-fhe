#!/usr/bin/env python3
"""
Compute the sign polynomial over Z_257 for FHE bitonic sort.

sign(x) over Z_257:
  - returns 1   for x in {1,...,128}
  - returns 256 (= -1 mod 257) for x in {129,...,256}
  - returns 0   for x = 0

Since sign is odd, sign(x) = x * Q(x^2) where Q is degree 63.
The correction polynomial R(x) = 129 * (x + x^2 * Q(x^2)) gives:
  - R(x) = x   if x in {1,...,128}    (positive diff -> swap)
  - R(x) = 0   if x in {0,129,...,256} (non-positive diff -> no swap)

129 = inverse of 2 mod 257.
"""

P = 257


def mod_inv(a, p):
    """Modular inverse via extended Euclidean algorithm."""
    if a % p == 0:
        raise ValueError(f"{a} has no inverse mod {p}")
    return pow(a, p - 2, p)


def compute_sign_table():
    """Compute sign(x) for all x in Z_257."""
    sign = [0] * P
    for x in range(1, 129):
        sign[x] = 1
    for x in range(129, 257):
        sign[x] = P - 1  # = 256 = -1 mod 257
    return sign


def compute_Q_coefficients():
    """
    Compute Q(y) such that sign(x) = x * Q(x^2) for all x in Z_257.

    For each x in {1,...,128}, we have sign(x)/x = x^{-1} mod 257 (since sign(x)=1).
    So Q(x^2) = x^{-1} for x in {1,...,128}.

    The 128 values x^2 mod 257 for x in {1,...,128} are exactly the 128
    quadratic residues mod 257. We interpolate Q over these 128 points.
    Q has degree at most 127 (128 points -> degree 127 polynomial).

    Actually since Q(x^2) = x^{-1} and we only need degree 63 (as per plan),
    let me verify: 128 points give degree 127 at most. But the plan says
    degree 63. Let me check...

    The 128 quadratic residues are distinct values in Z_257*. Interpolating
    128 points gives a polynomial of degree at most 127. However, the plan
    mentions Q is degree 63 with 64 coefficients. This may be an optimization
    or the plan may have an error. Let's compute the full interpolation and
    see what degree we actually need.
    """
    # Collect interpolation points: (x^2 mod P, x^{-1} mod P) for x in {1,...,128}
    points = []
    y_vals = set()
    for x in range(1, 129):
        y = (x * x) % P
        q_val = mod_inv(x, P)  # Q(y) = x^{-1} since sign(x) = 1 for x in {1,...,128}
        if y in y_vals:
            # Check consistency: -x has same square, sign(-x) = -1, (-x)^{-1} = -x^{-1}
            # So Q(y) should satisfy both: Q(y) = x^{-1} (from x) and
            # sign(P-x) = (P-x) * Q((P-x)^2) = (P-x) * Q(x^2) = -x * Q(y)
            # We need -x * Q(y) = -1, so Q(y) = x^{-1}. Consistent!
            continue
        y_vals.add(y)
        points.append((y, q_val))

    print(f"Number of interpolation points: {len(points)}")

    # Sort by y value for consistency
    points.sort(key=lambda p: p[0])

    n = len(points)
    ys = [p[0] for p in points]
    vs = [p[1] for p in points]

    # Lagrange interpolation via Gaussian elimination on Vandermonde system
    # V * coeffs = vs, where V[i][j] = ys[i]^j
    # Build augmented matrix
    mat = []
    for i in range(n):
        row = []
        val = 1
        for j in range(n):
            row.append(val)
            val = (val * ys[i]) % P
        row.append(vs[i])
        mat.append(row)

    # Gaussian elimination
    for col in range(n):
        # Find pivot
        pivot = -1
        for row in range(col, n):
            if mat[row][col] != 0:
                pivot = row
                break
        if pivot == -1:
            raise ValueError(f"Singular matrix at column {col}")

        # Swap
        mat[col], mat[pivot] = mat[pivot], mat[col]

        # Scale pivot row
        inv_pivot = mod_inv(mat[col][col], P)
        for j in range(col, n + 1):
            mat[col][j] = (mat[col][j] * inv_pivot) % P

        # Eliminate
        for row in range(n):
            if row == col:
                continue
            factor = mat[row][col]
            if factor == 0:
                continue
            for j in range(col, n + 1):
                mat[row][j] = (mat[row][j] - factor * mat[col][j]) % P

    coeffs = [mat[i][n] for i in range(n)]

    return coeffs


def eval_poly(coeffs, x, p):
    """Evaluate polynomial with given coefficients at x mod p."""
    result = 0
    power = 1
    for c in coeffs:
        result = (result + c * power) % p
        power = (power * x) % p
    return result


def verify_sign_polynomial(Q_coeffs):
    """Verify that sign(x) = x * Q(x^2) for all x in Z_257."""
    sign_table = compute_sign_table()
    errors = 0

    for x in range(P):
        y = (x * x) % P
        q_val = eval_poly(Q_coeffs, y, P)
        computed_sign = (x * q_val) % P
        expected = sign_table[x]

        if computed_sign != expected:
            print(f"  MISMATCH at x={x}: computed={computed_sign}, expected={expected}")
            errors += 1

    if errors == 0:
        print("PASS: sign(x) = x * Q(x^2) verified for all x in Z_257")
    else:
        print(f"FAIL: {errors} mismatches")

    return errors == 0


def verify_correction_polynomial(Q_coeffs):
    """
    Verify R(x) = 129 * (x + x^2 * Q(x^2)):
      R(x) = x for x in {1,...,128}
      R(x) = 0 for x in {0, 129,...,256}
    """
    INV2 = 129  # inverse of 2 mod 257
    errors = 0

    for x in range(P):
        y = (x * x) % P
        q_val = eval_poly(Q_coeffs, y, P)
        # x^2 * Q(x^2)
        product = (y * q_val) % P
        # x + x^2 * Q(x^2)
        inner = (x + product) % P
        # 129 * (...)
        R = (INV2 * inner) % P

        if 1 <= x <= 128:
            expected = x
        else:
            expected = 0

        if R != expected:
            print(f"  MISMATCH at x={x}: R={R}, expected={expected}")
            errors += 1

    if errors == 0:
        print("PASS: correction polynomial R(x) verified for all x in Z_257")
    else:
        print(f"FAIL: {errors} mismatches")

    return errors == 0


def simulate_bitonic_sort():
    """Simulate bitonic sort on plaintext to verify the network."""
    arr = [42, 17, 83, 5, 91, 33, 67, 12]
    n = 8
    print(f"\nSimulating bitonic sort on: {arr}")

    def compare_and_swap_plain(a_val, b_val):
        """Returns (min, max)."""
        if a_val > b_val:
            return b_val, a_val
        return a_val, b_val

    k = 2
    while k <= n:
        j = k >> 1
        while j >= 1:
            for i in range(n):
                l = i ^ j
                if l > i:
                    ascending = ((i & k) == 0)
                    if ascending:
                        arr[i], arr[l] = compare_and_swap_plain(arr[i], arr[l])
                    else:
                        arr[l], arr[i] = compare_and_swap_plain(arr[l], arr[i])
            j >>= 1
        k <<= 1

    print(f"Result: {arr}")
    expected = [5, 12, 17, 33, 42, 67, 83, 91]
    if arr == expected:
        print("PASS: bitonic sort network produces correct result")
    else:
        print(f"FAIL: expected {expected}")

    return arr == expected


def format_cpp_array(coeffs):
    """Format coefficients as a C++ array initializer."""
    lines = []
    lines.append(f"// Q(y) coefficients for sign(x) = x * Q(x^2) over Z_257")
    lines.append(f"// {len(coeffs)} coefficients (degree {len(coeffs)-1} polynomial)")
    lines.append(f"static const uint64_t Q_COEFFS[{len(coeffs)}] = {{")

    # Format 8 per line
    for i in range(0, len(coeffs), 8):
        chunk = coeffs[i:i+8]
        line = "    " + ", ".join(f"{c:3d}" for c in chunk)
        if i + 8 < len(coeffs):
            line += ","
        lines.append(line)

    lines.append("};")
    return "\n".join(lines)


def main():
    print("=" * 60)
    print("Computing sign polynomial over Z_257")
    print("=" * 60)

    # Step 1: Compute Q coefficients
    print("\nStep 1: Lagrange interpolation for Q(y)...")
    Q_coeffs = compute_Q_coefficients()
    print(f"  Q has {len(Q_coeffs)} coefficients (degree {len(Q_coeffs)-1})")

    # Find actual degree (highest non-zero coefficient)
    actual_degree = len(Q_coeffs) - 1
    while actual_degree > 0 and Q_coeffs[actual_degree] == 0:
        actual_degree -= 1
    print(f"  Actual degree: {actual_degree}")

    # Step 2: Verify sign polynomial
    print("\nStep 2: Verifying sign(x) = x * Q(x^2)...")
    ok1 = verify_sign_polynomial(Q_coeffs)

    # Step 3: Verify correction polynomial
    print("\nStep 3: Verifying correction polynomial R(x) = 129*(x + x^2*Q(x^2))...")
    ok2 = verify_correction_polynomial(Q_coeffs)

    # Step 4: Simulate bitonic sort
    ok3 = simulate_bitonic_sort()

    # Step 5: Output C++ array
    print("\n" + "=" * 60)
    print("C++ array literal:")
    print("=" * 60)
    cpp_code = format_cpp_array(Q_coeffs)
    print(cpp_code)

    # Also print the number of coefficients needed for baby-step size decisions
    print(f"\nNumber of Q coefficients: {len(Q_coeffs)}")
    print(f"For Paterson-Stockmeyer with baby step s=8:")
    print(f"  Number of giant steps: {(len(Q_coeffs) + 7) // 8}")
    print(f"  Baby powers needed: y^0 through y^7")
    print(f"  Giant power: z = y^8")

    if ok1 and ok2 and ok3:
        print("\nAll checks PASSED.")
    else:
        print("\nSome checks FAILED!")
        return 1

    return 0


if __name__ == "__main__":
    exit(main())
