# وصف الكود التفصيلي - لعبة "التقط النجوم!" (Catch the Stars!)

## نظرة عامة

هذا المشروع هو لعبة 2D بسيطة مبنية باستخدام **OpenGL 3.3** مع مكتبات **GLEW** و **GLFW**.  
اللاعب يتحكم بمجداف في أسفل الشاشة لالتقاط النجوم الساقطة من الأعلى.

---

## هيكل الملفات

```
CG-main/
├── lect4/
│   ├── External Libraries/
│   │   ├── GLEW/          ← مكتبة GLEW (headers + libs)
│   │   └── GLFW/          ← مكتبة GLFW (headers + libs)
│   └── project4/
│       ├── project1.slnx  ← ملف الحل (Solution)
│       └── project1/
│           ├── main.cpp    ← الكود المصدري الرئيسي
│           └── project1.vcxproj ← إعدادات المشروع
├── presentation.html       ← العرض التقديمي
├── CODE_DESCRIPTION.md     ← هذا الملف
├── README.md              ← وصف المشروع
└── upload.sh              ← سكريبت الرفع على GitHub
```

---

## شرح الكود سطراً بسطر

### 1. التضمينات (Includes) - الأسطر 21-31

```cpp
#include <iostream>     // للطباعة على الكونسول
#include <cstdlib>      // لدالة rand() و srand()
#include <ctime>        // لدالة time() (بذرة العشوائية)
#include <string>       // لمعالجة النصوص (عنوان النافذة)
#include <cmath>        // لدالة sinf() و cosf() (التأثيرات البصرية)

#define GLEW_STATIC     // نستخدم النسخة الساكنة من GLEW (glew32s.lib)
#include <GL/glew.h>    // مكتبة GLEW - يجب أن تكون قبل GLFW
#include <GLFW/glfw3.h> // مكتبة GLFW - لإدارة النوافذ والمدخلات
```

**ملاحظة مهمة:** يجب تضمين `GLEW` قبل `GLFW` دائماً وإلا ستحدث أخطاء في الترجمة.

---

### 2. إعدادات النافذة - الأسطر 33-35

```cpp
const unsigned int SCR_WIDTH = 800;   // عرض النافذة بالبكسل
const unsigned int SCR_HEIGHT = 600;  // ارتفاع النافذة بالبكسل
```

النافذة بحجم 800×600 بكسل. نظام الإحداثيات في OpenGL يتراوح من -1.0 إلى +1.0 في كلا المحورين.

---

### 3. متغيرات حالة اللعبة (Game State) - الأسطر 37-54

```cpp
// اللاعب (المجداف)
float playerX = 0.0f;              // الموقع الأفقي (يبدأ من المنتصف)
const float playerY = -0.85f;      // الموقع العمودي (ثابت قرب الأسفل)
const float playerWidth = 0.3f;    // عرض المجداف
const float playerHeight = 0.06f;  // ارتفاع المجداف
const float playerSpeed = 0.025f;  // سرعة الحركة لكل إطار (frame)

// النجمة الساقطة
float starX = 0.0f;                // الموقع الأفقي (يتغير عشوائياً)
float starY = 1.2f;                // الموقع العمودي (يبدأ فوق الشاشة)
float starSpeedBase = 0.012f;      // سرعة السقوط (تزداد مع اللعب)
float starR, starG, starB;         // لون النجمة (RGB عشوائي)

// نظام النقاط
int score = 0;                     // عدد النجوم الملتقطة
int lives = 3;                     // عدد المحاولات المتبقية
bool gameOver = false;             // هل انتهت اللعبة؟
```

**لماذا `playerY` ثابت (const)؟** لأن المجداف يتحرك أفقياً فقط (يمين/يسار).

---

### 4. كود الشيدرز (Shaders) - الأسطر 56-78

