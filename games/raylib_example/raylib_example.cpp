#include <raylib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// Global variables for raylib example
static const int screenWidth = 800;
static const int screenHeight = 450;
static int circleX = screenWidth / 2;
static int circleY = screenHeight / 2;
static Color circleColor = RED;

// Update and Draw function
void UpdateDrawFrame(void) {
    // Update
    if (IsKeyDown(KEY_RIGHT)) circleX += 2;
    if (IsKeyDown(KEY_LEFT)) circleX -= 2;
    if (IsKeyDown(KEY_UP)) circleY -= 2;
    if (IsKeyDown(KEY_DOWN)) circleY += 2;

    if (IsKeyPressed(KEY_R)) circleColor = RED;
    if (IsKeyPressed(KEY_G)) circleColor = GREEN;
    if (IsKeyPressed(KEY_B)) circleColor = BLUE;
    if (IsKeyPressed(KEY_Y)) circleColor = YELLOW;


    // Draw
    BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("move circle with arrow keys", 10, 10, 20, DARKGRAY);
        DrawText("press R, G, B, Y to change color", 10, 30, 20, DARKGRAY);
        DrawCircle(circleX, circleY, 50, circleColor);
    EndDrawing();
}

int main(void) {
    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60); // Set our game to run at 60 frames-per-second
    // Main game loop
    while (!WindowShouldClose()) { // Detect window close button or ESC key
        UpdateDrawFrame();
    }
#endif

    CloseWindow(); // Close window and unload OpenGL context

    return 0;
}
