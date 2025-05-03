#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#include <cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define BULLETTIME 0.30

#define MAX_BULLETS 10
#define MAX_ENEMIES 30
#define MAX_ENEMY_BULLETS 5
#define ENEMY_SIZEX 0.05f
#define ENEMY_SIZEY 0.05f
#define ENEMY_SIZEZ 0.05f
#define ENEMY_SPEED 0.002f
#define SCREEN_LIMIT_X 0.9f
#define PLAYER_HITS_TO_DIE 10
#define FORMATION_ROWS 3
#define FORMATION_COLS 9
#define H_SPACING 0.15f
#define V_SPACING 0.2f
#define DIVE_INTERVAL 7
#define DIVE_SPEED 0.002f
#define DIVE_ACCEL 0.00005f
#define PLAYER_COLLIDE_RX 0.05f
#define PLAYER_COLLIDE_RY 0.05f
#define STARTPLY -0.5f

const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "layout (location = 1) in vec3 aColour;\n"
                                 "uniform vec3 offset;\n"
                                 "uniform mat4 model;\n"
                                 "uniform mat4 view;\n"
                                 "uniform mat4 projection;\n"
                                 "out vec3 colour;\n"
                                 "void main()\n"
                                 "{\n"
                                 "    gl_Position = projection*view*model*vec4(aPos + offset, 1.0);\n"
                                 "    colour = aColour;\n"
                                 "}\n";

const char *fragmentShaderSource = "#version 330 core\n"
                                   "in vec3 colour;\n"
                                   "out vec4 FragColor;\n"
                                   "uniform int isHit;\n"
                                   "void main()\n"
                                   "{\n"
                                   "    if (isHit == 1)\n"
                                   "        FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
                                   "    else\n"
                                   "        FragColor = vec4(colour, 1.0f);\n"
                                   "}\n";

const char *primvetexshader = "#version 330 core\n"
                              "layout (location = 0) in vec3 aPos;\n"
                              "layout (location = 1) in vec2 aTexcoords;\n"
                              "out vec2 Texcoords;\n"
                              "void main()\n"
                              "{\n"
                              "    gl_Position = vec4(aPos, 1.0);\n"
                              "    Texcoords = aTexcoords;\n"
                              "}\n";
const char *primefragmentshader = "#version 330 core\n"
                                  "in vec2 Texcoords;\n"
                                  "out vec4 FragColor;\n"
                                  "uniform sampler2D texture1;\n"
                                  "void main()\n"
                                  "{\n"
                                  "FragColor = texture(texture1, Texcoords);\n"
                                  "}\n";

void checkShaderCompileErrors(unsigned int shader)
{
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    }
}

float last_timebul = 0, last_enemy_shot = 0, lastDiveTime = 0;
int playerHits = 0, kills = 0, playerIsHit = 0;
;

typedef struct
{
    float x, y;
    char active;
} Bullet;
typedef struct
{
    float x, y, speedX, speedY;
    int lives;
    char active, diving, hit;
} Enemy;

Bullet bullets[MAX_BULLETS];
Bullet enemyBullets[MAX_ENEMY_BULLETS];
Enemy enemies[MAX_ENEMIES];

void shootBullet(float px)
{
    if ((glfwGetTime() - last_timebul) > 0.65)
    {
        for (int i = 0; i < MAX_BULLETS; i++)
        {
            if (!bullets[i].active)
            {
                bullets[i].x = px;
                bullets[i].y = STARTPLY + ENEMY_SIZEY;
                bullets[i].active = 1;
                last_timebul = glfwGetTime();
                break;
            }
        }
    }
}

void updateBullets()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            bullets[i].y += 0.02f;
            if (bullets[i].y > 1.0f)
                bullets[i].active = 0;
        }
    }
}

