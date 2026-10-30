# Oblivious Error

* **Event:** UTCTF 2026
* **Problem Type:** Cryptography
* **Tools Required / Used:** Python

## Steps​

### Step 1
Look at the my-code.txt file. 

```py
while True:
    try:
        print("Please pick a value k.")
        k = int(input())
        break
    except ValueError:
        print("Invalid value. Please pick an integer.")
        print("Please pick a value k.")
        k = int(input())

v = (x0 + (int(k) ^ e)) % N
```

The first error you see is that the message choice prompt (0 or 1) is being set to k, not v. Immediately the obliviousness is lost because the sender is able to figure out which messages are being sent by calculating the two possible v values with the k value.

### Step 2
Next huge error to notice is v is calculated with k ^ e (xor) instead of k ** e. The second message seems to be undecipherable because the only choice for v’s calculation is with x0, but this k ^ e error makes it trivial to leak the second message as well.

Other than these two errors, the math for the oblivious transfer is the same. So, we can construct a payload for our k value that leaks the second message using simple arithmetic:

Since our k1 value is calculated by raising v - x1 to the power of d, we need a way to get rid of d (because we don’t know d) to make it easy to get our message. (Hint. 0 ** d = 0 no matter what d is!)

> v = (x0 + k ^ e) % N  
> k1 = ((v - x1) ** d) % N  
> c1 = (m1 + k1) % N  

### Step 3

To get 0 ** d, we need v - x1 = 0 or (x0 + k ^ e) - x1 = 0

Solve for k:
> x0 + k ^ e = x1  
> k ^ e = x1 - x0  
> k = (x1 - x0) ^ e  

Now, 
> v = x0 + (((x1 - x0) ^ e) ^ e) = x0 + x1 - x0 = x1  
> k1 = (x1 - x1)^d % N = 0  

Using x1, x0, e, and N, we can calculate this value and get our payload for k! 

### Step 4

Since c1 = (m1 + k1) % N we can extract our message m1 using c1 - k1. And since we set k1 = 0 using our payload, the returned integer value should be our fully decoded message as a long. Just convert the long to bytes then to a string to grab our flag.



