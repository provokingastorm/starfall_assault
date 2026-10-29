#include "raylib.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Window dimensions, storage limits, and encounter structure. */
enum { screen_width = 960 };
enum { screen_height = 720 };
static const int target_frames_per_second = 60;
enum { maximum_enemies = 24 };
enum { maximum_player_bullets = 128 };
enum { maximum_enemy_bullets = 256 };
enum { maximum_pickups = 24 };
enum { maximum_particles = 256 };
enum { star_count = 110 };
enum { maximum_boss_health = 360 };
enum { initial_player_health = 3 };
enum { initial_bomb_count = 1 };
enum { wave_count = 3 };
static const int wave_enemy_counts[wave_count] = {8, 10, 12};

/* Gameplay tuning. Keep these named constants together rather than scattering numbers. */
static const float player_speed = 350.0f;
static const float player_focus_speed = 170.0f;
static const float player_fire_interval = 0.14f;
static const float player_bullet_speed = 690.0f;
static const float player_invulnerability_duration = 1.35f;
static const float enemy_spawn_interval = 0.62f;
static const float enemy_bullet_speed = 205.0f;
static const float enemy_fire_interval = 1.7f;
static const float pickup_fall_speed = 115.0f;
static const float wave_break_duration = 1.8f;
static const float boss_entry_speed = 125.0f;
static const float boss_drift_speed = 90.0f;
static const float boss_phase_duration = 9.0f;
static const float boss_aimed_shot_interval = 0.72f;
static const float boss_radial_shot_interval = 1.0f;
static const float boss_fan_shot_interval = 0.48f;
static const float player_collision_radius = 13.0f;
static const float enemy_collision_radius = 19.0f;
static const float pickup_collision_radius = 21.0f;
static const float boss_collision_radius = 62.0f;
static const float boss_bullet_speed = 235.0f;
static const float screen_shake_decay = 28.0f;
static const unsigned int audio_sample_rate = 22050;
static const float enemy_vertical_speed = 60.0f;
static const float enemy_horizontal_speed = 58.0f;
static const float enemy_edge_margin = 45.0f;
static const float enemy_offscreen_margin = 35.0f;
static const float enemy_initial_fire_minimum = 1.05f;
static const float enemy_initial_fire_random_range = 1.2f;
static const float enemy_fire_random_range = 0.8f;
static const float initial_spawn_delay = 0.65f;
static const float subsequent_wave_spawn_delay = 0.8f;
static const float boss_hover_height = 135.0f;
static const float boss_edge_margin = 150.0f;
static const int enemy_health = 2;
static const int enemy_score_value = 125;
static const int pickup_score_value = 50;
static const int boss_score_value = 5000;
static const int pickup_drop_chance_percent = 22;
static const int boss_bomb_damage = 100;
static const int boss_radial_bullet_count = 18;
static const int boss_fan_half_count = 3;
static const int player_bullet_radius = 4;
static const int enemy_bullet_radius = 6;
static const int boss_bullet_radius = 7;

/* Shared colors. */
static const Color background_color = {7, 10, 29, 255};
static const Color panel_color = {12, 18, 44, 242};
static const Color cyan_color = {90, 238, 255, 255};
static const Color player_color = {105, 218, 255, 255};
static const Color enemy_color = {255, 107, 138, 255};
static const Color boss_color = {204, 88, 255, 255};
static const Color gold_color = {255, 211, 100, 255};
static const Color muted_color = {151, 170, 210, 255};

typedef enum GameState {
    game_state_title,
    game_state_playing,
    game_state_paused,
    game_state_victory,
    game_state_game_over
} GameState;

typedef struct Player {
    Vector2 position;
    float fire_timer;
    float invulnerability_timer;
    int health;
    int bombs;
    bool focused;
    bool alive;
} Player;

typedef struct Enemy {
    Vector2 position;
    float horizontal_direction;
    float fire_timer;
    int health;
    bool active;
} Enemy;

typedef struct Bullet {
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool active;
} Bullet;

typedef struct Pickup {
    Vector2 position;
    bool active;
} Pickup;

typedef struct Particle {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    float maximum_lifetime;
    float size;
    Color color;
    bool active;
} Particle;

typedef struct Star {
    Vector2 position;
    float speed;
    float radius;
    unsigned char alpha;
} Star;

