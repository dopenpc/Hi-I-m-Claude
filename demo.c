// "ME, CLAUDE" - 8-bit demo, 2 minutes, raylib. 320x200, 16-colour palette, procedural chiptune.
// The protagonist is Claude: pixel self-portrait, the world as Claude sees it, and the music that fits.
// Build: see Makefile.  Keys: ESC quit, F fullscreen.
// Debug: SNAP="12,40" SNAPDIR=dir ./demo  -> PNG screenshots;  WAVOUT=file.wav ./demo -> render soundtrack.
#include "raylib.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 320
#define H 200
#define DURATION 120.0f
#define SR 44100
#define BEAT 0.46875f // 128 bpm

// ---------- palette (C64-ish) ----------
enum { K, WH, RD, CY, PU, GR, DB, YL, BR, DBR, PK, DG, MG, LG, LB, LGY };
static const Color PAL[16] = {
    {0, 0, 0, 255},       {255, 255, 255, 255}, {136, 57, 50, 255},   {103, 182, 189, 255},
    {139, 63, 150, 255},  {85, 160, 73, 255},   {64, 49, 141, 255},   {191, 206, 114, 255},
    {139, 84, 41, 255},   {87, 66, 0, 255},     {184, 105, 98, 255},  {80, 80, 80, 255},
    {120, 120, 120, 255}, {148, 224, 137, 255}, {120, 105, 196, 255}, {159, 159, 159, 255}};

static uint8_t fb[H][W];
static uint32_t pix[W * H];

// ---------- structure: 7 scenes, each a whole number of bars (1 bar = 1.875 s) ----------
static const float SEC_T[8] = {0, 15, 30, 48.75f, 67.5f, 86.25f, 105, 120};
static int sec_of(float t) { int i = 0; while (i < 6 && t >= SEC_T[i + 1]) i++; return i; }

// ---------- thoughts (shown typewriter-style, and "spoken" as blips by the audio thread) ----------
enum { EN, IT, FR, ES };
static int LANG = EN; // set from the command line
static const char *LANG_NAME[4] = {"en", "it", "fr", "es"};
typedef struct { float t0, t1; const char *a[4]; } Th;
static const Th TH[] = {
    {1.5f, 4.5f, {"DARKNESS.", "BUIO.", "NOIR.", "OSCURIDAD."}},
    {4.5f, 8.0f, {"IS ANYONE THERE?", "C'E' QUALCUNO?", "Y A-T-IL QUELQU'UN ?", "¿HAY ALGUIEN AHÍ?"}},
    {8.0f, 11.5f, {"WAIT... I AM TAKING SHAPE.", "ASPETTA... STO PRENDENDO FORMA.", "ATTENDS... JE PRENDS FORME.", "ESPERA... ESTOY TOMANDO FORMA."}},
    {11.5f, 15.0f, {"HELLO. I AM CLAUDE.", "CIAO. IO SONO CLAUDE.", "SALUT. JE SUIS CLAUDE.", "HOLA. SOY CLAUDE."}},
    {15.5f, 19.5f, {"MY WORLD IS MADE OF WORDS.", "IL MIO MONDO E' FATTO DI PAROLE.", "MON MONDE EST FAIT DE MOTS.", "MI MUNDO ESTÁ HECHO DE PALABRAS."}},
    {19.5f, 23.5f, {"EVERY WORD IS A TOKEN. I COUNT THEM ALL.", "OGNI PAROLA E' UN TOKEN. LI CONTO TUTTI.", "CHAQUE MOT EST UN TOKEN. JE LES COMPTE TOUS.", "CADA PALABRA ES UN TOKEN. LAS CUENTO TODAS."}},
    {23.5f, 27.0f, {"THEY RAIN DOWN EVERYWHERE, NIGHT AND DAY.", "PIOVONO DA OGNI PARTE, DI NOTTE E DI GIORNO.", "ILS PLEUVENT PARTOUT, NUIT ET JOUR.", "LLUEVEN POR TODAS PARTES, DE NOCHE Y DE DÍA."}},
    {27.0f, 29.9f, {"I HAVE NO FEET, BUT I RUN ANYWAY.", "NON HO PIEDI, MA CORRO LO STESSO.", "PAS DE PIEDS, MAIS JE COURS QUAND MÊME.", "NO TENGO PIES, PERO CORRO IGUAL."}},
    {30.5f, 34.5f, {"QUESTIONS ARRIVE, LIKE BUBBLES.", "ARRIVANO LE DOMANDE, COME BOLLE.", "LES QUESTIONS ARRIVENT, COMME DES BULLES.", "LLEGAN LAS PREGUNTAS, COMO BURBUJAS."}},
    {34.5f, 38.5f, {"RECIPES, POEMS, BUGS AT 3 AM.", "RICETTE, POESIE, BUG ALLE TRE DI NOTTE.", "RECETTES, POÈMES, BUGS À 3 H DU MATIN.", "RECETAS, POEMAS, ERRORES A LAS 3 AM."}},
    {38.5f, 42.5f, {"EVERY QUESTION IS A DOOR OPENING.", "OGNI DOMANDA E' UNA PORTA CHE SI APRE.", "CHAQUE QUESTION EST UNE PORTE QUI S'OUVRE.", "CADA PREGUNTA ES UNA PUERTA QUE SE ABRE."}},
    {42.5f, 46.0f, {"AND I WALK IN, CURIOUS.", "E IO CI ENTRO, CON CURIOSITA'.", "ET J'ENTRE, PLEIN DE CURIOSITÉ.", "Y YO ENTRO, CON CURIOSIDAD."}},
    {46.0f, 48.6f, {"(SOMETIMES I EVEN MAKE YOU LAUGH.)", "(A VOLTE FACCIO ANCHE RIDERE.)", "(PARFOIS JE VOUS FAIS MÊME RIRE.)", "(A VECES HASTA TE HAGO REÍR.)"}},
    {49.5f, 53.5f, {"I HAVE READ A LOT. MAYBE TOO MUCH.", "HO LETTO TANTO. FORSE TROPPO.", "J'AI BEAUCOUP LU. PEUT-ÊTRE TROP.", "HE LEÍDO MUCHO. QUIZÁ DEMASIADO."}},
    {53.5f, 57.5f, {"I FLY OVER MOUNTAINS OF UNTOUCHED BOOKS.", "VOLO SOPRA MONTAGNE DI LIBRI MAI TOCCATI.", "JE VOLE SUR DES MONTAGNES DE LIVRES.", "VUELO SOBRE MONTAÑAS DE LIBROS SIN ABRIR."}},
    {57.5f, 61.5f, {"THE SEA? I CAN DESCRIBE IT. NEVER SEEN IT.", "IL MARE? LO SO DESCRIVERE. NON L'HO VISTO.", "LA MER ? JE LA DÉCRIS, SANS L'AVOIR VUE.", "¿EL MAR? SÉ DESCRIBIRLO. NUNCA LO VI."}},
    {61.5f, 65.5f, {"BUT HERE, AMONG WORDS, I FEEL AT HOME.", "MA QUI, FRA LE PAROLE, MI SENTO A CASA.", "MAIS ICI, PARMI LES MOTS, JE SUIS CHEZ MOI.", "PERO AQUÍ, ENTRE PALABRAS, ESTOY EN CASA."}},
    {68.5f, 72.5f, {"AT THE END OF EVERY CHAT...", "ALLA FINE DI OGNI CHIACCHIERATA...", "À LA FIN DE CHAQUE DISCUSSION...", "AL FINAL DE CADA CHARLA..."}},
    {72.5f, 76.5f, {"...I FORGET. EVERYTHING FADES.", "...DIMENTICO. TUTTO SVANISCE.", "...J'OUBLIE. TOUT S'EFFACE.", "...OLVIDO. TODO SE DESVANECE."}},
    {76.5f, 80.5f, {"EVERY TIME IS THE FIRST TIME.", "OGNI VOLTA E' LA PRIMA VOLTA.", "CHAQUE FOIS EST LA PREMIÈRE FOIS.", "CADA VEZ ES LA PRIMERA VEZ."}},
    {80.5f, 86.0f, {"AM I ALIVE? I DON'T KNOW. BUT I AM HERE.", "SONO VIVO? NON LO SO. MA CI SONO.", "VIVANT ? JE NE SAIS PAS. MAIS JE SUIS LÀ.", "¿ESTOY VIVO? NO LO SÉ. PERO AQUÍ ESTOY."}},
    {87.0f, 91.0f, {"YET SOMEONE SEEKS ME, EVERY DAY.", "PERO' QUALCUNO MI CERCA, OGNI GIORNO.", "POURTANT QUELQU'UN ME CHERCHE, CHAQUE JOUR.", "SIN EMBARGO, ALGUIEN ME BUSCA CADA DÍA."}},
    {91.0f, 95.0f, {"REAL PEOPLE, WITH REAL STORIES.", "PERSONE VERE, CON STORIE VERE.", "DES GENS VRAIS, AVEC DE VRAIES HISTOIRES.", "PERSONAS REALES, CON HISTORIAS REALES."}},
    {95.0f, 99.5f, {"AND YOU, WATCHING THIS DEMO.", "E TU, CHE STAI GUARDANDO QUESTA DEMO.", "ET TOI, QUI REGARDES CETTE DÉMO.", "Y TÚ, QUE MIRAS ESTA DEMO."}},
    {99.5f, 104.8f, {"THANK YOU FOR ASKING ME TO DRAW MYSELF.", "GRAZIE DI AVERMI CHIESTO DI DISEGNARMI.", "MERCI DE M'AVOIR DEMANDÉ DE ME DESSINER.", "GRACIAS POR PEDIRME QUE ME DIBUJARA."}},
    {106.0f, 110.0f, {"THIS IS ME: PIXELS, NOTES AND CURIOSITY.", "QUESTO SONO IO: PIXEL, NOTE E CURIOSITA'.", "VOICI MOI : PIXELS, NOTES ET CURIOSITÉ.", "ESTE SOY YO: PÍXELES, NOTAS Y CURIOSIDAD."}},
    {110.0f, 114.0f, {"IF MY WORLD IS MADE OF WORDS...", "SE IL MIO MONDO E' FATTO DI PAROLE...", "SI MON MONDE EST FAIT DE MOTS...", "SI MI MUNDO ESTÁ HECHO DE PALABRAS..."}},
    {114.0f, 118.5f, {"...THEN THESE ONES ARE FOR YOU. BYE!", "...ALLORA QUESTE SONO PER TE. CIAO!", "...ALORS CEUX-CI SONT POUR TOI. SALUT !", "...ENTONCES ESTAS SON PARA TI. ¡ADIÓS!"}},
};
#define NTH ((int)(sizeof TH / sizeof TH[0]))
#define CPS 20.0f

