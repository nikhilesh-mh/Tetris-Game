﻿# 🎮 Terminal Tetris Game (C++ Bitset Edition)

A terminal-based implementation of the classic **Tetris** game written in **C++**, optimized for Windows CMD with ANSI escape support. This version uses a grid-based approach powered by `std::bitset` to manage game state and rendering.

## 🧩 Features

* Full classic Tetris experience (7 Tetrominoes)
* Shadow projection for current piece
* Real-time keyboard controls (arrow keys, spacebar)
* Level progression with increasing speed
* Score display and clean board UI
* Flicker-free ANSI rendering with smooth transitions
* Powered by bitsets and intuitive logic

---

## 🗂 File Overview

* `main.cpp`:
  Contains the complete game logic. The screen state, piece rendering, input handling, scoring, and animation loop are all defined here. ANSI escape codes and Windows API are used for enhanced visual output in the terminal.

---

## 🎮 Controls

| Key          | Action                |
| ------------ | --------------------- |
| ⬅ / ➡        | Move block left/right |
| ⬇            | Drop block instantly  |
| ⬆ or `Space` | Rotate block          |
| `Esc` / `Q`  | Quit the game         |

---

## 🧮 Scoring

Scoring increases exponentially with the number of lines cleared at once:

- 1 line: +100 points  
- 2 lines: +400 points  
- 3 lines: +900 points  
- 4 lines: +1600 points  
- and so on...

In general:  
> **n lines = n² × 100 points**

The game speed increases as your score rises, making it progressively more challenging.

---

## 🛠 Developer Experience & Learning

Initially, I used **bitwise operations directly on `std::bitset`** to manage both the board and falling blocks. As announced on my social media handles, the goal was to implement efficient memory and logic handling through pure bitwise arithmetic.

However, this **approach led to several bugs and complexity**—especially with row shifting, piece placement, and boundary checks. While debugging, I realized that bitwise-only logic made the codebase hard to scale and debug.

Although I **shifted my implementation strategy**, I still kept `bitset` at the core to benefit from its memory efficiency and fast access. Instead of treating the board like a raw binary stream, I approached it as a 2D grid and used helper functions to interpret positions.

This refactor:

* Improved modularity and readability
* Made debugging smoother
* Helped me better **intuit bitwise logic** through actual trial and error
* Deepened my **understanding of C++ fundamentals**, including pointers, `std::bitset`, and terminal I/O

Despite the setbacks, it was a **massively rewarding learning experience** that grew my confidence in **low-level programming** and creative debugging.

---

## 📷 Screenshot

<p align="center">
  <img src="assets/TerminalScreenshot.png" alt="Tetris Screenshot" width="400"/>
</p>


---

## 🚀 How to Run

1. **OS**: Windows
2. **Compiler**: g++ (MinGW) or MSVC
3. **Command to Compile**:

   ```bash
   g++ -std=c++17 -O2 main.cpp -o tetris.exe
   ./tetris.exe
   ```

---

## 🧠 What I Learned

* How to build a game loop with real-time input
* Matrix-to-linear-bitset translation
* Efficient use of `bitset` for pixel-level logic
* Basic animation and rendering using ANSI in CMD
* The importance of designing **debuggable**, **modular**, and **scalable** logic from the start

---

## ✨ Future Improvements

* Add sound or music support
* Save high scores
* Support cross-platform terminals (e.g., Linux, macOS)
* Add more color themes or difficulty levels

---

Feel free to fork, learn, or contribute to this terminal-based Tetris challenge!
💻 Built with love and bitsets ❤️

