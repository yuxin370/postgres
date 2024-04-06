def is_prime(n):
    if n <= 1:
        return False
    if n <= 3:
        return True
    if n % 2 == 0 or n % 3 == 0:
        return False
    i = 5
    while i * i <= n:
        if n % i == 0 or n % (i + 2) == 0:
            return False
        i += 6
    return True

def find_largest_prime_below_limit(limit):
    max_prime = 0
    for i in range(limit, 1, -1):
        if is_prime(i):
            max_prime = i
            break
    return max_prime

if __name__ == "__main__":
    limit = 1000000
    max_prime = find_largest_prime_below_limit(limit)
    print(f"小于等于 {limit} 的最大质数是：{max_prime}")