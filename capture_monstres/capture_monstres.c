#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#define MAX_MONSTRES 5

typedef struct {
    int x, y;
    int vitesse_x, vitesse_y;
    bool actif;
} Monstre;

typedef struct {
    Monstre monstres[MAX_MONSTRES];
    int score;
    int precision;
    bool en_cours;
} JeuEtat;

static void jeu_dessiner_callback(Canvas* canvas, void* ctx) {
    JeuEtat* etat = ctx;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    char buf[32];
    snprintf(buf, sizeof(buf), "Score: %d", etat->score);
    canvas_draw_str(canvas, 2, 10, buf);
    snprintf(buf, sizeof(buf), "Precision: %d%%", etat->precision);
    canvas_draw_str(canvas, 70, 10, buf);

    for(int i = 0; i < MAX_MONSTRES; i++) {
        if(etat->monstres[i].actif) {
            canvas_draw_disc(canvas, etat->monstres[i].x, etat->monstres[i].y, 5);
            canvas_draw_circle(canvas, etat->monstres[i].x, etat->monstres[i].y, 6);
        }
    }
    if(!etat->en_cours) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 20, 40, "Appuyez pour rejouer");
    }
}

static void jeu_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static void jeu_initialiser(JeuEtat* etat) {
    etat->score = 0;
    etat->precision = 0;
    etat->en_cours = true;
    for(int i = 0; i < MAX_MONSTRES; i++) {
        etat->monstres[i].x = rand() % 100 + 14;
        etat->monstres[i].y = rand() % 30 + 20;
        etat->monstres[i].vitesse_x = (rand() % 3) - 1;
        etat->monstres[i].vitesse_y = (rand() % 3) - 1;
        etat->monstres[i].actif = true;
    }
}

static void jeu_mettre_a_jour(JeuEtat* etat) {
    if(!etat->en_cours) return;

    for(int i = 0; i < MAX_MONSTRES; i++) {
        if(!etat->monstres[i].actif) continue;
        etat->monstres[i].x += etat->monstres[i].vitesse_x;
        etat->monstres[i].y += etat->monstres[i].vitesse_y;
        if(etat->monstres[i].x < 6 || etat->monstres[i].x > 122) etat->monstres[i].vitesse_x *= -1;
        if(etat->monstres[i].y < 15 || etat->monstres[i].y > 55) etat->monstres[i].vitesse_y *= -1;
    }
}

int32_t capture_monstres_app(void* p) {
    UNUSED(p);
    JeuEtat* etat = malloc(sizeof(JeuEtat));
    jeu_initialiser(etat);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, jeu_dessiner_callback, etat);
    view_port_input_callback_set(view_port, jeu_input_callback, event_queue);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    
    InputEvent event;
    bool running = true;
    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if(event.type == InputTypePress) {
                switch(event.key) {
                    case InputKeyOk:
                        if(!etat->en_cours) { 
                            jeu_initialiser(etat); 
                        } else {
                            etat->score += 10;
                            etat->precision = (etat->precision < 100) ? etat->precision + 5 : 100;
                        }
                        break;
                    case InputKeyBack: running = false; break;
                    default: break;
                }
            }
        }
        jeu_mettre_a_jour(etat);
        view_port_update(view_port);
    }
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_GUI);
    free(etat);
    return 0;
}
}
