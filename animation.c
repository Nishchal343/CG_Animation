/*
 * ISRO Mars Orbiter Mission (Mangalyaan) - 5 Minute Animation
 * Complete 2D Animation using OpenGL GLUT
 * 
 * Scenes (300 seconds total):
 *   0-10s:   Rocket on launch pad with Earth, ground, trees
 *  10-20s:   Rocket ignition with flames and smoke
 *  20-30s:   Countdown 10 to 1 with dynamic background
 *  30-40s:   Rocket launches upward
 *  40-60s:   Rocket travels through atmospheric layers
 *  60-90s:   Two-stage rocket separation
 *  90-120s:  Normal rocket limitation (failure scenario)
 * 120-150s:  ISRO gravity-assist slingshot solution
 * 150-180s:  Mars orbit attempt and stabilization
 * 180-210s:  Close-up orbital visualization around Mars
 * 210-240s:  Data transmission Mars to Earth
 * 240-270s:  Scientific mission - scanning Mars surface
 * 270-300s:  Mission success finale
 *
 * Compile: gcc animation.c -o animation -lGL -lGLU -lglut -lm
 * Windows: gcc animation.c -o animation.exe -lfreeglut -lopengl32 -lglu32 -lm
 */

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */
static float g_time = 0.0f;          /* elapsed time in seconds */
static int   g_winW = 1200;
static int   g_winH = 800;
static float g_dt   = 0.016f;        /* ~60 FPS */

/* Star field */
#define MAX_STARS 300
static float starX[MAX_STARS], starY[MAX_STARS], starBright[MAX_STARS];

/* Smoke particles */
#define MAX_SMOKE 200
typedef struct {
    float x, y;
    float vx, vy;
    float life;
    float size;
    float alpha;
} Particle;
static Particle smoke[MAX_SMOKE];
static int smokeCount = 0;

/* Flame particles */
#define MAX_FLAMES 100
static Particle flames[MAX_FLAMES];
static int flameCount = 0;

/* Signal wave particles for data transmission */
#define MAX_SIGNALS 50
typedef struct {
    float x, y;
    float progress;
    float speed;
    int active;
} SignalWave;
static SignalWave signals[MAX_SIGNALS];

/* ============================================================
 * UTILITY FUNCTIONS
 * ============================================================ */

static float lerp(float a, float b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return a + (b - a) * t;
}

static float smoothstep(float edge0, float edge1, float x) {
    float t = (x - edge0) / (edge1 - edge0);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static float easeOutCubic(float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    t = 1.0f - t;
    return 1.0f - t * t * t;
}

static float easeInOutQuad(float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    if (t < 0.5f) return 2.0f * t * t;
    return 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) / 2.0f;
}

static float randf(void) {
    return (float)rand() / (float)RAND_MAX;
}

static float randf_range(float lo, float hi) {
    return lo + randf() * (hi - lo);
}

/* ============================================================
 * TEXT RENDERING
 * ============================================================ */

static void drawText(float x, float y, const char *str, void *font) {
    glRasterPos2f(x, y);
    for (const char *c = str; *c; c++)
        glutBitmapCharacter(font, *c);
}

static void drawTextCentered(float cx, float cy, const char *str, void *font) {
    int len = 0;
    for (const char *c = str; *c; c++)
        len += glutBitmapWidth(font, *c);
    drawText(cx - len * 0.5f, cy, str, font);
}

static void drawTextLarge(float x, float y, const char *str) {
    drawText(x, y, str, GLUT_BITMAP_HELVETICA_18);
}

static void drawTextSmall(float x, float y, const char *str) {
    drawText(x, y, str, GLUT_BITMAP_HELVETICA_12);
}

/* Stroke-based large text for countdown */
static void drawStrokeText(float x, float y, float scale, const char *str) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);
    for (const char *c = str; *c; c++)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();
}

/* ============================================================
 * INITIALIZATION
 * ============================================================ */

static void initStars(void) {
    for (int i = 0; i < MAX_STARS; i++) {
        starX[i] = randf_range(0, g_winW);
        starY[i] = randf_range(0, g_winH);
        starBright[i] = randf_range(0.3f, 1.0f);
    }
}

static void initParticles(void) {
    memset(smoke, 0, sizeof(smoke));
    memset(flames, 0, sizeof(flames));
    smokeCount = 0;
    flameCount = 0;
}

static void initSignals(void) {
    for (int i = 0; i < MAX_SIGNALS; i++) {
        signals[i].active = 0;
    }
}

/* ============================================================
 * PARTICLE SYSTEMS
 * ============================================================ */

static void spawnSmoke(float x, float y, float spread, int count) {
    for (int i = 0; i < count && smokeCount < MAX_SMOKE; i++) {
        Particle *p = &smoke[smokeCount++];
        p->x = x + randf_range(-spread, spread);
        p->y = y + randf_range(-5, 5);
        p->vx = randf_range(-20, 20);
        p->vy = randf_range(-10, 30);
        p->life = randf_range(1.0f, 3.0f);
        p->size = randf_range(5, 20);
        p->alpha = randf_range(0.3f, 0.7f);
    }
}

static void spawnFlame(float x, float y, float spread) {
    if (flameCount >= MAX_FLAMES) flameCount = 0;
    Particle *p = &flames[flameCount++];
    p->x = x + randf_range(-spread, spread);
    p->y = y;
    p->vx = randf_range(-5, 5);
    p->vy = randf_range(-80, -30);
    p->life = randf_range(0.2f, 0.6f);
    p->size = randf_range(3, 12);
    p->alpha = 1.0f;
}

static void updateParticles(float dt) {
    /* Update smoke */
    for (int i = 0; i < smokeCount; i++) {
        smoke[i].x += smoke[i].vx * dt;
        smoke[i].y += smoke[i].vy * dt;
        smoke[i].life -= dt;
        smoke[i].alpha -= dt * 0.2f;
        smoke[i].size += dt * 8.0f;
        if (smoke[i].life <= 0) {
            smoke[i] = smoke[--smokeCount];
            i--;
        }
    }
    /* Update flames */
    for (int i = 0; i < flameCount; i++) {
        flames[i].x += flames[i].vx * dt;
        flames[i].y += flames[i].vy * dt;
        flames[i].life -= dt;
        flames[i].alpha -= dt * 2.0f;
        if (flames[i].life <= 0 || flames[i].alpha <= 0) {
            flames[i] = flames[--flameCount];
            i--;
        }
    }
}

static void drawSmoke(void) {
    for (int i = 0; i < smokeCount; i++) {
        float a = smoke[i].alpha;
        if (a < 0) a = 0;
        glColor4f(0.7f, 0.7f, 0.7f, a);
        float s = smoke[i].size;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(smoke[i].x, smoke[i].y);
        for (int j = 0; j <= 12; j++) {
            float ang = j * 2.0f * M_PI / 12.0f;
            glVertex2f(smoke[i].x + cosf(ang) * s,
                       smoke[i].y + sinf(ang) * s);
        }
        glEnd();
    }
}

static void drawFlames(void) {
    for (int i = 0; i < flameCount; i++) {
        float a = flames[i].alpha;
        if (a < 0) a = 0;
        float lifeRatio = flames[i].life / 0.6f;
        /* Color transitions from white -> yellow -> orange -> red */
        float r = 1.0f;
        float g = lerp(0.2f, 1.0f, lifeRatio);
        float b = lerp(0.0f, 0.8f, lifeRatio);
        glColor4f(r, g, b, a);
        float s = flames[i].size * lifeRatio;
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(flames[i].x, flames[i].y);
        for (int j = 0; j <= 8; j++) {
            float ang = j * 2.0f * M_PI / 8.0f;
            glVertex2f(flames[i].x + cosf(ang) * s,
                       flames[i].y + sinf(ang) * s);
        }
        glEnd();
    }
}

/* ============================================================
 * DRAWING PRIMITIVES
 * ============================================================ */

static void drawCircle(float cx, float cy, float r, int segs) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segs; i++) {
        float ang = i * 2.0f * M_PI / segs;
        glVertex2f(cx + cosf(ang) * r, cy + sinf(ang) * r);
    }
    glEnd();
}

static void drawCircleOutline(float cx, float cy, float r, int segs) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segs; i++) {
        float ang = i * 2.0f * M_PI / segs;
        glVertex2f(cx + cosf(ang) * r, cy + sinf(ang) * r);
    }
    glEnd();
}

static void drawRing(float cx, float cy, float innerR, float outerR, int segs) {
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segs; i++) {
        float ang = i * 2.0f * M_PI / segs;
        float c = cosf(ang), s = sinf(ang);
        glVertex2f(cx + c * innerR, cy + s * innerR);
        glVertex2f(cx + c * outerR, cy + s * outerR);
    }
    glEnd();
}

static void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

static void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
    glBegin(GL_TRIANGLES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glVertex2f(x3, y3);
    glEnd();
}