#### أ. Vertex Shader

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;  // إدخال: موقع الرأس من VBO
uniform vec2 uOffset;                 // إزاحة الموقع (x, y)
uniform vec2 uScale;                  // مقياس الحجم (عرض, ارتفاع)

void main() {
    // الموقع النهائي = (الموقع الأصلي × المقياس) + الإزاحة
    gl_Position = vec4(
        aPos.x * uScale.x + uOffset.x,   // X النهائي
        aPos.y * uScale.y + uOffset.y,   // Y النهائي
        aPos.z,                           // Z (دائماً 0 في 2D)
        1.0                               // W (للإحداثيات المتجانسة)
    );
}
```

**الفكرة الأساسية:** بدلاً من إنشاء VBO منفصل لكل حجم وموقع، نستخدم VBO واحد (شكل وحدة 1×1) ونغير الحجم والموقع عبر Uniforms. هذا يوفر الذاكرة ويبسط الكود.

#### ب. Fragment Shader

```glsl
#version 330 core
out vec4 FragColor;      // إخراج: لون البكسل
uniform vec4 uColor;     // لون الكائن (RGBA)

void main() {
    FragColor = uColor;  // كل بكسلات الشكل بنفس اللون
}
```

**Uniform vs Attribute:** الـ Uniform قيمة واحدة لكل الرؤوس/البكسلات، بينما الـ Attribute (مثل aPos) قيمة مختلفة لكل رأس.

---

### 5. دالة معالجة المدخلات (processInput) - الأسطر 88-126

```cpp
void processInput(GLFWwindow* window) {
    // ESC = إغلاق النافذة
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (!gameOver) {
        // سهم يسار أو A = تحريك المجداف لليسار
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            playerX -= playerSpeed;
            // منع الخروج من حدود الشاشة
            if (playerX - playerWidth / 2.0f < -1.0f)
                playerX = -1.0f + playerWidth / 2.0f;
        }
        // سهم يمين أو D = تحريك المجداف لليمين
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            playerX += playerSpeed;
            if (playerX + playerWidth / 2.0f > 1.0f)
                playerX = 1.0f - playerWidth / 2.0f;
        }
    } else {
        // R = إعادة تشغيل اللعبة
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            score = 0; lives = 3; gameOver = false;
            starSpeedBase = 0.012f; playerX = 0.0f;
        }
    }
}
```

**حماية الحدود:** نتأكد أن المجداف لا يخرج من حافة الشاشة (-1.0 إلى 1.0) بحساب نصف العرض.

---

### 6. دالة إعادة تعيين النجمة (resetStar) - الأسطر 128-136

```cpp
void resetStar() {
    // موقع أفقي عشوائي بين -0.8 و +0.8
    starX = ((float)(rand() % 160) / 100.0f) - 0.8f;
    starY = 1.2f;  // فوق الشاشة
    
    // لون عشوائي (بين 0.4 و 1.0 لكل مكون)
    starR = (float)(rand() % 60 + 40) / 100.0f;
    starG = (float)(rand() % 60 + 40) / 100.0f;
    starB = (float)(rand() % 60 + 40) / 100.0f;
}
```

**لماذا بين 0.4 و 1.0؟** لضمان ألوان زاهية وواضحة (ألوان قريبة من 0 تكون معتمة جداً).

---

### 7. دالة فحص التصادم (checkCollision) - الأسطر 138-155

```cpp
bool checkCollision() {
    // حدود النجمة (نصف حجم = 0.05 لأن المقياس 0.1 × حجم الوحدة 0.5)
    float bLeft   = starX - 0.05f;
    float bRight  = starX + 0.05f;
    float bBottom = starY - 0.05f;

    // حدود المجداف
    float pLeft   = playerX - playerWidth / 2.0f;
    float pRight  = playerX + playerWidth / 2.0f;
    float pTop    = playerY + playerHeight / 2.0f;
    float pBottom = playerY - playerHeight / 2.0f;

    // فحص التداخل بين المستطيلين (AABB)
    return (bRight > pLeft && bLeft < pRight &&
            bBottom <= pTop && starY >= pBottom);
}
```

**خوارزمية AABB:** نفحص إذا كانت حدود الشكلين متداخلة في كلا المحورين (X و Y). إذا تداخلت = تصادم!

---

### 8. دوال تجميع الشيدر - الأسطر 174-216

```cpp
// تجميع شيدر واحد
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);        // إنشاء كائن الشيدر
    glShaderSource(shader, 1, &source, NULL);          // تحميل الكود المصدري
    glCompileShader(shader);                           // تجميع الكود
    // ... فحص الأخطاء ...
    return shader;
}

