#include "signal_utils.h"

void handle_signal(int sig) {   
    keep_running = 0; // Arrêt de la boucle principale
    const char *msg = "\nSignal reçu !\n";
    write(STDOUT_FILENO, msg, sizeof(msg)-1); // Affichage sûr dans un handler
    
    // Fermeture du socket serveur pour libérer les ressources
    if (server_fd != -1) {
        close(server_fd);
    }
}