static void drawGradientRect(float x, float y, float w, float h,
                              float r1, float g1, float b1,
                              float r2, float g2, float b2) {
    glBegin(GL_QUADS);
    glColor3f(r1, g1, b1);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glColor3f(r2, g2, b2);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

/* ============================================================
 * SCENE ELEMENTS
 * ============================================================ */

/* --- Stars --- */
static void drawStarField(float twinkleSpeed) {
    for (int i = 0; i < MAX_STARS; i++) {
        float bright = starBright[i] * (0.5f + 0.5f * sinf(g_time * twinkleSpeed + i * 1.7f));
        glColor3f(bright, bright, bright * 1.1f);
        float sz = 1.0f + bright * 2.0f;
        drawRect(starX[i] - sz * 0.5f, starY[i] - sz * 0.5f, sz, sz);
    }
}

/* --- Ground --- */
static void drawGround(float groundY) {
    /* Grass */
    glColor3f(0.15f, 0.5f, 0.1f);
    drawRect(0, 0, g_winW, groundY);
    /* Grass detail */
    glColor3f(0.1f, 0.4f, 0.08f);
    for (int i = 0; i < g_winW; i += 15) {
        float h = 5 + 8 * sinf(i * 0.1f + g_time);
        drawRect(i, groundY - 2, 3, h);
    }
}

/* --- Tree --- */
static void drawTree(float x, float y, float scale) {
    /* Trunk */
    glColor3f(0.4f, 0.25f, 0.1f);
    drawRect(x - 5 * scale, y, 10 * scale, 40 * scale);
    /* Foliage layers */
    glColor3f(0.1f, 0.55f, 0.15f);
    drawTriangle(x - 25 * scale, y + 30 * scale,
                 x + 25 * scale, y + 30 * scale,
                 x, y + 70 * scale);
    glColor3f(0.12f, 0.6f, 0.18f);
    drawTriangle(x - 20 * scale, y + 50 * scale,
                 x + 20 * scale, y + 50 * scale,
                 x, y + 85 * scale);
    glColor3f(0.15f, 0.65f, 0.2f);
    drawTriangle(x - 15 * scale, y + 65 * scale,
                 x + 15 * scale, y + 65 * scale,
                 x, y + 95 * scale);
}

/* --- Launch Pad --- */
static void drawLaunchPad(float x, float y) {
    /* Base platform */
    glColor3f(0.4f, 0.4f, 0.45f);
    drawRect(x - 60, y, 120, 15);
    /* Support structures */
    glColor3f(0.5f, 0.5f, 0.55f);
    drawRect(x - 55, y + 15, 8, 80);
    drawRect(x + 47, y + 15, 8, 80);
    /* Cross beam */
    glColor3f(0.45f, 0.45f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(x - 55, y + 60);
    glVertex2f(x + 55, y + 60);
    glVertex2f(x + 55, y + 65);
    glVertex2f(x - 55, y + 65);
    glEnd();
    /* Tower details */
    glColor3f(0.6f, 0.2f, 0.2f);
    drawRect(x - 58, y + 85, 14, 25);
    glColor3f(0.55f, 0.55f, 0.6f);
    drawRect(x + 44, y + 85, 14, 25);
}

/* --- Rocket --- */
static void drawRocket(float x, float y, float scale, int showFlame) {
    float w = 20 * scale;
    float h = 80 * scale;
    
    /* Nose cone */
    glColor3f(0.9f, 0.9f, 0.92f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - w * 0.4f, y + h);
    glVertex2f(x + w * 0.4f, y + h);
    glVertex2f(x, y + h + 30 * scale);
    glEnd();
    
    /* Orange tip */
    glColor3f(0.9f, 0.5f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - w * 0.25f, y + h + 15 * scale);
    glVertex2f(x + w * 0.25f, y + h + 15 * scale);
    glVertex2f(x, y + h + 30 * scale);
    glEnd();
    
    /* Main body */
    glColor3f(0.85f, 0.85f, 0.88f);
    drawRect(x - w * 0.4f, y, w * 0.8f, h);
    
    /* ISRO stripe */
    glColor3f(0.9f, 0.5f, 0.1f);
    drawRect(x - w * 0.42f, y + h * 0.6f, w * 0.84f, h * 0.08f);
    
    /* India flag colors band */
    float bandY = y + h * 0.35f;
    float bandH = h * 0.05f;
    glColor3f(1.0f, 0.6f, 0.2f); /* Saffron */
    drawRect(x - w * 0.42f, bandY + bandH * 2, w * 0.84f, bandH);
    glColor3f(1.0f, 1.0f, 1.0f); /* White */
    drawRect(x - w * 0.42f, bandY + bandH, w * 0.84f, bandH);
    glColor3f(0.1f, 0.6f, 0.2f); /* Green */
    drawRect(x - w * 0.42f, bandY, w * 0.84f, bandH);
    
    /* Window / payload section */
    glColor3f(0.3f, 0.5f, 0.8f);
    drawCircle(x, y + h * 0.75f, 5 * scale, 12);
    glColor3f(0.5f, 0.7f, 1.0f);
    drawCircle(x, y + h * 0.75f, 3 * scale, 12);
    
    /* Fins */
    glColor3f(0.7f, 0.3f, 0.1f);
    /* Left fin */
    glBegin(GL_TRIANGLES);
    glVertex2f(x - w * 0.4f, y);
    glVertex2f(x - w * 0.4f, y + h * 0.25f);
    glVertex2f(x - w * 0.8f, y - 5 * scale);
    glEnd();
    /* Right fin */
    glBegin(GL_TRIANGLES);
    glVertex2f(x + w * 0.4f, y);
    glVertex2f(x + w * 0.4f, y + h * 0.25f);
    glVertex2f(x + w * 0.8f, y - 5 * scale);
    glEnd();
    
    /* Engine nozzle */
    glColor3f(0.3f, 0.3f, 0.35f);
    glBegin(GL_QUADS);
    glVertex2f(x - w * 0.25f, y);
    glVertex2f(x + w * 0.25f, y);
    glVertex2f(x + w * 0.35f, y - 10 * scale);
    glVertex2f(x - w * 0.35f, y - 10 * scale);
    glEnd();
    
    /* "ISRO" text on body */
    glColor3f(0.1f, 0.1f, 0.3f);
    if (scale > 0.5f) {
        drawTextCentered(x, y + h * 0.5f, "ISRO", GLUT_BITMAP_HELVETICA_12);
    }
    
    /* Exhaust flame */
    if (showFlame) {
        float flameH = 30 * scale * (0.8f + 0.4f * sinf(g_time * 20));
        float flameW = w * 0.3f * (0.8f + 0.3f * sinf(g_time * 15 + 1));
        
        /* Outer flame - orange/red */
        glColor4f(1.0f, 0.4f, 0.1f, 0.8f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x - flameW * 1.5f, y - 10 * scale);
        glVertex2f(x + flameW * 1.5f, y - 10 * scale);
        glVertex2f(x + sinf(g_time * 25) * 3, y - 10 * scale - flameH);
        glEnd();
        
        /* Inner flame - yellow/white */
        glColor4f(1.0f, 0.9f, 0.3f, 0.9f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x - flameW, y - 10 * scale);
        glVertex2f(x + flameW, y - 10 * scale);
        glVertex2f(x + sinf(g_time * 30) * 2, y - 10 * scale - flameH * 0.6f);
        glEnd();
        
        /* Core - white hot */
        glColor4f(1.0f, 1.0f, 0.9f, 0.95f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x - flameW * 0.4f, y - 10 * scale);
        glVertex2f(x + flameW * 0.4f, y - 10 * scale);
        glVertex2f(x, y - 10 * scale - flameH * 0.35f);
        glEnd();
    }
}

/* --- Spacecraft (smaller, for space scenes) --- */
static void drawSpacecraft(float x, float y, float scale, float angle) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);
    
    /* Main body */
    glColor3f(0.8f, 0.8f, 0.85f);
    drawRect(-8 * scale, -5 * scale, 16 * scale, 10 * scale);
    
    /* Solar panels */
    glColor3f(0.2f, 0.3f, 0.7f);
    drawRect(-25 * scale, -3 * scale, 15 * scale, 6 * scale);
    drawRect(10 * scale, -3 * scale, 15 * scale, 6 * scale);
    
    /* Panel grid lines */
    glColor3f(0.15f, 0.2f, 0.5f);
    glLineWidth(1.0f);
    for (int i = 0; i < 4; i++) {
        float lx = -25 * scale + i * 5 * scale;
        glBegin(GL_LINES);
        glVertex2f(lx, -3 * scale);
        glVertex2f(lx, 3 * scale);
        glEnd();
        lx = 10 * scale + i * 5 * scale;
        glBegin(GL_LINES);
        glVertex2f(lx, -3 * scale);
        glVertex2f(lx, 3 * scale);
        glEnd();
    }
    
    /* Antenna dish */
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_TRIANGLES);
    glVertex2f(8 * scale, 5 * scale);
    glVertex2f(12 * scale, 12 * scale);
    glVertex2f(4 * scale, 12 * scale);
    glEnd();
    
    /* Thruster */
    glColor3f(0.4f, 0.4f, 0.45f);
    drawRect(-10 * scale, -7 * scale, 4 * scale, 2 * scale);
    
    glPopMatrix();
}

/* --- Earth --- */
static void drawEarth(float cx, float cy, float r) {
    /* Ocean */
    glColor3f(0.1f, 0.3f, 0.8f);
    drawCircle(cx, cy, r, 60);
    
    /* Land masses */
    glColor3f(0.2f, 0.6f, 0.2f);
    /* Simple continent shapes */
    float t = g_time * 0.05f;
    for (int i = 0; i < 5; i++) {
        float angle = i * 1.2f + t;
        float lx = cx + cosf(angle) * r * 0.5f;
        float ly = cy + sinf(angle) * r * 0.4f;
        float lr = r * 0.25f + sinf(i * 2.1f) * r * 0.1f;
        /* Only draw if within circle */
        float dist = sqrtf((lx - cx) * (lx - cx) + (ly - cy) * (ly - cy));
        if (dist + lr < r) {
            drawCircle(lx, ly, lr, 8);
        }
    }
    
    /* Ice caps */
    glColor3f(0.9f, 0.95f, 1.0f);
    float iceSz = r * 0.15f;
    drawCircle(cx, cy + r * 0.85f, iceSz, 8);
    drawCircle(cx, cy - r * 0.85f, iceSz, 8);
    
    /* Atmosphere glow */
    glColor4f(0.3f, 0.6f, 1.0f, 0.15f);
    drawCircle(cx, cy, r * 1.08f, 60);
    
    /* Outline */
    glColor3f(0.2f, 0.4f, 0.9f);
    glLineWidth(2.0f);
    drawCircleOutline(cx, cy, r, 60);
}

/* --- Mars --- */
static void drawMars(float cx, float cy, float r) {
    /* Base color */
    glColor3f(0.8f, 0.35f, 0.15f);
    drawCircle(cx, cy, r, 60);
    
    /* Surface features */
    glColor3f(0.7f, 0.28f, 0.1f);
    for (int i = 0; i < 8; i++) {
        float angle = i * 0.8f + g_time * 0.02f;
        float fx = cx + cosf(angle) * r * 0.4f;
        float fy = cy + sinf(angle * 1.3f) * r * 0.35f;
        float fr = r * 0.12f + sinf(i * 3.14f) * r * 0.05f;
        float dist = sqrtf((fx - cx) * (fx - cx) + (fy - cy) * (fy - cy));
        if (dist + fr < r * 0.9f) {
            drawCircle(fx, fy, fr, 8);
        }
    }
    
    /* Darker regions */
    glColor3f(0.6f, 0.25f, 0.1f);
    for (int i = 0; i < 4; i++) {
        float angle = i * 1.5f + 0.5f;
        float fx = cx + cosf(angle) * r * 0.3f;
        float fy = cy + sinf(angle) * r * 0.5f;
        float dist = sqrtf((fx - cx) * (fx - cx) + (fy - cy) * (fy - cy));
        if (dist < r * 0.7f) {
            drawCircle(fx, fy, r * 0.08f, 6);
        }
    }
    
    /* Polar ice */
    glColor3f(0.9f, 0.85f, 0.8f);
    drawCircle(cx, cy + r * 0.88f, r * 0.1f, 8);
    
    /* Atmosphere */
    glColor4f(0.9f, 0.5f, 0.3f, 0.1f);
    drawCircle(cx, cy, r * 1.05f, 60);
    
    /* Outline */
    glColor3f(0.9f, 0.4f, 0.15f);
    glLineWidth(2.0f);
    drawCircleOutline(cx, cy, r, 60);
}

/* --- Sun (small, background) --- */
static void drawSun(float cx, float cy, float r) {
    /* Glow */
    glColor4f(1.0f, 0.9f, 0.3f, 0.15f);
    drawCircle(cx, cy, r * 1.5f, 30);
    glColor4f(1.0f, 0.85f, 0.2f, 0.3f);
    drawCircle(cx, cy, r * 1.2f, 30);
    /* Body */
    glColor3f(1.0f, 0.9f, 0.4f);
    drawCircle(cx, cy, r, 30);
    /* Rays */
    glColor4f(1.0f, 0.95f, 0.5f, 0.4f);
    glLineWidth(2.0f);
    for (int i = 0; i < 12; i++) {
        float ang = i * M_PI / 6.0f + g_time * 0.3f;
        glBegin(GL_LINES);
        glVertex2f(cx + cosf(ang) * r * 1.2f, cy + sinf(ang) * r * 1.2f);
        glVertex2f(cx + cosf(ang) * r * 1.8f, cy + sinf(ang) * r * 1.8f);
        glEnd();
    }
}

/* --- Sky gradient --- */
static void drawSkyGradient(float topR, float topG, float topB,
                             float botR, float botG, float botB) {
    glBegin(GL_QUADS);
    glColor3f(botR, botG, botB);
    glVertex2f(0, 0);
    glVertex2f(g_winW, 0);
    glColor3f(topR, topG, topB);
    glVertex2f(g_winW, g_winH);
    glVertex2f(0, g_winH);
    glEnd();
}

/* --- Atmosphere layers --- */
static void drawAtmosphereLayer(float y, float h, float r, float g, float b, 
                                 float a, const char *name) {
    glColor4f(r, g, b, a);
    drawRect(0, y, g_winW, h);
    /* Label */
    glColor4f(1, 1, 1, 0.8f);
    drawTextSmall(20, y + h * 0.5f, name);
    /* Border line */
    glColor4f(1, 1, 1, 0.3f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2f(0, y + h);
    glVertex2f(g_winW, y + h);
    glEnd();
}

/* --- Orbital Path --- */
static void drawOrbitPath(float cx, float cy, float rx, float ry, 
                           float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; i++) {
        float ang = i * 2.0f * M_PI / 100.0f;
        glVertex2f(cx + cosf(ang) * rx, cy + sinf(ang) * ry);
    }
    glEnd();
}

/* --- Signal Wave --- */
static void drawSignalWave(float x1, float y1, float x2, float y2, 
                            float progress, float amplitude) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    
    glColor4f(0.3f, 0.8f, 1.0f, 0.6f * (1.0f - progress));
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    int steps = 40;
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / steps;
        float px = x1 + dx * t;
        float py = y1 + dy * t;
        /* Wave perpendicular to direction */
        float nx = -dy / len;
        float ny = dx / len;
        float wave = sinf(t * 20.0f + progress * 30.0f) * amplitude *
                     sinf(t * M_PI); /* fade at ends */
        px += nx * wave;
        py += ny * wave;
        glVertex2f(px, py);
    }
    glEnd();
}

/* ============================================================
 * TIMER DISPLAY
 * ============================================================ */
static void drawTimer(void) {
    int totalSec = (int)g_time;
    if (totalSec > 300) totalSec = 300;
    int min = totalSec / 60;
    int sec = totalSec % 60;
    
    char buf[32];
    sprintf(buf, "T+ %d:%02d / 5:00", min, sec);
    
    /* Background box */
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    drawRect(g_winW - 170, g_winH - 35, 160, 28);
    
    /* Timer text */
    glColor3f(0.0f, 1.0f, 0.4f);
    drawTextLarge(g_winW - 165, g_winH - 25, buf);
    
    /* Progress bar */
    float progress = g_time / 300.0f;
    if (progress > 1.0f) progress = 1.0f;
    glColor4f(0.2f, 0.2f, 0.2f, 0.5f);
    drawRect(g_winW - 170, g_winH - 42, 160, 5);
    glColor3f(0.0f, 0.8f, 0.3f);
    drawRect(g_winW - 170, g_winH - 42, 160 * progress, 5);
}