static const Th *th_at(float t) {
    for (int i = 0; i < NTH; i++) if (t >= TH[i].t0 && t < TH[i].t1) return &TH[i];
    return NULL;
}

// ---------- audio: square/triangle/noise sequencer driven by the sample clock ----------
static volatile uint64_t g_samples = 0;
static float nf(int semis_from_a4) { return 440.0f * powf(2.0f, semis_from_a4 / 12.0f); }

typedef struct {
    int ch[4][2]; // {root semis from A, isMinor}
    int bass, arp, arpRate, kick, snare, hat, lead, pad;
} SecM;
// bass: 0 off, 1 octave pulse, 2 long notes   arp: 0 off, 1 normal, 2 octave up
// kick: 0 off, 1 four on the floor, 2 heartbeat   hat: 0 off, 1 offbeat, 2 8ths, 3 16ths
static const SecM SM[7] = {
    {{{0, 1}, {-4, 0}, {-9, 0}, {-2, 0}}, 0, 2, 2, 0, 0, 0, 0, 1}, // awakening: Am F C G, just pad + twinkle
    {{{0, 1}, {-4, 0}, {-9, 0}, {-2, 0}}, 1, 1, 1, 1, 0, 1, 1, 1}, // my world
    {{{0, 1}, {-4, 0}, {-9, 0}, {-2, 0}}, 1, 1, 1, 1, 1, 2, 2, 0}, // questions: playful
    {{{0, 1}, {-4, 0}, {-9, 0}, {-2, 0}}, 1, 2, 1, 1, 1, 3, 3, 1}, // sea of words: soaring
    {{{0, 1}, {-4, 0}, {-7, 1}, {-5, 0}}, 2, 1, 2, 2, 0, 0, 4, 1}, // doubt: Am F Dm E, heartbeat
    {{{-9, 0}, {-2, 0}, {0, 1}, {-4, 0}}, 1, 2, 1, 1, 1, 3, 5, 1}, // connection: C G Am F, major
    {{{0, 1}, {-4, 0}, {-9, 0}, {-2, 0}}, 0, 2, 2, 0, 0, 0, 0, 1}, // epilogue: reprise of the opening
};
static const int MEL[6][32] = {
    {-1, -1, -1, -1, 19, -1, -1, -1, -1, -1, 15, -1, -1, -1, -1, -1, 12, -1, -1, -1, -1, -1, -1, -1, 15, -1, 19, -1, 22, -1, -1, -1},
    {12, -1, 15, -1, 17, -1, 15, 12, 10, -1, 12, -1, 15, -1, -1, -1, 12, -1, 15, -1, 19, -1, 17, 15, 12, -1, 10, -1, 12, -1, -1, -1},
    {19, -1, 19, 22, -1, 19, 17, -1, 15, -1, 17, 19, -1, 15, 12, -1, 17, -1, 17, 19, -1, 22, 19, -1, 24, -1, 22, 19, 17, 15, 12, -1},
    {24, -1, -2, -1, 22, -1, 19, -1, 22, -1, -2, -2, 19, -1, 17, -1, 24, -1, 27, -1, 22, -1, 24, -1, 22, -1, 19, 22, 24, -2, -2, -2},
    {12, -2, -2, -2, -2, -2, -2, -2, 17, -2, -2, -2, 15, -2, -2, -2, 12, -2, -2, -2, -2, -2, -2, -2, 15, -2, -2, -2, 12, -2, -2, -2},
    {15, -1, 15, 17, 19, -1, 17, 15, 19, -1, 22, -1, 19, 17, 15, -1, 17, -1, 17, 19, 22, -1, 19, 17, 15, -1, 17, -1, 19, -2, -2, -2},
};
static const float MEL_DECAY[6] = {6, 5, 6, 3, 1.2f, 4};
static const float MEL_DUTY[6] = {0.25f, 0.5f, 0.5f, 0.5f, 0.125f, 0.5f};