// إنشاء برنامج الشيدر (ربط Vertex + Fragment)
unsigned int createShaderProgram() {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    
    unsigned int program = glCreateProgram();          // إنشاء البرنامج
    glAttachShader(program, vs);                       // ربط Vertex Shader
    glAttachShader(program, fs);                       // ربط Fragment Shader
    glLinkProgram(program);                            // ربط البرنامج
    // ... فحص الأخطاء ...
    
    glDeleteShader(vs);  // حذف الشيدرز المنفصلة (تم ربطها)
    glDeleteShader(fs);
    return program;
}
```

**خطوات إنشاء Shader Program:**
1. `glCreateShader` → إنشاء كائن شيدر
2. `glShaderSource` → تحميل كود GLSL
3. `glCompileShader` → تجميع الكود
4. `glCreateProgram` → إنشاء برنامج
5. `glAttachShader` → ربط الشيدرز
6. `glLinkProgram` → ربط البرنامج النهائي

---

### 9. دوال الرسم (drawRect و drawTri) - الأسطر 218-240

```cpp
// رسم مستطيل (باستخدام EBO)
void drawRect(unsigned int program, unsigned int VAO,
              float x, float y, float w, float h,
              float r, float g, float b) {
    glUniform2f(glGetUniformLocation(program, "uOffset"), x, y);   // الموقع
    glUniform2f(glGetUniformLocation(program, "uScale"), w, h);    // الحجم
    glUniform4f(glGetUniformLocation(program, "uColor"), r, g, b, 1.0f);  // اللون
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // 6 فهارس = مستطيل
}

// رسم مثلث (بدون EBO)
void drawTri(unsigned int program, unsigned int VAO, ...) {
    // نفس Uniforms ...
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);  // 3 رؤوس = مثلث واحد
}
```

**الفرق الجوهري:**
| | drawRect | drawTri |
|---|---|---|
| **يستخدم EBO؟** | نعم | لا |
| **دالة الرسم** | `glDrawElements` | `glDrawArrays` |
| **عدد الرؤوس** | 4 (+ 6 فهارس) | 3 |
| **الشكل** | مستطيل (مثلثين) | مثلث واحد |

---

### 10. إعداد VAO للمستطيل (VBO + EBO) - الأسطر 281-315

```cpp
// رؤوس مستطيل الوحدة (1×1)
float rectVerts[] = {
    -0.5f, -0.5f, 0.0f,   // أسفل-يسار  (فهرس 0)
     0.5f, -0.5f, 0.0f,   // أسفل-يمين  (فهرس 1)
     0.5f,  0.5f, 0.0f,   // أعلى-يمين  (فهرس 2)
    -0.5f,  0.5f, 0.0f    // أعلى-يسار  (فهرس 3)
};

// فهارس لرسم مستطيل من مثلثين
unsigned int rectIdx[] = {
    0, 1, 2,   // المثلث الأول: أسفل-يسار → أسفل-يمين → أعلى-يمين
    2, 3, 0    // المثلث الثاني: أعلى-يمين → أعلى-يسار → أسفل-يسار
};

// إنشاء وربط الكائنات
glGenVertexArrays(1, &rectVAO);   // توليد VAO
glGenBuffers(1, &rectVBO);        // توليد VBO
glGenBuffers(1, &rectEBO);        // توليد EBO