/* ============================================================
 * BOTTOM INFO BOX
 * ============================================================ */
static void drawInfoBox(const char *title, const char *line1,
                         const char *line2, const char *line3,
                         const char *line4,
                         float titleR, float titleG, float titleB) {
    float boxW = g_winW - 40;
    float boxH = 105;
    float boxX = 20;
    float boxY = 10;
    
    /* Semi-transparent dark background */
    glColor4f(0.0f, 0.02f, 0.06f, 0.82f);
    drawRect(boxX, boxY, boxW, boxH);
    
    /* Accent border (top line) */
    glColor4f(titleR, titleG, titleB, 0.8f);
    drawRect(boxX, boxY + boxH - 3, boxW, 3);
    
    /* Side accent */
    glColor4f(titleR, titleG, titleB, 0.5f);
    drawRect(boxX, boxY, 3, boxH);
    
    /* Subtle inner glow at top */
    glColor4f(titleR, titleG, titleB, 0.06f);
    drawRect(boxX, boxY + boxH - 25, boxW, 22);
    
    /* Title */
    glColor4f(titleR, titleG, titleB, 1.0f);
    drawText(boxX + 15, boxY + boxH - 20, title, GLUT_BITMAP_HELVETICA_18);
    
    /* Separator line under title */
    glColor4f(titleR, titleG, titleB, 0.25f);
    drawRect(boxX + 15, boxY + boxH - 28, boxW - 30, 1);
    
    /* Description lines */
    float lineY = boxY + boxH - 45;
    float lineSpacing = 17;
    
    if (line1 && line1[0]) {
        glColor4f(0.85f, 0.9f, 0.95f, 0.9f);
        drawText(boxX + 15, lineY, line1, GLUT_BITMAP_HELVETICA_12);
        lineY -= lineSpacing;
    }
    if (line2 && line2[0]) {
        glColor4f(0.75f, 0.82f, 0.88f, 0.8f);
        drawText(boxX + 15, lineY, line2, GLUT_BITMAP_HELVETICA_12);
        lineY -= lineSpacing;
    }
    if (line3 && line3[0]) {
        glColor4f(0.65f, 0.72f, 0.78f, 0.7f);
        drawText(boxX + 15, lineY, line3, GLUT_BITMAP_HELVETICA_12);
        lineY -= lineSpacing;
    }
    if (line4 && line4[0]) {
        glColor4f(0.55f, 0.62f, 0.68f, 0.65f);
        drawText(boxX + 15, lineY, line4, GLUT_BITMAP_HELVETICA_12);
    }
}

/* ============================================================
 * SCENE RENDERING FUNCTIONS
 * ============================================================ */

/* Scene 1: 0-10s - Rocket on launch pad */
static void drawScene_PadIdle(float sceneTime) {
    float groundY = 120;
    
    /* Sky */
    drawSkyGradient(0.1f, 0.15f, 0.4f,  /* top - dark blue */
                    0.4f, 0.55f, 0.85f);  /* bottom - light blue */
    
    /* Sun */
    drawSun(g_winW - 120, g_winH - 100, 40);
    
    /* Clouds */
    glColor4f(1, 1, 1, 0.6f);
    for (int i = 0; i < 5; i++) {
        float cx = 100 + i * 250 + sinf(g_time * 0.1f + i) * 20;
        float cy = g_winH - 180 + sinf(i * 1.5f) * 40;
        drawCircle(cx, cy, 30, 12);
        drawCircle(cx + 25, cy + 5, 25, 12);
        drawCircle(cx - 20, cy - 5, 20, 12);
    }
    
    /* Mountains in background */
    glColor3f(0.3f, 0.35f, 0.25f);
    for (int i = 0; i < 8; i++) {
        float mx = i * 180 - 50;
        float mh = 60 + sinf(i * 1.2f) * 40;
        drawTriangle(mx, groundY, mx + 90, groundY, mx + 45, groundY + mh);
    }
    
    /* Ground */
    drawGround(groundY);
    
    /* Trees */
    drawTree(80, groundY, 1.0f);
    drawTree(200, groundY, 0.8f);
    drawTree(900, groundY, 1.1f);
    drawTree(1050, groundY, 0.7f);
    drawTree(1120, groundY, 0.9f);
    
    /* Launch pad */
    float padX = g_winW * 0.5f;
    drawLaunchPad(padX, groundY);
    
    /* Rocket */
    drawRocket(padX, groundY + 15, 1.2f, 0);
    
    /* Info Box */
    drawInfoBox("ISRO Mars Orbiter Mission (Mangalyaan)",
                "Launch Vehicle: PSLV-C25 | Payload: Mars Orbiter Spacecraft (MOM)",
                "Launch Site: Satish Dhawan Space Centre, Sriharikota, Andhra Pradesh",
                "Date: 5 November 2013 | Mission Cost: Rs 450 Crore (~$74 Million)",
                "India's first interplanetary mission - designed to orbit Mars and study its atmosphere",
                0.3f, 0.8f, 1.0f);
}

/* Scene 2: 10-20s - Ignition */
static void drawScene_Ignition(float sceneTime) {
    float groundY = 120;
    float padX = g_winW * 0.5f;
    
    /* Sky */
    float flashIntensity = 0.0f;
    if (sceneTime < 1.0f) flashIntensity = (1.0f - sceneTime) * 0.3f;
    drawSkyGradient(0.1f + flashIntensity, 0.15f + flashIntensity, 0.4f,
                    0.4f + flashIntensity, 0.55f + flashIntensity, 0.85f);
    
    drawSun(g_winW - 120, g_winH - 100, 40);
    
    /* Mountains */
    glColor3f(0.3f, 0.35f, 0.25f);
    for (int i = 0; i < 8; i++) {
        float mx = i * 180 - 50;
        float mh = 60 + sinf(i * 1.2f) * 40;
        drawTriangle(mx, groundY, mx + 90, groundY, mx + 45, groundY + mh);
    }
    
    drawGround(groundY);
    
    /* Trees */
    drawTree(80, groundY, 1.0f);
    drawTree(200, groundY, 0.8f);
    drawTree(900, groundY, 1.1f);
    drawTree(1050, groundY, 0.7f);
    
    drawLaunchPad(padX, groundY);
    
    /* Ignition intensity ramps up */
    float intensity = smoothstep(0.0f, 5.0f, sceneTime);
    
    /* Rocket with increasing flame */
    drawRocket(padX, groundY + 15, 1.2f, (sceneTime > 0.5f) ? 1 : 0);
    
    /* Spawn smoke/flames */
    if (sceneTime > 0.5f) {
        for (int i = 0; i < (int)(intensity * 3) + 1; i++) {
            spawnSmoke(padX, groundY, 40 * intensity, 1);
            spawnFlame(padX, groundY + 5, 10 * intensity);
        }
    }
    
    drawSmoke();
    drawFlames();
    
    /* Glow effect at base */
    if (sceneTime > 0.5f) {
        glColor4f(1.0f, 0.6f, 0.1f, 0.2f * intensity);
        drawCircle(padX, groundY, 80 * intensity, 20);
        glColor4f(1.0f, 0.8f, 0.3f, 0.15f * intensity);
        drawCircle(padX, groundY, 120 * intensity, 20);
    }
    
    /* Info Box */
    drawInfoBox("Engine Ignition Sequence Initiated",
                "The Vikas engine (liquid-fueled) ignites with a thrust of 725 kN.",
                "Four solid-fuel strap-on boosters provide additional 502 kN thrust each.",
                "Ground hold-down clamps keep the rocket stationary during thrust buildup.",
                "Engine health parameters are verified before release - all systems nominal.",
                1.0f, 0.5f, 0.1f);
}

/* Scene 3: 20-30s - Countdown */
static void drawScene_Countdown(float sceneTime) {
    float groundY = 120;
    float padX = g_winW * 0.5f;
    
    /* Shaking sky for tension */
    float shake = sinf(g_time * 30) * 2.0f;
    
    glPushMatrix();
    glTranslatef(shake * 0.5f, shake * 0.3f, 0);
    
    drawSkyGradient(0.1f, 0.12f, 0.3f, 0.35f, 0.45f, 0.7f);
    drawSun(g_winW - 120, g_winH - 100, 40);
    
    /* Mountains */
    glColor3f(0.3f, 0.35f, 0.25f);
    for (int i = 0; i < 8; i++) {
        float mx = i * 180 - 50;
        float mh = 60 + sinf(i * 1.2f) * 40;
        drawTriangle(mx, groundY, mx + 90, groundY, mx + 45, groundY + mh);
    }
    
    drawGround(groundY);
    
    /* Moving trees for dynamic feel */
    float treeOffset = sinf(g_time * 2) * 10;
    drawTree(80 + treeOffset, groundY, 1.0f);
    drawTree(200 - treeOffset, groundY, 0.8f);
    drawTree(900 + treeOffset * 0.5f, groundY, 1.1f);
    drawTree(1050 - treeOffset * 0.7f, groundY, 0.7f);
    
    drawLaunchPad(padX, groundY);
    drawRocket(padX, groundY + 15, 1.2f, 1);
    
    /* Heavy smoke and flames */
    for (int i = 0; i < 5; i++) {
        spawnSmoke(padX, groundY, 50, 1);
        spawnFlame(padX, groundY + 5, 12);
    }
    drawSmoke();
    drawFlames();
    
    /* Big glow */
    glColor4f(1.0f, 0.6f, 0.1f, 0.3f);
    drawCircle(padX, groundY, 100, 20);
    
    glPopMatrix();
    
    /* Countdown number */
    int countNum = 10 - (int)sceneTime;
    if (countNum < 1) countNum = 1;
    
    char countStr[8];
    sprintf(countStr, "%d", countNum);
    
    /* Pulsing countdown */
    float pulse = 1.0f + 0.3f * sinf(sceneTime * M_PI * 2);
    float frac = sceneTime - (int)sceneTime;
    float countAlpha = 1.0f - frac * 0.5f;
    
    glColor4f(1.0f, 0.3f, 0.1f, countAlpha);
    glLineWidth(3.0f);
    drawStrokeText(g_winW * 0.5f - 30 * pulse, g_winH * 0.6f, 
                   0.5f * pulse, countStr);
    
    /* Info Box */
    drawInfoBox("Launch Countdown in Progress",
                "All pre-launch checks complete. Flight computer in autonomous mode.",
                "Liquid propellant tanks pressurized. Strap-on boosters armed and ready.",
                "Range safety systems active. Weather conditions: Clear for launch.",
                "Mission control confirms GO for launch - countdown proceeding nominally.",
                1.0f, 0.9f, 0.3f);
}

/* Scene 4: 30-40s - Launch */
static void drawScene_Launch(float sceneTime) {
    float groundY = 120;
    float padX = g_winW * 0.5f;
    
    /* Rocket rises with acceleration */
    float rocketY = groundY + 15 + sceneTime * sceneTime * 15.0f;
    
    /* Camera doesn't follow initially */
    float camOffset = 0;
    if (sceneTime > 3.0f) {
        camOffset = (sceneTime - 3.0f) * (sceneTime - 3.0f) * 10.0f;
    }
    
    /* Screen shake during launch */
    float shake = (1.0f - smoothstep(0, 8, sceneTime)) * sinf(g_time * 40) * 3.0f;
    
    glPushMatrix();
    glTranslatef(shake, -camOffset + shake * 0.5f, 0);
    
    drawSkyGradient(0.08f, 0.1f, 0.3f, 0.3f, 0.45f, 0.75f);
    drawSun(g_winW - 120, g_winH - 100, 40);
    
    /* Mountains */
    glColor3f(0.3f, 0.35f, 0.25f);
    for (int i = 0; i < 8; i++) {
        float mx = i * 180 - 50;
        float mh = 60 + sinf(i * 1.2f) * 40;
        drawTriangle(mx, groundY, mx + 90, groundY, mx + 45, groundY + mh);
    }
    
    drawGround(groundY);
    drawTree(80, groundY, 1.0f);
    drawTree(200, groundY, 0.8f);
    drawTree(900, groundY, 1.1f);
    drawTree(1050, groundY, 0.7f);
    
    drawLaunchPad(padX, groundY);
    
    /* Smoke trail */
    for (int i = 0; i < 8; i++) {
        spawnSmoke(padX, rocketY - 10, 30, 1);
        spawnFlame(padX, rocketY, 15);
    }
    drawSmoke();
    drawFlames();
    
    /* Rocket ascending */
    drawRocket(padX, rocketY, 1.2f, 1);
    
    /* Ground glow fading */
    float glowFade = 1.0f - smoothstep(0, 5, sceneTime);
    glColor4f(1.0f, 0.5f, 0.1f, 0.3f * glowFade);
    drawCircle(padX, groundY, 120, 20);
    
    glPopMatrix();
    
    /* Info Box */
    {
        char altBuf[128];
        float alt = sceneTime * sceneTime * 0.5f;
        sprintf(altBuf, "Current Altitude: %.1f km | Velocity: %.0f km/h | All systems nominal.", alt, 1200 + sceneTime * 300);
        drawInfoBox("LIFTOFF! PSLV-C25 Has Cleared the Tower",
                    altBuf,
                    "The 44.4-meter tall PSLV-C25 rocket lifts off with 297 tonnes of thrust.",
                    "Four ground-lit strap-on boosters fire simultaneously with the core stage.",
                    "Pitch and roll programs initiated. Vehicle tracking nominal on all stations.",
                    1.0f, 0.85f, 0.2f);
    }
}

