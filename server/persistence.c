#include "persistence.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static cJSON *player_to_json(const ClientPlayer *p) {
    cJSON *obj = cJSON_CreateObject();
    if (!obj) return NULL;

    cJSON_AddStringToObject(obj, "name", p->name);

    cJSON *bio_obj = cJSON_AddObjectToObject(obj, "bio");
    if (bio_obj) {
        cJSON *lines = cJSON_AddArrayToObject(bio_obj, "lines");
        if (lines) {
            for (int j = 0; j < p->bio.lines.length; j++) {
                cJSON_AddItemToArray(lines,
                    cJSON_CreateString(p->bio.lines.data[j].value));
            }
        }
    }

    cJSON *friends = cJSON_AddArrayToObject(obj, "friends");
    if (friends) {
        for (int j = 0; j < p->friends.length; j++) {
            cJSON_AddItemToArray(friends,
                cJSON_CreateString(p->friends.data[j].value));
        }
    }

    cJSON_AddNumberToObject(obj, "private_mode", p->private_mode);
    cJSON_AddNumberToObject(obj, "match", p->match);
    cJSON_AddNumberToObject(obj, "side", p->side);

    return obj;
}

static cJSON *match_to_json(const Match *m) {
    cJSON *obj = cJSON_CreateObject();
    if (!obj) return NULL;

    cJSON_AddNumberToObject(obj, "active", m->actif);
    cJSON_AddNumberToObject(obj, "paused", m->paused);

    cJSON *names = cJSON_AddArrayToObject(obj, "player_names");
    if (names) {
        cJSON_AddItemToArray(names, cJSON_CreateString(m->noms[0]));
        cJSON_AddItemToArray(names, cJSON_CreateString(m->noms[1]));
    }

    cJSON *sides = cJSON_AddArrayToObject(obj, "side_of");
    if (sides) {
        cJSON_AddItemToArray(sides, cJSON_CreateNumber(m->side_of[0]));
        cJSON_AddItemToArray(sides, cJSON_CreateNumber(m->side_of[1]));
    }

    cJSON *game = cJSON_AddObjectToObject(obj, "game");
    if (game) {
        cJSON *plateau = cJSON_AddArrayToObject(game, "plateau");
        if (plateau) {
            for (int row = 0; row < NB_JOUEURS; row++) {
                cJSON *row_arr = cJSON_CreateArray();
                if (row_arr) {
                    for (int col = 0; col < NB_TROUS; col++) {
                        cJSON_AddItemToArray(row_arr,
                            cJSON_CreateNumber(m->g.plateau[row][col]));
                    }
                    cJSON_AddItemToArray(plateau, row_arr);
                }
            }
        }
        cJSON *grenier = cJSON_AddArrayToObject(game, "grenier");
        if (grenier) {
            cJSON_AddItemToArray(grenier, cJSON_CreateNumber(m->g.grenier[0]));
            cJSON_AddItemToArray(grenier, cJSON_CreateNumber(m->g.grenier[1]));
        }
        cJSON_AddNumberToObject(game, "joueur", m->g.joueur);
    }

    return obj;
}

