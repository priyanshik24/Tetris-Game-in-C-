#include<iostream>
#include<vector>
#include<windows.h>
#include<conio.h>
#include<ctime>
#include<sstream>
#include<algorithm>
#include<fstream>

using namespace std;

// Define if not already defined
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x004 
#endif

// Game Board
const int WIDTH = 15;
const int HEIGHT = 20;

// Tetromino Shape 
vector<vector<vector<int>>> tetrominoes = {
    {{1, 1, 1, 1}},               // I
    {{1, 1}, {1, 1}},             // O
    {{0, 1, 0}, {1, 1, 1}},       // T
    {{0, 1, 1}, {1, 1, 0}},       // Z
    {{1, 1, 0}, {0, 1, 1}},       // S
    {{1, 0, 0}, {1, 1, 1}},       // J
    {{0, 0, 1}, {1, 1, 1}}        // L
};

class Tetris{
    private:
        vector<vector<int>> grid;   // grid holds locked blocks' color index, -1 means empty
        int score, level, highScore;
        int fallInterval;   // ms delay b/w automatic falls
        pair<int,int> pos;   // top-left position of active piece
        vector<vector<int>> curPiece;
        int curColor;   // index for active piece's color
        HANDLE hConsole;
        bool gameOver;
        bool lifelineUsed = false;
        vector<string> colors;  // tetromino colors string

        vector<vector<int>> nextPiece;
        int nextColor;

        // Generates random next piece
        void generateNextPiece(){
            int index = rand() % tetrominoes.size();
            nextPiece = tetrominoes[index];
            nextColor = index;
        }

    public:
        Tetris() : gameOver(false){
            grid = vector<vector<int>>(HEIGHT, vector<int>(WIDTH, -1));
            score = 0;
            level = 1;
            fallInterval = 500;
            hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            highScore = loadHighScore();

            // Hide the console cursor
            CONSOLE_CURSOR_INFO cursorInfo;
            GetConsoleCursorInfo( hConsole, &cursorInfo );
            cursorInfo.bVisible = false;
            SetConsoleCursorInfo( hConsole, &cursorInfo );

            // color string using emojis
            colors = {"🔴", "🟠", "🟡", "🟢", "🔵", "🟣", "⚪"};

            // Generate next piece than spawn current piece
            generateNextPiece();
            spawnPiece();
        }

        // Use Pre-generated next piece as current piece than generate next piece
        void spawnPiece() {
            curPiece = nextPiece;  // Use pre-generated next piece
            curColor = nextColor;
            pos.first = 0;
            pos.second = WIDTH / 2 - (int)curPiece[0].size() / 2;
        
            // Check for game over condition
            if (!isValidMove(pos.first, pos.second, curPiece)) {
                gameOver = true;
                Beep(400, 500);  // Game over sound
            }
        
            // Generate next piece only *after* placing the current one
            generateNextPiece();
        }
        