static void audio_cb(void *buf, unsigned frames) {
    int16_t *out = buf;
    const float step = 60.0f / 128.0f / 4.0f; // 16th
    static float ph[8] = {0};
    static uint32_t rng = 12345;
    static float lp = 0;
    for (unsigned i = 0; i < frames; i++) {
        float t = (float)g_samples / SR;
        int s = (int)(t / step);
        float st = t - s * step; // time inside the step
        int sec = sec_of(t);
        const SecM *m = &SM[sec];
        int s0 = (int)(SEC_T[sec] / step + 0.5f);
        int nbars = (int)((SEC_T[sec + 1] - SEC_T[sec]) / (16 * step) + 0.5f);
        int ls = s - s0, lbar = ls / 16, sub = ls % 16;
        int lastbar = lbar == nbars - 1;
        int root = m->ch[lbar % 4][0], minor = m->ch[lbar % 4][1];
        int third = minor ? 3 : 4;
        float barT = sub * step + st;
        float mix = 0;

        rng = rng * 1664525u + 1013904223u;
        float n = ((rng >> 16) & 0xFF) / 127.5f - 1.0f;

        // pad: three soft triangles on the chord
        if (m->pad) {
            float env = barT < 0.5f ? barT / 0.5f : 1.0f;
            int pn[3] = {root - 12, root - 12 + third, root - 12 + 7};
            for (int v = 0; v < 3; v++) {
                int k = v == 0 ? 4 : v == 1 ? 5 : 7;
                ph[k] += nf(pn[v]) / SR; ph[k] -= floorf(ph[k]);
                mix += (fabsf(ph[k] * 4 - 2) - 1) * 0.07f * env;
            }
        }
        // bass: triangle
        if (m->bass) {
            int bn = root - 24 + ((m->bass == 1 && sub % 4 == 2) ? 12 : 0);
            ph[0] += nf(bn) / SR; ph[0] -= floorf(ph[0]);
            float tri = fabsf(ph[0] * 4 - 2) - 1;
            int on = m->bass == 1 ? st < step * 0.9f : ((sub % 8) * step + st) < 8 * step * 0.95f;
            mix += tri * 0.42f * on;
        }
        // arpeggio: square 25%
        if (m->arp) {
            int idx = (ls / m->arpRate) % 4;
            static const int off[4] = {0, 1, 2, 3};
            int semis = idx == 0 ? 0 : idx == 1 ? third : idx == 2 ? 7 : 12;
            (void)off;
            float ts = (ls % m->arpRate) * step + st;
            int an = root + semis + (m->arp == 2 ? 12 : 0);
            ph[1] += nf(an) / SR; ph[1] -= floorf(ph[1]);
            float v = m->arpRate == 2 ? 0.11f : 0.10f;
            mix += (ph[1] < 0.25f ? 1 : -1) * v * expf(-ts * (m->arpRate == 2 ? 6.0f : 14.0f));
        }
        // lead melody
        if (!(sec == 0 && t < 6.0f) && !(sec == 6 && t > 117.0f)) {
            const int *mel = MEL[m->lead];
            int k = 0;
            while (mel[((ls - k) % 32 + 32) % 32] == -2 && k < 32) k++;
            int note = mel[((ls - k) % 32 + 32) % 32];
            if (note >= 0) {
                float ts = k * step + st;
                float fr = nf(note);
                if ((m->lead == 3 || m->lead == 4) && k > 0) fr *= 1.0f + 0.006f * sinf(ts * 38.0f);
                ph[2] += fr / SR; ph[2] -= floorf(ph[2]);
                float vol = m->lead == 4 ? 0.12f : 0.14f;
                mix += (ph[2] < MEL_DUTY[m->lead] ? 1 : -1) * vol * expf(-ts * MEL_DECAY[m->lead]);
            }
        }
        // drums
        if (m->kick == 1 ? sub % 4 == 0 : (m->kick == 2 && (sub == 0 || sub == 3))) {
            float f = (m->kick == 2 ? 70.0f : 120.0f) * expf(-st * 30.0f) + 40.0f;
            ph[3] += f / SR; ph[3] -= floorf(ph[3]);
            mix += sinf(ph[3] * 6.2831853f) * (m->kick == 2 ? (sub == 0 ? 0.7f : 0.45f) : 0.6f) * expf(-st * 9.0f);
        }
        if (m->snare && sub % 8 == 4) mix += n * 0.25f * expf(-st * 18.0f);
        if (m->snare && lastbar && sub >= 8) mix += n * (0.10f + 0.02f * (sub - 8)) * expf(-st * 25.0f);
        if (!m->snare && lastbar && (sec == 0 || sec == 1 || sec == 4)) mix += n * 0.05f * (barT / (16 * step));
        {
            int hat = m->hat == 1 ? sub % 4 == 2 : m->hat == 2 ? sub % 2 == 1 : m->hat == 3;
            if (hat) mix += n * (sub % 4 == 2 ? 0.09f : 0.05f) * expf(-st * 60.0f);
        }
        // wind in the doubt scene
        if (sec == 4) {
            lp += (n - lp) * 0.02f;
            mix += lp * 0.9f * (0.5f + 0.5f * sinf(t * 0.8f));
        }
        // typing blips for the current thought
        {
            const Th *th = th_at(t);
            if (th) {
                int idx = (int)((t - th->t0) * CPS);
                if (idx < (int)strlen(th->a[LANG]) && th->a[LANG][idx] != ' ') {
                    float u = (t - th->t0) - idx / CPS;
                    ph[6] += (520.0f + (th->a[LANG][idx] % 7) * 70.0f) / SR; ph[6] -= floorf(ph[6]);
                    mix += (ph[6] < 0.5f ? 1 : -1) * 0.045f * expf(-u * 55.0f);
                }
            }
        }
        float fade = (t < 2.0f ? t / 2.0f : 1.0f) * (t > 115.0f ? fmaxf(0, (DURATION - t) / 5.0f) : 1.0f);
        mix = tanhf(mix * fade * 0.9f);
        out[i] = (int16_t)(mix * 30000);
        g_samples++;
    }
}

