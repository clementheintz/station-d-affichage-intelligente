# station d'affichage intelligente

Un affichage intelligent pour bureau connecté et autonome, propulsé par un **ESP32-S3-N16R8** et un écran **E-Paper Waveshare de 4.2 pouces** (monochrome Noir & Blanc). Le tout est intégré dans un boîtier sur mesure imprimé en 3D couleur or.

Le système synchronise automatiquement et sans aucun bouton physique vos agendas (**Apple iCloud & Pronote**) ainsi que vos tâches (**Google Tasks via iOS Reminders**), avec un rafraîchissement optimisé basé sur un système de détection de changement par horodatage.

---

## 🚀 Fonctionnalités principales

- **Agenda Combiné :** Récupération et fusion en temps réel des cours (Pronote) et des événements personnels (iCloud) avec conversion automatique des fuseaux horaires (UTC vers Paris) et tri chronologique de la journée.
  
- **Rappels Synchronisés :** Affichage des tâches en cours (Google Tasks) synchronisées instantanément via un raccourci iOS relié à l'application *Rappels* d'Apple.
  
- **Mise à jour Intelligente (Polling) :** L'ESP32 interroge l'API toutes les 15 secondes en comparant un simple horodatage (`lastUpdate`), déclenchant un rafraîchissement complet de l'écran e-paper uniquement en cas de modification des données.
  
- **Horloge Live :** Rafraîchissement partiel de l'heure et de la date chaque minute pour garder une horloge fluide sans artéfacts visuels ni clignotement gênant.
  
- **Design & Matériel :** Boîtier 3D ergonomique avec passage de câble USB-C optimisé, support de rail mural et composants câblés proprement, uniquement en impression 3D.

---

## 🛠️ Schéma de câblage (ESP32-S3 & Waveshare 4.2")

| Fil (Couleur) | Écran E-Paper | Broche ESP32-S3 | Rôle / Description |
| :--- | :--- | :--- | :--- |
| 🔘 **gris** | VCC | 3V3 | Alimentation 3,3 V |
| 🟤 **marron** | GND | GND | Masse |
| 🔵 **bleu** | DIN | GPIO 11 | Données SPI (MOSI) |
| 🟡 **jaune** | CLK | GPIO 12 | Horloge SPI |
| 🟠 **orange** | CS | GPIO 5 | Sélection de l'écran (Chip Select) |
| 🟢 **vert** | DC | GPIO 17 | Données / Commande |
| ⚪ **blanc** | RST | GPIO 16 | Réinitialisation (Reset) |
| 🟣 **violet** | BUSY | GPIO 4 | Signal d'état (Écran occupé) |

---

Configuration & Installation
1. Backend (Google Apps Script)
Créez un nouveau projet sur Google Apps Script.

Collez le code du fichier backend/Code.gs (en y insérant vos liens iCal iCloud/Pronote et en activant le service Google Tasks).

Déployez le script en tant qu'Application Web (accès autorisé pour Tout le monde).

2. Firmware (ESP32-S3)
Ouvrez le projet dans PlatformIO ou l'IDE Arduino.

Installez les bibliothèques requises : GxEPD2, ArduinoJson, HTTPClient.

Configurez vos identifiants Wi-Fi et l'URL de votre application web Google Apps Script dans src/main.cpp :

<<

#define WIFI_SSID "VOTRE_WIFI"

#define WIFI_PASSWORD "VOTRE_MOT_DE_PASSE"

#define SCRIPT_URL "VOTRE_URL_GOOGLE_APPS_SCRIPT_EXEC"

>>

Téléversez le code sur votre carte ESP32-S3.

## 📂 Structure du projet

```text
├── firmware/
│   └── main.cpp             # Code C++ (PlatformIO / Arduino) pour l'ESP32-S3
├── backend/
│   └── Code.gs              # Script Google Apps Script (Parser iCal & Google Tasks)
├── cad/
│   └── boitier_calendrier.scad # Fichier OpenSCAD du boîtier 3D doré
└── index.html               # Page web du portfolio interactif & documentation