typedef struct Boss {
    Vector2 position;
    float phase_timer;
    float attack_timer;
    float flash_timer;
    float horizontal_direction;
    int health;
    int phase;
    bool active;
    bool defeated;
} Boss;

static Player player;
static Enemy enemies[maximum_enemies];
static Bullet player_bullets[maximum_player_bullets];
static Bullet enemy_bullets[maximum_enemy_bullets];
static Pickup pickups[maximum_pickups];
static Particle particles[maximum_particles];
static Star stars[star_count];
static Boss boss;
static GameState game_state = game_state_title;
static int current_wave;
static int enemies_spawned;
static int score;
static float spawn_timer;
static float transition_timer;
static float screen_shake;
static float simulation_time;
static Sound player_shot_sound;
static Sound explosion_sound;
static Sound pickup_sound;
static Sound boss_hit_sound;

static float clamp_value(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static Vector2 vector_add(Vector2 first, Vector2 second)
{
    return (Vector2){first.x + second.x, first.y + second.y};
}

static Vector2 vector_subtract(Vector2 first, Vector2 second)
{
    return (Vector2){first.x - second.x, first.y - second.y};
}

static Vector2 vector_scale(Vector2 value, float scale)
{
    return (Vector2){value.x * scale, value.y * scale};
}

static float vector_length(Vector2 value)
{
    return sqrtf(value.x * value.x + value.y * value.y);
}

static Vector2 vector_normalize(Vector2 value)
{
    float length = vector_length(value);
    if (length <= 0.0001f) return (Vector2){0.0f, 0.0f};
    return vector_scale(value, 1.0f / length);
}

static float distance_squared(Vector2 first, Vector2 second)
{
    float horizontal = first.x - second.x;
    float vertical = first.y - second.y;
    return horizontal * horizontal + vertical * vertical;
}

static bool circles_overlap(Vector2 first, float first_radius, Vector2 second, float second_radius)
{
    float combined_radius = first_radius + second_radius;
    return distance_squared(first, second) <= combined_radius * combined_radius;
}

static void add_particle_burst(Vector2 position, Color color, int count, float speed)
{
    for (int particle_index = 0; particle_index < maximum_particles && count > 0; particle_index++) {
        if (particles[particle_index].active) continue;
        float angle = (float)GetRandomValue(0, 359) * DEG2RAD;
        float particle_speed = speed * (0.35f + (float)GetRandomValue(0, 100) / 100.0f);
        float lifetime = 0.28f + (float)GetRandomValue(0, 100) / 180.0f;
        particles[particle_index] = (Particle){
            .position = position,
            .velocity = {cosf(angle) * particle_speed, sinf(angle) * particle_speed},
            .lifetime = lifetime,
            .maximum_lifetime = lifetime,
            .size = 1.5f + (float)GetRandomValue(0, 30) / 10.0f,
            .color = color,
            .active = true
        };
        count--;
    }
}

static void spawn_bullet(Bullet *bullet_array, int capacity, Vector2 position, Vector2 direction,
                         float speed, float radius)
{
    Vector2 velocity = vector_scale(vector_normalize(direction), speed);
    for (int bullet_index = 0; bullet_index < capacity; bullet_index++) {
        if (bullet_array[bullet_index].active) continue;
        bullet_array[bullet_index] = (Bullet){position, velocity, radius, true};
        return;
    }
}

static void initialize_stars(void)
{
    for (int star_index = 0; star_index < star_count; star_index++) {
        stars[star_index] = (Star){
            .position = {(float)GetRandomValue(0, screen_width), (float)GetRandomValue(0, screen_height)},
            .speed = 15.0f + (float)GetRandomValue(0, 100) * 0.7f,
            .radius = 0.7f + (float)GetRandomValue(0, 15) / 10.0f,
            .alpha = (unsigned char)GetRandomValue(75, 220)
        };
    }
}

static void initialize_run(void)
{
    memset(enemies, 0, sizeof(enemies));
    memset(player_bullets, 0, sizeof(player_bullets));
    memset(enemy_bullets, 0, sizeof(enemy_bullets));
    memset(pickups, 0, sizeof(pickups));
    memset(particles, 0, sizeof(particles));
    player = (Player){
        .position = {(float)screen_width * 0.5f, (float)screen_height - 94.0f},
        .health = initial_player_health,
        .bombs = initial_bomb_count,
        .alive = true
    };
    boss = (Boss){0};
    current_wave = 0;
    enemies_spawned = 0;
    score = 0;
    spawn_timer = initial_spawn_delay;
    transition_timer = 0.0f;
    screen_shake = 0.0f;
    simulation_time = 0.0f;
    game_state = game_state_playing;
}

static void spawn_enemy(void)
{
    for (int enemy_index = 0; enemy_index < maximum_enemies; enemy_index++) {
        if (enemies[enemy_index].active) continue;
        float lane = (float)(enemies_spawned % 5);
        float row = (float)((enemies_spawned / 5) % 2);
        enemies[enemy_index] = (Enemy){
            .position = {120.0f + lane * 180.0f, -32.0f - row * 44.0f},
            .horizontal_direction = (enemies_spawned % 2 == 0) ? 1.0f : -1.0f,
            .fire_timer = enemy_initial_fire_minimum + (float)GetRandomValue(0, (int)(enemy_initial_fire_random_range * 100.0f)) / 100.0f,
            .health = enemy_health,
            .active = true
        };
        enemies_spawned++;
        return;
    }
}

static void spawn_energy_pickup(Vector2 position)
{
    for (int pickup_index = 0; pickup_index < maximum_pickups; pickup_index++) {
        if (pickups[pickup_index].active) continue;
        pickups[pickup_index] = (Pickup){position, true};
        return;
    }
}

static void destroy_enemy(Enemy *enemy)
{
    enemy->active = false;
    score += enemy_score_value;
    screen_shake = fmaxf(screen_shake, 2.5f);
    add_particle_burst(enemy->position, enemy_color, 14, 115.0f);
    if (GetRandomValue(0, 99) < pickup_drop_chance_percent) spawn_energy_pickup(enemy->position);
    PlaySound(explosion_sound);
}

static void damage_player(void)
{
    if (player.invulnerability_timer > 0.0f || !player.alive) return;
    player.health--;
    player.invulnerability_timer = player_invulnerability_duration;
    screen_shake = 10.0f;
    add_particle_burst(player.position, player_color, 22, 160.0f);
    PlaySound(explosion_sound);
    if (player.health <= 0) {
        player.alive = false;
        game_state = game_state_game_over;
    }
}

static void activate_bomb(void)
{
    if (player.bombs <= 0) return;
    player.bombs--;
    screen_shake = 12.0f;
    for (int bullet_index = 0; bullet_index < maximum_enemy_bullets; bullet_index++) {
        enemy_bullets[bullet_index].active = false;
    }
    for (int enemy_index = 0; enemy_index < maximum_enemies; enemy_index++) {
        if (enemies[enemy_index].active) destroy_enemy(&enemies[enemy_index]);
    }
    if (boss.active && !boss.defeated) {
        boss.health -= boss_bomb_damage;
        boss.flash_timer = 0.16f;
        if (boss.health <= 0) {
            boss.health = 0;
            boss.defeated = true;
            score += boss_score_value;
            game_state = game_state_victory;
            add_particle_burst(boss.position, boss_color, 72, 270.0f);
        }
    }
    add_particle_burst(player.position, cyan_color, 54, 310.0f);
}

static void update_player(float delta_time)
{
    Vector2 movement = {0.0f, 0.0f};
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) movement.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) movement.x += 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) movement.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) movement.y += 1.0f;
    player.focused = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    float movement_speed = player.focused ? player_focus_speed : player_speed;
    player.position = vector_add(player.position,
                                 vector_scale(vector_normalize(movement), movement_speed * delta_time));
    player.position.x = clamp_value(player.position.x, 32.0f, (float)screen_width - 32.0f);
    player.position.y = clamp_value(player.position.y, 48.0f, (float)screen_height - 34.0f);
    player.fire_timer -= delta_time;
    player.invulnerability_timer = fmaxf(0.0f, player.invulnerability_timer - delta_time);
    if ((IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_Z)) && player.fire_timer <= 0.0f) {
        float spread = player.focused ? 10.0f : 15.0f;
        spawn_bullet(player_bullets, maximum_player_bullets,
                     (Vector2){player.position.x - spread, player.position.y - 16.0f},
                     (Vector2){0.0f, -1.0f}, player_bullet_speed, (float)player_bullet_radius);
        spawn_bullet(player_bullets, maximum_player_bullets,
                     (Vector2){player.position.x + spread, player.position.y - 16.0f},
                     (Vector2){0.0f, -1.0f}, player_bullet_speed, (float)player_bullet_radius);
        player.fire_timer = player_fire_interval;
        PlaySound(player_shot_sound);
    }
}

