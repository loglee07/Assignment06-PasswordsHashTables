# Assignment 06: Passwords and Hash Tables

This C++17 program demonstrates a small authentication system backed by a custom hash table. It does not use `std::unordered_map` to store users.

## Features

- Stores username keys and user records
- Implements `insert`, `find`, `remove`, and `loadFactor`
- Automatically resizes and rehashes when the load factor exceeds 0.70
- Implements separate chaining, linear probing, quadratic probing, and double hashing
- Stores a salt and a hash of `salt + password + pepper` for each user
- Demonstrates successful login, incorrect password, unknown user, removal, collisions, and rehashing

## Compile and Run

```bash
clang++ -std=c++17 main.cpp -o assignment06
./assignment06
