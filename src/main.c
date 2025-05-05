#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <cglm/cglm.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define BULLETTIME 0.70

#define MAX_BULLETS 10
#define MAX_ENEMIES 30
#define MAX_ENEMY_BULLETS 5
#define ENEMY_SIZEX 0.1f
#define ENEMY_SIZEY 0.1f
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
#define STARTPLY -0.4f

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
const char *modelvertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "layout (location = 1) in vec2 aTexCoords;\n"
                                 "layout (location = 2) in vec3 aNormal;\n"
                                 "uniform vec3 offset;\n"
                                 "uniform mat4 model;\n"
                                 "uniform mat4 view;\n"
                                 "uniform mat4 projection;\n"
                                 "out vec2 TexCoords;\n"
                                 "out vec3 Normal;\n"
                                 "out vec3 FragPos;\n"
                                 "void main()\n"
                                 "{\n"
                                 "FragPos = vec3(model * vec4(aPos + offset, 1.0));\n"
                                 "Normal = mat3(transpose(inverse(model))) * aNormal;\n"
                                 "TexCoords = aTexCoords;\n"
                                 "gl_Position = projection * view * vec4(FragPos, 1.0);\n"
                                 "}\n";

const char *modelfragmentShaderSource = "#version 330 core\n"
                                   "in vec2 TexCoords;\n"
                                   "in vec3 Normal;\n"
                                   "in vec3 FragPos;\n"
                                   "out vec4 FragColor;\n"
                                   "uniform int isHit;\n"
                                   "uniform sampler2D texture1;\n"
                                   "void main()\n"
                                   "{\n"
                                   "    if (isHit == 1)\n"
                                   "        FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
                                   "    else\n"
                                   "        FragColor = texture(texture1, TexCoords);\n"
                                   "}\n";


float last_timebul = 0, last_enemy_shot = 0, lastDiveTime = 0;
int playerHits = 0, kills = 0, playerIsHit = 0;


typedef struct {
    unsigned int vertexIndex;
    unsigned int uvIndex;
    unsigned int normalIndex;
} Face;

typedef struct {
    vec3* vertices;
    vec2* texCoords;
    vec3* normals;
    Face* faces;

    unsigned int numVertices;
    unsigned int numTexCoords;
    unsigned int numNormals;
    unsigned int numFaces;
} Model;


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

void drawBullets(unsigned int prog, unsigned int VAO, mat4 model, mat4 view, mat4 projection)
{
    glUseProgram(prog);
    glBindVertexArray(VAO);
    int off = glGetUniformLocation(prog, "offset");
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            glUniform3f(off, bullets[i].x, bullets[i].y, 0.0f);
            glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(prog, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, GL_FALSE, &projection[0][0]);
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

void drawEnemyBullets(unsigned int prog, unsigned int VAO, mat4 model, mat4 view, mat4 projection)
{
    glUseProgram(prog);
    glBindVertexArray(VAO);
    int off = glGetUniformLocation(prog, "offset");
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (enemyBullets[i].active)
        {
            glUniform3f(off, enemyBullets[i].x, enemyBullets[i].y, 0.0f);
            glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(prog, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, GL_FALSE, &projection[0][0]);
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

void drawEnemy(unsigned int prog, unsigned int VAO, Model* enemymodel, unsigned int texture, mat4 model, mat4 view, mat4 projection)
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
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glUniform1i(glGetUniformLocation(prog, "texture1"), 0);
            glUniformMatrix4fv(glGetUniformLocation(prog, "model"), 1, GL_FALSE, &model[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(prog, "view"), 1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(prog, "projection"), 1, GL_FALSE, &projection[0][0]);
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, enemymodel->numFaces);
            enemies[i].hit = 0;
        }
    }
}

void processInput(GLFWwindow *w, float *x) // Обработка ввода
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

unsigned int loadTexture(const char *path)
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        printf("Failed to load texture: %s\n", stbi_failure_reason());
        return -1;
    }
    stbi_image_free(data);
    return texture;
}