static void update_enemies(float delta_time)
{
    bool active_enemies = false;
    for (int enemy_index = 0; enemy_index < maximum_enemies; enemy_index++) {
        Enemy *enemy = &enemies[enemy_index];
        if (!enemy->active) continue;
        active_enemies = true;
        enemy->position.y += enemy_vertical_speed * delta_time;
        enemy->position.x += enemy->horizontal_direction * enemy_horizontal_speed * delta_time;
        if (enemy->position.x < enemy_edge_margin || enemy->position.x > (float)screen_width - enemy_edge_margin) {
            enemy->horizontal_direction *= -1.0f;
        }
        enemy->fire_timer -= delta_time;
        if (enemy->fire_timer <= 0.0f && enemy->position.y > 30.0f) {
            spawn_bullet(enemy_bullets, maximum_enemy_bullets, enemy->position,
                         vector_subtract(player.position, enemy->position), enemy_bullet_speed, (float)enemy_bullet_radius);
            enemy->fire_timer = enemy_fire_interval + (float)GetRandomValue(0, (int)(enemy_fire_random_range * 100.0f)) / 100.0f;
        }
        if (enemy->position.y > (float)screen_height + enemy_offscreen_margin) enemy->active = false;
        if (circles_overlap(enemy->position, enemy_collision_radius,
                            player.position, player_collision_radius)) {
            enemy->active = false;
            damage_player();
        }
    }
    if (!active_enemies && enemies_spawned >= wave_enemy_counts[current_wave] && !boss.active) {
        transition_timer += delta_time;
        if (transition_timer >= wave_break_duration) {
            if (current_wave + 1 < wave_count) {
                current_wave++;
                enemies_spawned = 0;
                spawn_timer = subsequent_wave_spawn_delay;
                transition_timer = 0.0f;
            } else {
                boss.active = true;
                boss.position.x = ((float)screen_width * 0.5f);
                boss.position.y = -110.0f;
                boss.health = maximum_boss_health;
                boss.horizontal_direction = 1.0f;
                transition_timer = 0.0f;
            }
        }
    }
}