void drawBullets(unsigned int prog, unsigned int VAO)
{
    glUseProgram(prog);
    glBindVertexArray(VAO);
    int off = glGetUniformLocation(prog, "offset");
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            glUniform3f(off, bullets[i].x, bullets[i].y, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
}

void shootEnemyBullet(float ex, float ey, float interval)
{
    if ((glfwGetTime() - last_enemy_shot) > interval)
    {
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
        {
            if (!enemyBullets[i].active)
            {
                enemyBullets[i].x = ex;
                enemyBullets[i].y = ey;
                enemyBullets[i].active = 1;
                last_enemy_shot = glfwGetTime();
                break;
            }
        }
    }
}

void updateEnemyBullets()
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (enemyBullets[i].active)
        {
            enemyBullets[i].y -= 0.01f;
            if (enemyBullets[i].y < -1.0f)
                enemyBullets[i].active = 0;
        }
    }
}

void drawEnemyBullets(unsigned int prog, unsigned int VAO)
{
    glUseProgram(prog);
    glBindVertexArray(VAO);
    int off = glGetUniformLocation(prog, "offset");
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (enemyBullets[i].active)
        {
            glUniform3f(off, enemyBullets[i].x, enemyBullets[i].y, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
}

void updateEnemy()
{
    for (int j = 0; j < MAX_ENEMIES; j++)
    {
        if (enemies[j].active)
        {
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (bullets[i].active &&
                    fabs(bullets[i].x - enemies[j].x) <= ENEMY_SIZEX &&
                    fabs(bullets[i].y - enemies[j].y) <= ENEMY_SIZEY)
                {
                    enemies[j].lives--;
                    bullets[i].active = 0;
                    enemies[j].hit = 1;
                    if (enemies[j].lives == 0)
                    {
                        enemies[j].active = 0;
                        kills++;
                    }
                }
            }
        }
    }
}

void spawnFormation()
{
    for (int r = 0; r < FORMATION_ROWS; r++)
        for (int c = 0; c < FORMATION_COLS; c++)
        {
            int i = r * FORMATION_COLS + c;
            enemies[i].x = -H_SPACING * (FORMATION_COLS - 1) / 2 + c * H_SPACING;
            enemies[i].y = 0.8f - r * V_SPACING;
            enemies[i].speedX = ENEMY_SPEED;
            enemies[i].speedY = 0.0f;
            enemies[i].lives = 2;
            enemies[i].active = 1;
            enemies[i].diving = 0;
            enemies[i].hit = 0;
        }
}

void updateEnemyMovement(float playerX)
{
    for (int j = 0; j < MAX_ENEMIES; j++)
    {
        if (enemies[j].active && !enemies[j].diving)
        {
            if (enemies[j].x + ENEMY_SIZEX >= SCREEN_LIMIT_X ||
                enemies[j].x - ENEMY_SIZEX <= -SCREEN_LIMIT_X)
            {
                for (int k = 0; k < MAX_ENEMIES; k++)
                {
                    if (enemies[k].active && !enemies[k].diving)
                    {
                        enemies[k].speedX = -enemies[k].speedX;
                    }
                }
                break;
            }
        }
    }

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active)
            continue;

        if (enemies[i].diving)
        {
            float dx = playerX - enemies[i].x;
            float dy = STARTPLY - enemies[i].y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 0.0f)
            {
                dx /= dist;
                dy /= dist;
                enemies[i].speedX += DIVE_ACCEL * dx;
                enemies[i].speedY += DIVE_ACCEL * dy;
            }
            enemies[i].x += enemies[i].speedX;
            enemies[i].y += enemies[i].speedY;

            if (enemies[i].y < STARTPLY - 0.5f ||
                enemies[i].x < -1.0f || enemies[i].x > 1.0f)
            {

                float deltaX = 0.0f;
                float repInitX = 0.0f;
                float repSpeed = ENEMY_SPEED;
                for (int k = 0; k < MAX_ENEMIES; k++)
                {
                    if (enemies[k].active && !enemies[k].diving)
                    {
                        int c = k % FORMATION_COLS;
                        repInitX = -H_SPACING * (FORMATION_COLS - 1) / 2 + c * H_SPACING;
                        deltaX = enemies[k].x - repInitX;
                        repSpeed = enemies[k].speedX;
                        break;
                    }
                }

                int r = i / FORMATION_COLS, c = i % FORMATION_COLS;
                float initX = -H_SPACING * (FORMATION_COLS - 1) / 2 + c * H_SPACING;
                enemies[i].x = initX + deltaX;
                enemies[i].y = 0.8f - r * V_SPACING;
                enemies[i].speedX = repSpeed;
                enemies[i].speedY = 0.0f;
                enemies[i].diving = 0;
            }
        }
        else
        {
            enemies[i].x += enemies[i].speedX;
        }
    }
}

