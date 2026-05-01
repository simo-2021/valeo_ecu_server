# Connecto to my original repository
git remote add origin git@github.com:simo-2021/valeo_ecu_server.git

#set up the README.md file & repository
echo "# valeo_ecu_server" >> README.md
git init
git add README.md
git commit -m "first commit"
git branch -M main
git remote add origin git@github.com:simo-2021/valeo_ecu_server.git
git push -u origin main


#Yocto Installation
# Packages à installer pour Yocto
sudo apt-get install build-essential chrpath cpio debianutils diffstat file gawk gcc git iputils-ping libacl1 locales python3 python3-git python3-jinja2 python3-pexpect python3-pip python3-subunit socat texinfo unzip wget xz-utils zstd
 
 locale --all-locales | grep en_US.utf8 #Vérifier que la locale en_US.UTF-8 est disponible sur votre système. Si elle n'est pas listée, vous devrez peut-être l'installer ou la générer.
 
 
 # use git to clone the poky repository (Yocto Project reference distribution) and set up the build environment:
 # bitbake sert à construire les images et les paquets à partir des recettes Yocto. Il gère les dépendances, les configurations et les processus de construction pour créer des systèmes embarqués personnalisés.   
 git clone https://git.openembedded.org/bitbake
 
 # set up build environment
 ./bitbake/bin/bitbake-setup init
 
# clone poky repository with the kirkstone branch (yocto project reference distribution for the kirkstone release)
#take too long to clone the poky repository
git clone -b kirkstone git://git.yoctoproject.org/poky


 # Init build environment
 source oe-init-build-env     

# stop
# Build the minimal image (this will take a while)
bitbake core-image-minimal


 
 # Examine current build configuration
 bitbake-config-build list-fragments
 
 # fragments activations
 bitbake-config-build enable-fragment machine/qemux86-64 // Activer la configuration spécifique à la machine qemux86-64
 bitbake-config-build enable-fragment distro/poky // Activer la configuration spécifique à la distribution Poky
 
 # Activer la configuration pour permettre les connexions root sans mot de passe (utile pour les tests et le développement, mais à éviter en production)
bitbake-config-build enable-fragment core/yocto/root-login-with-empty-password

# Activer la configuration pour utiliser un miroir de cache partagé pour les artefacts de construction (améliore les performances en évitant de reconstruire les mêmes artefacts à plusieurs reprises)
bitbake-config-build enable-fragment core/yocto/sstate-mirror-cdn
 
# ajouter le sous-module Kirstone
git submodule add -b kirkstone https://yoctoproject.org

# Valider les configurations et les changements dans votre dépôt Git
# On s'assure que l'index de Git est à jour pour les deux sous-modules
git add poky
git add buildroot  # C'est ici que vous validez le hash spécifique à buildroot
git add .gitmodules

# On enregistre le tout
git commit -m "Add Poky Kirkstone submodule and update buildroot reference"
git push origin main

 