static void update_boss(float delta_time)
{
    if (!boss.active || boss.defeated) return;
    if (boss.position.y < boss_hover_height) {
        boss.position.y += boss_entry_speed * delta_time;
        if (boss.position.y > boss_hover_height) boss.position.y = boss_hover_height;
    } else {
        boss.position.x += boss.horizontal_direction * boss_drift_speed * delta_time;
        if (boss.position.x < boss_edge_margin || boss.position.x > (float)screen_width - boss_edge_margin) {
            boss.horizontal_direction *= -1.0f;
        }
        boss.phase_timer += delta_time;
        boss.attack_timer -= delta_time;
        if (boss.phase_timer >= boss_phase_duration) {
            boss.phase = (boss.phase + 1) % 3;
            boss.phase_timer = 0.0f;
            boss.attack_timer = 0.6f;
        }
        if (boss.attack_timer <= 0.0f) {
            if (boss.phase == 0) {
                Vector2 direction = vector_subtract(player.position, boss.position);
                spawn_bullet(enemy_bullets, maximum_enemy_bullets,
                             (Vector2){boss.position.x - 48.0f, boss.position.y + 35.0f},
                             direction, boss_bullet_speed, (float)boss_bullet_radius);
                spawn_bullet(enemy_bullets, maximum_enemy_bullets,
                             (Vector2){boss.position.x + 48.0f, boss.position.y + 35.0f},
                             direction, boss_bullet_speed, (float)boss_bullet_radius);
                boss.attack_timer = boss_aimed_shot_interval;
            } else if (boss.phase == 1) {
                for (int bullet_index = 0; bullet_index < boss_radial_bullet_count; bullet_index++) {
                    float angle = ((float)bullet_index / (float)boss_radial_bullet_count) * 2.0f * PI + simulation_time * 0.35f;
                    spawn_bullet(enemy_bullets, maximum_enemy_bullets,
                                 boss.position, (Vector2){cosf(angle), sinf(angle)},
                                 boss_bullet_speed * 0.82f, 6.0f);
                }
                boss.attack_timer = boss_radial_shot_interval;
            } else {
                float sweep_direction = (fmodf(simulation_time, 1.5f) < 0.75f) ? 1.0f : -1.0f;
                for (int bullet_index = -boss_fan_half_count; bullet_index <= boss_fan_half_count; bullet_index++) {
                    Vector2 direction = {(float)bullet_index * 0.24f * sweep_direction, 1.0f};
                    spawn_bullet(enemy_bullets, maximum_enemy_bullets,
                                 boss.position, direction, boss_bullet_speed * 1.05f, 6.0f);
                }
                boss.attack_timer = boss_fan_shot_interval;
            }
        }
    }
    boss.flash_timer = fmaxf(0.0f, boss.flash_timer - delta_time);
    if (circles_overlap(boss.position, boss_collision_radius,
                        player.position, player_collision_radius)) damage_player();
}