void loadObj(const char* path, Model* obmodel, float scale, float zoffset, float ydir, float yoffset, int change) {
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Failed to open OBJ file: %s\n", path);
        return;
    }

    unsigned int maxVertices = 100000, maxTexCoords = 100000, maxNormals = 100000, maxFaces = 100000;
    obmodel->vertices = malloc(maxVertices * sizeof(vec3));
    obmodel->texCoords = malloc(maxTexCoords * sizeof(vec2));
    obmodel->normals = malloc(maxNormals * sizeof(vec3));
    obmodel->faces = malloc(maxFaces * sizeof(Face));

    int y = 1;
    int z = 2;
    if(change){ // Если перепутаны y и z
        z=1;
        y=2;
    }
    char line[512];
    int offset = 0;
    while (fgets(line, sizeof(line), file)) {
        char*ind = line;
        if (strncmp(line, "v ", 2) == 0) {
            sscanf(line, "v %f %f %f", &(obmodel->vertices[obmodel->numVertices])[0],
                   &(obmodel->vertices[obmodel->numVertices])[y],
                   &(obmodel->vertices[obmodel->numVertices])[z]);

            obmodel->vertices[obmodel->numVertices][0] *= scale; // Смещения и масштаб
            obmodel->vertices[obmodel->numVertices][z] *= scale;
            obmodel->vertices[obmodel->numVertices][y] *= (scale*ydir);
            obmodel->vertices[obmodel->numVertices][1] += zoffset;
            obmodel->vertices[obmodel->numVertices][1] += yoffset;

            obmodel->numVertices++;
        } else if (strncmp(line, "vt ", 3) == 0) {
            // Текстурная координата
            sscanf(line, "vt %f %f", &obmodel->texCoords[obmodel->numTexCoords][0],
                   &obmodel->texCoords[obmodel->numTexCoords][1]);
            obmodel->numTexCoords++;
        } else if (strncmp(line, "vn ", 3) == 0) {
            // Нормаль
            sscanf(line, "vn %f %f %f", &obmodel->normals[obmodel->numNormals][0],
                   &obmodel->normals[obmodel->numNormals][1],
                   &obmodel->normals[obmodel->numNormals][2]);
            obmodel->numNormals++;
        } else if (strncmp(line, "f ", 2) == 0) {
            // Грань
            unsigned int v[3], vt[3], vn[3];
            int result = sscanf(ind, "f %u/%u/%u %u/%u/%u %u/%u/%u%n",
                                &v[0], &vt[0], &vn[0],
                                &v[1], &vt[1], &vn[1],
                                &v[2], &vt[2], &vn[2],&offset);
            ind+=offset;
            offset = 0;
            for (int i = 0; i < 3; i++) { // Сборка полигонов (если после 3 вершин есть что то еще, создается несколько треугольников)
                obmodel->faces[obmodel->numFaces].vertexIndex = v[i] - 1;
                obmodel->faces[obmodel->numFaces].uvIndex = vt[i] - 1;
                obmodel->faces[obmodel->numFaces].normalIndex = vn[i] - 1;
                obmodel->numFaces++;
            }
            while(sscanf(ind, "%u/%u/%u%n", &v[1], &vt[1], &vn[1],&offset) == 3){
                obmodel->faces[obmodel->numFaces].vertexIndex = v[0] - 1;
                obmodel->faces[obmodel->numFaces].uvIndex = vt[0] - 1;
                obmodel->faces[obmodel->numFaces].normalIndex = vn[0] - 1;
                obmodel->numFaces++;
                obmodel->faces[obmodel->numFaces].vertexIndex = v[2] - 1;
                obmodel->faces[obmodel->numFaces].uvIndex = vt[2] - 1;
                obmodel->faces[obmodel->numFaces].normalIndex = vn[2] - 1;
                obmodel->numFaces++;
                obmodel->faces[obmodel->numFaces].vertexIndex = v[1] - 1;
                obmodel->faces[obmodel->numFaces].uvIndex = vt[1] - 1;
                obmodel->faces[obmodel->numFaces].normalIndex = vn[1] - 1;
                obmodel->numFaces++;
                v[2] = v[1];
                vt[2] = vt[1];
                vn[2] = vn[1];
                ind+=offset;
                offset = 0;
            }
        }
    }

    fclose(file);
}

void freeModel(Model* obmodel) {
    free(obmodel->vertices);
    free(obmodel->texCoords);
    free(obmodel->normals);
    free(obmodel->faces);

    obmodel->numVertices = 0;
    obmodel->numTexCoords = 0;
    obmodel->numNormals = 0;
    obmodel->numFaces = 0;
}

