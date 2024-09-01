// options log=true,optimize=true,log_optimization_passes=false
[sideeffects]
def fib(n)
    if (n < 2)
        return n
    return fib(n - 1) + fib(n - 2)

[export]
def main
    print("{fib(42)}")