static void update_bullets(float delta_time)
{
    for (int bullet_index = 0; bullet_index < maximum_player_bullets; bullet_index++) {
        Bullet *bullet = &player_bullets[bullet_index];
        if (!bullet->active) continue;
        bullet->position = vector_add(bullet->position, vector_scale(bullet->velocity, delta_time));
        if (bullet->position.y < -player_bullet_radius) { bullet->active = false; continue; }
        for (int enemy_index = 0; enemy_index < maximum_enemies; enemy_index++) {
            Enemy *enemy = &enemies[enemy_index];
            if (!enemy->active || !circles_overlap(bullet->position, bullet->radius,
                                                    enemy->position, enemy_collision_radius)) continue;
            bullet->active = false;
            enemy->health--;
            if (enemy->health <= 0) destroy_enemy(enemy);
            break;
        }
        if (bullet->active && boss.active && !boss.defeated &&
            circles_overlap(bullet->position, bullet->radius, boss.position, boss_collision_radius)) {
            bullet->active = false;
            boss.health--;
            boss.flash_timer = 0.08f;
            if (boss.health % 8 == 0) PlaySound(boss_hit_sound);
            if (boss.health <= 0) {
                boss.health = 0;
                boss.defeated = true;
                score += boss_score_value;
                game_state = game_state_victory;
                screen_shake = 16.0f;
                add_particle_burst(boss.position, boss_color, 96, 300.0f);
                PlaySound(explosion_sound);
            }
        }
    }
    for (int bullet_index = 0; bullet_index < maximum_enemy_bullets; bullet_index++) {
        Bullet *bullet = &enemy_bullets[bullet_index];
        if (!bullet->active) continue;
        bullet->position = vector_add(bullet->position, vector_scale(bullet->velocity, delta_time));
        if (bullet->position.x < -20.0f || bullet->position.x > (float)screen_width + 20.0f ||
            bullet->position.y < -20.0f || bullet->position.y > (float)screen_height + 20.0f) {
            bullet->active = false;
        } else if (circles_overlap(bullet->position, bullet->radius,
                                   player.position, player_collision_radius)) {
            bullet->active = false;
            damage_player();
        }
    }
}

static void update_pickups(float delta_time)
{
    for (int pickup_index = 0; pickup_index < maximum_pickups; pickup_index++) {
        Pickup *pickup = &pickups[pickup_index];
        if (!pickup->active) continue;
        pickup->position.y += pickup_fall_speed * delta_time;
        pickup->position.x += sinf(simulation_time * 3.0f + pickup->position.y * 0.02f) * 20.0f * delta_time;
        if (pickup->position.y > (float)screen_height + 20.0f) pickup->active = false;
        if (pickup->active && circles_overlap(pickup->position, pickup_collision_radius,
                                               player.position, player_collision_radius)) {
            pickup->active = false;
            if (player.health < initial_player_health) player.health++;
            score += pickup_score_value;
            PlaySound(pickup_sound);
            add_particle_burst(pickup->position, cyan_color, 12, 90.0f);
        }
    }
}

static void update_particles(float delta_time)
{
    for (int particle_index = 0; particle_index < maximum_particles; particle_index++) {
        Particle *particle = &particles[particle_index];
        if (!particle->active) continue;
        particle->lifetime -= delta_time;
        particle->position = vector_add(particle->position, vector_scale(particle->velocity, delta_time));
        particle->velocity = vector_scale(particle->velocity, 0.97f);
        if (particle->lifetime <= 0.0f) particle->active = false;
    }
}

