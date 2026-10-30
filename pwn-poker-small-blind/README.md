# Small Blind

* **Event:** UTCTF 2026
* **Problem Type:** Binary Exploitation
* **Tools Required / Used:** None

## Steps​

### Step 1

The challenge does not provide any files, only the connection information. Connecting to it and entering a name reveals that we are playing Texas Hold'em poker. We can play the game as normal, but as the description suggests, we should be looking for some alternative way to win.

### Step 2

One place to start is by experimenting with the different inputs to the program. For example, during each round of betting, we have to state our action, and if we are raising, we also have to specify the amount. However, plugging in weird values here doesn't seem to do anything interesting. But another place input is accepted is at the very beginning, when we are asked for our name. One potential idea is to enter a really long string to attempt a buffer overflow, however this doesn't seem to work either (the input is capped by fgets). But if we enter C format specifiers such as %d, we see that when our name gets printed in the introduction it actually prints out values from the stack! So we have a format string vulnerability, and we can use this to read and write values in memory.

### Step 3

Now that we have an idea of the vulnerability, we can start trying to read or write values to try to get the flag. First, we may try to read pointers from the stack using %s, in case the flag is stored somewhere in memory. However, this will not work (as the flag does not get read from the file until the win condition is already met). Instead, we can try to look for pointers that we can attempt to write to. Using the %x specifier a few times, we see some potentially interesting values: 
> d015c7d0 0 0 16 16 d015ef28 d015ef2c d015ef40 4034c3 0 0 0 0 0 0 0 0 0 1007 400040!
Here, there are 6 values that look like pointers, so we can try to write to them using the %n specifier and see if anything changes in the game that we can notice. We can try writing to each of these pointers one by one to see what happens. After some trial and error, we may find that a format string such as "%d %d %d %d %d %n" will set the dealer's chip value (in this case, to 22), or " %d %d %d %d %d %d %n" which sets our own chip value (in this case to 31). So we can set the dealer's value really high and then win them back, or the easier option is to just set our own chip value to something really high and cash out immediately to win the flag. Specifically, we can use the format string "%1000d %d %d %d %d %d %n" to set our chip value to 1023, and then cash out to win the flag.
