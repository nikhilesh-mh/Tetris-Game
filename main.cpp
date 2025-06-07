#include <iostream>
#include <bitset>
#include <cmath>
#include <iomanip>
#include <chrono>
#include <vector>
#include <random>
#include <string>
#include <conio.h>
#include <windows.h>
#include <climits>
#include <array>

// Define the constant if it's not available
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

using namespace std;

// Game constants
constexpr int ROW = 15, COL = 10;
constexpr int MAX_SQUARE = ROW * COL;

// Pre-computed constants for better performance
constexpr bitset<MAX_SQUARE> BOTTOM_EDGE_MASK = (1ULL << COL) - 1;

// Helper functions to compute edge masks at runtime
bitset<MAX_SQUARE> compute_right_edge_mask() {
    bitset<MAX_SQUARE> mask;
    for (int i = 0; i < ROW; i++) {
        mask.set(i * COL + (COL - 1));
    }
    return mask;
}

bitset<MAX_SQUARE> compute_left_edge_mask() {
    bitset<MAX_SQUARE> mask;
    for (int i = 0; i < ROW; ++i) {
        mask.set(i * COL);
    }
    return mask;
}

const bitset<MAX_SQUARE> RIGHT_EDGE_MASK = compute_right_edge_mask();
const bitset<MAX_SQUARE> LEFT_EDGE_MASK = compute_left_edge_mask();

// ANSI color constants
const string BOLD         = "\033[1m";
const string RESET        = "\033[0m";
const string GREY         = "\033[38;2;170;170;170m";
const string BLUE         = "\033[38;2;0;170;255m";
const string BLUE_SHADOW  = "\033[38;2;50;100;150m";
const string DARK_GREY    = "\033[38;2;80;80;80m";
const string CORAL        = "\033[38;2;244;100;103m"; 
const string CLEAR_SCREEN = "\033[2J\033[H";
const string HIDE_CURSOR  = "\033[?25l";
const string SHOW_CURSOR  = "\033[?25h";
const string MOVE_HOME    = "\033[H";

struct Point {
    int col, row;

    constexpr Point(int col = 1, int row = 1) : col(col), row(row) {}

    constexpr bool operator==(const Point& other) const noexcept {
        return col == other.col && row == other.row;
    }

    constexpr Point operator+(const Point& other) const noexcept {
        return {col + other.col, row + other.row};
    }
    
    // Rotation around origin (0,0)
    constexpr Point rotate_cw() const noexcept {
        // Rotate 90 degrees clockwise
        return {-row, col};
    }
    
    constexpr Point rotate_ccw() const noexcept {
        // Rotate 90 degrees counter-clockwise
        return {row, -col};
    }
};

using Tetromino = vector<Point>;

// Tetromino definitions with proper shapes [Noting down all offsets from a point]
const array<Tetromino, 10> TETROMINOS = {{
    {{0,0}, {0,1}},                         // - - Mini- I
    {{0,0}},                                // . - Dot
    {{0,0}, {0,1}, {1,0}, {1,1}},           // O - Square
    {{-1,0}, {0,0}, {1,0}, {2,0}},          // I - Line
    {{-1,0}, {-1,1}, {0,0}, {1,0}},         // L - L-piece
    {{-1,0}, {0,0}, {1,0}, {1,1}},          // J - Reverse L
    {{-1,1}, {0,1}, {0,0}, {1,0}},          // S - S-piece
    {{-1,0}, {0,0}, {0,1}, {1,1}},          // Z - Z-piece
    {{-1,0}, {0,0}, {1,0}, {0,1}},          // T - T-piece
    {{0,0}, {1,1}},                         // / - slash
}};

// Current piece state
struct BlockPiece {
    Tetromino shape;
    Point position;
    int rotation = 0;
    int type = 0; // Store the tetromino type index
    
    BlockPiece(const Tetromino& t, Point pos, int piece_type) : shape(t), position(pos), type(piece_type) {}
    
    // Get current rotated shape
    Tetromino get_rotated_shape() const {
        Tetromino rotated = shape;
        for (int r = 0; r < rotation; ++r) {
            for (auto& pt : rotated) {
                pt = pt.rotate_cw();
            }
        }
        return rotated;
    }
    
    // Get absolute positions of all blocks
    vector<Point> get_absolute_positions() const {
        vector<Point> positions;
        Tetromino rotated = get_rotated_shape();
        positions.reserve(rotated.size());
        for (const auto& pt : rotated) {
            positions.push_back(position + pt);
        }
        return positions;
    }
};

// Global random number generator (initialized once)
static mt19937 rng(static_cast<unsigned int>(chrono::steady_clock::now().time_since_epoch().count()));