static void update_gameplay(float delta_time)
{
    simulation_time += delta_time;
    update_player(delta_time);
    if (IsKeyPressed(KEY_X)) activate_bomb();
    if (!boss.active && enemies_spawned < wave_enemy_counts[current_wave]) {
        spawn_timer -= delta_time;
        if (spawn_timer <= 0.0f) {
            spawn_enemy();
            spawn_timer = enemy_spawn_interval;
        }
    }
    update_enemies(delta_time);
    update_boss(delta_time);
    update_bullets(delta_time);
    update_pickups(delta_time);
    update_particles(delta_time);
    screen_shake = fmaxf(0.0f, screen_shake - screen_shake_decay * delta_time);
}

static void draw_stars(float delta_time)
{
    for (int star_index = 0; star_index < star_count; star_index++) {
        Star *star = &stars[star_index];
        star->position.y += star->speed * delta_time;
        if (star->position.y > (float)screen_height) {
            star->position.y = 0.0f;
            star->position.x = (float)GetRandomValue(0, screen_width);
        }
        DrawCircleV(star->position, star->radius,
                    (Color){155, 190, 255, star->alpha});
    }
}

static void draw_player(void)
{
    if (!player.alive) return;
    if (player.invulnerability_timer > 0.0f && ((int)(simulation_time * 16.0f) % 2 == 0)) return;
    Vector2 ship_tip = {player.position.x, player.position.y - 24.0f};
    Vector2 ship_left = {player.position.x - 19.0f, player.position.y + 18.0f};
    Vector2 ship_right = {player.position.x + 19.0f, player.position.y + 18.0f};
    DrawTriangle(ship_tip, ship_right, ship_left, player_color);
    DrawTriangle((Vector2){player.position.x, player.position.y - 8.0f},
                 (Vector2){player.position.x - 7.0f, player.position.y + 12.0f},
                 (Vector2){player.position.x + 7.0f, player.position.y + 12.0f},
                 (Color){22, 44, 76, 255});
    if (player.focused) DrawCircleLines((int)player.position.x, (int)player.position.y, 8.0f, WHITE);
    DrawCircle((int)player.position.x, (int)player.position.y + 21,
               (int)(4.0f + sinf(simulation_time * 22.0f) * 1.5f), (Color){255, 154, 75, 220});
}

static void draw_enemy(const Enemy *enemy)
{
    DrawTriangle((Vector2){enemy->position.x, enemy->position.y + 20.0f},
                 (Vector2){enemy->position.x + 18.0f, enemy->position.y - 12.0f},
                 (Vector2){enemy->position.x - 18.0f, enemy->position.y - 12.0f}, enemy_color);
    DrawCircleV(enemy->position, 6.0f, (Color){50, 21, 56, 255});
}

static void draw_boss(void)
{
    if (!boss.active || boss.defeated) return;
    Color shell_color = boss.flash_timer > 0.0f ? WHITE : boss_color;
    DrawEllipse((int)boss.position.x, (int)boss.position.y, 104.0f, 52.0f, shell_color);
    DrawEllipse((int)boss.position.x, (int)boss.position.y + 5, 72.0f, 38.0f,
                (Color){57, 29, 100, 255});
    DrawCircle((int)boss.position.x, (int)boss.position.y + 4, 22.0f,
               boss.flash_timer > 0.0f ? WHITE : cyan_color);
    DrawCircle((int)boss.position.x, (int)boss.position.y + 4, 10.0f, (Color){244, 250, 255, 255});
    DrawTriangle((Vector2){boss.position.x - 75.0f, boss.position.y + 8.0f},
                 (Vector2){boss.position.x - 122.0f, boss.position.y + 45.0f},
                 (Vector2){boss.position.x - 65.0f, boss.position.y + 34.0f}, shell_color);
    DrawTriangle((Vector2){boss.position.x + 75.0f, boss.position.y + 8.0f},
                 (Vector2){boss.position.x + 122.0f, boss.position.y + 45.0f},
                 (Vector2){boss.position.x + 65.0f, boss.position.y + 34.0f}, shell_color);
}