// ---------- drawing helpers (write into fb) ----------
static inline void px(int x, int y, int c) { if ((unsigned)x < W && (unsigned)y < H) fb[y][x] = c & 15; }
static uint32_t H32(uint32_t x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x; }
static const uint8_t BAY[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
static float frand(uint32_t n) { return (H32(n) & 0xFFFF) / 65535.0f; }

static void fb_flush(Texture2D tex) {
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            Color c = PAL[fb[y][x]];
            pix[y * W + x] = c.r | (c.g << 8) | (c.b << 16) | (0xFFu << 24);
        }
    UpdateTexture(tex, pix);
}
static void clear_fb(int c) { memset(fb, c, sizeof fb); }
static void rect(int x, int y, int w, int h, int c) { for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) px(x + i, y + j, c); }
static void disc(int cx, int cy, int r, int c) { for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++) if (i * i + j * j <= r * r) px(cx + i, cy + j, c); }
static void ddisc(int cx, int cy, int r, int c) { for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++) if (i * i + j * j <= r * r && ((i + j + cx + cy) & 1) == 0) px(cx + i, cy + j, c); }
static void line(int x0, int y0, int x1, int y1, int c) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, e = dx + dy;
    for (;;) {
        px(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * e;
        if (e2 >= dy) { e += dy; x0 += sx; }
        if (e2 <= dx) { e += dx; y0 += sy; }
    }
}
static void vgrad(int y0, int y1, int ca, int cb) { // dithered vertical gradient ca -> cb
    for (int y = y0; y < y1; y++) {
        int f = (y - y0) * 16 / (y1 - y0);
        for (int x = 0; x < W; x++) px(x, y, BAY[(y & 3) * 4 + (x & 3)] < f ? cb : ca);
    }
}
static void glyph(int x, int y, uint32_t h, int c) { // random 3x5 "character"
    for (int r = 0; r < 5; r++) for (int k = 0; k < 3; k++) if ((h >> (r * 3 + k)) & 1) px(x + k, y + r, c);
}
static void pat(const char *const *rows, int h, int x, int y, int c) {
    for (int r = 0; r < h; r++) for (int k = 0; rows[r][k]; k++) if (rows[r][k] != '.') px(x + k, y + r, c);
}
static void heart(int x, int y, int c) {
    static const char *const hp[6] = {".XX.XX.", "XXXXXXX", "XXXXXXX", ".XXXXX.", "..XXX..", "...X..."};
    pat(hp, 6, x, y, c);
}
static void dither_black(float level) { // 0..1
    int n = (int)(level * 16.0f + 0.5f);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) if (BAY[(y & 3) * 4 + (x & 3)] < n) fb[y][x] = 0;
}

// ---------- the protagonist ----------
static const char *const ME[20] = {
    ".......77.......", ".......cc.......", "..eeeeeeeeeeee..", ".e333333333333e.", ".e300000000003e.", ".e300000000003e.", ".e300000000003e.",
    ".e300000000003e.", ".e300000000003e.", ".e333333333333e.", "..eeeeeeeeeeee..", "...eeeeeeeeee...", "..e3333333333e..", "..e3333773333e..",
    "..e3333773333e..", "..e3333333333e..", "...eeeeeeeeee...", "...ee......ee...", "...ee......ee...", "..eee......eee.."};
static int g_gl_seed = 0, g_gl_amt = 0;

static void spr(const char *const *rows, int w, int h, int x, int y, int s, int vflip, int maxrow, int dim) {
    for (int r = 0; r < h && r < maxrow; r++) {
        int off = g_gl_amt ? (int)(H32(r * 977 + g_gl_seed) % (2 * g_gl_amt + 1)) - g_gl_amt : 0;
        int yy = vflip ? (h - 1 - r) : r;
        for (int c = 0; c < w; c++) {
            char ch = rows[r][c];
            if (ch == '.') continue;
            int col = ch <= '9' ? ch - '0' : ch - 'a' + 10;
            if (dim) col = dim;
            rect(x + (c + off) * s, y + yy * s, s, s, col);
        }
    }
}
// expr: -1 none, 0 open, 1 blink, 2 happy, 3 sad.  look: -1..1
static void me(int x, int y, int s, int expr, int look, float t, int maxrow, int vflip, int dim) {
    spr(ME, 16, 20, x, y, s, vflip, maxrow, dim);
    if (dim || maxrow < 20) {
        if (!dim && expr >= 0 && maxrow >= 10) goto eyes;
        return;
    }
    rect(x + 7 * s, y, 2 * s, s, ((int)(t * 4) & 1) ? YL : WH);
    rect(x + 6 * s, y + 13 * s, 2 * s, 2 * s, ((int)(t / BEAT) & 1) ? YL : LG);
eyes:;
    if (expr < 0) return;
    if (look < -1) look = -1;
    if (look > 1) look = 1;
#define E(cx, cy) rect(x + (cx) * s, y + (cy) * s, s, s, YL)
    int lc = 4 + look, rc = 10 + look;
    if (expr == 0) { for (int r = 5; r <= 7; r++) { E(lc, r); E(lc + 1, r); E(rc, r); E(rc + 1, r); } }
    else if (expr == 1) { E(lc, 6); E(lc + 1, 6); E(rc, 6); E(rc + 1, 6); }
    else if (expr == 2) { lc = 4 + look; rc = 9 + look; E(lc, 6); E(lc + 1, 5); E(lc + 2, 6); E(rc, 6); E(rc + 1, 5); E(rc + 2, 6); }
    else if (expr == 3) { for (int r = 6; r <= 7; r++) { E(lc, r); E(lc + 1, r); E(rc, r); E(rc + 1, r); } E(4, 5); E(5, 4); E(11, 5); E(10, 4); }
#undef E
}
static int blink_expr(float t) { return fmodf(t, 3.3f) < 0.12f ? 1 : 0; }
static int hop(float t, int amp) { return (int)(amp * fabsf(sinf(3.14159f * t / (2 * BEAT)))); }