/* Scene 5: 40-60s - Atmospheric layers */
static void drawScene_Atmosphere(float sceneTime) {
    /* Background transitions from blue sky to dark space */
    float spaceT = smoothstep(0, 20, sceneTime);
    
    float topR = lerp(0.05f, 0.0f, spaceT);
    float topG = lerp(0.08f, 0.0f, spaceT);
    float topB = lerp(0.2f, 0.05f, spaceT);
    float botR = lerp(0.3f, 0.05f, spaceT);
    float botG = lerp(0.5f, 0.1f, spaceT);
    float botB = lerp(0.8f, 0.3f, spaceT);
    
    drawSkyGradient(topR, topG, topB, botR, botG, botB);
    
    /* Stars appear as we go higher */
    if (spaceT > 0.3f) {
        float starAlpha = (spaceT - 0.3f) / 0.7f;
        for (int i = 0; i < MAX_STARS; i++) {
            float bright = starBright[i] * starAlpha * 
                          (0.5f + 0.5f * sinf(g_time * 2 + i));
            glColor3f(bright, bright, bright);
            float sz = 1.0f + bright;
            drawRect(starX[i] - sz * 0.5f, starY[i] - sz * 0.5f, sz, sz);
        }
    }
    
    /* Atmospheric layers scroll down */
    float scrollY = sceneTime * 40.0f;
    
    /* Layer definitions */
    typedef struct { float baseY; float height; float r, g, b, a; const char *name; } Layer;
    Layer layers[] = {
        { 0,   150, 0.3f, 0.6f, 0.9f, 0.3f, "Troposphere (0-12 km)" },
        { 150, 120, 0.2f, 0.4f, 0.8f, 0.25f, "Stratosphere (12-50 km)" },
        { 270, 100, 0.15f, 0.2f, 0.6f, 0.2f, "Mesosphere (50-80 km)" },
        { 370, 100, 0.1f, 0.1f, 0.4f, 0.15f, "Thermosphere (80-700 km)" },
        { 470, 150, 0.05f, 0.05f, 0.15f, 0.1f, "Exosphere (700+ km)" },
    };
    int nLayers = 5;
    
    for (int i = 0; i < nLayers; i++) {
        float ly = layers[i].baseY - scrollY + g_winH * 0.3f;
        if (ly + layers[i].height > 0 && ly < g_winH) {
            drawAtmosphereLayer(ly, layers[i].height,
                               layers[i].r, layers[i].g, layers[i].b, layers[i].a,
                               layers[i].name);
        }
    }
    
    /* Rocket in center moving up */
    float rocketScale = lerp(1.2f, 0.8f, spaceT);
    float rocketY = g_winH * 0.4f + sinf(sceneTime * 0.5f) * 20;
    drawRocket(g_winW * 0.5f, rocketY, rocketScale, 1);
    
    /* Flames and particles */
    for (int i = 0; i < 3; i++) {
        spawnFlame(g_winW * 0.5f, rocketY, 8 * rocketScale);
    }
    drawFlames();
    
    /* Info Box */
    {
        char line1Buf[128];
        float speed = 2000 + sceneTime * 500;
        float alt = 10 + sceneTime * 35;
        sprintf(line1Buf, "Velocity: %.0f km/h | Altitude: %.0f km | Ascending through atmosphere.", speed, alt);
        drawInfoBox("Ascending Through Earth's Atmosphere",
                    line1Buf,
                    "Earth's atmosphere has 5 layers: Troposphere, Stratosphere, Mesosphere, Thermosphere, Exosphere.",
                    "As altitude increases, air density drops and the sky transitions from blue to black.",
                    "Aerodynamic forces decrease as the rocket leaves the dense lower atmosphere.",
                    0.3f, 0.7f, 1.0f);
    }
}

/* Scene 6: 60-90s - Stage separation */
static void drawScene_StageSeparation(float sceneTime) {
    /* Space background */
    glClearColor(0.02f, 0.02f, 0.06f, 1.0f);
    glColor3f(0.02f, 0.02f, 0.06f);
    drawRect(0, 0, g_winW, g_winH);
    
    drawStarField(3.0f);
    
    /* Earth below */
    float earthY = -200 - sceneTime * 5;
    drawEarth(g_winW * 0.5f, earthY, 300);
    
    /* Stage separation phases */
    float cx = g_winW * 0.5f;
    float cy = g_winH * 0.5f;
    
    if (sceneTime < 10.0f) {
        /* Phase 1: Full rocket, approaching separation */
        float rocketY = cy - 40;
        
        /* First stage (lower) */
        float sepDist = 0;
        if (sceneTime > 7.0f) {
            sepDist = (sceneTime - 7.0f) * (sceneTime - 7.0f) * 15.0f;
        }
        
        /* Draw first stage (booster) */
        glColor3f(0.75f, 0.75f, 0.8f);
        drawRect(cx - 12, rocketY - sepDist - 50, 24, 40);
        /* Booster fins */
        glColor3f(0.6f, 0.25f, 0.1f);
        drawTriangle(cx - 12, rocketY - sepDist - 50,
                     cx - 12, rocketY - sepDist - 35,
                     cx - 22, rocketY - sepDist - 55);
        drawTriangle(cx + 12, rocketY - sepDist - 50,
                     cx + 12, rocketY - sepDist - 35,
                     cx + 22, rocketY - sepDist - 55);
        /* Booster nozzle */
        glColor3f(0.3f, 0.3f, 0.35f);
        drawRect(cx - 8, rocketY - sepDist - 58, 16, 8);
        
        /* Second stage + payload (upper) */
        glColor3f(0.85f, 0.85f, 0.88f);
        drawRect(cx - 10, rocketY - 10, 20, 60);
        /* Nose cone */
        glColor3f(0.9f, 0.5f, 0.1f);
        glBegin(GL_TRIANGLES);
        glVertex2f(cx - 10, rocketY + 50);
        glVertex2f(cx + 10, rocketY + 50);
        glVertex2f(cx, rocketY + 75);
        glEnd();
        /* ISRO marking */
        glColor3f(0.9f, 0.5f, 0.1f);
        drawRect(cx - 11, rocketY + 25, 22, 4);
        
        /* Flame on upper stage */
        drawRocket(cx, rocketY - 10, 0.6f, 1);
        
        /* Separation flash */
        if (sceneTime > 7.0f && sceneTime < 8.0f) {
            float flashA = 1.0f - (sceneTime - 7.0f);
            glColor4f(1, 0.9f, 0.5f, flashA * 0.5f);
            drawCircle(cx, rocketY - 15, 30, 16);
            
            /* Separation debris */
            for (int i = 0; i < 8; i++) {
                float ang = i * M_PI / 4.0f + g_time;
                float dist = (sceneTime - 7.0f) * 50;
                float dx = cosf(ang) * dist;
                float dy = sinf(ang) * dist;
                glColor4f(0.8f, 0.8f, 0.5f, 0.5f * (1.0f - (sceneTime - 7.0f)));
                drawCircle(cx + dx, rocketY - 15 + dy, 2, 4);
            }
        }
        
        /* (text moved to bottom info box) */
    }
    else if (sceneTime < 20.0f) {
        /* Phase 2: Second stage burn with first stage falling away */
        float t2 = sceneTime - 10.0f;
        
        /* Falling first stage */
        float fallY = cy - 80 - t2 * t2 * 8;
        float fallRot = t2 * 15;
        if (fallY > -100) {
            glPushMatrix();
            glTranslatef(cx - t2 * 10, fallY, 0);
            glRotatef(fallRot, 0, 0, 1);
            glColor3f(0.5f, 0.5f, 0.55f);
            drawRect(-12, -25, 24, 40);
            glColor3f(0.4f, 0.2f, 0.1f);
            drawTriangle(-12, -25, -12, -10, -22, -30);
            drawTriangle(12, -25, 12, -10, 22, -30);
            glPopMatrix();
        }
        
        /* Upper stage continuing */
        float upperY = cy + t2 * 3;
        drawRocket(cx, upperY, 0.7f, 1);
        
        /* Flames */
        for (int i = 0; i < 3; i++) {
            spawnFlame(cx, upperY, 6);
        }
        drawFlames();
        
        /* Payload fairing separation at t2 > 5 */
        if (t2 > 5.0f) {
            float fairSep = (t2 - 5.0f) * 20;
            /* Left fairing */
            glPushMatrix();
            glTranslatef(cx - fairSep, upperY + 50, 0);
            glRotatef(-fairSep * 2, 0, 0, 1);
            glColor4f(0.8f, 0.8f, 0.85f, 1.0f - (t2 - 5.0f) * 0.15f);
            glBegin(GL_TRIANGLES);
            glVertex2f(-8, 0);
            glVertex2f(0, 0);
            glVertex2f(-4, 20);
            glEnd();
            glPopMatrix();
            /* Right fairing */
            glPushMatrix();
            glTranslatef(cx + fairSep, upperY + 50, 0);
            glRotatef(fairSep * 2, 0, 0, 1);
            glColor4f(0.8f, 0.8f, 0.85f, 1.0f - (t2 - 5.0f) * 0.15f);
            glBegin(GL_TRIANGLES);
            glVertex2f(0, 0);
            glVertex2f(8, 0);
            glVertex2f(4, 20);
            glEnd();
            glPopMatrix();
            
            /* (text moved to bottom info box) */
        }
    }
    else {
        /* Phase 3: Final stage - spacecraft deployment */
        float t3 = sceneTime - 20.0f;
        
        /* Upper stage */
        float stageY = cy - t3 * 5;
        glColor3f(0.6f, 0.6f, 0.65f);
        drawRect(cx - 8, stageY - 20, 16, 30);
        glColor3f(0.3f, 0.3f, 0.35f);
        drawRect(cx - 6, stageY - 25, 12, 5);
        
        /* Spacecraft separating upward */
        float scSep = t3 * t3 * 3;
        float scY = stageY + 20 + scSep;
        float scAngle = t3 * 5;
        drawSpacecraft(cx, scY, 2.0f, scAngle);
        
        /* (text moved to bottom info box) */
    }
    
    /* Info Box */
    if (sceneTime < 10) {
        drawInfoBox("Stage Separation - Phase 1: Booster Jettison",
                    "PSLV uses a 4-stage propulsion system. The first stage (solid motor) has burned out.",
                    "Explosive bolts fire to detach the spent first stage from the upper stages.",
                    "Separation occurs at ~110 km altitude. Second stage (Vikas engine) ignites.",
                    "Precise timing is critical - even milliseconds of delay can alter the trajectory.",
                    1.0f, 0.5f, 0.2f);
    } else if (sceneTime < 20) {
        drawInfoBox("Stage Separation - Phase 2: Upper Stage Burn & Fairing Jettison",
                    "Second stage Vikas engine burns liquid fuel (UDMH + N2O4) for 150 seconds.",
                    "Payload fairing (nose cone) is jettisoned - no longer needed above atmosphere.",
                    "This reduces weight significantly, allowing higher velocity for orbit insertion.",
                    "Third stage (solid) and fourth stage (liquid) will complete orbit injection.",
                    0.8f, 0.6f, 0.2f);
    } else {
        drawInfoBox("Spacecraft Deployment - Mars Orbiter Released",
                    "The Mars Orbiter Spacecraft separates from the PSLV 4th stage at 298 km altitude.",
                    "Solar panels deploy automatically to begin charging the onboard batteries.",
                    "Spacecraft mass: 1,337 kg (852 kg dry + 852 kg fuel). Power: 840W solar panels.",
                    "MOM enters Earth parking orbit. Orbit-raising maneuvers begin in the coming days.",
                    0.3f, 1.0f, 0.5f);
    }
}