void diveAttack(float px)
{
    if ((glfwGetTime() - lastDiveTime) < DIVE_INTERVAL)
        return;
    int start = (FORMATION_ROWS - 1) * FORMATION_COLS, end = start + FORMATION_COLS;
    int cand[FORMATION_COLS], cnt = 0;
    for (int i = start; i < end; i++)
        if (enemies[i].active && !enemies[i].diving)
            cand[cnt++] = i;
    if (!cnt)
        return;
    int pick = cand[rand() % cnt];
    float dx = px - enemies[pick].x, dy = STARTPLY - enemies[pick].y;
    float len = sqrtf(dx * dx + dy * dy);
    enemies[pick].speedX = DIVE_SPEED * dx / len;
    enemies[pick].speedY = DIVE_SPEED * dy / len;
    enemies[pick].diving = 1;
    lastDiveTime = glfwGetTime();
}

void checkDiveCollisions(float playerX)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active && enemies[i].diving)
        {
            float dx = enemies[i].x - playerX;
            float dy = enemies[i].y - STARTPLY;
            if ((fabsf(dx) <= PLAYER_COLLIDE_RX + ENEMY_SIZEX) && (fabsf(dy) <= PLAYER_COLLIDE_RY + ENEMY_SIZEY))
            {
                playerHits++;
                playerIsHit = 1;
                enemies[i].lives = 0;
                enemies[i].active = 0;
                kills++;
                enemies[i].diving = 0;
                if (playerHits >= PLAYER_HITS_TO_DIE)
                    exit(0);
            }
        }
    }
}

void updatePlayerHits(float px)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (enemyBullets[i].active &&
            (fabs(enemyBullets[i].x - px) <= PLAYER_COLLIDE_RX) &&
            (fabs(enemyBullets[i].y - STARTPLY) <= PLAYER_COLLIDE_RY))
        {
            playerHits++;
            playerIsHit = 1;
            enemyBullets[i].active = 0;
            if (playerHits >= PLAYER_HITS_TO_DIE)
            {
                printf("Skill issue get good");
                exit(0);
            }
        }
    }
}

void drawEnemy(unsigned int prog, unsigned int VAO)
{
    glUseProgram(prog);
    glBindVertexArray(VAO);
    int off = glGetUniformLocation(prog, "offset");
    int hitLoc = glGetUniformLocation(prog, "isHit");
    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
        {
            glUniform3f(off, enemies[i].x, enemies[i].y, 0.0f);
            glUniform1i(hitLoc, enemies[i].hit);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            enemies[i].hit = 0;
        }
    }
}

void processInput(GLFWwindow *w, float *x)
{
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, 1);
    if (glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS && *x > -SCREEN_LIMIT_X)
        *x -= 0.01f;
    if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS && *x < SCREEN_LIMIT_X)
        *x += 0.01f;
    if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS)
        shootBullet(*x);
}

