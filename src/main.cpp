#include <mbed.h>
#include <threadLvgl.h>
#include <cstdio>

// Thread LVGL pour gérer les mises à jour de l'interface graphique
ThreadLvgl threadLvgl(30);

// Crée un objet DigitalOut pour basculer une LED à chaque fois que des données sont reçues.
static DigitalOut led(LED1);

// Crée un objet BufferedSerial avec un taux de baud par défaut.
static BufferedSerial serial_port(D1, D0); // Ajustez la broche RX en conséquence
static DigitalOut gache(A0); // Sortie numérique pour contrôler la gâche

// Définition de la taille maximale du buffer et de l'UUID accepté pour le contrôle d'accès
#define MAXIMUM_BUFFER_SIZE  32
#define ACCEPTED_UUID "0700AE2D0F8B"

// Objets LVGL pour l'interface graphique
lv_obj_t *text_area;
lv_obj_t *status_label;
lv_obj_t *bg_img;
lv_obj_t *title_label;

// Inclure les données d'image générées
LV_IMG_DECLARE(rfid_bg); // Supposons que votre tableau d'images s'appelle rfid_bg

// Fonction pour mettre à jour la zone de texte avec l'UUID de la carte et afficher le statut
void update_text_area(const char* uuid) {
    lv_textarea_set_text(text_area, uuid); // Afficher l'UUID dans la zone de texte
    printf("uuid : %s \r\n", uuid);
    printf("ACCEPTED_UUID : %s \r\n", ACCEPTED_UUID);
    if (strcmp(uuid, ACCEPTED_UUID) == 0) {
        // Si l'UUID correspond à l'UUID accepté, afficher "Accepted" et déverrouiller la gâche
        lv_label_set_text(status_label, "Accepted");
        gache = 1; // Déverrouiller la gâche
        ThisThread::sleep_for(1000ms); // Garder la gâche déverrouillée pendant 1 seconde
        gache = 0; // Verrouiller la gâche
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0); // Couleur verte pour accepté
    } else {
        // Si l'UUID ne correspond pas, afficher "Rejected"
        gache = 0; // Assurer que la gâche est verrouillée
        lv_label_set_text(status_label, "Rejected");
        lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), 0); // Couleur rouge pour rejeté
    }
}

// Fonction pour initialiser l'interface graphique LVGL
void gui_init() {
    lv_init();

    // Définir la couleur de fond en blanc
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    // Créer et définir l'image de fond
    bg_img = lv_img_create(lv_scr_act());
    lv_img_set_src(bg_img, &rfid_bg);
    lv_obj_align(bg_img, LV_ALIGN_LEFT_MID, 0, 0); // Aligner à gauche au milieu

    // Créer une étiquette pour le titre
    title_label = lv_label_create(lv_scr_act());
    lv_label_set_text(title_label, "Projet Lecteur RFID");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10); // Aligner en haut au centre

    // Créer une zone de texte pour afficher l'UUID
    text_area = lv_textarea_create(lv_scr_act());
    lv_obj_set_size(text_area, 200, 50);
    lv_obj_align(text_area, LV_ALIGN_CENTER, 120, 0); // Aligner à droite

    // Créer une étiquette pour le statut (Accepted ou Rejected)
    status_label = lv_label_create(lv_scr_act());
    lv_obj_align(status_label, LV_ALIGN_CENTER, 120, 50); // Positionner sous la zone de texte
}

int main(void) {
    // Définir les propriétés souhaitées (9600-8-N-1).
    serial_port.set_baud(9600);
    serial_port.set_format(
        /* bits */ 8,
        /* parity */ BufferedSerial::None,
        /* stop bit */ 1
    );

    // Initialiser l'interface graphique
    gui_init();

    // Buffer d'application pour recevoir les données
    char buf;
    char uuid[MAXIMUM_BUFFER_SIZE] = {0};
    int index = -1;

    while (1) {
        // Lire les données du port série
        if (serial_port.read(&buf, 1)) {
            // Basculer la LED pour indiquer que des données sont reçues.
            led = !led;
            if (index >= 0) {
                if (buf == 0x03) { // Fin de la chaîne
                    printf("\n");
                    uuid[index] = '\0'; // Terminer la chaîne avec un caractère nul
                    update_text_area(uuid); // Mettre à jour l'affichage de l'UUID et le statut
                    index = -1; // Réinitialiser l'index
                } else {
                    printf("%c", buf);
                    uuid[index] = buf; // Ajouter le caractère au buffer de l'UUID
                    index++;
                }
            } else {
                if (buf == 0x02) { // Début de la chaîne
                    index = 0; // Initialiser l'index à 0
                    printf("?");
                }
            }
        }

        // Gérer les tâches LVGL
        lv_task_handler();
        ThisThread::sleep_for(5ms); // Pause pour permettre à LVGL de traiter les tâches
    }
}
