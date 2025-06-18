#include "raylib.h"
#include <vector>
#include <cmath>
#include <raymath.h>

// Constantes do jogo
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
    bool fromEnemy;
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

bool CanBuildTowerAt(Vector2 pos, const std::vector<Tower>& towers) {
    int x = (int)(pos.x / TILE_SIZE);
    int y = (int)(pos.y / TILE_SIZE);
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT || map[y][x] != 0) return false;
    for (const auto& t : towers) {
        if ((int)(t.position.x / TILE_SIZE) == x && (int)(t.position.y / TILE_SIZE) == y) return false;
    }
    return true;
}

enum GameState { MENU, OPTIONS, PLAYING, GAMEOVER, EXIT };

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
    int menuSelection = 0, gameOverSelection = 0;
    const char* menuOptions[3] = { "Jogar", "Opções", "Sair" };
    const char* gameOverOptions[2] = { "Jogar novamente", "Voltar ao menu" };

    std::vector<Enemy> enemies;
    std::vector<Tower> towers;
    std::vector<Bullet> bullets;

    float enemySpawnTimer = 0, shooterSpawnTimer = 0;
    int coins = 200, playerHP = 200;
    bool paused = false;

    while (!WindowShouldClose() && gameState != EXIT) {
        float delta = GetFrameTime();

        switch (gameState) {
        case MENU: {
            if (IsKeyPressed(KEY_DOWN)) menuSelection = (menuSelection + 1) % 3;
            if (IsKeyPressed(KEY_UP)) menuSelection = (menuSelection - 1 + 3) % 3;
            if (IsKeyPressed(KEY_ENTER)) {
                if (menuSelection == 0) { ResetGame(enemies, towers, bullets, coins, playerHP, enemySpawnTimer, shooterSpawnTimer, paused); gameState = PLAYING; }
                else if (menuSelection == 1) gameState = OPTIONS;
                else if (menuSelection == 2) gameState = EXIT;
            }

            BeginDrawing();
            ClearBackground(DARKGREEN);
            DrawText("Tower Defense", GetScreenWidth() / 2 - MeasureText("Tower Defense", 40) / 2, 80, 40, YELLOW);
            for (int i = 0; i < 3; i++)
                DrawText(menuOptions[i], GetScreenWidth() / 2 - MeasureText(menuOptions[i], 30) / 2, 200 + i * 50, 30, (i == menuSelection) ? RED : DARKGRAY);
            EndDrawing();
            break;
        }
        case OPTIONS: {
            if (IsKeyPressed(KEY_ESCAPE)) gameState = MENU;
            BeginDrawing();
            ClearBackground(DARKGREEN);
            DrawText("Opções", GetScreenWidth() / 2 - MeasureText("Opções", 40) / 2, 80, 40, YELLOW);
            DrawText("Futuras opções aqui", 50, 200, 20, RAYWHITE);
            EndDrawing();
            break;
        }
        case PLAYING: {
            if (IsKeyPressed(KEY_ESCAPE)) paused = !paused;

            if (!paused) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    Vector2 mouse = GetMousePosition();
                    if (coins >= 50 && CanBuildTowerAt(mouse, towers)) {
                        int tileX = (int)(mouse.x / TILE_SIZE), tileY = (int)(mouse.y / TILE_SIZE);
                        Vector2 towerPos = { tileX * TILE_SIZE + TILE_SIZE / 2.0f, tileY * TILE_SIZE + TILE_SIZE / 2.0f };
                        towers.push_back({ towerPos, 100.0f, 0.0f, 1.0f, 10 });
                        coins -= 50;
                    }
                }

                enemySpawnTimer += delta;
                if (enemySpawnTimer >= 2.5f) {
                    enemies.push_back({ path[0], 60.0f, 3, 1, false, 0.0f });
                    enemySpawnTimer = 0;
                }

                shooterSpawnTimer += delta;
                if (shooterSpawnTimer >= 3.0f) {
                    enemies.push_back({ path[0], 50.0f, 5, 1, true, 0.0f });
                    shooterSpawnTimer = 0;
                }

                for (auto& e : enemies) {
                    if (e.pathIndex < (int)path.size()) {
                        Vector2 target = path[e.pathIndex];
                        e.position = MoveTowards(e.position, target, e.speed * delta);
                        if (Vector2Distance(e.position, target) < 2.0f) e.pathIndex++;
                    }

                    e.shootCooldown -= delta;
                    if (e.isShooter && e.shootCooldown <= 0) {
                        for (auto& t : towers) {
                            if (Vector2Distance(e.position, t.position) < 100) {
                                Vector2 dir = Vector2Normalize(Vector2Subtract(t.position, e.position));
                                bullets.push_back({ e.position, dir, 150.0f, true });
                                e.shootCooldown = 1.5f;
                                break;
                            }
                        }
                    }
                }

                for (auto& t : towers) {
                    t.fireCooldown -= delta;
                    if (t.fireCooldown <= 0) {
                        Enemy* target = nullptr;
                        float closest = t.range + 1;
                        for (auto& e : enemies) {
                            float dist = Vector2Distance(t.position, e.position);
                            if (dist <= t.range && dist < closest) {
                                closest = dist;
                                target = &e;
                            }
                        }
                        if (target) {
                            Vector2 dir = Vector2Normalize(Vector2Subtract(target->position, t.position));
                            bullets.push_back({ t.position, dir, 400.0f, false });
                            t.fireCooldown = t.fireRate;
                        }
                    }
                }

                for (int i = 0; i < (int)bullets.size(); i++) {
                    bullets[i].position = Vector2Add(bullets[i].position, Vector2Scale(bullets[i].velocity, bullets[i].speed * delta));
                    bool hit = false;

                    if (bullets[i].fromEnemy) {
                        for (int j = 0; j < (int)towers.size(); j++) {
                            if (Vector2Distance(bullets[i].position, towers[j].position) < 15) {
                                towers[j].health--;
                                if (towers[j].health <= 0) towers.erase(towers.begin() + j);
                                hit = true;
                                break;
                            }
                        }
                    }
                    else {
                        for (int j = 0; j < (int)enemies.size(); j++) {
                            if (Vector2Distance(bullets[i].position, enemies[j].position) < 15) {
                                enemies[j].health--;
                                if (enemies[j].health <= 0) { coins += 20; enemies.erase(enemies.begin() + j); }
                                hit = true;
                                break;
                            }
                        }
                    }

                    if (hit || bullets[i].position.x < 0 || bullets[i].position.y < 0 || bullets[i].position.x > GetScreenWidth() || bullets[i].position.y > GetScreenHeight()) {
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
            ClearBackground(DARKGREEN);

            for (int y = 0; y < MAP_HEIGHT; y++)
                for (int x = 0; x < MAP_WIDTH; x++)
                    DrawRectangle(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, map[y][x] == 1 ? BROWN : GREEN);

            for (auto& t : towers) {
                DrawCircleV(t.position, TILE_SIZE / 2 - 4, DARKBLUE);
                DrawCircleLines(t.position.x, t.position.y, t.range, BLUE);
            }

            for (auto& e : enemies) {
                DrawCircleV(e.position, TILE_SIZE / 3.0f, e.isShooter ? DARKGREEN : RED);
            }

            for (auto& b : bullets)
                DrawCircleV(b.position, 5, b.fromEnemy ? MAROON : BLACK);

            DrawText(TextFormat("HP: %d", playerHP), 10, 10, 20, RED);
            DrawText(TextFormat("Coins: %d", coins), 10, 40, 20, GOLD);

            if (paused) DrawText("PAUSADO", GetScreenWidth() / 2 - 100, GetScreenHeight() / 2, 40, YELLOW);

            EndDrawing();

            if (playerHP <= 0) { gameState = GAMEOVER; gameOverSelection = 0; }
            break;
        }
        case GAMEOVER: {
            if (IsKeyPressed(KEY_DOWN)) gameOverSelection = (gameOverSelection + 1) % 2;
            if (IsKeyPressed(KEY_UP)) gameOverSelection = (gameOverSelection - 1 + 2) % 2;
            if (IsKeyPressed(KEY_ENTER)) {
                if (gameOverSelection == 0) { ResetGame(enemies, towers, bullets, coins, playerHP, enemySpawnTimer, shooterSpawnTimer, paused); gameState = PLAYING; }
                else gameState = MENU;
            }

            BeginDrawing();
            ClearBackground(DARKGREEN);
            DrawText("GAME OVER", GetScreenWidth() / 2 - MeasureText("GAME OVER", 50) / 2, 100, 50, RED);
            for (int i = 0; i < 2; i++)
                DrawText(gameOverOptions[i], GetScreenWidth() / 2 - MeasureText(gameOverOptions[i], 30) / 2, 250 + i * 50, 30, (i == gameOverSelection) ? YELLOW : GRAY);
            EndDrawing();
            break;
        }
        }
    }

    CloseWindow();
    return 0;
}