static void json_to_player(cJSON *obj, ClientPlayer *p) {
    memset(p, 0, sizeof(ClientPlayer));
    vec_init(&p->bio.lines);
    vec_init(&p->friends);
    p->match = -1;
    p->side = -1;

    cJSON *name = cJSON_GetObjectItemCaseSensitive(obj, "name");
    if (name && cJSON_IsString(name)) {
        strncpy(p->name, name->valuestring, MAX_NOM - 1);
        p->name[MAX_NOM - 1] = '\0';
    }

    cJSON *bio = cJSON_GetObjectItemCaseSensitive(obj, "bio");
    if (bio && cJSON_IsObject(bio)) {
        cJSON *lines = cJSON_GetObjectItemCaseSensitive(bio, "lines");
        if (lines && cJSON_IsArray(lines)) {
            int n = cJSON_GetArraySize(lines);
            for (int j = 0; j < n; j++) {
                cJSON *line = cJSON_GetArrayItem(lines, j);
                if (line && cJSON_IsString(line)) {
                    BioLine bl;
                    strncpy(bl.value, line->valuestring, MAX_BIO_LENGTH - 1);
                    bl.value[MAX_BIO_LENGTH - 1] = '\0';
                    vec_push(&p->bio.lines, bl);
                }
            }
        }
    }

    cJSON *friends = cJSON_GetObjectItemCaseSensitive(obj, "friends");
    if (friends && cJSON_IsArray(friends)) {
        int n = cJSON_GetArraySize(friends);
        for (int j = 0; j < n; j++) {
            cJSON *f = cJSON_GetArrayItem(friends, j);
            if (f && cJSON_IsString(f)) {
                FriendName fn;
                strncpy(fn.value, f->valuestring, MAX_NOM - 1);
                fn.value[MAX_NOM - 1] = '\0';
                vec_push(&p->friends, fn);
            }
        }
    }

    cJSON *pm = cJSON_GetObjectItemCaseSensitive(obj, "private_mode");
    if (pm && cJSON_IsNumber(pm)) p->private_mode = pm->valueint;

    cJSON *match = cJSON_GetObjectItemCaseSensitive(obj, "match");
    if (match && cJSON_IsNumber(match)) p->match = match->valueint;

    cJSON *side = cJSON_GetObjectItemCaseSensitive(obj, "side");
    if (side && cJSON_IsNumber(side)) p->side = side->valueint;
}

/* ---- Désérialisation d'un objet cJSON vers un Match ---- */
static void json_to_match(cJSON *obj, Match *m) {
    memset(m, 0, sizeof(Match));
    vec_init(&m->observers);
    m->saved = 0;

    cJSON *active = cJSON_GetObjectItemCaseSensitive(obj, "active");
    if (active && cJSON_IsNumber(active)) m->actif = active->valueint;

    cJSON *paused = cJSON_GetObjectItemCaseSensitive(obj, "paused");
    if (paused && cJSON_IsNumber(paused)) m->paused = paused->valueint;

    cJSON *names = cJSON_GetObjectItemCaseSensitive(obj, "player_names");
    if (names && cJSON_IsArray(names)) {
        for (int j = 0; j < 2; j++) {
            cJSON *n = cJSON_GetArrayItem(names, j);
            if (n && cJSON_IsString(n)) {
                strncpy(m->noms[j], n->valuestring, MAX_NOM - 1);
                m->noms[j][MAX_NOM - 1] = '\0';
            }
        }
    }

    cJSON *sides = cJSON_GetObjectItemCaseSensitive(obj, "side_of");
    if (sides && cJSON_IsArray(sides)) {
        for (int j = 0; j < 2; j++) {
            cJSON *s = cJSON_GetArrayItem(sides, j);
            if (s && cJSON_IsNumber(s)) m->side_of[j] = s->valueint;
        }
    }

    cJSON *game = cJSON_GetObjectItemCaseSensitive(obj, "game");
    if (game && cJSON_IsObject(game)) {
        cJSON *plateau = cJSON_GetObjectItemCaseSensitive(game, "plateau");
        if (plateau && cJSON_IsArray(plateau)) {
            for (int row = 0; row < NB_JOUEURS; row++) {
                cJSON *row_arr = cJSON_GetArrayItem(plateau, row);
                if (row_arr && cJSON_IsArray(row_arr)) {
                    for (int col = 0; col < NB_TROUS; col++) {
                        cJSON *v = cJSON_GetArrayItem(row_arr, col);
                        if (v && cJSON_IsNumber(v))
                            m->g.plateau[row][col] = v->valueint;
                    }
                }
            }
        }
        cJSON *grenier = cJSON_GetObjectItemCaseSensitive(game, "grenier");
        if (grenier && cJSON_IsArray(grenier)) {
            for (int j = 0; j < 2; j++) {
                cJSON *v = cJSON_GetArrayItem(grenier, j);
                if (v && cJSON_IsNumber(v)) m->g.grenier[j] = v->valueint;
            }
        }
        cJSON *joueur = cJSON_GetObjectItemCaseSensitive(game, "joueur");
        if (joueur && cJSON_IsNumber(joueur)) m->g.joueur = joueur->valueint;
    }
}

