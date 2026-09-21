#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#define MAX_MONSTRES 5
#define TAILLE_MONSTRE 4

typedef struct {
    int x, y;
    int vx, vy;
    bool actif;
} Monstre;

typedef struct {
    Monstre monstres[MAX_MONSTRES];
    int score;
    int lancers;
    int captures;
    bool en_cours;
    bool mode_auto;
    int balle_x, balle_y, balle_vx, balle_vy;
    bool balle_active;
    int compteur_auto;
} JeuEtat;

static void dessiner(Canvas* canvas, void* ctx) {
    JeuEtat* etat = ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    char buf[32];
    int precision = (etat->lancers > 0) ? (etat->captures * 100 / etat->lancers) : 0;
    snprintf(buf, sizeof(buf), "S:%d P:%d%%", etat->score, precision);
    canvas_draw_str(canvas, 2, 10, buf);
    if(etat->mode_auto) {
        canvas_draw_str(canvas, 96, 10, "AUTO");
    }
    for(int i = 0; i < MAX_MONSTRES; i++) {
        if(etat->monstres[i].actif) {
            canvas_draw_disc(canvas, etat->monstres[i].x, etat->monstres[i].y, TAILLE_MONSTRE);
        }
    }
    if(etat->balle_active) {
        canvas_draw_disc(canvas, etat->balle_x, etat->balle_y, 3);
    }
    if(!etat->en_cours) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 8, 35, "Partie finie!");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 10, 50, "OK pour rejouer");
    }
}

static void input_cb(InputEvent* event, void* ctx) {
    FuriMessageQueue* queue = ctx;
    furi_message_queue_put(queue, event, FuriWaitForever);
}

static void reinit(JeuEtat* etat) {
    etat->score = 0;
    etat->lancers = 0;
    etat->captures = 0;
    etat->en_cours = true;
    etat->balle_active = false;
    etat->compteur_auto = 0;
    etat->mode_auto = false;
    for(int i = 0; i < MAX_MONSTRES; i++) {
        etat->monstres[i].x = 20 + rand() % 88;
        etat->monstres[i].y = 22 + rand() % 28;
        etat->monstres[i].vx = (rand() % 3) - 1;
        etat->monstres[i].vy = (rand() % 3) - 1;
        if(etat->monstres[i].vx == 0 && etat->monstres[i].vy == 0) {
            etat->monstres[i].vx = 1;
        }
        etat->monstres[i].actif = true;
    }
}

static void update(JeuEtat* etat) {
    if(!etat->en_cours) return;
    for(int i = 0; i < MAX_MONSTRES; i++) {
        if(!etat->monstres[i].actif) continue;
        etat->monstres[i].x += etat->monstres[i].vx;
        etat->monstres[i].y += etat->monstres[i].vy;
        if(etat->monstres[i].x < TAILLE_MONSTRE || etat->monstres[i].x > 128 - TAILLE_MONSTRE) {
            etat->monstres[i].vx *= -1;
        }
        if(etat->monstres[i].y < 14 + TAILLE_MONSTRE || etat->monstres[i].y > 62 - TAILLE_MONSTRE) {
            etat->monstres[i].vy *= -1;
        }
    }
    if(etat->balle_active) {
        etat->balle_x += etat->balle_vx;
        etat->balle_y += etat->balle_vy;
        if(etat->balle_x < 0 || etat->balle_x > 128 || etat->balle_y < 12 || etat->balle_y > 64) {
            etat->balle_active = false;
        } else {
            for(int i = 0; i < MAX_MONSTRES; i++) {
                if(!etat->monstres[i].actif) continue;
                int dx = etat->balle_x - etat->monstres[i].x;
                int dy = etat->balle_y - etat->monstres[i].y;
                if(dx*dx + dy*dy < (TAILLE_MONSTRE + 3) * (TAILLE_MONSTRE + 3)) {
                    etat->monstres[i].actif = false;
                    etat->balle_active = false;
                    etat->score += 10;
                    etat->captures++;
                    break;
                }
            }
        }
    }
    if(etat->mode_auto && !etat->balle_active) {
        etat->compteur_auto++;
        if(etat->compteur_auto > 15) {
            etat->compteur_auto = 0;
            int cible = -1;
            for(int i = 0; i < MAX_MONSTRES; i++) {
                if(etat->monstres[i].actif) { cible = i; break; }
            }
            if(cible >= 0) {
                etat->balle_x = 64;
                etat->balle_y = 60;
                int dx = etat->monstres[cible].x - 64;
                int dy = etat->monstres[cible].y - 60;
                etat->balle_vx = (dx > 0) ? 2 : ((dx < 0) ? -2 : 0);
                etat->balle_vy = (dy > 0) ? 1 : -1;
                etat->balle_active = true;
                etat->lancers++;
            }
        }
    }
    bool tous_inactifs = true;
    for(int i = 0; i < MAX_MONSTRES; i++) {
        if(etat->monstres[i].actif) { tous_inactifs = false; break; }
    }
    if(tous_inactifs) {
        etat->en_cours = false;
    }
}

int32_t capture_monstres_app(void* p) {
    UNUSED(p);
    JeuEtat* etat = malloc(sizeof(JeuEtat));
    reinit(etat);
    FuriMessageQueue* queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    ViewPort* vp = view_port_alloc();
    view_port_draw_callback_set(vp, dessiner, etat);
    view_port_input_callback_set(vp, input_cb, queue);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, vp, GuiLayerFullscreen);
    InputEvent event;
    bool running = true;
    while(running) {
        if(furi_message_queue_get(queue, &event, 30) == FuriStatusOk) {
            if(event.type == InputTypePress) {
                if(event.key == InputKeyBack) {
                    running = false;
                } else if(event.key == InputKeyOk) {
                    if(!etat->en_cours) {
                        reinit(etat);
                    } else if(!etat->balle_active && !etat->mode_auto) {
                        etat->balle_x = 64;
                        etat->balle_y = 60;
                        etat->balle_vx = (rand() % 3) - 1;
                        etat->balle_vy = -2;
                        if(etat->balle_vx == 0) etat->balle_vx = 1;
                        etat->balle_active = true;
                        etat->lancers++;
                    }
                } else if(event.key == InputKeyUp) {
                    if(etat->en_cours) {
                        etat->mode_auto = !etat->mode_auto;
                        etat->compteur_auto = 0;
                    }
                }
            }
        }
        update(etat);
        view_port_update(vp);
    }
    view_port_enabled_set(vp, false);
    gui_remove_view_port(gui, vp);
    view_port_free(vp);
    furi_message_queue_free(queue);
    furi_record_close(RECORD_GUI);
    free(etat);
    return 0;
}