        bool isValidMove (int newRow, int newCol, const vector<vector<int>> &piece){
            for (int i = 0; i < piece.size(); i++){
                for (int j = 0; j < piece[i].size(); j++){
                    if (piece[i][j]){
                        int r = newRow + i;
                        int c = newCol + j;
                        if (r < 0 || r >= HEIGHT || c < 0 || c >= WIDTH || grid[r][c] != -1){
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        // Rotate active piece clockwise
        vector<vector<int>> rotatePieceMatrix(const vector<vector<int>> &piece){
            int rows = piece.size();
            int cols = piece[0].size();
            vector<vector<int>> rotated(cols, vector<int>(rows, 0));
            for (int i = 0; i < rows; i++){
                for (int j = 0; j < cols; j++){
                    rotated[j][rows - 1 - i] = piece[i][j];
                }
            }
            return rotated;
        }

        void rotatePiece(){
            vector<vector<int>> rotated = rotatePieceMatrix(curPiece);
            int newHeight = rotated.size();
            int newWidth = rotated[0].size();
            int newRow = pos.first;
            int newCol = pos.second;

            if (newRow + newHeight > HEIGHT)
                newRow = HEIGHT - newHeight;
            if (newRow < 0)
                newRow = 0;
            if (newCol + newWidth > WIDTH)
                newCol = WIDTH - newWidth;
            if (newCol < 0)
                newCol = 0;
            if (isValidMove(newRow, newCol, rotated)){
                pos.first = newRow;
                pos.second = newCol;
                curPiece = rotated;
                Beep(500,100);   // Rotation sound
            }
        }

        void movePiece (int dx){
            if (isValidMove(pos.first, pos.second + dx, curPiece)){
                pos.second += dx;
                Beep(300, 50);   // Move sound
            }
        }

        bool moveDown(){
            if (isValidMove(pos.first + 1, pos.second, curPiece)){
                pos.first++;
                return true;
            }
            return false;
        }

        void dropPiece(){
            while (isValidMove(pos.first + 1, pos.second, curPiece)){
                pos.first++;
            }
            Beep(600, 100);   // Hard drop sound
        }

        void lockPiece(){
            for (int i = 0; i < curPiece.size(); i++){
                for (int j = 0; j < curPiece[i].size(); j++){
                    if (curPiece[i][j] && pos.first + i >= 0){
                        grid[pos.first + i][pos.second + j] = curColor;
                    }
                }
            }
            clearLines();
            spawnPiece();
        }

        void clearLines(){
            int LinesCleared = 0;
            for (int i = HEIGHT - 1; i >= 0; i--){
                bool full = true;
                for (int j = 0; j < WIDTH; j++){
                    if (grid[i][j] == -1){
                        full = false;
                        break;
                    }
                }
                if (full){
                    LinesCleared++;
                    grid.erase(grid.begin() + i);
                    grid.insert(grid.begin(), vector<int>(WIDTH, -1));
                    i++;   // Re-check same row after shifting
                    Beep(800, 100);   // Line clear sound
                }
            }

            score += LinesCleared * 100;
            if (score >= level * 300){
                level++;
                fallInterval = max(100, fallInterval - 100);
                Beep(1000, 150);   // Level up sound
            }
        }

        int loadHighScore() {
            ifstream file("highscore.txt");
            int hs = 0;
            if (file) file >> hs;
            file.close();
            return hs;
        }
    
        void saveHighScore() {
            if (score > highScore) {
                highScore = score;
                ofstream file("highscore.txt");
                file << highScore;
                cout<<"New Highscore Achieved🥳"<<"\n";
                file.close();
            }
        }

        void useLifeline() {
            for (int i = HEIGHT - 1; i >= HEIGHT - 3; i--) {
                for (int j = 0; j < WIDTH; j++)
                    grid[i][j] = 0;
            }
            Beep(1200, 200);  
        }

        bool isGameOver(){
            return gameOver;
        }

        // Draw() builds complete frame (grid with borders, ghost piece, score/level, next piece preview)
        void drawBoard(){
            // Calculate ghost piece position
            pair<int,int> GhostPos = pos;
            while (isValidMove(GhostPos.first + 1, GhostPos.second, curPiece)){
                GhostPos.first++;
            }

            ostringstream frame;
            // Top border
            frame << "+";
            for (int j = 0; j < WIDTH * 2; j++)
                frame << "-";
            frame << "+\n";

            // Grid rows
            for (int i = 0; i < HEIGHT; i++){
                frame << "|";
                for (int j = 0; j < WIDTH; j++){
                    bool activeCell = false;
                    bool ghostCell = false;
                    int cellColor = -1;
                    for (int pi = 0; pi < curPiece.size(); pi++){
                        for (int pj = 0; pj < curPiece[pi].size(); pj++){
                            if (curPiece[pi][pj]){
                                if (pos.first + pi == i && pos.second + pj == j){
                                    activeCell = true;
                                    cellColor = curColor;
                                }
                                if (GhostPos.first + pi == i && GhostPos.second + pj == j){
                                    ghostCell = true;
                                }
                            }
                        }
                    }
                    if (activeCell)
                        frame << colors[cellColor];
                    else if (grid[i][j] != -1)
                        frame << colors[grid[i][j]];
                    else if (ghostCell)
                        frame << "\033[2m" << colors[curColor] << "\033[22m";
                    else
                        frame << "\033[48;5;235m  \033[0m";
                }
                frame << "|\n";
            }

            // Bottom border
            frame << "+";
            for (int j = 0; j < WIDTH * 2; j++){
                frame << "-";
            }
            frame << "+\n";

            // Score, Level and lifeline
            frame << "Score : " << score << "   " << "Level : " << level << "   " << "Highscore : " << highScore << "\n";
            frame << "Press 'L' to use lifeline \n";
            frame << "Lifeline Available : " << (lifelineUsed ? "NOPE" : "YEP❤️") << "\n";

            // Next piece preview
            frame << "\nNext Piece :\n";
            if (!nextPiece.empty()){
                frame << "+";
                for (int j = 0; j < nextPiece[0].size() * 2; j++){
                    frame << "-";
                }
                frame << "+\n";
                for (int i = 0; i < nextPiece.size(); i++){
                    frame << "|";
                    for (int j = 0; j < nextPiece[i].size(); j++){
                        if (nextPiece[i][j]){
                            frame << colors[nextColor];
                        }
                        else{
                            frame << "  ";
                        }
                    }
                    frame << "|\n";
                }
                frame << "+";
                for (int j = 0; j < nextPiece[0].size() * 2; j++){
                    frame << "-";
                }
                frame << "+\n";
            }

            SetConsoleCursorPosition(hConsole, {0,0});
            cout << frame.str();
        }

        void handleInput(){
            if (_kbhit()){
                char key = _getch();
                if (key == 0 || key == -32){   // arrow keys
                    key = _getch();
                    if (key == 75)   // left key
                        movePiece(-1);
                    else if (key == 77)   // right key
                        movePiece(1);
                    else if (key == 80){    // down key
                        if (!moveDown()){
                            lockPiece();
                        }
                    }
                    else if (key == 72)   // up key
                        rotatePiece();
                }
                else if (key == 27){    // esc key
                    exit(0);
                }
                else if (key == 32){    // hard drop - space bar
                    dropPiece();
                    lockPiece();
                }
                else if (key == 'l' && !lifelineUsed){
                    useLifeline();
                    lifelineUsed = true;
                }
            }
        }

        void play(){
            DWORD lastFall = GetTickCount();
            const int frameDelay = 16;    // 60 FPS
            while (!isGameOver()){
                handleInput();
                DWORD now = GetTickCount();
                if (now - lastFall >= (DWORD)fallInterval){
                    if (!moveDown())
                        lockPiece();
                    lastFall = now;
                }
                drawBoard();
                Sleep(frameDelay);
            }
            drawBoard();
            saveHighScore();
            cout << "\nGame Over😢! Final Score : " << score << "\n";
        }
};

int main(){
    srand((unsigned int)time(0));
    SetConsoleOutputCP(CP_UTF8);   // enable UTF8 o/p
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);   // enable ANSI escape codes
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    char choice;
    do{
        Tetris game;
        game.play();
        cout << "\nPress R to restart, ESC to exit...";
        while(true){
            if (_kbhit()){
                char key = _getch();
                if (key == 'r' || key == 'R'){
                    choice = 'r';
                    break;
                }
                else if(key == 27){    // ESC key
                    choice = 27;
                    break;
                }
            }
        }
    }while (choice == 'r');
    return  0;
}