static void draw_bullets(void)
{
    for (int bullet_index = 0; bullet_index < maximum_player_bullets; bullet_index++) {
        if (!player_bullets[bullet_index].active) continue;
        DrawRectangle((int)player_bullets[bullet_index].position.x - 2,
                      (int)player_bullets[bullet_index].position.y - 8, 4, 16, cyan_color);
    }
    for (int bullet_index = 0; bullet_index < maximum_enemy_bullets; bullet_index++) {
        if (!enemy_bullets[bullet_index].active) continue;
        DrawCircleV(enemy_bullets[bullet_index].position, enemy_bullets[bullet_index].radius,
                    (Color){255, 128, 114, 255});
        DrawCircleV(enemy_bullets[bullet_index].position, 2.5f, WHITE);
    }
}

static void draw_particles(void)
{
    for (int particle_index = 0; particle_index < maximum_particles; particle_index++) {
        Particle *particle = &particles[particle_index];
        if (!particle->active) continue;
        float opacity = clamp_value(particle->lifetime / particle->maximum_lifetime, 0.0f, 1.0f);
        Color color = particle->color;
        color.a = (unsigned char)(opacity * 255.0f);
        DrawCircleV(particle->position, particle->size * opacity, color);
    }
}

static void draw_hud(void)
{
    DrawRectangle(0, 0, screen_width, 68, (Color){7, 10, 28, 230});
    DrawText("STARFALL ASSAULT", 22, 18, 19, cyan_color);
    DrawText(TextFormat("WAVE %d / %d", current_wave + 1, wave_count), 290, 20, 18, WHITE);
    DrawText(TextFormat("SCORE %06d", score), 450, 20, 18, gold_color);
    DrawText("HULL", 704, 22, 15, muted_color);
    for (int health_index = 0; health_index < initial_player_health; health_index++) {
        DrawCircle(758 + health_index * 23, 30, 8.0f,
                   health_index < player.health ? enemy_color : (Color){48, 55, 83, 255});
    }
    DrawText(TextFormat("BOMB x%d", player.bombs), 850, 20, 15, cyan_color);
    if (boss.active && !boss.defeated && boss.position.y >= 110.0f) {
        float bar_width = 420.0f;
        DrawText("DREADNOUGHT", 22, 85, 15, boss_color);
        DrawRectangle(22, 108, (int)bar_width, 12, (Color){38, 29, 62, 255});
        DrawRectangle(22, 108, (int)(bar_width * ((float)boss.health / (float)maximum_boss_health)),
                      12, boss_color);
    }
}

static void draw_centered_text(const char *text, int y, int font_size, Color color)
{
    DrawText(text, (screen_width - MeasureText(text, font_size)) / 2, y, font_size, color);
}

static void draw_overlay(const char *title, const char *subtitle, const char *prompt)
{
    Rectangle panel = {188, 205, 584, 300};
    DrawRectangle(0, 0, screen_width, screen_height, (Color){4, 6, 20, 205});
    DrawRectangleRounded(panel, 0.08f, 14, panel_color);
    DrawRectangleRoundedLinesEx(panel, 0.08f, 14, 1, (Color){63, 104, 154, 255});
    draw_centered_text(title, 246, 42, cyan_color);
    draw_centered_text(subtitle, 320, 20, WHITE);
    draw_centered_text(prompt, 396, 18, gold_color);
}