void save_persistence_data(const PlayerPtrVec *players, const MatchVec *matches)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        fprintf(stderr, "Failed to create JSON root object\n");
        return;
    }

    cJSON *clients_arr = cJSON_AddArrayToObject(root, "clients");
    if (clients_arr) {
        for (int i = 0; i < players->length; i++) {
            ClientPlayer *p = players->data[i];
            if (!p || p->name[0] == '\0') continue;
            cJSON *obj = player_to_json(p);
            if (obj) cJSON_AddItemToArray(clients_arr, obj);
        }
    }

    cJSON *matches_arr = cJSON_AddArrayToObject(root, "matches");
    if (matches_arr) {
        for (int i = 0; i < matches->length; i++) {
            Match *m = &matches->data[i];
            if (!m->actif) continue;
            cJSON *obj = match_to_json(m);
            if (obj) cJSON_AddItemToArray(matches_arr, obj);
        }
    }

    /* Écriture atomique : fichier temporaire puis rename(). */
    char temp_file[256];
    snprintf(temp_file, sizeof(temp_file), "%s.tmp", DATA_FILE);

    FILE *f = fopen(temp_file, "w");
    if (f) {
        char *json_str = cJSON_Print(root);
        if (json_str) {
            fprintf(f, "%s\n", json_str);
            free(json_str);
        }
        fclose(f);
        rename(temp_file, DATA_FILE);
    } else {
        f = fopen(DATA_FILE, "w");
        if (f) {
            char *json_str = cJSON_Print(root);
            if (json_str) {
                fprintf(f, "%s\n", json_str);
                free(json_str);
            }
            fclose(f);
        }
    }

    cJSON_Delete(root);
}

void load_persistence_data(PlayerPtrVec *players, MatchVec *matches)
{
    /* Nettoie les fichiers temporaires d'un éventuel crash précédent. */
    char temp_file[256];
    snprintf(temp_file, sizeof(temp_file), "%s.tmp", DATA_FILE);
    remove(temp_file);

    FILE *f = fopen(DATA_FILE, "r");
    if (!f) return;

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (file_size <= 0) { fclose(f); return; }

    char *buffer = (char *)malloc((size_t)file_size + 1);
    if (!buffer) { fclose(f); return; }

    size_t bytes_read = fread(buffer, 1, (size_t)file_size, f);
    buffer[bytes_read] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) return;

    /* Charge les joueurs : alloue chaque ClientPlayer sur le tas et pousse
     * le pointeur dans le vec (pointeurs stables malgré les realloc). */
    cJSON *clients_arr = cJSON_GetObjectItemCaseSensitive(root, "clients");
    if (clients_arr && cJSON_IsArray(clients_arr)) {
        int n = cJSON_GetArraySize(clients_arr);
        for (int i = 0; i < n && players->length < MAX_CLIENTS; i++) {
            cJSON *obj = cJSON_GetArrayItem(clients_arr, i);
            if (!obj || !cJSON_IsObject(obj)) continue;
            ClientPlayer *p = calloc(1, sizeof(ClientPlayer));
            if (!p) continue;
            json_to_player(obj, p);
            vec_push(players, p);
        }
    }

    /* Charge les matchs : pousse chaque Match dans le vec. */
    cJSON *matches_arr = cJSON_GetObjectItemCaseSensitive(root, "matches");
    if (matches_arr && cJSON_IsArray(matches_arr)) {
        int n = cJSON_GetArraySize(matches_arr);
        for (int i = 0; i < n; i++) {
            cJSON *obj = cJSON_GetArrayItem(matches_arr, i);
            if (!obj || !cJSON_IsObject(obj)) continue;
            Match m;
            json_to_match(obj, &m);
            vec_push(matches, m);
        }
    }

    cJSON_Delete(root);
}

ClientPlayer *find_persistent_player(const PlayerPtrVec *players, const char *name)
{
    for (int i = 0; i < players->length; i++) {
        ClientPlayer *p = players->data[i];
        if (p && p->name[0] != '\0' && strcmp(p->name, name) == 0) {
            return p;
        }
    }
    return NULL;
}