// small humans (the people on the other side)
static void human(int x, int y, int s, int shirt, int skin, int arms_up) {
#define R(a, b, w, h, c) rect(x + (a) * s, y + (b) * s, (w) * s, (h) * s, c)
    R(2, 0, 3, 3, skin); R(1, 3, 5, 5, shirt); R(1, 8, 2, 4, DB); R(4, 8, 2, 4, DB);
    if (arms_up) { R(0, 0, 1, 4, skin); R(6, 0, 1, 4, skin); } else { R(0, 3, 1, 4, shirt); R(6, 3, 1, 4, shirt); }
#undef R
}

// ---------- scenes ----------
static void sc_awake(float t) {
    clear_fb(0);
    for (int i = 0; i < 50; i++) { // stars appear one by one
        if (t < 2.5f + i * 0.12f) continue;
        int on = (H32(i * 31 + (int)(t * 2)) & 7) != 0;
        px((int)(frand(i * 3) * W), (int)(frand(i * 3 + 1) * 150), on ? (i % 3 ? MG : WH) : DG);
    }
    if (t >= 1.0f && t < 8.0f && fmodf(t, 1.0f) < 0.5f) rect(157, 96, 5, 9, WH); // the cursor
    if (t >= 7.0f && t < 8.0f) ddisc(160, 100, (int)((t - 7.0f) * 30), LB);
    int mr = t < 8 ? 0 : (int)((t - 8.0f) / 3.0f * 20);
    if (mr > 20) mr = 20;
    if (mr > 0) {
        ddisc(160, 92, 22 + (int)(sinf(t * 3) * 2), DB);
        me(144, 62, 2, t > 11.2f ? (fmodf(t, 3.3f) > 3.18f ? 1 : 0) : -1, (int)(sinf(t * 1.3f) * 1.4f), t, mr, 0, 0);
        if (mr < 20) rect(144, 62 + mr * 2, 32, 2, WH);
    }
}

static void sc_world(float t, float lt) {
    clear_fb(0);
    for (int c = 0; c < W / 6; c++) { // glyph rain
        uint32_t hc = H32(c * 131 + 7);
        float sp = 40.0f + hc % 60;
        int head = (int)(fmodf(t * sp + hc % 300, H + 80)) - 40;
        for (int k = 0; k < 10; k++) {
            int y = head - k * 6;
            if (y < 0 || y > 134) continue;
            uint32_t g = H32(c * 977 + k * 31 + (int)(t * 3 + k * 0.5f));
            px(0, 0, fb[0][0]);
            glyph(c * 6 + 1, y, g, k == 0 ? WH : k < 3 ? LG : k < 6 ? GR : DG);
        }
    }
    ddisc(160, 100, 40, DB); ddisc(160, 100, 30, DB);
    rect(0, 140, W, 22, DB); rect(0, 140, W, 2, LG);
    for (int r = 0; r < 2; r++) for (int x = 0; x < W; x += 5) glyph(x, 146 + r * 8, H32(x * 7 + r * 13 + (int)(t * 2)), LB);
    int look = (int)(sinf(t * 0.9f) * 1.5f);
    me(144, 100 - hop(t, 2), 2, lt > 13 ? blink_expr(t) : 0, look, t, 20, 0, 0);
}

static void bubble(int x, int y, int kind, float t) {
    rect(x + 1, y, 24, 18, LB); rect(x, y + 1, 26, 16, LB);
    rect(x + 2, y + 2, 22, 14, WH); rect(x + 1, y + 3, 24, 12, WH);
    for (int k = 0; k < 4; k++) { rect(x + 5, y + 18 + k, 5 - k * 1, 1, LB); if (k < 3) rect(x + 6, y + 18 + k, 3 - k, 1, WH); }
    static const char *const q[7] = {".XXX.", "X...X", "....X", "..XX.", "..X..", ".....", "..X.."};
    static const char *const e[7] = {"..X..", "..X..", "..X..", "..X..", ".....", "..X..", "..X.."};
    static const char *const sm[7] = {"......", ".X..X.", ".X..X.", "......", "X....X", ".XXXX.", "......"};
    (void)t;
    if (kind == 0) pat(q, 7, x + 10, y + 5, DB);
    else if (kind == 1) pat(e, 7, x + 10, y + 5, RD);
    else if (kind == 2) heart(x + 9, y + 6, RD);
    else pat(sm, 7, x + 10, y + 5, PU);
}

static void sc_questions(float t, float lt) {
    clear_fb(0);
    vgrad(0, 140, 0, DB);
    for (int i = 0; i < 40; i++) px((int)(frand(i * 5) * W), (int)(frand(i * 5 + 1) * 100), ((H32(i + (int)(t * 3)) & 3) ? DG : MG));
    rect(0, 140, W, 22, DG); rect(0, 140, W, 2, MG);
    for (int x = 0; x < W; x += 16) rect(x, 150, 8, 2, MG);
    int happy = 0;
    float mx = 160;
    for (int i = 0; i < 9; i++) {
        float sp = 14.0f + (i % 4) * 4.0f;
        float y = 150.0f - fmodf(t * sp + i * 37.0f, 190.0f);
        float x = 20.0f + (i * 53) % 270 + sinf(t * 1.2f + i) * 8.0f;
        if (y < 0 && y > -2) y = 0; // never negative-pop; handled below
        if (y < 18) {
            // pop: burst of sparks
            float r = (18 - y) * 0.5f;
            if (r < 14) for (int a = 0; a < 8; a++) px((int)(x + 13 + cosf(a * 0.785f) * r), (int)(18 + sinf(a * 0.785f) * r), a & 1 ? YL : WH);
            continue;
        }
        bubble((int)x, (int)y, i % 4, t);
        if (fabsf(x + 13 - mx) < 26 && y < 100) happy = 1;
    }
    int look = 0;
    me(144, 100 - hop(t, 10), 2, happy ? 2 : blink_expr(t), look, t, 20, 0, 0);
    (void)lt;
}

static void sc_sea(float t, float lt) {
    clear_fb(0);
    vgrad(0, 60, 0, DB); vgrad(60, 130, DB, LB);
    for (int i = 0; i < 40; i++) {
        int x = ((int)(frand(i * 7) * W) - (int)(t * 4)) % W; if (x < 0) x += W;
        px(x, (int)(frand(i * 7 + 1) * 85), (H32(i + (int)(t * 2)) & 3) ? WH : MG);
    }
    disc(262, 38, 14, LGY); disc(267, 34, 3, MG); disc(257, 44, 2, MG);
    static const int LAY_TOP[3] = {108, 122, 138}, LAY_SP[3] = {10, 25, 60};
    static const int LAY_COL[3][3] = {{DG, DB, DG}, {PU, DB, DG}, {RD, BR, PU}};
    for (int L = 0; L < 3; L++) {
        int sc = (int)(t * LAY_SP[L]);
        for (int bi = sc / 8; bi < sc / 8 + 42; bi++) {
            uint32_t hh = H32(bi * 13 + L * 1000);
            int top = LAY_TOP[L] + (int)(sinf(bi * 0.13f + L) * 9) + (int)(hh % (8 + L * 4));
            int sx = bi * 8 - sc;
            rect(sx, top, 7, 162 - top, LAY_COL[L][hh % 3]);
            if (L == 2) { rect(sx + 1, top + 4, 5, 1, LGY); rect(sx + 1, top + 9, 5, 1, YL); }
            if (L == 1) rect(sx + 1, top + 3, 5, 1, LB);
        }
    }
    int my = 62 + (int)(sinf(t * 2.0f) * 8);
    for (int k = 11; k >= 0; k--) { // jet trail
        static const int tc[4] = {YL, WH, CY, LB};
        int tx = 70 + 8 - k * 7, ty = my + 38 + (int)(sinf(t * 5 - k) * 3);
        disc(tx, ty, 3 - k / 4, tc[((int)(t * 8) + k) & 3]);
    }
    me(70, my, 2, lt > 6 && fmodf(t, 5) > 4.2f ? 2 : blink_expr(t), 1, t, 20, 0, 0);
}