void setupModelBuffers(Model* model, unsigned int* VAO, unsigned int* VBO) {
    glGenVertexArrays(1, VAO);
    glGenBuffers(1, VBO);
    glBindVertexArray(*VAO);

    float* vertexData = malloc(model->numFaces * 3 * 8 * sizeof(float)); // 8 = 3 (позиция) + 2 (текстура) + 3 (нормаль)
    unsigned int index = 0;
    for (unsigned int i = 0; i < model->numFaces; i++) {
        Face face = model->faces[i];

        // Позиция
        vertexData[index++] = model->vertices[face.vertexIndex][0];
        vertexData[index++] = model->vertices[face.vertexIndex][1];
        vertexData[index++] = model->vertices[face.vertexIndex][2];

        // Текстурные координаты
        vertexData[index++] = model->texCoords[face.uvIndex][0];
        vertexData[index++] = model->texCoords[face.uvIndex][1];

        // Нормали
        vertexData[index++] = model->normals[face.normalIndex][0];
        vertexData[index++] = model->normals[face.normalIndex][1];
        vertexData[index++] = model->normals[face.normalIndex][2];
    }

    // Загрузка данных вершин в VBO
    glBindBuffer(GL_ARRAY_BUFFER, *VBO);
    glBufferData(GL_ARRAY_BUFFER, model->numFaces * 3 * 8 * sizeof(float), vertexData, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); 
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); 
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    free(vertexData);
    glBindVertexArray(0);
}
int main()
{
    glfwInit(); // Создание контекста opengl
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "Galaxian3D", NULL, NULL);
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

    mat4 model, view, projection; // Блок обработки камеры
    glm_mat4_identity(model);

    glm_perspective(glm_rad(45.0f), (float)(mode->width) / (float)(mode->height), 0.1f, 100.0f, projection);

    vec3 eye = {0.0f, -2.0f, 1.0f};   
    vec3 center = {0.0f, 0.0f, 0.0f}; 
    vec3 up = {0.0f, 1.0f, 1.0f};     
    glm_lookat(eye, center, up, view);

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER); // Блок комплиляции шейдеров
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
    checkShaderCompileErrors(pvs);
    unsigned int pfs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(pfs, 1, &primefragmentshader, NULL);
    glCompileShader(pfs);
    checkShaderCompileErrors(pfs);
    unsigned int primprog = glCreateProgram();
    glAttachShader(primprog, pvs);
    glAttachShader(primprog, pfs);
    glLinkProgram(primprog);
    glDeleteShader(pvs);
    glDeleteShader(pfs);

    unsigned int mvs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(mvs, 1, &modelvertexShaderSource, NULL);
    glCompileShader(mvs);
    checkShaderCompileErrors(mvs);
    unsigned int mfs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(mfs, 1, &modelfragmentShaderSource, NULL);
    glCompileShader(mfs);
    checkShaderCompileErrors(mfs);
    unsigned int mprog = glCreateProgram();
    glAttachShader(mprog, mvs);
    glAttachShader(mprog, mfs);
    glLinkProgram(mprog);
    glDeleteShader(mvs);
    glDeleteShader(mfs);

    float backgroundVertices[] = { // Буфер фона и его обработка
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,  
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f   
    };

    unsigned int indices[] = {
        0, 1, 3, 
        1, 2, 3  
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

    float vb[] = { // Буффер пули и его обработка
        -0.005f, -0.07f, 0, 1.0f, 0.65f, 0.0f, 0.005f, -0.07f, 0, 1.0f, 0.65f, 0.0f, 0.005f, -0.03f, 0, 1.0f, 0.65f, 0.0f,
        -0.005f, -0.07f, 0, 1.0f, 0.65f, 0.0f, 0.005f, -0.03f, 0, 1.0f, 0.65f, 0.0f, -0.005f, -0.03f, 0, 1.0f, 0.65f, 0.0f};

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

    Model enemymodel; // Загрузка модели врага
    memset(&enemymodel, 0, sizeof(Model));
    loadObj("../res/fighter.obj", &enemymodel,.05f, 0.2f, 1.0f, -0.3f, 0);
    unsigned int VBO_e, VAO_e;
    setupModelBuffers(&enemymodel,&VAO_e,&VBO_e);
    unsigned int enemytexture = loadTexture("../res/fighter_texture.jpg");


    Model playermodel; //Загрузка модели игрока
    memset(&playermodel, 0, sizeof(Model));
    loadObj("../res/SpaseShip.obj", &playermodel,.05f, -0.2f, 1.0f, -0.3f, 1);
    unsigned int VBO, VAO;
    setupModelBuffers(&playermodel,&VAO,&VBO);

    unsigned int shiptexture = loadTexture("../res/Ship_texture.png");

    srand((unsigned)time(NULL));
    spawnFormation();
    lastDiveTime = glfwGetTime();

    float x = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        processInput(window,&x);
        glClear(GL_COLOR_BUFFER_BIT); // Фон
        glUseProgram(primprog);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO_bg);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        updateEnemy(); // Юлок обработки врагов и пуль
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

        drawBullets(prog, VAO_b, model, view, projection);
        drawEnemyBullets(prog, VAO_b,model, view, projection);

        glUseProgram(mprog); //Блок обработки игрока
        int off = glGetUniformLocation(mprog, "offset");
        int hitLoc = glGetUniformLocation(mprog, "isHit");

        glUniform3f(off, x, 0.0f, 0.0f);
        glUniform1i(hitLoc, playerIsHit);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shiptexture);
        glUniform1i(glGetUniformLocation(mprog, "texture1"), 0);

        glUniformMatrix4fv(glGetUniformLocation(mprog, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(mprog, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(mprog, "projection"), 1, GL_FALSE, &projection[0][0]);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, playermodel.numFaces);

        drawEnemy(mprog, VAO_e, &enemymodel, enemytexture,model, view, projection);

        playerIsHit = 0;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(prog);
    glDeleteVertexArrays(1, &VAO_e);
    glDeleteBuffers(1, &VBO_e);
    glDeleteProgram(mprog);
    glDeleteVertexArrays(1, &VAO_bg);
    glDeleteBuffers(1, &VBO_bg);
    glDeleteProgram(primprog);
    glDeleteVertexArrays(1, &VAO_b);
    glDeleteBuffers(1, &VBO_b);
    freeModel(&playermodel);
    freeModel(&enemymodel);
    glfwTerminate();
    return 0;
}
