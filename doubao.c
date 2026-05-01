/**********************************************************************
 * Serveur TCP SIMULATION ECU + CAN BUS
 * Fonctionnalités :
 *  - Ecoute sur port 9000 (une connexion à la fois)
 *  - Réception de données client TCP
 *  - Génération automatique de trames CAN simulées (RPM, vitesse, température)
 *  - Enregistrement TOUTES les données dans /var/tmp/ecu_can_data.log
 *  - Mode daemon (-d)
 *  - Gestion des signaux (arrêt propre)
 *  - Prêt pour présentation technique
 * 
 * Auteur : Adapté pour l'entretien chez Simo
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#include "daemon_utils.h"

// Configuration
#define PORT 9000
#define BUFFER_SIZE 1024
#define LOG_FILE "/var/tmp/ecu_can_data.log"
#define CAN_INTERVAL 1  // Génère une trame CAN toutes les 1 secondes

// Variables globales (signal handler + socket)
volatile sig_atomic_t keep_running = 1;
int server_fd = -1;
int daemon_mode = 0;

// -----------------------------------------------------------------------------
// Gestionnaire de signaux (Ctrl+C, arrêt)
// -----------------------------------------------------------------------------
void handle_signal(int sig) {   
    keep_running = 0;
    if (server_fd != -1) close(server_fd);
    syslog(LOG_INFO, "Signal reçu - Arrêt du serveur ECU/CAN");
}

// -----------------------------------------------------------------------------
// Génération d'une trame CAN SIMULÉE (format standard automobile)
// -----------------------------------------------------------------------------
void generate_can_frame(char *frame, size_t max_len) {
    // Valeurs aléatoires réalistes pour un véhicule
    int rpm = rand() % 8000 + 1000;        // 1000 - 9000 tr/min
    float speed = (rand() % 200) + 10.5f;  // 10 - 210 km/h
    float temp = (rand() % 120) + 20.0f;   // 20 - 140 °C
    float pressure = (rand() % 50) + 1.0f; // 1 - 51 bar

    // Format trame CAN : ID | DATA | TIMESTAMP
    time_t now = time(NULL);
    snprintf(frame, max_len,
        "[CAN] Timestamp:%ld | ID:0x123 | RPM:%d | Vitesse:%.1f km/h | Temp:%.1f°C | Pression:%.1f bar\n",
        now, rpm, speed, temp, pressure);
}

// -----------------------------------------------------------------------------
// Écriture sécurisée dans le fichier log (ajout sans écrasement)
// -----------------------------------------------------------------------------
void log_data(const char *data) {
    FILE *file = fopen(LOG_FILE, "a");
    if (!file) {
        perror("Erreur ouverture fichier log");
        return;
    }
    fwrite(data, 1, strlen(data), file);
    fflush(file); // Écriture immédiate sur disque
    fclose(file);
}

// -----------------------------------------------------------------------------
// Initialisation des signaux
// -----------------------------------------------------------------------------
void init_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT,  &sa, NULL);  // Ctrl+C
    sigaction(SIGTERM, &sa, NULL);  // Arrêt système
    sigaction(SIGQUIT, &sa, NULL);  // Ctrl+\
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    // Initialisation aléatoire pour les trames CAN
    srand(time(NULL));

    // Options : -d = mode daemon
    int opt;
    while ((opt = getopt(argc, argv, "d")) != -1) {
        if (opt == 'd') daemon_mode = 1;
        else {
            fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Initialisation syslog + signaux
    openlog("ECU_CAN_SERVER", LOG_PID, LOG_USER);
    init_signals();

    // Mode daemon
    if (daemon_mode) {
        if (daemon(1, 0) == -1) {
            perror("daemon");
            exit(EXIT_FAILURE);
        }
    }

    // Création socket TCP
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    // Réutilisation du port
    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    // Configuration adresse
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // Bind + Listen
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); exit(1);
    }
    if (listen(server_fd, 5) < 0) { perror("listen"); exit(1); }

    syslog(LOG_INFO, "Serveur ECU/CAN démarré sur port %d | Log: %s", PORT, LOG_FILE);
    printf("Serveur prêt | Log: %s\n", LOG_FILE);

    // -------------------------------------------------------------------------
    // BOUCLE PRINCIPALE
    // -------------------------------------------------------------------------
    while (keep_running) {
        // ----------------------------------------------------
        // 1. Génération et enregistrement TRAME CAN
        // ----------------------------------------------------
        char can_frame[256];
        generate_can_frame(can_frame, sizeof(can_frame));
        log_data(can_frame);
        printf("%s", can_frame); // Affichage console (si pas daemon)

        // ----------------------------------------------------
        // 2. Attente connexion client (timeout pour générer CAN régulièrement)
        // ----------------------------------------------------
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        // Timeout 1s : on ne bloque pas indéfiniment → génère CAN toutes les 1s
        struct timeval timeout = {CAN_INTERVAL, 0};
        int activity = select(server_fd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0 && errno != EINTR) break;
        if (activity == 0) continue; // Timeout → nouvelle boucle = nouvelle trame CAN

        // ----------------------------------------------------
        // 3. Acceptation connexion client
        // ----------------------------------------------------
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            if (errno == EINTR || !keep_running) break;
            perror("accept");
            continue;
        }

        // Info client
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Connexion client : %s", client_ip);

        // ----------------------------------------------------
        // 4. Réception données client + enregistrement
        // ----------------------------------------------------
        char buffer[BUFFER_SIZE];
        ssize_t recv_len = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

        if (recv_len > 0) {
            buffer[recv_len] = '\0';
            char log_msg[BUFFER_SIZE + 64];
            snprintf(log_msg, sizeof(log_msg), "[TCP] Client %s : %s\n", client_ip, buffer);
            log_data(log_msg);
            printf("%s", log_msg);
        }

        // ----------------------------------------------------
        // 5. Fermeture connexion client
        // ----------------------------------------------------
        close(client_fd);
        syslog(LOG_INFO, "Déconnexion client : %s", client_ip);
    }

    // -------------------------------------------------------------------------
    // NETTOYAGE FINAL
    // -------------------------------------------------------------------------
    syslog(LOG_INFO, "Arrêt complet du serveur ECU/CAN");
    close(server_fd);
    closelog();

    return 0;
}