/* Scene 7: 90-120s - Normal rocket failure */
static void drawScene_RocketFailure(float sceneTime) {
    /* Deep space background */
    glColor3f(0.01f, 0.01f, 0.04f);
    drawRect(0, 0, g_winW, g_winH);
    drawStarField(2.0f);
    
    /* Earth on left */
    drawEarth(150, g_winH * 0.5f, 100);
    glColor4f(1, 1, 1, 0.6f);
    drawTextSmall(120, g_winH * 0.5f - 120, "Earth");
    
    /* Mars on right (far) */
    drawMars(g_winW - 150, g_winH * 0.5f + 50, 80);
    glColor4f(1, 1, 1, 0.6f);
    drawTextSmall(g_winW - 180, g_winH * 0.5f - 50, "Mars");
    
    /* Sun in background */
    drawSun(g_winW * 0.5f, g_winH - 80, 30);
    
    if (sceneTime < 15.0f) {
        /* Show normal direct trajectory attempt */
        /* Spacecraft moving in straight line */
        float progress = sceneTime / 15.0f;
        float scX = lerp(250, g_winW - 250, progress);
        float scY = lerp(g_winH * 0.5f, g_winH * 0.5f + 50, progress);
        
        drawSpacecraft(scX, scY, 1.5f, 45.0f);
        
        /* Direct path line */
        glColor4f(0.5f, 0.5f, 1.0f, 0.3f);
        glLineWidth(1.0f);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 20; i++) {
            float t = (float)i / 20.0f;
            glVertex2f(lerp(250, g_winW - 250, t),
                      lerp(g_winH * 0.5f, g_winH * 0.5f + 50, t));
        }
        glEnd();
        
        /* Fuel gauge depleting */
        float fuelW = 150;
        float fuelH = 15;
        float fuelX = 20;
        float fuelY = g_winH - 120;
        float fuel = 1.0f - progress * 1.3f; /* Runs out before reaching */
        if (fuel < 0) fuel = 0;
        
        glColor4f(0.2f, 0.2f, 0.2f, 0.7f);
        drawRect(fuelX, fuelY, fuelW, fuelH);
        
        float fuelColor = fuel > 0.3f ? 0.3f : 1.0f;
        glColor3f(1.0f - fuel + fuelColor, fuel, 0.1f);
        drawRect(fuelX, fuelY, fuelW * fuel, fuelH);
        
        glColor3f(1, 1, 1);
        drawTextSmall(fuelX, fuelY + fuelH + 5, "FUEL REMAINING");
        
        /* Distance remaining */
        float distRemaining = (1.0f - progress) * 225; /* Million km */
        char distBuf[64];
        sprintf(distBuf, "Distance to Mars: %.0f M km", distRemaining);
        glColor4f(1, 1, 1, 0.7f);
        drawTextSmall(fuelX, fuelY - 20, distBuf);
        
        /* (title moved to info box) */
    }
    else {
        /* Failure visualization */
        float failT = sceneTime - 15.0f;
        
        /* Spacecraft stranded in middle */
        float scX = g_winW * 0.55f;
        float scY = g_winH * 0.5f + 30;
        
        /* Spacecraft tumbling */
        float tumbleAngle = failT * 30;
        drawSpacecraft(scX, scY, 1.5f, tumbleAngle);
        
        /* "Out of fuel" warning */
        float blink = sinf(g_time * 5) > 0 ? 1.0f : 0.3f;
        glColor4f(1.0f, 0.2f, 0.1f, blink);
        drawTextCentered(scX, scY + 40, "FUEL DEPLETED",
                        GLUT_BITMAP_HELVETICA_12);
        
        /* X markers on path */
        glColor4f(1.0f, 0.2f, 0.1f, 0.6f);
        glLineWidth(3.0f);
        float xX = g_winW * 0.65f;
        float xY = g_winH * 0.5f + 40;
        glBegin(GL_LINES);
        glVertex2f(xX - 15, xY - 15); glVertex2f(xX + 15, xY + 15);
        glVertex2f(xX + 15, xY - 15); glVertex2f(xX - 15, xY + 15);
        glEnd();
    }
    
    /* Info Box */
    if (sceneTime < 15.0f) {
        drawInfoBox("The Challenge: Direct Trajectory to Mars",
                    "A direct Hohmann transfer from Earth to Mars requires an enormous delta-v budget.",
                    "PSLV's payload capacity limits the fuel available for such a long burn.",
                    "Distance to Mars: ~225 million km. Travel time: ~9 months. Fuel: Insufficient.",
                    "Without enough propellant, the spacecraft cannot reach escape velocity for Mars.",
                    0.5f, 0.5f, 1.0f);
    } else {
        drawInfoBox("MISSION FAILURE: Fuel Depleted Before Reaching Mars",
                    "The direct approach fails - fuel runs out midway through the transfer orbit.",
                    "A conventional rocket would need 10x more fuel than PSLV could carry.",
                    "ISRO's engineers needed a revolutionary, cost-effective alternative approach.",
                    "Solution: Use Earth's own gravity to gradually boost the spacecraft's velocity!",
                    1.0f, 0.3f, 0.1f);
    }
}

/* Scene 8: 120-150s - Gravity assist (slingshot) */
static void drawScene_GravityAssist(float sceneTime) {
    glColor3f(0.01f, 0.01f, 0.04f);
    drawRect(0, 0, g_winW, g_winH);
    drawStarField(2.0f);
    
    /* Earth center-left */
    float earthCX = g_winW * 0.3f;
    float earthCY = g_winH * 0.45f;
    drawEarth(earthCX, earthCY, 80);
    glColor3f(1, 1, 1);
    drawTextCentered(earthCX, earthCY - 100, "Earth", GLUT_BITMAP_HELVETICA_12);
    
    /* Mars upper-right */
    float marsCX = g_winW * 0.8f;
    float marsCY = g_winH * 0.6f;
    drawMars(marsCX, marsCY, 60);
    glColor3f(1, 1, 1);
    drawTextCentered(marsCX, marsCY - 80, "Mars", GLUT_BITMAP_HELVETICA_12);
    
    /* Sun center background */
    drawSun(g_winW * 0.5f, g_winH * 0.2f, 25);
    
    /* Draw the gravity assist elliptical orbit path */
    /* Multiple orbit raising - showing the slingshot trajectory */
    
    /* Inner orbits around Earth (orbit raising) */
    glLineWidth(1.0f);
    for (int orbit = 0; orbit < 6; orbit++) {
        float orbitR = 120 + orbit * 25;
        float eccentricity = 0.3f + orbit * 0.08f;
        float alphaVal = 0.15f + orbit * 0.05f;
        
        float showT = orbit * 3.0f;
        if (sceneTime > showT) {
            float fadeIn = smoothstep(showT, showT + 2, sceneTime);
            glColor4f(0.3f, 0.6f, 1.0f, alphaVal * fadeIn);
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 80; i++) {
                float ang = i * 2.0f * M_PI / 80.0f;
                float px = earthCX + cosf(ang) * orbitR;
                float py = earthCY + sinf(ang) * orbitR * (1.0f - eccentricity);
                glVertex2f(px, py);
            }
            glEnd();
        }
    }
    
    /* Transfer orbit to Mars */
    if (sceneTime > 15.0f) {
        float transferFade = smoothstep(15, 18, sceneTime);
        glColor4f(1.0f, 0.8f, 0.2f, 0.4f * transferFade);
        glLineWidth(2.0f);
        
        /* Hohmann-like transfer ellipse */
        glBegin(GL_LINE_STRIP);
        float transferCX = (earthCX + marsCX) * 0.5f;
        float transferCY = (earthCY + marsCY) * 0.5f;
        float transferRX = (marsCX - earthCX) * 0.6f;
        float transferRY = (marsCY - earthCY) * 1.2f;
        for (int i = 0; i <= 50; i++) {
            float t = (float)i / 50.0f;
            float ang = M_PI + t * M_PI; /* Half ellipse */
            float px = transferCX + cosf(ang) * transferRX;
            float py = transferCY + sinf(ang) * transferRY;
            glVertex2f(px, py);
        }
        glEnd();
        
        /* Label */
        glColor4f(1.0f, 0.8f, 0.2f, transferFade * 0.8f);
        drawTextSmall(transferCX - 40, transferCY + transferRY + 20,
                     "Trans-Mars Injection");
    }
    
    /* Spacecraft moving along the path */
    float scAngle, scX, scY;
    if (sceneTime < 18.0f) {
        /* Orbiting Earth with increasing radius */
        int currentOrbit = (int)(sceneTime / 3.0f);
        float orbitFrac = (sceneTime - currentOrbit * 3.0f) / 3.0f;
        float orbitR = 120 + currentOrbit * 25;
        float eccentricity = 0.3f + currentOrbit * 0.08f;
        
        scAngle = orbitFrac * 2.0f * M_PI;
        scX = earthCX + cosf(scAngle) * orbitR;
        scY = earthCY + sinf(scAngle) * orbitR * (1.0f - eccentricity);
        
        float scDrawAngle = scAngle * 180.0f / M_PI + 90;
        drawSpacecraft(scX, scY, 1.2f, scDrawAngle);
        
        /* Velocity boost arrows at perigee */
        if (orbitFrac > 0.4f && orbitFrac < 0.6f) {
            glColor4f(0.3f, 1.0f, 0.5f, 0.6f);
            float arrX = scX + 20;
            float arrY = scY;
            glBegin(GL_TRIANGLES);
            glVertex2f(arrX, arrY + 5);
            glVertex2f(arrX + 15, arrY);
            glVertex2f(arrX, arrY - 5);
            glEnd();
            drawTextSmall(arrX + 18, arrY - 5, "BOOST");
        }
    }
    else {
        /* Transfer to Mars */
        float t = (sceneTime - 18.0f) / 12.0f;
        if (t > 1.0f) t = 1.0f;
        
        float transferCX = (earthCX + marsCX) * 0.5f;
        float transferCY = (earthCY + marsCY) * 0.5f;
        float transferRX = (marsCX - earthCX) * 0.6f;
        float transferRY = (marsCY - earthCY) * 1.2f;
        
        float transferAng = M_PI + t * M_PI;
        scX = transferCX + cosf(transferAng) * transferRX;
        scY = transferCY + sinf(transferAng) * transferRY;
        
        float scDrawAngle = transferAng * 180.0f / M_PI;
        drawSpacecraft(scX, scY, 1.2f, scDrawAngle);
    }
    
    /* Info Box */
    if (sceneTime < 10.0f) {
        drawInfoBox("ISRO's Solution: Gravity-Assisted Trajectory (Slingshot Effect)",
                    "Instead of a direct burn, MOM performs 6 orbit-raising maneuvers around Earth.",
                    "Each perigee pass fires thrusters briefly, using Earth's gravity to gain speed.",
                    "This technique saves ~60% fuel compared to a direct transfer orbit.",
                    "Orbit apogee increases with each burn: 28,825 km -> 40,186 km -> ... -> 192,874 km.",
                    0.3f, 1.0f, 0.5f);
    } else if (sceneTime < 20.0f) {
        drawInfoBox("Hohmann Transfer Orbit - Trans-Mars Injection (TMI)",
                    "After 6 orbit-raising burns, the final TMI burn on Dec 1, 2013 sends MOM to Mars.",
                    "The spacecraft exits Earth's sphere of influence at 11.14 km/s velocity.",
                    "The elliptical transfer orbit is carefully timed with Mars' orbital position.",
                    "Journey covers ~680 million km over 300 days through interplanetary space.",
                    1.0f, 0.8f, 0.2f);
    } else {
        drawInfoBox("Trans-Mars Injection Complete - Cruising to Mars",
                    "MOM has left Earth orbit and is now on a heliocentric transfer trajectory.",
                    "4 mid-course correction burns are planned during the 300-day cruise phase.",
                    "The spacecraft's 440N liquid apogee motor will be used for Mars orbit insertion.",
                    "Expected Mars arrival: September 24, 2014. All systems nominal.",
                    0.5f, 0.8f, 1.0f);
    }
}

