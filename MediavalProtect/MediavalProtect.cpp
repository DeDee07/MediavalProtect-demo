#include "raylib.h"
#include <vector>
#include <cmath>
#include <raymath.h>

const int TILE_SIZE = 40;
const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 15;

struct Enemy {
    Vector2 position;
    float speed;
    int health;
    int pathIndex;
    bool isShooter;
    float shootCooldown;
};

struct Tower {
    Vector2 position;
    float range;
    float fireCooldown;
    float fireRate;
    int health;
};

struct Bullet {
    Vector2 position;
    Vector2 velocity;
    float speed;
};

int map[MAP_HEIGHT][MAP_WIDTH] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
    {0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

std::vector<Vector2> path = {
    {1 * TILE_SIZE + TILE_SIZE / 2, 1 * TILE_SIZE + TILE_SIZE / 2},
    {18 * TILE_SIZE + TILE_SIZE / 2, 1 * TILE_SIZE + TILE_SIZE / 2},
    {18 * TILE_SIZE + TILE_SIZE / 2, 13 * TILE_SIZE + TILE_SIZE / 2},
    {1 * TILE_SIZE + TILE_SIZE / 2, 13 * TILE_SIZE + TILE_SIZE / 2},
    {1 * TILE_SIZE + TILE_SIZE / 2, 14 * TILE_SIZE + TILE_SIZE / 2}
};

Vector2 MoveTowards(Vector2 current, Vector2 target, float maxDistance) {
    Vector2 delta = Vector2Subtract(target, current);
    float distance = Vector2Length(delta);
    if (distance <= maxDistance || distance == 0.0f)
        return target;
    return Vector2Add(current, Vector2Scale(Vector2Normalize(delta), maxDistance));
}

bool CanBuildTowerAt(Vector2 pos, int map[MAP_HEIGHT][MAP_WIDTH], const std::vector<Tower>& towers) {
    int x = (int)(pos.x / TILE_SIZE);
    int y = (int)(pos.y / TILE_SIZE);
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT || map[y][x] != 0) return false;
    for (const auto& t : towers) {
        if ((int)(t.position.x / TILE_SIZE) == x && (int)(t.position.y / TILE_SIZE) == y) return false;
    }
    return true;
}

enum GameState {
    MENU,
    OPTIONS,
    PLAYING,
    GAMEOVER,
    EXIT
};

void ResetGame(std::vector<Enemy>& enemies, std::vector<Tower>& towers, std::vector<Bullet>& bullets, int& coins, int& playerHP, float& enemySpawnTimer, float& shooterSpawnTimer, bool& paused) {
    enemies.clear();
    towers.clear();
    bullets.clear();
    enemySpawnTimer = 0.0f;
    shooterSpawnTimer = 0.0f;
    coins = 200;
    playerHP = 200;
    paused = false;
}

