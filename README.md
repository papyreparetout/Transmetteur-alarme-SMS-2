# Transmetteur-alarme-SMS-2
Transmission des messages et alarmes de l'alarme ARITECH CD 34 via SMS, version 2 à base d'un module TTGO T Call comportant un ESP32 et un SIM800 intégré.

Le module ESP32 TTGO est raccordé sur le bus clavier de l'alarme ARITECH CD34 il écoute les messages envoyés sur la liaison série entre le boitier de l'alarme et les claviers et envoie un SMS lorsqu'il détecte le bit correspondant à l'allumage de la LED orange (défaut alarme) ou de la LED rouge (Alarme) puis au retour à la normale. Le SMS indique la raison et donne le dernier message affiché sur le clavier (pas toujours OK pour l'alarme).
Une interface Web permet de changer le numéro de téléphone destinataire de l'alarme (à vérifier).

Le code C+ utilise la bibliothèque FONA de Adafruit et est basé sur le code développé par Ozmo: https://www.instructables.com/House-Alarm-Internet-Dialer-With-Arduino-Reverse-E/