// Creates spawn point for tetromino
Point create_spawn_point() {
    return Point(COL / 2, ROW - 2);
}

// Check-Function
inline bool is_valid_position(int col, int row) noexcept {
    return col >= 0 && col < COL && row >= 0 && row < ROW;
}

// Add a block to the image at the given position
inline void mark_point_on_img(Point pt, bitset<MAX_SQUARE>& image) noexcept {
    if (is_valid_position(pt.col, pt.row)) {
        int index = pt.row * COL + pt.col;
        image.set(index);
    }
}

// Create image of a tetrominoe (BlockPiece)
bitset<MAX_SQUARE> create_image(const BlockPiece& piece) {
    bitset<MAX_SQUARE> image;
    auto positions = piece.get_absolute_positions();
    for (const auto& pos : positions) {
        mark_point_on_img(pos, image);
    }
    return image;
}

// Random Number Generation
int random_number(int min, int max) {
    uniform_int_distribution<int> dist(min, max - 1);
    return dist(rng);
}

void enable_ansi_support() {
    static bool initialized = false;
    if (!initialized) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        initialized = true;
    }
}

// Function to paste the image of a tetrominoe onto the screen
inline void paste_image(bitset<MAX_SQUARE>& screen, bitset<MAX_SQUARE>& image) noexcept {
    screen |= image;
    image.reset();
}

// Check Function
bool can_move_piece(const BlockPiece& piece, const bitset<MAX_SQUARE>& screen) {
    auto positions = piece.get_absolute_positions();
    for (const auto& pos : positions) {
        if (!is_valid_position(pos.col, pos.row)) return false;
        if (pos.row < ROW && screen.test(pos.row * COL + pos.col)) return false;
    }
    return true;
}

// Function to drop a piece by one row
bool block_fall(bitset<MAX_SQUARE>& screen, BlockPiece& piece, bitset<MAX_SQUARE>& image) {
    BlockPiece next_piece = piece;
    next_piece.position.row--;
    
    if (can_move_piece(next_piece, screen)) {
        piece = next_piece;
        image = create_image(piece);
        return true;
    } else {
        paste_image(screen, image);
        return false;
    }
}

// Function to drop a piece by one row
void block_drop(bitset<MAX_SQUARE>& screen, BlockPiece& piece, bitset<MAX_SQUARE>& image) {
    while (block_fall(screen, piece, image)) 
    {
        // Keep falling until it can't
    }
}

// Function to create image of piece at potential ground position
bitset<MAX_SQUARE> block_shadow(const bitset<MAX_SQUARE>& screen, BlockPiece piece) {
    BlockPiece shadow_piece = piece;
    while (true) {
        BlockPiece next_piece = shadow_piece;
        next_piece.position.row--;
        if (can_move_piece(next_piece, screen)) {
            shadow_piece = next_piece;
        } else {
            break;
        }
    }
    return create_image(shadow_piece);
}

// Display the next piece preview
void display_next_piece(const BlockPiece& next_piece, string *output_buffer) {
    *output_buffer += ("Next Piece:" + RESET + "\n");
    
    // Create a small preview of the next piece
    auto shape = next_piece.shape;
    
    // Find bounds of the piece
    int min_col = 0, max_col = 0;
    for (const auto& pt : shape) {
        min_col = min(min_col, pt.col);
        max_col = max(max_col, pt.col);
    }
    
    // Always display a consistent 4x4 preview grid
    const int preview_size = 4;
    for (int row = preview_size - 1; row >= 0; row--) {
        *output_buffer += (GREY + " ." + RESET);
        for (int col = min_col; col <= min_col + preview_size - 1; col++) {
            bool found = false;
            for (const auto& pt : shape) {
                if (pt.col == col && pt.row == row) {
                    found = true;
                    break;
                }
            }
            if (found) {
                *output_buffer += (CORAL + "[]" + RESET);
            } else {
                *output_buffer += (GREY + " ." + RESET);
            }
        }
        *output_buffer += "\n";
    }
    *output_buffer += "\n";
}