glBindVertexArray(rectVAO);       // تفعيل VAO (يسجل كل ما يلي)

glBindBuffer(GL_ARRAY_BUFFER, rectVBO);                              // ربط VBO
glBufferData(GL_ARRAY_BUFFER, sizeof(rectVerts), rectVerts, GL_STATIC_DRAW);  // نسخ البيانات

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectEBO);                      // ربط EBO
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIdx), rectIdx, GL_STATIC_DRAW);

// تعريف سمة الموقع (location = 0 في الشيدر)
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
//                    ^  ^  ^         ^         ^                    ^
//                    |  |  |         |         |                    الإزاحة
//                    |  |  |         |         الخطوة (stride)
//                    |  |  |         لا تطبيع
//                    |  |  نوع البيانات
//                    |  عدد المكونات (x,y,z)
//                    الموقع في الشيدر
glEnableVertexAttribArray(0);     // تفعيل السمة

glBindVertexArray(0);             // فك ربط VAO
```

**لماذا EBO؟** بدون EBO نحتاج 6 رؤوس لمستطيل (3 × مثلثين). مع EBO نحتاج 4 رؤوس فقط + 6 فهارس. هذا يوفر الذاكرة خاصة مع الأشكال المعقدة.

---

### 11. إعداد VAO للمثلث (VBO فقط، بدون EBO) - الأسطر 317-340

```cpp
float triVerts[] = {
     0.0f,  0.5f, 0.0f,   // أعلى
    -0.5f, -0.5f, 0.0f,   // أسفل-يسار
     0.5f, -0.5f, 0.0f    // أسفل-يمين
};

glGenVertexArrays(1, &triVAO);
glGenBuffers(1, &triVBO);

glBindVertexArray(triVAO);
glBindBuffer(GL_ARRAY_BUFFER, triVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(triVerts), triVerts, GL_STATIC_DRAW);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
glBindVertexArray(0);
```

**ملاحظة:** المثلث لا يحتاج EBO لأنه 3 رؤوس فقط (لا تكرار). يُرسم مباشرة بـ `glDrawArrays`.

---

### 12. حلقة الرسم (Render Loop) - الأسطر 345-446

```
كل إطار (frame) يمر بالمراحل التالية:

┌─────────────────────────────────────┐
│  1. processInput()                  │  ← قراءة لوحة المفاتيح
├─────────────────────────────────────┤
│  2. تحديث حالة اللعبة:             │
│     - تحريك النجمة للأسفل          │
│     - فحص التصادم                   │
│     - تحديث النقاط/الحياة           │
├─────────────────────────────────────┤
│  3. الرسم بالترتيب:                │
│     a. glClearColor → تنظيف الخلفية │
│     b. drawRect → الأرضية           │
│     c. drawTri × 8 → نجوم الخلفية  │
│     d. drawRect → المجداف           │
│     e. drawTri → النجمة الساقطة     │
│     f. drawRect × lives → الحياة   │
│     g. drawRect × 2 → Game Over     │
├─────────────────────────────────────┤
│  4. glfwSwapBuffers() → عرض الإطار │
│  5. glfwPollEvents() → أحداث النظام│
└─────────────────────────────────────┘
```

#### التأثيرات البصرية في حلقة الرسم:

```cpp
float t = (float)glfwGetTime();  // الوقت بالثواني

// لون الخلفية يتغير مع الوقت
glClearColor(0.02f, 0.02f, 0.15f + 0.03f * sinf(t * 0.3f), 1.0f);

// نجوم الخلفية تتلألأ
float tw = 0.3f + 0.15f * sinf(t * (1.5f + i * 0.4f));  // سطوع متغير

// المجداف يتغير لونه
float pR = 0.3f + 0.15f * sinf(t * 2.0f);  // أحمر نابض
float pG = 0.5f + 0.15f * sinf(t * 2.5f);  // أخضر نابض