// Функция для загрузки текстуры
unsigned int loadTexture(const char *path)
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Установите параметры текстуры
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Загрузите изображение
    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        // Создайте текстуру
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        printf("Failed to load texture\n");
    }
    stbi_image_free(data);
    return texture;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "LearnOpenGL", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;
    glViewport(0, 0, mode->width, mode->height);

    mat4 model, view, projection;
    glm_mat4_identity(model);

    glm_perspective(glm_rad(45.0f), (float)(mode->width) / (float)(mode->height), 1.0f, 10.0f, projection);

    vec3 eye = {0.0f, -2.0f, 1.0f};   // Позиция камеры
    vec3 center = {0.0f, 0.0f, 0.0f}; // Точка, на которую смотрит камера
    vec3 up = {0.0f, 1.0f, 1.0f};     // Вектор "вверх"
    glm_lookat(eye, center, up, view);

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    unsigned int pvs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(pvs, 1, &primvetexshader, NULL);
    glCompileShader(pvs);
    // checkShaderCompileErrors(pvs);
    unsigned int pfs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(pfs, 1, &primefragmentshader, NULL);
    glCompileShader(pfs);
    // checkShaderCompileErrors(pfs);
    unsigned int primprog = glCreateProgram();
    glAttachShader(primprog, pvs);
    glAttachShader(primprog, pfs);
    glLinkProgram(primprog);
    glDeleteShader(pvs);
    glDeleteShader(pfs);

    float backgroundVertices[] = {
        // positions        // texture coords
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // top right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // top left
    };

    unsigned int indices[] = {
        0, 1, 3, // Первый треугольник
        1, 2, 3  // Второй треугольник
    };

    unsigned int texture = loadTexture("../res/back.png");

    unsigned int VBO_bg, VAO_bg, EBO;
    glGenVertexArrays(1, &VAO_bg);
    glGenBuffers(1, &VBO_bg);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO_bg);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_bg);
    glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundVertices), backgroundVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float vb[] = {
        -0.005f, -0.07f, 0, 0.56f, 0.93f, 0.56f, 0.005f, -0.07f, 0, 0.56f, 0.93f, 0.56f, 0.005f, -0.03f, 0, 0.56f, 0.93f, 0.56f,
        -0.005f, -0.07f, 0, 0.56f, 0.93f, 0.56f, 0.005f, -0.03f, 0, 0.56f, 0.93f, 0.56f, -0.005f, -0.03f, 0, 0.56f, 0.93f, 0.56f};

    float vp[] = {
        // Передняя грань
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Задняя грань
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Левая грань
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Правая грань
        ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Верхняя грань
        -ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Нижняя грань
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY + STARTPLY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f};
    float ve[] = {
        // Передняя грань
        -ENEMY_SIZEX, -ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Задняя грань
        -ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Левая грань
        -ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Правая грань
        ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Верхняя грань
        -ENEMY_SIZEX, ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,

        // Нижняя грань
        -ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY, -ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        ENEMY_SIZEX, -ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f,
        -ENEMY_SIZEX, -ENEMY_SIZEY, ENEMY_SIZEZ, 0.56f, 0.93f, 0.56f};

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vp), vp, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int VBO_b, VAO_b;
    glGenVertexArrays(1, &VAO_b);
    glGenBuffers(1, &VBO_b);
    glBindVertexArray(VAO_b);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_b);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vb), vb, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int VBO_e, VAO_e;
    glGenVertexArrays(1, &VAO_e);
    glGenBuffers(1, &VBO_e);
    glBindVertexArray(VAO_e);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_e);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ve), ve, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    srand((unsigned)time(NULL));
    spawnFormation();
    lastDiveTime = glfwGetTime();

    float x = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        processInput(window, &x);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            shootBullet(x);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(primprog);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO_bg);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        updateEnemy();
        updateBullets();
        for (int j = 0; j < MAX_ENEMIES; j++)
            if (enemies[j].active && rand() % 500 == 0)
                shootEnemyBullet(enemies[j].x, enemies[j].y, 2);
        updateEnemyBullets();
        updateEnemyMovement(x);
        diveAttack(x);
        for (int i = 0; i < MAX_ENEMIES; i++)
            if (enemies[i].active && enemies[i].diving)
                shootEnemyBullet(enemies[i].x, enemies[i].y, 0.2f);
        checkDiveCollisions(x);
        updatePlayerHits(x);

        updateEnemy();
        updateBullets();
        drawBullets(prog, VAO_b);
        drawEnemy(prog, VAO_e);
        drawEnemyBullets(prog, VAO_b);

        glUseProgram(prog);
        int off = glGetUniformLocation(prog, "offset");
        int hitLoc = glGetUniformLocation(prog, "isHit");

        glUniform3f(off, x, 0.0f, 0.0f);
        glUniform1i(hitLoc, playerIsHit);

        glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(prog, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, GL_FALSE, &projection[0][0]);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        playerIsHit = 0;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(prog);
    glfwTerminate();
    return 0;
}