/* Scene 9: 150-180s - Mars orbit attempt */
static void drawScene_MarsOrbitAttempt(float sceneTime) {
    glColor3f(0.01f, 0.01f, 0.04f);
    drawRect(0, 0, g_winW, g_winH);
    drawStarField(2.5f);
    
    /* Mars large */
    float marsCX = g_winW * 0.5f;
    float marsCY = g_winH * 0.4f;
    float marsR = 120;
    drawMars(marsCX, marsCY, marsR);
    
    if (sceneTime < 10.0f) {
        /* Approaching Mars */
        float approachT = sceneTime / 10.0f;
        float scX = lerp(g_winW + 50, marsCX + marsR + 150, approachT);
        float scY = lerp(g_winH * 0.8f, marsCY + 50, approachT);
        
        drawSpacecraft(scX, scY, 1.5f, -135 + approachT * 45);
        
        /* Approach trajectory */
        glColor4f(0.5f, 0.8f, 1.0f, 0.3f);
        glLineWidth(1.0f);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 30; i++) {
            float t = (float)i / 30.0f;
            float px = lerp(g_winW + 50, marsCX + marsR + 150, t);
            float py = lerp(g_winH * 0.8f, marsCY + 50, t);
            glVertex2f(px, py);
        }
        glEnd();
        
        /* (text moved to info box) */
    }
    else if (sceneTime < 20.0f) {
        /* Struggling to enter orbit - hyperbolic passes */
        float orbitT = sceneTime - 10.0f;
        float orbitAngle = orbitT * 0.5f;
        
        /* Draw attempted orbit (wobbly, unstable) */
        float orbitR = marsR + 100 + sinf(orbitT * 2) * 40;
        float scX = marsCX + cosf(orbitAngle) * orbitR;
        float scY = marsCY + sinf(orbitAngle) * orbitR * 0.7f;
        
        drawSpacecraft(scX, scY, 1.5f, orbitAngle * 180 / M_PI + 90);
        
        /* Unstable orbit path */
        glColor4f(1.0f, 0.3f, 0.2f, 0.4f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 60; i++) {
            float t = (float)i / 60.0f * orbitAngle;
            float r = marsR + 100 + sinf(t * 4) * 40;
            float px = marsCX + cosf(t) * r;
            float py = marsCY + sinf(t) * r * 0.7f;
            glVertex2f(px, py);
        }
        glEnd();
        
        /* Thruster firings */
        if ((int)(orbitT * 3) % 2 == 0) {
            glColor4f(1, 0.8f, 0.3f, 0.6f);
            drawCircle(scX - 15, scY, 5, 6);
        }
    }
    else {
        /* Successful orbit insertion */
        float stableT = sceneTime - 20.0f;
        float orbitAngle = stableT * 0.4f;
        float orbitR = marsR + 80;
        
        float scX = marsCX + cosf(orbitAngle) * orbitR;
        float scY = marsCY + sinf(orbitAngle) * orbitR * 0.6f;
        
        drawSpacecraft(scX, scY, 1.5f, orbitAngle * 180 / M_PI + 90);
        
        /* Stable orbit path */
        glColor4f(0.2f, 0.8f, 0.4f, 0.5f);
        glLineWidth(1.5f);
        drawOrbitPath(marsCX, marsCY, orbitR, orbitR * 0.6f,
                     0.2f, 0.8f, 0.4f, 0.4f);
        
        /* Success flash */
        if (stableT < 2.0f) {
            float flashA = (1.0f - stableT * 0.5f) * 0.3f;
            glColor4f(0.3f, 1.0f, 0.5f, flashA);
            drawCircle(marsCX, marsCY, marsR + 150, 30);
        }
        
        /* (text moved to info box) */
    }
    
    /* Info Box */
    if (sceneTime < 10.0f) {
        drawInfoBox("Approaching Mars - Preparing for Orbit Insertion",
                    "After 300 days and 680 million km, MOM approaches Mars' sphere of influence.",
                    "The 440N Liquid Apogee Motor (LAM) must fire for 24 minutes to slow down.",
                    "If the burn fails, MOM will fly past Mars forever - there is no second chance.",
                    "Communication delay: 12.5 minutes. The burn must execute autonomously.",
                    0.5f, 0.8f, 1.0f);
    } else if (sceneTime < 20.0f) {
        drawInfoBox("Mars Orbit Insertion - LAM Engine Burn in Progress",
                    "The LAM engine fires retrograde to reduce velocity and achieve Mars capture.",
                    "Orbit is initially unstable - trajectory corrections are being computed.",
                    "Engine burn duration: 24 min 22 sec. Delta-v: 1,098.7 m/s.",
                    "Mission control at ISTRAC, Bangalore monitors telemetry with 12.5 min delay.",
                    1.0f, 0.6f, 0.2f);
    } else {
        drawInfoBox("MARS ORBIT INSERTION SUCCESSFUL! - September 24, 2014",
                    "India becomes the FIRST nation to succeed in Mars orbit on its maiden attempt!",
                    "MOM enters a highly elliptical orbit: 421.7 km x 76,993.6 km, period 72.7 hrs.",
                    "India joins USA, Russia, and ESA as the 4th space agency to reach Mars.",
                    "Total mission cost: Rs 450 crore ($74M) - less than the budget of the movie 'Gravity'!",
                    0.2f, 1.0f, 0.4f);
    }
}