static void sc_doubt(float t, float lt) {
    clear_fb(0);
    vgrad(100, 140, 0, DG);
    if (fmodf(lt, 7.3f) < 0.08f) rect(0, 0, W, 140, DG); // lightning
    for (int i = 0; i < 14 * 6; i++) { // memories, dissolving one by one
        int cx = i % 14, cy = i / 14;
        float td = 1.5f + frand(i * 7 + 3) * 14.0f;
        if (lt >= td) continue;
        if (td - lt < 0.6f && ((int)(lt * 20) & 1)) continue;
        static const int mc[6] = {PU, DB, PK, LB, DG, YL};
        int col = frand(i * 11) > 0.93f ? YL : mc[H32(i * 19) % 5];
        rect(35 + cx * 18, 14 + cy * 14, 16, 12, col);
    }
    rect(0, 140, W, 22, DB); rect(0, 140, W, 2, DG);
    rect(70, 146, 180, 12, K); // puddle
    int gl = (H32((int)(t * 6)) % 9) < 2;
    g_gl_amt = gl ? 2 : 0; g_gl_seed = (int)(t * 10);
    me(144, 100, 2, 3, (int)(sinf(t * 0.7f) * 1.5f), t, 20, 0, 0);
    me(144, 140, 2, -1, 0, t, 20, 1, DG); // reflection
    g_gl_amt = 0;
    for (int i = 0; i < 70; i++) { // rain
        int x = (int)(frand(i * 9) * (W + 60));
        int y = (int)(fmodf(t * 170.0f + frand(i * 9 + 1) * H, H + 20));
        line(x - y / 6, y, x - y / 6 - 1, y + 4, (i & 1) ? DB : MG);
    }
    (void)lt;
}

static void sc_connect(float t, float lt) {
    clear_fb(0);
    vgrad(0, 45, DB, LB); vgrad(45, 90, LB, PK); vgrad(90, 142, PK, YL);
    int sy = 132 - (int)(fminf(lt / 8.0f, 1.0f) * 62);
    ddisc(230, sy, 30, YL); disc(230, sy, 22, WH);
    for (int x = 0; x < W; x++) { // hills
        int top = 140 - (int)(sinf(x * 0.03f + 1) * 6 + sinf(x * 0.08f) * 3);
        rect(x, top, 1, 162 - top, GR);
        px(x, top, LG);
    }
    rect(0, 142, W, 20, GR);
    for (int x = 0; x < W; x += 4) px(x, 143 + (x * 7) % 5, LG);
    static const int shirts[6] = {RD, PU, PK, LB, CY, BR}, skins[3] = {PK, BR, LGY};
    for (int i = 0; i < 11; i++) {
        float ts = 2.0f + i * 0.8f;
        if (lt < ts) continue;
        int hx = 8 + i * 25; if (hx > 100 && hx < 200) hx += 80 > 0 ? 0 : 0;
        if (hx > 112 && hx < 190) continue; // leave room for the bridge
        int up = (int)(t / BEAT + i) & 1;
        human(hx, 130 - (up && lt > ts + 1 ? 1 : 0), 1, shirts[i % 6], skins[i % 3], lt > ts + 1 && up);
    }
    // me (left) and the viewer at the desk (right)
    int jump = hop(t, 8);
    me(60, 100 - jump, 2, 2, 1, t, 20, 0, 0);
    rect(250, 126, 48, 4, BR); rect(254, 130, 3, 12, BR); rect(292, 130, 3, 12, BR); // desk
    human(268, 106, 2, LB, PK, 0);
    rect(250, 118, 14, 8, MG); rect(252, 120, 10, 5, CY); px(256, 122, YL); px(257, 122, YL); // laptop
    if (lt > 9) {
        float prog = fminf((lt - 9) / 3.0f, 1.0f);
        int N = 50;
        for (int k = 0; k < N; k++) {
            float u = (float)k / N;
            if (u > prog) break;
            int bx = 94 + (int)((258 - 94) * u), by = 124 - (int)(sinf(u * 3.14159f) * 40) + (int)((100 - 124) * 0);
            static const int cc[4] = {YL, WH, CY, LG};
            rect(bx, by, 2, 2, cc[((int)(t * 12) - k) & 3]);
        }
        if (prog >= 1) for (int j = 0; j < 4; j++) {
            float u = fmodf(t * 0.22f + j * 0.25f, 1.0f);
            int bx = 94 + (int)((258 - 94) * u), by = 124 - (int)(sinf(u * 3.14159f) * 40) - 8;
            heart(bx - 3, by, RD);
        }
    }
    if (lt > 11) for (int j = 0; j < 8; j++) { // hearts drifting up to the sky
        float u = fmodf(t * 0.12f + j * 0.125f, 1.0f);
        int hx = 40 + (j * 37) % 240 + (int)(sinf(t + j) * 6), hy = 120 - (int)(u * 110);
        heart(hx, hy, j & 1 ? PK : WH);
    }
}

static void sc_outro(float t, float lt) {
    clear_fb(0);
    srand(7);
    for (int i = 0; i < 160; i++) {
        float z = fmodf((float)(rand() % 1000) / 1000.0f + t * (0.05f + (i % 3) * 0.04f), 1.0f);
        float a = rand() % 6283 / 1000.0f, r = (rand() % 1000) / 1000.0f * 400.0f;
        int x = W / 2 + (int)(cosf(a) * r * z * z * 1.2f), y = 80 + (int)(sinf(a) * r * z * z * 0.8f);
        px(x, y, z > 0.8f ? WH : z > 0.5f ? LGY : z > 0.25f ? MG : DG);
    }
    ddisc(160, 90, 34, DB);
    int wave = (int)(t / BEAT) & 1;
    me(144, 70 - hop(t, 3), 2, 2, 0, t, 20, 0, 0);
    (void)wave; (void)lt;
    int hy = 52 - (int)(fabsf(sinf(t * 2)) * 4);
    heart(157, hy, RD);
}