static void draw_game(float delta_time)
{
    BeginDrawing();
    ClearBackground(background_color);
    draw_stars(delta_time);
    Camera2D camera = {0};
    if (game_state == game_state_playing && screen_shake > 0.1f) {
        camera.offset = (Vector2){(float)GetRandomValue((int)-screen_shake, (int)screen_shake),
                                  (float)GetRandomValue((int)-screen_shake, (int)screen_shake)};
    }
    camera.zoom = 1.0f;
    BeginMode2D(camera);
    for (int enemy_index = 0; enemy_index < maximum_enemies; enemy_index++) {
        if (enemies[enemy_index].active) draw_enemy(&enemies[enemy_index]);
    }
    draw_boss();
    for (int pickup_index = 0; pickup_index < maximum_pickups; pickup_index++) {
        if (!pickups[pickup_index].active) continue;
        DrawCircleV(pickups[pickup_index].position, 12.0f, cyan_color);
        DrawCircleLines((int)pickups[pickup_index].position.x, (int)pickups[pickup_index].position.y,
                        17.0f + sinf(simulation_time * 6.0f) * 2.0f, WHITE);
        DrawText("+", (int)pickups[pickup_index].position.x - 5,
                 (int)pickups[pickup_index].position.y - 9, 18, background_color);
    }
    draw_bullets();
    draw_particles();
    draw_player();
    EndMode2D();
    if (game_state != game_state_title) draw_hud();

    if (game_state == game_state_title) {
        DrawRectangle(0, 0, screen_width, screen_height, (Color){4, 6, 20, 128});
        draw_centered_text("STARFALL", 205, 70, cyan_color);
        draw_centered_text("ASSAULT", 276, 42, WHITE);
        draw_centered_text("A lone interceptor. Three enemy waves. One final battle.",
                           354, 19, muted_color);
        draw_centered_text("ENTER TO LAUNCH", 438, 22, gold_color);
        draw_centered_text("WASD / ARROWS TO FLY     SPACE TO FIRE     X TO BOMB",
                           488, 15, WHITE);
        draw_centered_text("HOLD SHIFT TO FOCUS     ESC TO PAUSE", 516, 15, muted_color);
    } else if (game_state == game_state_paused) {
        draw_overlay("PAUSED", "Take a breath, pilot.", "PRESS ENTER OR ESC TO RESUME");
    } else if (game_state == game_state_victory) {
        draw_overlay("SECTOR SECURE", TextFormat("FINAL SCORE: %06d", score),
                     "PRESS ENTER TO FLY AGAIN");
    } else if (game_state == game_state_game_over) {
        draw_overlay("SHIP LOST", TextFormat("FINAL SCORE: %06d", score),
                     "PRESS ENTER TO RETRY");
    } else if (game_state == game_state_playing && transition_timer > 0.0f &&
               !boss.active && enemies_spawned >= wave_enemy_counts[current_wave]) {
        draw_centered_text(current_wave + 1 < wave_count ? "WAVE CLEAR" : "WARNING: CAPITAL SHIP",
                           230, 25, gold_color);
    }
    EndDrawing();
}

static Sound load_tone(float frequency, float duration, float volume)
{
    unsigned int frame_count = (unsigned int)(duration * (float)audio_sample_rate);
    short *samples = MemAlloc((int)(frame_count * sizeof(short)));
    for (unsigned int frame_index = 0; frame_index < frame_count; frame_index++) {
        float progress = (float)frame_index / (float)frame_count;
        float envelope = 1.0f - progress;
        float sample_phase = 2.0f * PI * frequency * (float)frame_index / (float)audio_sample_rate;
        samples[frame_index] = (short)(sinf(sample_phase) * volume * envelope * 32767.0f);
    }
    Wave generated_wave = {
        .frameCount = frame_count,
        .sampleRate = audio_sample_rate,
        .sampleSize = 16,
        .channels = 1,
        .data = samples
    };
    Sound sound = LoadSoundFromWave(generated_wave);
    MemFree(samples);
    return sound;
}

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(screen_width, screen_height, "Starfall Assault");
    SetTargetFPS(target_frames_per_second);
    InitAudioDevice();
    SetRandomSeed((unsigned int)GetTime());
    initialize_stars();

    player_shot_sound = load_tone(720.0f, 0.045f, 0.045f);
    explosion_sound = load_tone(105.0f, 0.19f, 0.13f);
    pickup_sound = load_tone(980.0f, 0.10f, 0.09f);
    boss_hit_sound = load_tone(260.0f, 0.035f, 0.05f);

    while (!WindowShouldClose()) {
        float delta_time = fminf(GetFrameTime(), 0.033f);
        if (game_state == game_state_title) {
            if (IsKeyPressed(KEY_ENTER)) initialize_run();
        } else if (game_state == game_state_playing) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) game_state = game_state_paused;
            else update_gameplay(delta_time);
        } else if (game_state == game_state_paused) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
                game_state = game_state_playing;
            }
        } else if (game_state == game_state_victory || game_state == game_state_game_over) {
            update_particles(delta_time);
            if (IsKeyPressed(KEY_ENTER)) initialize_run();
        }
        draw_game(delta_time);
    }

    UnloadSound(player_shot_sound);
    UnloadSound(explosion_sound);
    UnloadSound(pickup_sound);
    UnloadSound(boss_hit_sound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}