/* Scene 10: 180-210s - Close-up orbital visualization */
static void drawScene_OrbitalVisualization(float sceneTime) {
    glColor3f(0.01f, 0.01f, 0.04f);
    drawRect(0, 0, g_winW, g_winH);
    drawStarField(2.0f);
    
    /* Large Mars with slow rotation effect */
    float marsCX = g_winW * 0.45f;
    float marsCY = g_winH * 0.45f;
    float marsR = 180;
    
    /* Mars with rotation simulation */
    glColor3f(0.8f, 0.35f, 0.15f);
    drawCircle(marsCX, marsCY, marsR, 80);
    
    /* Rotating surface features */
    float rot = g_time * 0.1f;
    for (int i = 0; i < 12; i++) {
        float angle = i * 0.52f + rot;
        float fx = marsCX + cosf(angle) * marsR * 0.5f;
        float fy = marsCY + sinf(angle * 1.3f) * marsR * 0.4f;
        float fr = marsR * 0.1f + sinf(i * 2.1f) * marsR * 0.04f;
        float dist = sqrtf((fx - marsCX) * (fx - marsCX) + (fy - marsCY) * (fy - marsCY));
        if (dist + fr < marsR * 0.9f) {
            glColor3f(0.7f, 0.28f, 0.1f);
            drawCircle(fx, fy, fr, 8);
        }
    }
    
    /* Atmosphere glow */
    glColor4f(0.9f, 0.4f, 0.2f, 0.08f);
    drawCircle(marsCX, marsCY, marsR * 1.08f, 60);
    glColor4f(0.9f, 0.4f, 0.2f, 0.04f);
    drawCircle(marsCX, marsCY, marsR * 1.15f, 60);
    
    /* Mars outline */
    glColor3f(0.9f, 0.4f, 0.15f);
    glLineWidth(2.0f);
    drawCircleOutline(marsCX, marsCY, marsR, 80);
    
    /* Multiple orbital paths with different inclinations */
    float orbitAng = sceneTime * 0.3f;
    
    /* Primary orbit */
    float orbitRX = marsR + 60;
    float orbitRY = (marsR + 60) * 0.5f;
    drawOrbitPath(marsCX, marsCY, orbitRX, orbitRY,
                 0.2f, 0.8f, 0.4f, 0.6f);
    
    /* Secondary orbit visualization (different inclination) */
    glPushMatrix();
    glTranslatef(marsCX, marsCY, 0);
    glRotatef(30, 0, 0, 1);
    glTranslatef(-marsCX, -marsCY, 0);
    drawOrbitPath(marsCX, marsCY, orbitRX * 1.1f, orbitRY * 0.8f,
                 0.6f, 0.4f, 0.8f, 0.3f);
    glPopMatrix();
    
    /* Spacecraft on primary orbit */
    float scX = marsCX + cosf(orbitAng) * orbitRX;
    float scY = marsCY + sinf(orbitAng) * orbitRY;
    drawSpacecraft(scX, scY, 1.5f, orbitAng * 180 / M_PI + 90);
    
    /* Orbital data display */
    /* Periapsis/Apoapsis markers */
    glColor4f(0.3f, 1.0f, 0.5f, 0.6f);
    drawCircle(marsCX + orbitRX, marsCY, 4, 6);
    drawTextSmall(marsCX + orbitRX + 8, marsCY - 5, "Apoapsis");
    
    glColor4f(1.0f, 0.5f, 0.3f, 0.6f);
    drawCircle(marsCX - orbitRX, marsCY, 4, 6);
    drawTextSmall(marsCX - orbitRX - 60, marsCY - 5, "Periapsis");
    
    /* Orbital parameters panel */
    float panelX = g_winW - 280;
    float panelY = g_winH * 0.3f;
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    drawRect(panelX, panelY, 270, 180);
    glColor4f(0.2f, 0.6f, 0.8f, 0.5f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(panelX, panelY);
    glVertex2f(panelX + 270, panelY);
    glVertex2f(panelX + 270, panelY + 180);
    glVertex2f(panelX, panelY + 180);
    glEnd();
    
    glColor3f(0.3f, 0.9f, 0.5f);
    drawTextSmall(panelX + 10, panelY + 160, "ORBITAL PARAMETERS");
    glColor3f(0.8f, 0.9f, 1.0f);
    drawTextSmall(panelX + 10, panelY + 140, "Periapsis: 421.7 km");
    drawTextSmall(panelX + 10, panelY + 120, "Apoapsis: 76,993.6 km");
    drawTextSmall(panelX + 10, panelY + 100, "Inclination: 150.0 deg");
    drawTextSmall(panelX + 10, panelY + 80, "Period: 72.7 hours");
    
    float orbitalV = 3500 + sinf(orbitAng) * 1500;
    char vBuf[64];
    sprintf(vBuf, "Velocity: %.0f m/s", orbitalV);
    glColor3f(0.5f, 1.0f, 0.7f);
    drawTextSmall(panelX + 10, panelY + 55, vBuf);
    
    float altitude = 420 + (1 + sinf(orbitAng)) * 38000;
    char altBuf[64];
    sprintf(altBuf, "Altitude: %.0f km", altitude);
    drawTextSmall(panelX + 10, panelY + 35, altBuf);
    
    /* "Camera" perspective change simulation - grid lines */
    glColor4f(0.3f, 0.5f, 0.7f, 0.1f);
    glLineWidth(1.0f);
    float gridSpacing = 80;
    float gridShift = fmodf(sceneTime * 5, gridSpacing);
    for (float gx = -gridSpacing + gridShift; gx < g_winW + gridSpacing; gx += gridSpacing) {
        glBegin(GL_LINES);
        glVertex2f(gx, 0);
        glVertex2f(gx, g_winH);
        glEnd();
    }
    for (float gy = -gridSpacing + gridShift * 0.7f; gy < g_winH + gridSpacing; gy += gridSpacing) {
        glBegin(GL_LINES);
        glVertex2f(0, gy);
        glVertex2f(g_winW, gy);
        glEnd();
    }
    
    /* Info Box */
    {
        char line1Buf[128];
        float orbitalV = 3500 + sinf(orbitAng) * 1500;
        float altitude = 420 + (1 + sinf(orbitAng)) * 38000;
        sprintf(line1Buf, "Current Altitude: %.0f km | Velocity: %.0f m/s", altitude, orbitalV);
        drawInfoBox("Mars Orbital Observation - MOM Science Operations",
                    line1Buf,
                    "Orbit: 421.7 km (periapsis) x 76,993.6 km (apoapsis) | Inclination: 150 deg",
                    "Orbital period: 72.7 hours. Spacecraft completes one Mars orbit every 3 days.",
                    "All 5 science instruments active. Collecting atmospheric and surface data.",
                    0.3f, 0.8f, 1.0f);
    }
}

/* Scene 11: 210-240s - Data transmission */
static void drawScene_DataTransmission(float sceneTime) {
    glColor3f(0.01f, 0.01f, 0.04f);
    drawRect(0, 0, g_winW, g_winH);
    drawStarField(1.5f);
    
    /* Mars on left */
    float marsCX = 200;
    float marsCY = g_winH * 0.5f;
    float marsR = 80;
    drawMars(marsCX, marsCY, marsR);
    
    /* Spacecraft orbiting Mars */
    float orbitAng = sceneTime * 0.5f;
    float scX = marsCX + cosf(orbitAng) * (marsR + 40);
    float scY = marsCY + sinf(orbitAng) * (marsR + 40) * 0.6f;
    drawSpacecraft(scX, scY, 1.0f, orbitAng * 180 / M_PI + 90);
    
    /* Earth on right */
    float earthCX = g_winW - 200;
    float earthCY = g_winH * 0.5f - 30;
    float earthR = 70;
    drawEarth(earthCX, earthCY, earthR);
    
    /* Control center on Earth */
    float ccX = earthCX - 30;
    float ccY = earthCY - earthR - 50;
    glColor3f(0.3f, 0.3f, 0.35f);
    drawRect(ccX, ccY, 60, 25);
    glColor3f(0.2f, 0.5f, 0.8f);
    drawRect(ccX + 5, ccY + 5, 50, 15);
    /* Antenna dish on building */
    glColor3f(0.6f, 0.6f, 0.65f);
    glBegin(GL_TRIANGLES);
    glVertex2f(ccX + 30, ccY + 25);
    glVertex2f(ccX + 20, ccY + 45);
    glVertex2f(ccX + 40, ccY + 45);
    glEnd();
    /* Antenna stick */
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(ccX + 30, ccY + 45);
    glVertex2f(ccX + 30, ccY + 55);
    glEnd();
    
    glColor3f(1, 1, 1);
    drawTextSmall(ccX - 10, ccY - 10, "ISTRAC Ground Station");
    
    /* Signal waves between spacecraft and Earth */
    int numWaves = 5;
    for (int i = 0; i < numWaves; i++) {
        float waveProgress = fmodf(sceneTime * 0.3f + i * 0.2f, 1.0f);
        float amplitude = 15.0f * (1.0f - fabsf(waveProgress - 0.5f) * 2.0f);
        
        drawSignalWave(scX + 15, scY, ccX + 30, ccY + 55,
                      waveProgress, amplitude);
    }
    
    /* Reverse signal (commands from Earth) - dimmer */
    for (int i = 0; i < 3; i++) {
        float waveProgress = fmodf(sceneTime * 0.2f + i * 0.33f + 0.5f, 1.0f);
        float amplitude = 8.0f * (1.0f - fabsf(waveProgress - 0.5f) * 2.0f);
        
        glColor4f(0.2f, 1.0f, 0.4f, 0.3f * (1.0f - waveProgress));
        glLineWidth(1.0f);
        /* Simple wave from Earth to Mars */
        glBegin(GL_LINE_STRIP);
        int steps = 30;
        float dx = scX - ccX;
        float dy = scY - ccY;
        float len = sqrtf(dx*dx + dy*dy);
        for (int j = 0; j <= steps; j++) {
            float t = (float)j / steps;
            float px = ccX + 30 + dx * t;
            float py = ccY + 55 + dy * t;
            float nx = -dy / len;
            float ny = dx / len;
            float wave = sinf(t * 15.0f + waveProgress * 25.0f) * amplitude * sinf(t * M_PI);
            glVertex2f(px + nx * wave, py + ny * wave);
        }
        glEnd();
    }
    
    /* Data visualization panel */
    float panelX = g_winW * 0.35f;
    float panelY = 30;
    float panelW = g_winW * 0.3f;
    float panelH = 120;
    
    glColor4f(0, 0, 0, 0.7f);
    drawRect(panelX, panelY, panelW, panelH);
    glColor4f(0.2f, 0.6f, 0.8f, 0.5f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(panelX, panelY);
    glVertex2f(panelX + panelW, panelY);
    glVertex2f(panelX + panelW, panelY + panelH);
    glVertex2f(panelX, panelY + panelH);
    glEnd();
    
    glColor3f(0.3f, 0.9f, 0.5f);
    drawTextSmall(panelX + 10, panelY + panelH - 15, "TELEMETRY DATA");
    
    /* Simulated data stream */
    glColor3f(0.2f, 0.8f, 0.4f);
    float dataY = panelY + panelH - 35;
    for (int i = 0; i < 5; i++) {
        char dataBuf[64];
        float val = sinf(g_time * 2 + i * 1.5f) * 50 + 100;
        sprintf(dataBuf, "CH%d: %.1f  |  ", i + 1, val);
        drawTextSmall(panelX + 10 + (i % 3) * 110, dataY - (i / 3) * 18, dataBuf);
    }
    
    /* Signal strength indicator */
    glColor4f(1, 1, 1, 0.7f);
    drawTextSmall(panelX + 10, panelY + 15, "Signal: ");
    float sigStrength = 0.6f + 0.3f * sinf(g_time * 0.5f);
    int bars = (int)(sigStrength * 5);
    for (int i = 0; i < 5; i++) {
        if (i < bars)
            glColor3f(0.2f, 0.9f, 0.3f);
        else
            glColor3f(0.3f, 0.3f, 0.3f);
        drawRect(panelX + 60 + i * 12, panelY + 12, 8, 5 + i * 3);
    }
    
    /* Info Box */
    drawInfoBox("Deep Space Communication - Mars to Earth Data Link",
                "Signal travel time: 4 to 24 minutes depending on Mars-Earth distance.",
                "Data transmitted via S-band (2.2 GHz) and received at ISTRAC ground stations.",
                "Indian Deep Space Network (IDSN) at Byalalu, near Bangalore handles all comms.",
                "Telemetry, science data, and spacecraft health info relayed to mission control.",
                0.3f, 0.8f, 1.0f);
}

/* Scene 12: 240-270s - Scientific mission / surface scanning */
static void drawScene_SurfaceScanning(float sceneTime) {
    glColor3f(0.01f, 0.01f, 0.04f);
    drawRect(0, 0, g_winW, g_winH);
    drawStarField(1.5f);
    
    /* Large Mars surface view (top-down feel) */
    float marsCX = g_winW * 0.4f;
    float marsCY = g_winH * 0.4f;
    float marsR = 220;
    
    /* Mars base */
    glColor3f(0.75f, 0.32f, 0.12f);
    drawCircle(marsCX, marsCY, marsR, 80);
    
    /* Detailed surface features */
    /* Olympus Mons */
    glColor3f(0.85f, 0.4f, 0.18f);
    drawCircle(marsCX - 60, marsCY + 30, 40, 20);
    glColor3f(0.9f, 0.45f, 0.2f);
    drawCircle(marsCX - 60, marsCY + 30, 25, 15);
    
    /* Valles Marineris (canyon) */
    glColor3f(0.5f, 0.2f, 0.08f);
    glLineWidth(4.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < 20; i++) {
        float t = (float)i / 19.0f;
        float px = marsCX - 80 + t * 200;
        float py = marsCY - 20 + sinf(t * 3) * 15;
        float dist = sqrtf((px - marsCX) * (px - marsCX) + (py - marsCY) * (py - marsCY));
        if (dist < marsR * 0.85f)
            glVertex2f(px, py);
    }
    glEnd();
    
    /* Polar regions */
    glColor3f(0.9f, 0.88f, 0.85f);
    drawCircle(marsCX, marsCY + marsR * 0.85f, marsR * 0.12f, 10);
    drawCircle(marsCX, marsCY - marsR * 0.85f, marsR * 0.08f, 10);
    
    /* Mars outline */
    glColor4f(0.9f, 0.4f, 0.15f, 0.5f);
    glLineWidth(2.0f);
    drawCircleOutline(marsCX, marsCY, marsR, 80);
    
    /* Scanning grid overlay */
    float scanProgress = fmodf(sceneTime * 0.1f, 1.0f);
    
    /* Horizontal scan lines */
    glColor4f(0.2f, 0.8f, 1.0f, 0.15f);
    glLineWidth(1.0f);
    for (float sy = marsCY - marsR; sy < marsCY + marsR; sy += 20) {
        float halfW = sqrtf(marsR * marsR - (sy - marsCY) * (sy - marsCY));
        if (halfW > 0) {
            glBegin(GL_LINES);
            glVertex2f(marsCX - halfW, sy);
            glVertex2f(marsCX + halfW, sy);
            glEnd();
        }
    }
    
    /* Active scan line (bright, moving) */
    float activeScanY = marsCY - marsR + scanProgress * marsR * 2;
    float scanHalfW = 0;
    float dy = activeScanY - marsCY;
    if (fabsf(dy) < marsR) {
        scanHalfW = sqrtf(marsR * marsR - dy * dy);
    }
    if (scanHalfW > 0) {
        glColor4f(0.3f, 1.0f, 0.5f, 0.8f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glVertex2f(marsCX - scanHalfW, activeScanY);
        glVertex2f(marsCX + scanHalfW, activeScanY);
        glEnd();
        
        /* Scan glow */
        glColor4f(0.3f, 1.0f, 0.5f, 0.1f);
        drawRect(marsCX - scanHalfW, activeScanY - 10, scanHalfW * 2, 20);
    }
    
    /* Glowing markers for discovered features */
    typedef struct { float x, y; const char *label; float showTime; } Feature;
    Feature features[] = {
        { marsCX - 60, marsCY + 30, "Olympus Mons", 3.0f },
        { marsCX + 20, marsCY - 15, "Valles Marineris", 8.0f },
        { marsCX + 80, marsCY + 60, "Hellas Basin", 13.0f },
        { marsCX - 30, marsCY - 70, "Water Ice Traces", 18.0f },
        { marsCX + 50, marsCY + 100, "Methane Signal", 23.0f },
    };
    int nFeatures = 5;
    
    for (int i = 0; i < nFeatures; i++) {
        if (sceneTime > features[i].showTime) {
            float fadeIn = smoothstep(features[i].showTime, features[i].showTime + 2, sceneTime);
            float dist = sqrtf((features[i].x - marsCX) * (features[i].x - marsCX) + 
                              (features[i].y - marsCY) * (features[i].y - marsCY));
            if (dist < marsR * 0.85f) {
                /* Pulsing glow */
                float pulse = 0.5f + 0.5f * sinf(g_time * 3 + i * 2);
                glColor4f(0.3f, 1.0f, 0.5f, 0.3f * fadeIn * pulse);
                drawCircle(features[i].x, features[i].y, 15, 8);
                glColor4f(1.0f, 0.9f, 0.3f, 0.8f * fadeIn);
                drawCircleOutline(features[i].x, features[i].y, 12, 8);
                
                /* Label */
                glColor4f(1, 1, 1, fadeIn * 0.9f);
                drawTextSmall(features[i].x + 18, features[i].y - 5, features[i].label);
                
                /* Crosshair */
                glColor4f(0.3f, 1.0f, 0.5f, 0.5f * fadeIn);
                glLineWidth(1.0f);
                glBegin(GL_LINES);
                glVertex2f(features[i].x - 8, features[i].y);
                glVertex2f(features[i].x + 8, features[i].y);
                glVertex2f(features[i].x, features[i].y - 8);
                glVertex2f(features[i].x, features[i].y + 8);
                glEnd();
            }
        }
    }
    
    /* Spacecraft position */
    float orbitAng = sceneTime * 0.4f;
    float scX = marsCX + cosf(orbitAng) * (marsR + 40);
    float scY = marsCY + sinf(orbitAng) * (marsR + 40) * 0.5f;
    drawSpacecraft(scX, scY, 1.0f, orbitAng * 180 / M_PI + 90);
    
    /* Scan beam from spacecraft to surface */
    glColor4f(0.2f, 0.8f, 1.0f, 0.2f);
    glBegin(GL_TRIANGLES);
    glVertex2f(scX, scY);
    /* Beam spreads on surface */
    float beamTargetX = marsCX + cosf(orbitAng * 0.5f) * marsR * 0.3f;
    float beamTargetY = marsCY + sinf(orbitAng * 0.7f) * marsR * 0.3f;
    glVertex2f(beamTargetX - 30, beamTargetY);
    glVertex2f(beamTargetX + 30, beamTargetY);
    glEnd();
    
    /* Data panel */
    float panelX = g_winW - 300;
    float panelY = g_winH * 0.35f;
    glColor4f(0, 0, 0, 0.75f);
    drawRect(panelX, panelY, 290, 250);
    glColor4f(0.2f, 0.6f, 0.8f, 0.5f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(panelX, panelY);
    glVertex2f(panelX + 290, panelY);
    glVertex2f(panelX + 290, panelY + 250);
    glVertex2f(panelX, panelY + 250);
    glEnd();
    
    glColor3f(0.3f, 0.9f, 0.5f);
    drawTextSmall(panelX + 10, panelY + 230, "SCIENCE PAYLOAD DATA");
    
    glColor3f(0.8f, 0.9f, 1.0f);
    drawTextSmall(panelX + 10, panelY + 205, "Mars Colour Camera (MCC): Active");
    drawTextSmall(panelX + 10, panelY + 185, "Thermal IR Imaging: Active");
    drawTextSmall(panelX + 10, panelY + 165, "Lyman Alpha Photometer: Active");
    drawTextSmall(panelX + 10, panelY + 145, "Methane Sensor (MSM): Active");
    drawTextSmall(panelX + 10, panelY + 125, "MENCA: Active");
    
    /* Scanning stats */
    float coverage = sceneTime / 30.0f * 100;
    if (coverage > 100) coverage = 100;
    char covBuf[64];
    sprintf(covBuf, "Surface Coverage: %.1f%%", coverage);
    glColor3f(0.5f, 1.0f, 0.7f);
    drawTextSmall(panelX + 10, panelY + 95, covBuf);
    
    /* Coverage bar */
    glColor3f(0.2f, 0.2f, 0.2f);
    drawRect(panelX + 10, panelY + 75, 200, 10);
    glColor3f(0.2f, 0.8f, 0.4f);
    drawRect(panelX + 10, panelY + 75, 200 * coverage / 100.0f, 10);
    
    /* Images captured */
    int imgCount = (int)(sceneTime * 3);
    char imgBuf[64];
    sprintf(imgBuf, "Images Captured: %d", imgCount);
    drawTextSmall(panelX + 10, panelY + 55, imgBuf);
    
    /* Data volume */
    float dataVol = sceneTime * 0.5f;
    char dataBuf[64];
    sprintf(dataBuf, "Data Volume: %.1f GB", dataVol);
    drawTextSmall(panelX + 10, panelY + 35, dataBuf);
    
    /* Info Box */
    {
        char covLine[128];
        float coverage = sceneTime / 30.0f * 100;
        if (coverage > 100) coverage = 100;
        int imgCount = (int)(sceneTime * 3);
        sprintf(covLine, "Surface Coverage: %.1f%% | Images Captured: %d | Data Volume: %.1f GB", coverage, imgCount, sceneTime * 0.5f);
        drawInfoBox("Scientific Mission - Mars Surface & Atmosphere Analysis",
                    covLine,
                    "5 Instruments: Mars Colour Camera, Thermal IR, Lyman Alpha, Methane Sensor, MENCA.",
                    "Key discoveries: Detailed Mars surface images, atmospheric composition data.",
                    "MOM detected deuterium (heavy hydrogen) in Mars upper atmosphere - a first!",
                    0.3f, 0.85f, 0.5f);
    }
}

/* Scene 13: 270-300s - Mission success */
static void drawScene_MissionSuccess(float sceneTime) {
    /* Calm deep space */
    glColor3f(0.01f, 0.015f, 0.05f);
    drawRect(0, 0, g_winW, g_winH);
    
    /* Beautiful starfield */
    for (int i = 0; i < MAX_STARS; i++) {
        float bright = starBright[i] * (0.6f + 0.4f * sinf(g_time * 1.5f + i * 1.3f));
        float sz = 1.0f + bright * 2.5f;
        
        /* Colorful stars */
        float hue = fmodf(i * 0.37f, 3.0f);
        if (hue < 1.0f) {
            glColor3f(bright, bright * 0.8f, bright * 0.6f);
        } else if (hue < 2.0f) {
            glColor3f(bright * 0.7f, bright * 0.8f, bright);
        } else {
            glColor3f(bright, bright * 0.9f, bright * 0.7f);
        }
        drawRect(starX[i] - sz * 0.5f, starY[i] - sz * 0.5f, sz, sz);
    }
    
    /* Sun glow in background */
    drawSun(100, g_winH - 100, 20);
    
    /* Small Earth in distance */
    drawEarth(200, g_winH - 180, 25);
    glColor4f(1, 1, 1, 0.5f);
    drawTextSmall(185, g_winH - 215, "Earth");
    
    /* Mars prominent */
    float marsCX = g_winW * 0.5f + 50;
    float marsCY = g_winH * 0.4f;
    float marsR = 140;
    drawMars(marsCX, marsCY, marsR);
    
    /* Orbital path */
    float orbitRX = marsR + 70;
    float orbitRY = (marsR + 70) * 0.55f;
    drawOrbitPath(marsCX, marsCY, orbitRX, orbitRY,
                 0.2f, 0.7f, 0.4f, 0.5f);
    
    /* Spacecraft orbiting calmly */
    float orbitAng = sceneTime * 0.3f;
    float scX = marsCX + cosf(orbitAng) * orbitRX;
    float scY = marsCY + sinf(orbitAng) * orbitRY;
    drawSpacecraft(scX, scY, 1.5f, orbitAng * 180 / M_PI + 90);
    
    /* Success message */
    if (sceneTime > 5.0f) {
        float fadeIn = smoothstep(5, 8, sceneTime);
        
        /* Glowing background for text */
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f * fadeIn);
        drawRect(g_winW * 0.15f, g_winH * 0.6f, g_winW * 0.7f, 100);
        
        /* Main success text */
        glColor4f(1.0f, 0.85f, 0.2f, fadeIn);
        drawTextCentered(g_winW * 0.5f, g_winH * 0.7f + 10,
                        "MISSION MANGALYAAN - SUCCESSFUL",
                        GLUT_BITMAP_HELVETICA_18);
        
        /* Subtitle */
        glColor4f(0.8f, 0.9f, 1.0f, fadeIn * 0.9f);
        drawTextCentered(g_winW * 0.5f, g_winH * 0.7f - 15,
                        "India's First Interplanetary Mission",
                        GLUT_BITMAP_HELVETICA_12);
        
        /* Stats */
        if (sceneTime > 10.0f) {
            float statsFade = smoothstep(10, 13, sceneTime);
            glColor4f(0.7f, 0.9f, 1.0f, statsFade * 0.8f);
            drawTextCentered(g_winW * 0.5f, g_winH * 0.7f - 40,
                            "Total Cost: $74 Million | Journey: 300 Days | Distance: 680 Million km",
                            GLUT_BITMAP_HELVETICA_12);
        }
    }
    
    if (sceneTime > 15.0f) {
        float fadeIn2 = smoothstep(15, 18, sceneTime);
        
        glColor4f(0.0f, 0.0f, 0.0f, 0.4f * fadeIn2);
        drawRect(g_winW * 0.2f, g_winH * 0.15f, g_winW * 0.6f, 80);
        
        /* Achievement text */
        glColor4f(0.3f, 1.0f, 0.5f, fadeIn2);
        drawTextCentered(g_winW * 0.5f, g_winH * 0.21f + 15,
                        "First nation to reach Mars orbit on first attempt",
                        GLUT_BITMAP_HELVETICA_12);
        glColor4f(0.3f, 0.8f, 1.0f, fadeIn2);
        drawTextCentered(g_winW * 0.5f, g_winH * 0.21f - 5,
                        "Most cost-effective Mars mission in history",
                        GLUT_BITMAP_HELVETICA_12);
    }
    
    /* Final "Mission Successful" with golden glow */
    if (sceneTime > 22.0f) {
        float finalFade = smoothstep(22, 25, sceneTime);
        float pulse = 0.8f + 0.2f * sinf(g_time * 2);
        
        /* Big golden text */
        glColor4f(1.0f, 0.85f, 0.3f, finalFade * pulse);
        glLineWidth(2.5f);
        float textScale = 0.35f;
        float textW = 920 * textScale; /* approx width */
        drawStrokeText(g_winW * 0.5f - textW * 0.5f, g_winH * 0.5f - 10,
                      textScale, "MISSION SUCCESSFUL");
        
        /* Glow behind text */
        glColor4f(1.0f, 0.8f, 0.2f, 0.05f * finalFade * pulse);
        drawCircle(g_winW * 0.5f, g_winH * 0.5f, 200, 30);
    }
    
    /* Indian flag tribute at the end */
    if (sceneTime > 25.0f) {
        float flagFade = smoothstep(25, 27, sceneTime);
        float flagX = g_winW * 0.5f - 30;
        float flagY = g_winH * 0.08f;
        float flagW = 60;
        float flagH = 12;
        
        /* Saffron */
        glColor4f(1.0f, 0.6f, 0.2f, flagFade * 0.8f);
        drawRect(flagX, flagY + flagH * 2, flagW, flagH);
        /* White */
        glColor4f(1.0f, 1.0f, 1.0f, flagFade * 0.8f);
        drawRect(flagX, flagY + flagH, flagW, flagH);
        /* Green */
        glColor4f(0.1f, 0.6f, 0.2f, flagFade * 0.8f);
        drawRect(flagX, flagY, flagW, flagH);
        /* Ashoka Chakra */
        glColor4f(0.0f, 0.0f, 0.5f, flagFade * 0.8f);
        drawCircleOutline(flagX + flagW * 0.5f, flagY + flagH * 1.5f, 4, 12);
        
        /* JAI HIND */
        glColor4f(1, 0.9f, 0.7f, flagFade);
        drawTextCentered(g_winW * 0.5f, flagY - 15, "Jai Hind!",
                        GLUT_BITMAP_HELVETICA_12);
    }
    
    /* Info Box */
    drawInfoBox("MISSION MANGALYAAN - A Giant Leap for India",
                "Mars Orbiter Mission operated for 8 years (designed for 6 months!) until 2022.",
                "Total cost: $74 million - cheapest interplanetary mission ever. Less than a Hollywood film.",
                "First Asian nation to reach Mars. First nation to succeed on maiden attempt.",
                "MOM proved India's capability for deep space exploration. Jai Hind!",
                1.0f, 0.85f, 0.2f);
}

/* ============================================================
 * MAIN DISPLAY FUNCTION
 * ============================================================ */

static void display(void) {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    
    /* Enable blending for transparency */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    /* Determine current scene based on time */
    float t = g_time;
    
    if (t < 10.0f) {
        drawScene_PadIdle(t);
    }
    else if (t < 20.0f) {
        drawScene_Ignition(t - 10.0f);
    }
    else if (t < 30.0f) {
        drawScene_Countdown(t - 20.0f);
    }
    else if (t < 40.0f) {
        drawScene_Launch(t - 30.0f);
    }
    else if (t < 60.0f) {
        drawScene_Atmosphere(t - 40.0f);
    }
    else if (t < 90.0f) {
        drawScene_StageSeparation(t - 60.0f);
    }
    else if (t < 120.0f) {
        drawScene_RocketFailure(t - 90.0f);
    }
    else if (t < 150.0f) {
        drawScene_GravityAssist(t - 120.0f);
    }
    else if (t < 180.0f) {
        drawScene_MarsOrbitAttempt(t - 150.0f);
    }
    else if (t < 210.0f) {
        drawScene_OrbitalVisualization(t - 180.0f);
    }
    else if (t < 240.0f) {
        drawScene_DataTransmission(t - 210.0f);
    }
    else if (t < 270.0f) {
        drawScene_SurfaceScanning(t - 240.0f);
    }
    else if (t <= 305.0f) {  /* Extra 5s for fade */
        drawScene_MissionSuccess(t - 270.0f);
    }
    
    /* Scene transition overlay */
    float transitionTimes[] = { 10, 20, 30, 40, 60, 90, 120, 150, 180, 210, 240, 270 };
    int nTransitions = 12;
    float transDuration = 1.0f;
    
    for (int i = 0; i < nTransitions; i++) {
        float diff = t - transitionTimes[i];
        if (diff > -0.5f && diff < transDuration) {
            float fadeAlpha;
            if (diff < 0) {
                fadeAlpha = (-diff) / 0.5f;
            } else {
                fadeAlpha = 1.0f - diff / transDuration;
            }
            fadeAlpha *= 0.5f; /* Don't fully black out */
            glColor4f(0, 0, 0, fadeAlpha);
            drawRect(0, 0, g_winW, g_winH);
            break;
        }
    }
    
    /* Draw timer (always on top) */
    drawTimer();
    
    /* Final fade to black at 298s */
    if (t > 298.0f) {
        float finalFade = smoothstep(298, 300, t);
        glColor4f(0, 0, 0, finalFade);
        drawRect(0, 0, g_winW, g_winH);
    }
    
    glutSwapBuffers();
}

/* ============================================================
 * TIMER & UPDATE
 * ============================================================ */

static void timer(int value) {
    g_time += g_dt;
    
    /* Stop at 300 seconds (5 minutes) */
    if (g_time > 302.0f) g_time = 302.0f;
    
    /* Update particles */
    updateParticles(g_dt);
    
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); /* ~60 FPS */
}

/* ============================================================
 * WINDOW RESHAPE
 * ============================================================ */

static void reshape(int w, int h) {
    g_winW = w;
    g_winH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    /* Reinit stars for new size */
    initStars();
}

/* ============================================================
 * MAIN
 * ============================================================ */

int main(int argc, char **argv) {
    srand((unsigned)time(NULL));
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(g_winW, g_winH);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("ISRO Mars Orbiter Mission - Mangalyaan Animation");
    
    /* OpenGL setup */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    /* Initialize */
    initStars();
    initParticles();
    initSignals();
    
    /* Set callbacks */
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(16, timer, 0);
    
    printf("=== ISRO Mars Orbiter Mission - 5 Minute Animation ===\n");
    printf("Scene Timeline:\n");
    printf("  0:00 - 0:10  Rocket on Launch Pad\n");
    printf("  0:10 - 0:20  Engine Ignition\n");
    printf("  0:20 - 0:30  Countdown (10 to 1)\n");
    printf("  0:30 - 0:40  Liftoff!\n");
    printf("  0:40 - 1:00  Atmospheric Ascent\n");
    printf("  1:00 - 1:30  Stage Separation\n");
    printf("  1:30 - 2:00  Direct Approach Failure\n");
    printf("  2:00 - 2:30  Gravity Assist Solution\n");
    printf("  2:30 - 3:00  Mars Orbit Insertion\n");
    printf("  3:00 - 3:30  Orbital Visualization\n");
    printf("  3:30 - 4:00  Data Transmission\n");
    printf("  4:00 - 4:30  Surface Scanning\n");
    printf("  4:30 - 5:00  Mission Success!\n");
    printf("=====================================================\n");
    
    glutMainLoop();
    return 0;
}
