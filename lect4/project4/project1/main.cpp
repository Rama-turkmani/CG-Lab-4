// =============================================================================
// لعبة "التقط النجوم!" - Catch the Stars!
// مشروع عملي لمقرر الرسوميات الحاسوبية
// =============================================================================
// التقنيات المستخدمة:
//   - VBO  (Vertex Buffer Object)  : لتخزين بيانات الرؤوس
//   - EBO  (Element Buffer Object) : للرسم بالفهارس (المستطيلات)
//   - VAO  (Vertex Array Object)   : لتنظيم سمات الرؤوس
//   - Shaders (Vertex + Fragment)  : للتحكم بالموقع واللون
//   - Uniforms                     : لتمرير الموقع والحجم واللون ديناميكياً
// =============================================================================
// طريقة اللعب:
//   - أسهم يمين/يسار أو A/D لتحريك المجداف
//   - التقط النجوم الساقطة لزيادة النقاط
//   - لديك 3 محاولات، كل نجمة تفوتك تخسر محاولة
//   - السرعة تزداد تدريجياً مع كل نجمة تلتقطها
//   - عند انتهاء اللعبة اضغط R لإعادة اللعب
//   - اضغط ESC للخروج
// =============================================================================

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <cmath>

// 1. تضمين المكتبات
// ملاحظة: يجب تضمين GLEW قبل GLFW دائماً
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// ==================== إعدادات النافذة ====================
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// ==================== حالة اللعبة (Game State) ====================
// اللاعب (المجداف)
float playerX = 0.0f;
const float playerY = -0.85f;
const float playerWidth = 0.3f;
const float playerHeight = 0.06f;
const float playerSpeed = 0.025f;

// النجمة الساقطة
float starX = 0.0f;
float starY = 1.2f;
float starSpeedBase = 0.012f;
float starR = 1.0f, starG = 0.8f, starB = 0.2f;

// نظام النقاط والمحاولات
int score = 0;
int lives = 3;
bool gameOver = false;

// ==================== كود الـ Vertex Shader ====================
// يستقبل موقع الرأس ويطبق عليه الإزاحة (uOffset) والمقياس (uScale)
// هذا يسمح لنا برسم أشكال مختلفة الأحجام والمواقع باستخدام VBO واحد
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform vec2 uOffset;\n"
"uniform vec2 uScale;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x * uScale.x + uOffset.x,\n"
"                      aPos.y * uScale.y + uOffset.y,\n"
"                      aPos.z, 1.0);\n"
"}\0";

// ==================== كود الـ Fragment Shader ====================
// يستقبل اللون عبر Uniform ويطبقه على كل بكسل
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 uColor;\n"
"void main()\n"
"{\n"
"   FragColor = uColor;\n"
"}\n\0";

// ==================== دوال مساعدة ====================

// معالجة تغيير حجم النافذة
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// معالجة مدخلات لوحة المفاتيح
void processInput(GLFWwindow* window)
{
    // الخروج بزر ESC
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!gameOver)
    {
        // تحريك المجداف لليسار
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            playerX -= playerSpeed;
            if (playerX - playerWidth / 2.0f < -1.0f)
                playerX = -1.0f + playerWidth / 2.0f;
        }
        // تحريك المجداف لليمين
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            playerX += playerSpeed;
            if (playerX + playerWidth / 2.0f > 1.0f)
                playerX = 1.0f - playerWidth / 2.0f;
        }
    }
    else
    {
        // إعادة تشغيل اللعبة بزر R
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        {
            score = 0;
            lives = 3;
            gameOver = false;
            starSpeedBase = 0.012f;
            playerX = 0.0f;
        }
    }
}

// إعادة تعيين النجمة بموقع ولون عشوائي جديد
void resetStar()
{
    starX = ((float)(rand() % 160) / 100.0f) - 0.8f;
    starY = 1.2f;
    starR = (float)(rand() % 60 + 40) / 100.0f;
    starG = (float)(rand() % 60 + 40) / 100.0f;
    starB = (float)(rand() % 60 + 40) / 100.0f;
}