// النجمة الساقطة تنبض
float pulse = 0.85f + 0.15f * sinf(t * 6.0f);  // نبض سريع
```

**دالة sinf()** تعطي قيمة بين -1 و +1. بضربها في معامل صغير (0.15) وإضافة قيمة أساسية (0.85)، نحصل على تذبذب لطيف بين 0.7 و 1.0.

---

### 13. التنظيف النهائي - الأسطر 448-458

```cpp
glDeleteVertexArrays(1, &rectVAO);  // حذف VAOs
glDeleteVertexArrays(1, &triVAO);
glDeleteBuffers(1, &rectVBO);       // حذف VBOs
glDeleteBuffers(1, &triVBO);
glDeleteBuffers(1, &rectEBO);       // حذف EBO
glDeleteProgram(shaderProgram);     // حذف برنامج الشيدر

glfwTerminate();                    // إنهاء GLFW
```

**مهم جداً:** يجب تحرير كل الموارد التي أنشأناها لمنع تسرب الذاكرة (Memory Leak).

---

## ملخص المفاهيم المستخدمة

| المفهوم | الاستخدام في المشروع |
|---------|----------------------|
| **VBO** | `rectVBO` لتخزين 4 رؤوس المستطيل، `triVBO` لتخزين 3 رؤوس المثلث |
| **EBO** | `rectEBO` لتخزين 6 فهارس لرسم مستطيل من مثلثين |
| **VAO** | `rectVAO` يحفظ إعدادات المستطيل، `triVAO` يحفظ إعدادات المثلث |
| **Vertex Shader** | يحسب الموقع النهائي: `position = vertex × scale + offset` |
| **Fragment Shader** | يطبق اللون الموحد: `color = uColor` |
| **Uniform: uOffset** | يحدد موقع كل كائن (مجداف، نجمة، أرضية...) |
| **Uniform: uScale** | يحدد حجم كل كائن من نفس VBO |
| **Uniform: uColor** | يحدد لون كل كائن (مع تأثيرات sin(time)) |
| **glDrawElements** | رسم المستطيلات باستخدام EBO |
| **glDrawArrays** | رسم المثلثات بدون EBO |

---

## أسئلة مراجعة محتملة

1. **ما الفرق بين VBO و EBO؟**
   - VBO يخزن بيانات الرؤوس (مواقع، ألوان، إلخ). EBO يخزن فهارس الرؤوس لتجنب التكرار.

2. **ما فائدة VAO؟**
   - يحفظ تكوين سمات الرؤوس بحيث لا نحتاج لإعادة تعريفها كل إطار. فقط نربط VAO ونرسم.

3. **لماذا نستخدم Uniform بدل تعديل VBO مباشرة؟**
   - تعديل VBO مكلف (نسخ بيانات للـ GPU كل إطار). الـ Uniform سريع وخفيف لتمرير قيم بسيطة.

4. **كيف يعمل فحص التصادم (AABB)؟**
   - نقارن حدود الشكلين (يسار، يمين، أعلى، أسفل). إذا تداخلت في كلا المحورين = تصادم.

5. **ما وظيفة `glfwSwapInterval(1)`؟**
   - تفعيل VSync: تزامن تبديل الإطارات مع معدل تحديث الشاشة لمنع التمزق وتثبيت السرعة.

6. **لماذا `#define GLEW_STATIC` قبل include؟**
   - لأننا نربط مع `glew32s.lib` (النسخة الساكنة). بدونها سيحاول البحث عن DLL.

7. **كيف تم تحقيق تأثير التلألؤ؟**
   - باستخدام `sinf(glfwGetTime() * speed)` الذي يعطي قيمة متذبذبة، نضربها في اللون.

8. **لماذا نحذف الشيدرز بعد ربط البرنامج؟**
   - لأن البرنامج يحتوي على نسخة مربوطة. الشيدرز المنفصلة لم تعد مطلوبة وتشغل ذاكرة.