int main() {
    InitWindow(MAP_WIDTH * TILE_SIZE, MAP_HEIGHT * TILE_SIZE, "Tower Defense");
    SetTargetFPS(60);

    GameState gameState = MENU;
    int menuSelection = 0;
    int gameOverSelection = 0;
    const int menuOptionsCount = 3;
    const int gameOverOptionsCount = 2;

    std::vector<Enemy> enemies;
    std::vector<Tower> towers;
    std::vector<Bullet> bullets;

    float enemySpawnTimer = 0.0f;
    float shooterSpawnTimer = 0.0f;
    int coins = 200;
    int playerHP = 200;
    bool paused = false;

    while (!WindowShouldClose() && gameState != EXIT) {
        float delta = GetFrameTime();

        switch (gameState) {
        case MENU: {
            if (IsKeyPressed(KEY_DOWN)) {
                menuSelection = (menuSelection + 1) % menuOptionsCount;
            }
            if (IsKeyPressed(KEY_UP)) {
                menuSelection = (menuSelection - 1 + menuOptionsCount) % menuOptionsCount;
            }
            if (IsKeyPressed(KEY_ENTER)) {
                if (menuSelection == 0) { // Jogar
                    ResetGame(enemies, towers, bullets, coins, playerHP, enemySpawnTimer, shooterSpawnTimer, paused);
                    gameState = PLAYING;
                }
                else if (menuSelection == 1) { // Opções
                    gameState = OPTIONS;
                }
                else if (menuSelection == 2) { // Sair
                    gameState = EXIT;
                }
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawText("Tower Defense", GetScreenWidth() / 2 - MeasureText("Tower Defense", 40) / 2, 80, 40, DARKGREEN);

            const char* menuOptions[3] = { "Jogar", "Opções", "Sair" };
            for (int i = 0; i < menuOptionsCount; i++) {
                Color color = (i == menuSelection) ? RED : DARKGRAY;
                DrawText(menuOptions[i], GetScreenWidth() / 2 - MeasureText(menuOptions[i], 30) / 2, 200 + i * 50, 30, color);
            }

            EndDrawing();
            break;
        }
        case OPTIONS: {
            if (IsKeyPressed(KEY_ESCAPE)) {
                gameState = MENU;
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawText("Opções", GetScreenWidth() / 2 - MeasureText("Opções", 40) / 2, 80, 40, DARKGREEN);
            DrawText("Aqui podem ser adicionadas opções.", 50, 200, 20, DARKGRAY);
            DrawText("Pressione ESC para voltar.", 50, 230, 20, DARKGRAY);

            EndDrawing();
            break;
        }
        case PLAYING: {
            if (IsKeyPressed(KEY_ESCAPE)) paused = !paused;

            if (!paused) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePos = GetMousePosition();
                    if (coins >= 50 && CanBuildTowerAt(mousePos, map, towers)) {
                        int tileX = (int)(mousePos.x / TILE_SIZE);
                        int tileY = (int)(mousePos.y / TILE_SIZE);
                        Vector2 towerPos = { tileX * TILE_SIZE + TILE_SIZE / 2.0f, tileY * TILE_SIZE + TILE_SIZE / 2.0f };
                        towers.push_back(Tower{ towerPos, 100.0f, 0.0f, 1.0f, 10 });
                        coins -= 50;
                    }
                }

                enemySpawnTimer += delta;
                if (enemySpawnTimer >= 2.5f) {
                    enemies.push_back(Enemy{ path[0], 60.0f, 3, 1, false, 0.0f });
                    enemySpawnTimer = 0;
                }

                shooterSpawnTimer += delta;
                if (shooterSpawnTimer >= 3.0f) {
                    enemies.push_back(Enemy{ path[0], 50.0f, 5, 1, true, 0.0f });
                    shooterSpawnTimer = 0;
                }

                for (auto& e : enemies) {
                    if (e.pathIndex < (int)path.size()) {
                        Vector2 target = path[e.pathIndex];
                        e.position = MoveTowards(e.position, target, e.speed * delta);
                        if (Vector2Distance(e.position, target) < 2.0f) {
                            e.pathIndex++;
                        }
                    }
                }

                for (auto& tower : towers) {
                    tower.fireCooldown -= delta;
                    if (tower.fireCooldown <= 0) {
                        Enemy* targetEnemy = nullptr;
                        float closestDist = tower.range + 1;
                        for (auto& e : enemies) {
                            float dist = Vector2Distance(tower.position, e.position);
                            if (dist <= tower.range && dist < closestDist) {
                                closestDist = dist;
                                targetEnemy = &e;
                            }
                        }
                        if (targetEnemy != nullptr) {
                            Vector2 direction = Vector2Normalize(Vector2Subtract(targetEnemy->position, tower.position));
                            bullets.push_back(Bullet{ tower.position, direction, 400.0f });
                            tower.fireCooldown = tower.fireRate;
                        }
                    }
                }

                for (int i = 0; i < (int)bullets.size(); i++) {
                    bullets[i].position.x += bullets[i].velocity.x * bullets[i].speed * delta;
                    bullets[i].position.y += bullets[i].velocity.y * bullets[i].speed * delta;

                    bool bulletRemoved = false;
                    for (int j = 0; j < (int)enemies.size(); j++) {
                        if (Vector2Distance(bullets[i].position, enemies[j].position) < 15) {
                            enemies[j].health--;
                            bullets.erase(bullets.begin() + i);
                            i--;
                            bulletRemoved = true;
                            if (enemies[j].health <= 0) {
                                enemies.erase(enemies.begin() + j);
                                coins += 20;
                            }
                            break;
                        }
                    }

                    if (bulletRemoved) continue;

                    if (bullets[i].position.x < 0 || bullets[i].position.x > GetScreenWidth() ||
                        bullets[i].position.y < 0 || bullets[i].position.y > GetScreenHeight()) {
                        bullets.erase(bullets.begin() + i);
                        i--;
                    }
                }

                for (int i = 0; i < (int)enemies.size(); i++) {
                    if (enemies[i].pathIndex >= (int)path.size()) {
                        playerHP -= 20;
                        enemies.erase(enemies.begin() + i);
                        i--;
                    }
                }
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);

            for (int y = 0; y < MAP_HEIGHT; y++) {
                for (int x = 0; x < MAP_WIDTH; x++) {
                    Color c = map[y][x] == 1 ? BROWN : GREEN;
                    DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, c);
                }
            }

            for (size_t i = 1; i < path.size(); i++) {
                DrawLineV(path[i - 1], path[i], BLUE);
            }

            for (const auto& tower : towers) {
                DrawCircleV(tower.position, TILE_SIZE / 2.0f - 4, DARKGREEN);
                DrawCircleLines((int)tower.position.x, (int)tower.position.y, tower.range, GREEN);
                DrawText(TextFormat("%d", tower.health), tower.position.x - 10, tower.position.y - 30, 12, BLACK);
            }

            for (const auto& e : enemies) {
                if (e.isShooter)
                   DrawCircleV(e.position, TILE_SIZE / 3.0f, DARKGREEN);
                else
                    DrawCircleV(e.position, TILE_SIZE / 3.0f, RED);
                DrawText(TextFormat("%d", e.health), e.position.x - 10, e.position.y - 20, 12, BLACK);
            }

            for (const auto& b : bullets) {
                DrawCircleV(b.position, 5, BLACK);
            }

            DrawText(TextFormat("Coins: %d", coins), 10, 10, 20, GOLD);
            DrawText(TextFormat("HP: %d", playerHP), 10, 40, 20, RED);

            if (paused) {
                DrawText("PAUSADO", GetScreenWidth() / 2 - MeasureText("PAUSADO", 40) / 2, GetScreenHeight() / 2, 40, DARKGRAY);
            }

            EndDrawing();

            if (playerHP <= 0) {
                gameState = GAMEOVER;
                gameOverSelection = 0;
            }

            break;
        }
        case GAMEOVER: {
            if (IsKeyPressed(KEY_DOWN)) {
                gameOverSelection = (gameOverSelection + 1) % gameOverOptionsCount;
            }
            if (IsKeyPressed(KEY_UP)) {
                gameOverSelection = (gameOverSelection - 1 + gameOverOptionsCount) % gameOverOptionsCount;
            }
            if (IsKeyPressed(KEY_ENTER)) {
                if (gameOverSelection == 0) { // Jogar novamente
                    ResetGame(enemies, towers, bullets, coins, playerHP, enemySpawnTimer, shooterSpawnTimer, paused);
                    gameState = PLAYING;
                }
                else if (gameOverSelection == 1) { // Voltar ao menu
                    gameState = MENU;
                }
            }

            BeginDrawing();
            ClearBackground(DARKGREEN);

            DrawText("GAME OVER", GetScreenWidth() / 2 - MeasureText("GAME OVER", 50) / 2, 100, 50, RED);

            const char* gameOverOptions[2] = { "Jogar novamente", "Voltar ao menu" };
            for (int i = 0; i < gameOverOptionsCount; i++) {
                Color color = (i == gameOverSelection) ? YELLOW : GRAY;
                DrawText(gameOverOptions[i], GetScreenWidth() / 2 - MeasureText(gameOverOptions[i], 30) / 2, 250 + i * 50, 30, color);
            }

            EndDrawing();

            break;
        }
        }
    }

    CloseWindow();
    return 0;
}
