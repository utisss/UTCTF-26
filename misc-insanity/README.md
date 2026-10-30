# Insanity Check: Hat Trick Denied

* **Event:** UTCTF 2026
* **Problem Type:** Misc
* **Tools Required / Used:** Browser Dev Tools (Inspect Element)

## Steps​

### Step 1

From the description, we know to look for the flag in the CTFd site. So we can start testing various parts that websites have. This can involve looking at the HTML or JS console messages of different files.

### Step 2

One file that a lot of websites have is a robots.txt file to tell web crawlers pages they should not index. Looking at the robots.txt for utctf.live, we see two suspicious entries:

```
User-agent: *
Disallow: /admin
Disallow: /2065467898
Disallow: /3037802467
```

### Step 3

Visiting these webpages, we see what appears to be a 404 page, as though we had visited any other non-existent page. But looking at the HTML source code, we see that it differs from the normal 404 - each one has a comment with a list of integers. XORing these as a one time pad and converting to ASCII, we obtain the flag!