// Display the game board
void display_board(
        const bitset<MAX_SQUARE>& screen, const bitset<MAX_SQUARE>& image, 
        const BlockPiece& piece, const BlockPiece& next_piece, unsigned score, unsigned speed, bool display_shadow = true
    ) {
    static string output_buffer;
    output_buffer.clear();
    output_buffer.reserve(3072);
    
    output_buffer += MOVE_HOME;

    bitset<MAX_SQUARE> shadow;
    if (display_shadow && image.any()) {
        shadow = block_shadow(screen, piece);
    }

    const bool larger_mode = (COL >= 10);
    
    // Score and controls
    output_buffer += "Score: " + to_string(score) + "  |  Speed: " + to_string(speed) + " |  Space=Rotate ESC=Quit\n\n";

    cout << output_buffer;
    output_buffer.clear();

    // Display next piece
    display_next_piece(next_piece, &output_buffer);
    
    cout << output_buffer;
    output_buffer.clear();

    // Header row
    output_buffer += "    |";
    for (int i = 0; i < COL; i++) {
        output_buffer += (i < 9 ? " " : "") + to_string(i + 1);
        if (larger_mode) output_buffer += " ";
    }
    output_buffer += "|\n";

    // Pre-compute block strings
    const string grey_block = BOLD + GREY + "[" + (larger_mode ? " " : "") + "]" + RESET;
    const string blue_block = BOLD + BLUE + "[" + (larger_mode ? " " : "") + "]" + RESET;
    const string shadow_block = BOLD + BLUE_SHADOW + "[" + (larger_mode ? " " : "") + "]" + RESET;
    const string empty_block = BOLD + DARK_GREY + " ." + (larger_mode ? " " : "") + RESET;

    // Grid rendering (top to bottom)
    for (int row = ROW - 1; row >= 0; row--) {
        const int row_num = ROW - row;
        output_buffer += (row_num < 10 ? "  " : " ") + to_string(row_num) + " |";
        
        for (int col = 0; col < COL; col++) {
            const int bit_index = row * COL + col;
            if (screen.test(bit_index)) {
                output_buffer += grey_block;
            } else if (image.test(bit_index)) {
                output_buffer += blue_block;
            } else if (display_shadow && shadow.test(bit_index)) {
                output_buffer += shadow_block;
            } else {
                output_buffer += empty_block;
            }
        }
        output_buffer += "|\n";
    }
    
    // Bottom border
    output_buffer += "    +";
    for (int i = 0; i < COL * (larger_mode ? 3 : 2); i++) {
        output_buffer += "-";
    }
    output_buffer += "+\n";
    
    cout << output_buffer << flush;
}

// Function to move blocks horizontally in given direction as -1 (left), or 1 (right)
bool block_move_horizontal(bitset<MAX_SQUARE>& screen, BlockPiece& piece, bitset<MAX_SQUARE>& image, int direction) {
    BlockPiece next_piece = piece;
    next_piece.position.col += direction;
    
    if (can_move_piece(next_piece, screen)) {
        piece = next_piece;
        image = create_image(piece);
        return true;
    }
    return false;
}

// Function to rotate blocks
bool block_rotate(bitset<MAX_SQUARE>& screen, BlockPiece& piece, bitset<MAX_SQUARE>& image) {
    BlockPiece rotated_piece = piece;
    rotated_piece.rotation = (rotated_piece.rotation + 1) % 4;
    
    if (can_move_piece(rotated_piece, screen)) {
        piece = rotated_piece;
        image = create_image(piece);
        return true;
    }
    
    // Try wall kicks - simple implementation
    for (int kick_x = -1; kick_x <= 1; kick_x++) {
        for (int kick_y = -1; kick_y <= 1; kick_y++) {
            if (kick_x == 0 && kick_y == 0) continue;
            
            BlockPiece kicked_piece = rotated_piece;
            kicked_piece.position.col += kick_x;
            kicked_piece.position.row += kick_y;
            
            if (can_move_piece(kicked_piece, screen)) {
                piece = kicked_piece;
                image = create_image(piece);
                return true;
            }
        }
    }
    
    return false;
}

// Function check if any fully filled rows exist,
// if yes, them remove it, fall upper part downwards and increase score by suitable amount
void check_for_score(bitset<MAX_SQUARE>& screen, unsigned& score) {
    int lines_cleared = 0;
    
    for (int row = 0; row < ROW; ++row) {
        bool full_line = true;
        for (int col = 0; col < COL; ++col) {
            if (!screen.test(row * COL + col)) {
                full_line = false;
                break;
            }
        }
        
        if (full_line) {
            lines_cleared++;
            
            // Clear the line
            for (int col = 0; col < COL; ++col) {
                screen.reset(row * COL + col);
            }
            
            // Move all lines above down by one
            for (int move_row = row + 1; move_row < ROW; ++move_row) {
                for (int col = 0; col < COL; ++col) {
                    int from_idx = move_row * COL + col;
                    int to_idx = (move_row - 1) * COL + col;
                    screen[to_idx] = screen[from_idx];
                    screen[from_idx] = 0;
                }
            }
            
            // Check the same row again since we moved lines down
            row--;
        }
    }
    
    // Scoring system: more lines = exponentially more points
    if (lines_cleared > 0) {
        score += lines_cleared * lines_cleared * 100;
    }
}

