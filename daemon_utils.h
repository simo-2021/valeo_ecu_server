// daemon_utils.h - Bibliothèque pour créer un daemon sous Linux
#ifndef DAEMON_UTILS_H
#define DAEMON_UTILS_H

// Inclure les en-têtes nécessaires
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * @brief Convertit le processus courant en daemon (détaché du terminal)
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
static inline int become_daemon(void) {
    pid_t pid;

    // Étape 1 : Créer un processus fils et quitter le parent
    pid = fork();
    if (pid < 0) {
        perror("fork échoué (daemon)");
        return -1;
    }
    if (pid > 0) {
        // Quitter le processus parent (daemon continue en arrière-plan)
        exit(EXIT_SUCCESS);
    }

    // Étape 2 : Créer une nouvelle session (détacher du terminal)
    if (setsid() < 0) {
        perror("setsid échoué (daemon)");
        return -1;
    }

    // Étape 3 : Fork à nouveau pour éviter de devenir leader de session
    pid = fork();
    if (pid < 0) {
        perror("fork 2 échoué (daemon)");
        return -1;
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Étape 4 : Changer le répertoire de travail (éviter de bloquer des disques)
    if (chdir("/") < 0) {
        perror("chdir échoué (daemon)");
        return -1;
    }

    // Étape 5 : Réinitialiser les permissions de fichiers
    umask(0);

    // Étape 6 : Fermer tous les descripteurs de fichiers (stdin/stdout/stderr)
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Rediriger stdin/stdout/stderr vers /dev/null (évite des erreurs)
    open("/dev/null", O_RDONLY);  // stdin (fd 0)
    open("/dev/null", O_WRONLY); // stdout (fd 1)
    open("/dev/null", O_WRONLY); // stderr (fd 2)

    return 0;
}

#endif // DAEMON_UTILS_H
