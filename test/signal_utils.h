


void handle_signal(int sig) {   
    keep_running = 0; // On demande à la boucle de s'arrêter
    const char *msg = "\nSignal reçu !\n";
    write(STDOUT_FILENO, msg, sizeof(msg)-1); // Affiche un message simple pour indiquer que le signal a été reçu (sans utiliser printf qui n'est pas sûr dans les handlers de signal)      
    if (server_fd != -1) {
        //shutdown(server_fd, SHUT_RDWR); // Réveille accept() immédiatement
        close(server_fd); // Ferme le socket du serveur pour libérer la ressource et faire échouer les futurs accept()
    }
    
}