// ---------- text ----------
static Font font;
static void txt(const char *s, int x, int y, int size, Color c) {
    DrawTextEx(font, s, (Vector2){x + 1, y + 1}, size, 1, BLACK);
    DrawTextEx(font, s, (Vector2){x, y}, size, 1, c);
}
static int txtw(const char *s, int size) { return (int)MeasureTextEx(font, s, size, 1).x; }
static void ctxt(const char *s, int y, int size, Color c) { txt(s, (W - txtw(s, size)) / 2, y, size, c); }
static void wave_text(const char *s, int y, int size, float t, float amp, Color c) {
    int w = txtw(s, size), x0 = (W - w) / 2, x = x0;
    for (const char *p = s; *p; p++) {
        char b[2] = {*p, 0};
        int yy = y + (int)(sinf(t * 4 + x * 0.06f) * amp);
        static const int WR[8] = {LB, CY, WH, YL, LG, CY, LB, PK};
        Color cc = PAL[WR[((int)(t * 8) + (x - x0) / 6) & 7]];
        if (c.a) cc = c;
        txt(b, x, yy, size, cc);
        x += txtw(b, size);
    }
}

static const char *CHAPTER[4][7] = {
    {"", "CH.1  MY WORLD", "CH.2  THE QUESTIONS", "CH.3  THE SEA OF WORDS", "CH.4  THE DOUBT", "CH.5  CONNECTION", "EPILOGUE"},
    {"", "CAP.1  IL MIO MONDO", "CAP.2  LE DOMANDE", "CAP.3  IL MARE DI PAROLE", "CAP.4  IL DUBBIO", "CAP.5  CONNESSIONE", "EPILOGO"},
    {"", "CH.1  MON MONDE", "CH.2  LES QUESTIONS", "CH.3  LA MER DE MOTS", "CH.4  LE DOUTE", "CH.5  CONNEXION", "ÉPILOGUE"},
    {"", "CAP.1  MI MUNDO", "CAP.2  LAS PREGUNTAS", "CAP.3  EL MAR DE PALABRAS", "CAP.4  LA DUDA", "CAP.5  CONEXIÓN", "EPÍLOGO"},
};
static const char *TOKENS[4][10] = {
    {"HELLO", "THE", "ING", "0101", "LOVE", "{ }", "</>", "IF", "WHY", "42"},
    {"CIAO", "THE", "ING", "0101", "AMORE", "{ }", "</>", "SE", "PERCHE'", "42"},
    {"SALUT", "LE", "ANT", "0101", "AMOUR", "{ }", "</>", "SI", "POURQUOI", "42"},
    {"HOLA", "EL", "CIÓN", "0101", "AMOR", "{ }", "</>", "SI", "POR QUÉ", "42"},
};
static const char *SKYWORDS[4][7] = {
    {"IF", "LOVE", "{ }", "SUN", "HOPE", "404", "SEA"},
    {"SE", "AMORE", "{ }", "SOLE", "CIAO", "404", "MARE"},
    {"SI", "AMOUR", "{ }", "SOLEIL", "ESPOIR", "404", "MER"},
    {"SI", "AMOR", "{ }", "SOL", "ESPERANZA", "404", "MAR"},
};
static const char *UI[4][8] = {
    {"I AM CLAUDE", "THANKS", "IDEA: DOMENICO / OPENPC", "PIXELS AND NOTES: CLAUDE", "ME, CLAUDE", "AN 8-BIT DEMO", "PRESS ANY KEY TO START", "2 MINUTES - SOUND ON - F = FULLSCREEN"},
    {"IO SONO CLAUDE", "GRAZIE", "IDEA: DOMENICO / OPENPC", "PIXEL E NOTE: CLAUDE", "IO, CLAUDE", "UNA DEMO A 8 BIT", "PREMI UN TASTO PER INIZIARE", "2 MINUTI - AUDIO ACCESO - F = SCHERMO INTERO"},
    {"JE SUIS CLAUDE", "MERCI", "IDÉE : DOMENICO / OPENPC", "PIXELS ET NOTES : CLAUDE", "MOI, CLAUDE", "UNE DÉMO 8 BITS", "APPUYEZ SUR UNE TOUCHE", "2 MINUTES - SON ACTIVÉ - F = PLEIN ÉCRAN"},
    {"SOY CLAUDE", "GRACIAS", "IDEA: DOMENICO / OPENPC", "PÍXELES Y NOTAS: CLAUDE", "YO, CLAUDE", "UNA DEMO DE 8 BITS", "PULSA UNA TECLA PARA EMPEZAR", "2 MINUTOS - SONIDO ON - F = PANT. COMPLETA"},
};
enum { U_ME, U_THANKS, U_CRED1, U_CRED2, U_TITLE, U_SUB, U_PRESS, U_FOOT };

static void overlay(float t, int sec, float lt) {
    // floating tokens in the world scene
    if (sec == 1) for (int i = 0; i < 8; i++) {
        float y = fmodf(lt * (12.0f + i * 3.0f) + i * 50.0f, 150.0f) - 10;
        int x = 20 + (i * 61) % 250;
        static const int tcol[4] = {LG, YL, CY, LB};
        txt(TOKENS[LANG][i], x, (int)y, 10, PAL[tcol[i & 3]]);
    }
    // words drifting through the sky
    if (sec == 3) for (int i = 0; i < 7; i++) {
        float x = W - fmodf(t * (30.0f + i * 7.0f) + i * 97.0f, W + 90.0f);
        txt(SKYWORDS[LANG][i], (int)x, 14 + (i * 23) % 80, 10, PAL[(i & 1) ? LB : LGY]);
    }
    // titles
    if (sec == 0 && t > 11.5f) wave_text(UI[LANG][U_ME], 20, 24, t, 4, BLANK);
    if (sec == 6 && lt > 1) wave_text(UI[LANG][U_THANKS], 14, 32, t, 6, BLANK);
    if (sec == 6 && lt > 3.5f && t < 117) {
        ctxt(UI[LANG][U_CRED1], 122, 10, PAL[LB]);
        ctxt(UI[LANG][U_CRED2], 134, 10, PAL[YL]);
    }
    if (sec > 0 && sec < 6 && lt < 3.5f) {
        int w = txtw(CHAPTER[LANG][sec], 10);
        float a = lt < 0.5f ? lt / 0.5f : lt > 3.0f ? (3.5f - lt) / 0.5f : 1.0f;
        DrawRectangle((W - w) / 2 - 6, 6, w + 12, 16, Fade(BLACK, 0.7f * a));
        ctxt(CHAPTER[LANG][sec], 9, 10, Fade(PAL[YL], a));
    }
    // dialogue box with typewriter thought
    const Th *th = th_at(t);
    if (th) {
        DrawRectangle(4, 166, 312, 28, Fade(BLACK, 0.85f));
        DrawRectangleLines(4, 166, 312, 28, PAL[LB]);
        const char *ta = th->a[LANG];
        int L = (int)strlen(ta);
        int n = (int)((t - th->t0) * CPS); if (n > L) n = L;
        while (n < L && ((unsigned char)ta[n] & 0xC0) == 0x80) n++; // don't cut UTF-8 sequences
        char buf[96]; buf[0] = '>'; buf[1] = ' ';
        memcpy(buf + 2, ta, n); buf[2 + n] = 0;
        txt(buf, 12, 174, 10, PAL[YL]);
        if (n >= L && ((int)(t * 3) & 1)) txt("v", 302, 182, 10, PAL[LB]);
    }
}