// فحص التصادم بين النجمة ومجداف اللاعب (AABB Collision)
bool checkCollision()
{
    float bHalfW = 0.05f;
    float bHalfH = 0.05f;

    float bLeft   = starX - bHalfW;
    float bRight  = starX + bHalfW;
    float bBottom = starY - bHalfH;

    float pLeft   = playerX - playerWidth / 2.0f;
    float pRight  = playerX + playerWidth / 2.0f;
    float pTop    = playerY + playerHeight / 2.0f;
    float pBottom = playerY - playerHeight / 2.0f;

    return (bRight > pLeft && bLeft < pRight &&
            bBottom <= pTop && starY >= pBottom);
}

// تحديث عنوان النافذة بالنقاط والمحاولات
void updateTitle(GLFWwindow* window)
{
    if (gameOver)
    {
        std::string t = "GAME OVER! Final Score: " + std::to_string(score) +
                        " | Press R to Restart";
        glfwSetWindowTitle(window, t.c_str());
    }
    else
    {
        std::string t = "Catch the Stars! | Score: " + std::to_string(score) +
                        " | Lives: " + std::to_string(lives);
        glfwSetWindowTitle(window, t.c_str());
    }
}

// تجميع شيدر واحد (Vertex أو Fragment)
unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

// إنشاء برنامج الشيدر (ربط Vertex + Fragment)
unsigned int createShaderProgram()
{
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // حذف الشيدرز المنفصلة بعد ربطها
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// رسم مستطيل باستخدام EBO - يعتمد على Uniforms للموقع والحجم واللون
void drawRect(unsigned int program, unsigned int VAO,
              float x, float y, float w, float h,
              float r, float g, float b)
{
    glUniform2f(glGetUniformLocation(program, "uOffset"), x, y);
    glUniform2f(glGetUniformLocation(program, "uScale"), w, h);
    glUniform4f(glGetUniformLocation(program, "uColor"), r, g, b, 1.0f);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

// رسم مثلث باستخدام glDrawArrays (بدون EBO)
void drawTri(unsigned int program, unsigned int VAO,
             float x, float y, float w, float h,
             float r, float g, float b)
{
    glUniform2f(glGetUniformLocation(program, "uOffset"), x, y);
    glUniform2f(glGetUniformLocation(program, "uScale"), w, h);
    glUniform4f(glGetUniformLocation(program, "uColor"), r, g, b, 1.0f);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

// ==================== الدالة الرئيسية ====================
int main()
{
    srand((unsigned int)time(NULL));

    // --- 1. تهيئة GLFW ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // إنشاء النافذة
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Catch the Stars! | Score: 0 | Lives: 3", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1); // تفعيل VSync لتثبيت سرعة اللعبة

    // --- 2. تهيئة GLEW ---
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // --- 3. إنشاء برنامج الشيدر ---
    unsigned int shaderProgram = createShaderProgram();

    // =================================================================
    // --- 4. إعداد VAO للمستطيل (باستخدام VBO + EBO) ---
    // =================================================================
    // مستطيل وحدة (1×1) متمركز عند نقطة الأصل
    // يُستخدم لرسم: المجداف، الأرضية، مؤشرات الحياة
    float rectVerts[] = {
        -0.5f, -0.5f, 0.0f,   // أسفل-يسار  (0)
         0.5f, -0.5f, 0.0f,   // أسفل-يمين  (1)
         0.5f,  0.5f, 0.0f,   // أعلى-يمين  (2)
        -0.5f,  0.5f, 0.0f    // أعلى-يسار  (3)
    };
    // فهارس لرسم مستطيل من مثلثين (باستخدام EBO)
    unsigned int rectIdx[] = {
        0, 1, 2,   // المثلث الأول
        2, 3, 0    // المثلث الثاني
    };

    unsigned int rectVAO, rectVBO, rectEBO;
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glGenBuffers(1, &rectEBO);

    glBindVertexArray(rectVAO);

    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVerts), rectVerts, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIdx), rectIdx, GL_STATIC_DRAW);

    // تعريف سمة الموقع (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // =================================================================
    // --- 5. إعداد VAO للمثلث (باستخدام VBO فقط، بدون EBO) ---
    // =================================================================
    // مثلث وحدة متمركز عند نقطة الأصل
    // يُستخدم لرسم: النجوم الساقطة، نجوم الخلفية
    float triVerts[] = {
         0.0f,  0.5f, 0.0f,   // أعلى
        -0.5f, -0.5f, 0.0f,   // أسفل-يسار
         0.5f, -0.5f, 0.0f    // أسفل-يمين
    };

    unsigned int triVAO, triVBO;
    glGenVertexArrays(1, &triVAO);
    glGenBuffers(1, &triVBO);

    glBindVertexArray(triVAO);

    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVerts), triVerts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // تهيئة أول نجمة
    resetStar();

    // =================================================================
    // --- 6. حلقة الرسم الرئيسية (Render Loop) ---
    // =================================================================
    while (!glfwWindowShouldClose(window))
    {
        // أ. معالجة المدخلات
        processInput(window);

        // ب. تحديث حالة اللعبة
        if (!gameOver)
        {
            // تحريك النجمة للأسفل
            starY -= starSpeedBase;

            // فحص التصادم مع المجداف
            if (checkCollision())
            {
                score++;
                starSpeedBase += 0.0004f; // زيادة السرعة تدريجياً
                resetStar();
                updateTitle(window);
            }

            // النجمة سقطت بدون التقاطها
            if (starY < -1.3f)
            {
                lives--;
                if (lives <= 0)
                    gameOver = true;
                resetStar();
                updateTitle(window);
            }
        }

        // ج. الرسم
        float t = (float)glfwGetTime();

        // لون الخلفية (سماء ليلية)
        if (gameOver)
            glClearColor(0.15f, 0.0f, 0.0f, 1.0f);
        else
            glClearColor(0.02f, 0.02f, 0.15f + 0.03f * sinf(t * 0.3f), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // --- رسم الأرضية (مستطيل أخضر) ---
        drawRect(shaderProgram, rectVAO,
                 0.0f, -0.95f, 2.2f, 0.12f,
                 0.15f, 0.45f, 0.15f);

        // --- رسم نجوم الخلفية الثابتة (تأثير تلألؤ) ---
        for (int i = 0; i < 8; i++)
        {
            float sx = sinf((float)i * 2.1f + 0.5f) * 0.85f;
            float sy = cosf((float)i * 1.7f + 0.3f) * 0.25f + 0.55f;
            float tw = 0.3f + 0.15f * sinf(t * (1.5f + i * 0.4f));
            drawTri(shaderProgram, triVAO,
                    sx, sy, 0.025f, 0.025f,
                    tw, tw, tw * 1.3f);
        }

        // --- رسم مجداف اللاعب (مستطيل بلون متغير) ---
        float pR = 0.3f + 0.15f * sinf(t * 2.0f);
        float pG = 0.5f + 0.15f * sinf(t * 2.5f);
        drawRect(shaderProgram, rectVAO,
                 playerX, playerY, playerWidth, playerHeight,
                 pR, pG, 0.95f);

        // --- رسم النجمة الساقطة (مثلث بلون نابض) ---
        if (!gameOver)
        {
            float pulse = 0.85f + 0.15f * sinf(t * 6.0f);
            drawTri(shaderProgram, triVAO,
                    starX, starY, 0.1f, 0.1f,
                    starR * pulse, starG * pulse, starB * pulse);
        }

        // --- رسم مؤشرات الحياة (مستطيلات حمراء صغيرة) ---
        for (int i = 0; i < lives; i++)
        {
            drawRect(shaderProgram, rectVAO,
                     -0.92f + i * 0.07f, 0.93f, 0.05f, 0.04f,
                     0.9f, 0.15f, 0.15f);
        }

        // --- شاشة انتهاء اللعبة (علامة + حمراء) ---
        if (gameOver)
        {
            float flash = 0.7f + 0.3f * sinf(t * 4.0f);
            drawRect(shaderProgram, rectVAO,
                     0.0f, 0.0f, 0.6f, 0.04f,
                     flash, 0.1f, 0.1f);
            drawRect(shaderProgram, rectVAO,
                     0.0f, 0.0f, 0.04f, 0.6f,
                     flash, 0.1f, 0.1f);
        }

        // د. تبديل الـ Buffers ومعالجة الأحداث
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- 7. التنظيف النهائي ---
    glDeleteVertexArrays(1, &rectVAO);
    glDeleteBuffers(1, &rectVBO);
    glDeleteBuffers(1, &rectEBO);
    glDeleteVertexArrays(1, &triVAO);
    glDeleteBuffers(1, &triVBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}