inline void clear_screen() {
    cout << CLEAR_SCREEN;
}

inline void hide_cursor() {
    cout << HIDE_CURSOR;
}

inline void show_cursor() {
    cout << SHOW_CURSOR;
}

// Precise sleep function
void precise_sleep(int milliseconds) {
    auto start = chrono::steady_clock::now();
    auto end = start + chrono::milliseconds(milliseconds);
    while (chrono::steady_clock::now() < end);
}

// Function to handle keyboard event
void handle_kb_event(bitset<MAX_SQUARE>& screen, BlockPiece& piece, bitset<MAX_SQUARE>& image, bool& running) {
    int key = _getch();
    // Extended keys (arrow keys)
    if (key == 0 || key == 224) {
        switch (_getch()) {
            case 80:  // Down arrow
                block_drop(screen, piece, image);
                break;
            case 75:  // Left arrow
                block_move_horizontal(screen, piece, image, -1);
                break;
            case 77:  // Right arrow
                block_move_horizontal(screen, piece, image, 1);
                break;
            case 72:  // Up arrow (alternative rotate)
                block_rotate(screen, piece, image);
                break;
        }
    } else {
        switch (key) {
            case 's':  // Down arrow
                block_drop(screen, piece, image);
                break;
            case 'a':  // Left arrow
                block_move_horizontal(screen, piece, image, -1);
                break;
            case 'd':  // Right arrow
                block_move_horizontal(screen, piece, image, 1);
                break;
            case 'w':  // Up arrow (alternative rotate)
                block_rotate(screen, piece, image);
                break;
            case 27:  // ESC
                running = false;
                break;
            case 32:  // Space
                block_rotate(screen, piece, image);
                break;
            case 'q':
            case 'Q':
                running = false;
                break;
        }
    }
}

// Function to spawn new random piece
BlockPiece spawn_new_piece() {
    int piece_type = random_number(0, TETROMINOS.size());
    return BlockPiece(TETROMINOS[piece_type], create_spawn_point(), piece_type);
}

// Main game loop
void game_loop() { 
    bitset<MAX_SQUARE> screen;
    bitset<MAX_SQUARE> image;
    unsigned score = 0;
    bool running = true;
    int level = 1;
    
    // Initial setup
    clear_screen();
    hide_cursor();
    
    BlockPiece current_piece = spawn_new_piece();
    BlockPiece next_piece = spawn_new_piece(); // Generate the next piece
    image = create_image(current_piece);
    
    auto last_fall = chrono::steady_clock::now();
    const int base_fall_time = 800; // milliseconds
    
    while (running) {
        const auto frame_start = chrono::high_resolution_clock::now();
        
        // Handle input
        if (_kbhit()) {
            handle_kb_event(screen, current_piece, image, running);
            if (!running) break;
        }
        
        // Auto-fall logic
        auto now = chrono::steady_clock::now();
        int fall_interval = max(50, base_fall_time - (level - 1) * 50);
        
        if (chrono::duration_cast<chrono::milliseconds>(now - last_fall).count() >= fall_interval) {
            if (!block_fall(screen, current_piece, image)) {
                // Piece has landed, check for completed lines
                check_for_score(screen, score);
                level = score / 250 + 1;
                
                // Move next piece to current and generate new next piece
                current_piece = next_piece;
                next_piece = spawn_new_piece();
                image = create_image(current_piece);
                
                // Check game over
                if (!can_move_piece(current_piece, screen)) {
                    running = false;
                    break;
                }
            }
            last_fall = now;
        }
        
        // Render
        display_board(screen, image, current_piece, next_piece, score, level);
        
        // Maintain ~60 FPS
        const auto frame_end = chrono::high_resolution_clock::now();
        const auto elapsed = chrono::duration_cast<chrono::milliseconds>(frame_end - frame_start);
        const int target_frame_time = 16; // ~60 FPS
        
        const int sleep_time = target_frame_time - static_cast<int>(elapsed.count());
        if (sleep_time > 0) {
            precise_sleep(sleep_time);
        }
    }
    
    // Game over screen
    cout << "\n\n" << BOLD << "GAME OVER!" << RESET << "\n";
    cout << "Final Score: " << score << "\n";
    cout << "Level Reached: " << level << "\n";
    cout << "Press any key to exit...\n";
    show_cursor();
    _getch();
}

int main() {
    // Some Setting Checks
    if (COL < 5){
        cerr << "Error: Screen width must be at least 5 characters.\n";
        return 1;
    } else if (ROW < 3) {
        cerr << "Error: Screen height must be at least 5 characters.\n";
        return 1;
    }

    enable_ansi_support();
    game_loop();
    return 0;
}