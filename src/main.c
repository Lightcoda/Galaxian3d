#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define MAX_BULLETS 10

#define MAX_ENEMIES 20

#define ENEMY_SIZEX 0.10f
#define ENEMY_SIZEY 0.10f

const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "layout (location = 1) in vec3 aColour;\n"
                                 "uniform vec3 offset;\n"
                                 "out vec3 colour;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos + offset, 1.0);\n"
                                 "   colour = aColour;\n"
                                 "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
                                   "in vec3 colour;\n"
                                   "out vec4 FragColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = vec4(colour, 1.0f);\n"
                                   "}\n\0";

time_t last_timebul = 0;
time_t now_time = 0;

typedef struct
{
    float x, y;
    char active;
} Bullet;

Bullet bullets[MAX_BULLETS];

typedef struct
{
    float x, y;
    int lives;
    char active;
} Enemy;

Enemy enemies[MAX_ENEMIES];
void shootBullet(float playerX, float playerY)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if ((!bullets[i].active) && ((time(NULL) - last_timebul) > 1))
        {
            bullets[i].x = playerX;
            bullets[i].y = -0.25f;
            bullets[i].active = 1;
            last_timebul = time(NULL);
            break;
        }
    }
}
void updateBullets()
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            bullets[i].y += 0.01f;
            if (bullets[i].y > 1.0f)
            {
                bullets[i].active = 0;
            }
        }
    }
}
void drawBullets(unsigned int shaderProgram, unsigned int VAO)
{
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (bullets[i].active)
        {
            int offsetLoc = glGetUniformLocation(shaderProgram, "offset");
            glUniform3f(offsetLoc, bullets[i].x, bullets[i].y, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, 3);
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
                if (bullets[i].active)
                {
                    if ((fabs(bullets[i].x - enemies[j].x) <= ENEMY_SIZEX) && (fabs(bullets[i].y - enemies[j].y) <= ENEMY_SIZEY))
                    {
                        enemies[j].lives -= 1;
                        bullets[i].active = 0;
                    }
                }
            }
        }
        if (!enemies[j].lives)
        {
            enemies[j].active = 0;
        }
    }
}
void drawEnemy(unsigned int shaderProgram, unsigned int VAO)
{
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (enemies[i].active)
        {
            int offsetLoc = glGetUniformLocation(shaderProgram, "offset");
            glUniform3f(offsetLoc, enemies[i].x, enemies[i].y, 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
    }
}

void processInput(GLFWwindow *window, float *xOffset)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);

    if ((glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) && (fabs(*xOffset - 0.01f) <= 0.75f))
        *xOffset -= 0.01f;
    if ((glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) && (fabs(*xOffset + 0.01f) <= 0.75f))
        *xOffset += 0.01f;
}

int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        printf("Failed to create GLFW window");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD");
        return -1;
    }

    glViewport(0, 0, 800, 600);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices_bullet[] = {
        0.0f, 0.05f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.05f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        0.05f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

    float vertices_player[] = {
        -0.25f, -0.75f, 0.0f, 1.0f, 1.0f, 1.0f,
        0.25f, -0.75f, 0.0f, 1.0f, 1.0f, 1.0f,
        0.25f, -0.25f, 0.0f, 1.0f, 1.0f, 1.0f,

        -0.25f, -0.75f, 0.0f, 1.0f, 1.0f, 1.0f,
        0.25f, -0.25f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.25f, -0.25f, 0.0f, 1.0f, 1.0f, 1.0f};
    float vertices_enemy[] = {
        -ENEMY_SIZEX, -ENEMY_SIZEY, 0.0f, 1.0f, 1.0f, 1.0f,
        ENEMY_SIZEX, -ENEMY_SIZEY, 0.0f, 1.0f, 1.0f, 1.0f,
        ENEMY_SIZEX, ENEMY_SIZEY, 0.0f, 1.0f, 1.0f, 1.0f,

        -ENEMY_SIZEX, -ENEMY_SIZEY, 0.0f, 1.0f, 1.0f, 1.0f,
        ENEMY_SIZEX, ENEMY_SIZEY, 0.0f, 1.0f, 1.0f, 1.0f,
        -ENEMY_SIZEX, ENEMY_SIZEY, 0.0f, 1.0f, 1.0f, 1.0f

    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_player), vertices_player, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float xOffset = 0.0f;

    unsigned int VBO_bullet, VAO_bullet;
    glGenVertexArrays(1, &VAO_bullet);
    glGenBuffers(1, &VBO_bullet);

    glBindVertexArray(VAO_bullet);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_bullet);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_bullet), vertices_bullet, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int VBO_enemy, VAO_enemy;
    glGenVertexArrays(1, &VAO_enemy);
    glGenBuffers(1, &VBO_enemy);

    glBindVertexArray(VAO_enemy);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_enemy);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_enemy), vertices_enemy, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    enemies[0].x = 0.0f;
    enemies[0].y = 0.75f;
    enemies[0].lives = 2;
    enemies[0].active = 1;

    enemies[1].x = -0.30f;
    enemies[1].y = 0.75f;
    enemies[1].lives = 2;
    enemies[1].active = 1;

    // Основной цикл рендеринга
    while (!glfwWindowShouldClose(window))
    {
        // Обработка ввода
        processInput(window, &xOffset);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            shootBullet(xOffset, 0.0f);
        }
        // Очистка экрана
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        updateEnemy();
        updateBullets();
        // Использование программы шейдеров
        glUseProgram(shaderProgram);
        int offsetLoc = glGetUniformLocation(shaderProgram, "offset");
        glUniform3f(offsetLoc, xOffset, 0.0f, 0.0f);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        drawBullets(shaderProgram, VAO_bullet);
        drawEnemy(shaderProgram, VAO_enemy);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}