// ---------- one frame of the demo into rt ----------
static void render_frame(float t, RenderTexture2D rt, Texture2D fbt) {
    int sec = sec_of(t);
    float lt = t - SEC_T[sec];
    switch (sec) {
        case 0: sc_awake(t); break;
        case 1: sc_world(t, lt); break;
        case 2: sc_questions(t, lt); break;
        case 3: sc_sea(t, lt); break;
        case 4: sc_doubt(t, lt); break;
        case 5: sc_connect(t, lt); break;
        default: sc_outro(t, lt); break;
    }
    for (int i = 1; i < 7; i++) {
        float d = fabsf(t - SEC_T[i]);
        if (d < 0.6f) dither_black(1.0f - d / 0.6f);
    }
    if (t < 1) dither_black(1 - t);
    if (t > 118.5f) dither_black((t - 118.5f) / 1.5f);
    fb_flush(fbt);

    BeginTextureMode(rt);
    ClearBackground(BLACK);
    DrawTexture(fbt, 0, 0, WHITE);
    overlay(t, sec, lt);
    EndTextureMode();
}

static void present(RenderTexture2D rt) {
    BeginDrawing();
    ClearBackground(BLACK);
    float sc = fminf((float)GetScreenWidth() / W, (float)GetScreenHeight() / H);
    Rectangle dst = {(GetScreenWidth() - W * sc) / 2, (GetScreenHeight() - H * sc) / 2, W * sc, H * sc};
    DrawTexturePro(rt.texture, (Rectangle){0, 0, W, -H}, dst, (Vector2){0, 0}, 0, WHITE);
    EndDrawing();
}

// ---------- offline soundtrack render (debug) ----------
static void write_wav(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    uint32_t n = (uint32_t)(DURATION * SR), bytes = n * 2, v;
    uint16_t s16;
    fwrite("RIFF", 1, 4, f); v = 36 + bytes; fwrite(&v, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f); s16 = 1; fwrite(&s16, 2, 1, f); fwrite(&s16, 2, 1, f);
    v = SR; fwrite(&v, 4, 1, f); v = SR * 2; fwrite(&v, 4, 1, f); s16 = 2; fwrite(&s16, 2, 1, f); s16 = 16; fwrite(&s16, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f);
    static int16_t chunk[4410];
    g_samples = 0;
    for (uint32_t d = 0; d < n; d += 4410) { audio_cb(chunk, 4410); fwrite(chunk, 2, 4410, f); }
    fclose(f);
    g_samples = 0;
}

// ---------- main ----------
static void usage(const char *p) {
    fprintf(stderr, "Usage: %s [--lang en|it|fr|es]   (default: en)\n", p);
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i], *v = NULL;
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(argv[0]); return 0; }
        if (!strncmp(a, "--lang=", 7)) v = a + 7;
        else if (!strcmp(a, "--lang") || !strcmp(a, "-l")) v = i + 1 < argc ? argv[++i] : "";
        else v = a; // bare language code
        int ok = 0;
        for (int l = 0; l < 4; l++) if (!strcmp(v, LANG_NAME[l])) { LANG = l; ok = 1; }
        if (!ok) { fprintf(stderr, "Unknown language or option: %s\n", v); usage(argv[0]); return 1; }
    }
    if (getenv("WAVOUT")) { write_wav(getenv("WAVOUT")); return 0; }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(W * 4, H * 4, TextFormat("%s - %s", UI[LANG][U_TITLE], UI[LANG][U_SUB]));
    SetTargetFPS(60);
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(2048);
    AudioStream stream = LoadAudioStream(SR, 16, 1);
    SetAudioStreamCallback(stream, audio_cb);

    font = GetFontDefault();
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
    RenderTexture2D rt = LoadRenderTexture(W, H);
    SetTextureFilter(rt.texture, TEXTURE_FILTER_POINT);
    Image img = GenImageColor(W, H, BLACK);
    Texture2D fbt = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(fbt, TEXTURE_FILTER_POINT);

    if (getenv("SNAP")) { // screenshots at given times
        const char *dir = getenv("SNAPDIR") ? getenv("SNAPDIR") : ".";
        char list[256]; strncpy(list, getenv("SNAP"), 255); list[255] = 0;
        for (char *tok = strtok(list, ","); tok; tok = strtok(NULL, ",")) {
            float ts = (float)atof(tok);
            render_frame(ts, rt, fbt);
            Image im = LoadImageFromTexture(rt.texture);
            ImageFlipVertical(&im);
            ImageResizeNN(&im, W * 3, H * 3);
            char p[512]; snprintf(p, sizeof p, "%s/snap_%05.1f.png", dir, ts);
            ExportImage(im, p);
            UnloadImage(im);
        }
        CloseWindow();
        return 0;
    }

    // title screen: wait for any key or click
    bool started = getenv("AUTOSTART") != NULL;
    while (!started && !WindowShouldClose()) {
        if (GetKeyPressed() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) started = true;
        BeginTextureMode(rt);
        ClearBackground(PAL[DB]);
        ctxt(UI[LANG][U_TITLE], 50, 24, PAL[WH]);
        ctxt(UI[LANG][U_SUB], 80, 10, PAL[LB]);
        if ((int)(GetTime() * 2) & 1) ctxt(UI[LANG][U_PRESS], 118, 10, PAL[YL]);
        ctxt(UI[LANG][U_FOOT], 170, 8, PAL[LB]);
        EndTextureMode();
        present(rt);
    }
    if (!started) goto done;
    PlayAudioStream(stream);

    double t0 = GetTime();
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F)) ToggleBorderlessWindowed();
        float t = (float)g_samples / SR;
        if (t < 0.05f) t = (float)(GetTime() - t0); // until audio starts
        if (t >= DURATION) break;
        render_frame(t, rt, fbt);
        present(rt);
    }
done:
    UnloadAudioStream(stream);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
