#include "horloge.h"

// ===== MODULE HORLOGE DS1302 (FS2) - version projet final =====
// Fonctions a utiliser dans le reste du projet :
//   horlogeInit()            -> a appeler dans setup()
//   horlogeMettreAJour()     -> a appeler dans loop() (lit le module toutes les 200 ms)
//   horlogeTexte(buf)        -> remplit buf avec "HH:MM:SS" (buf de 9 caracteres minimum)
//   horlogeRegler(h, mi, s)  -> regle l'heure
//   horlogeCommandeSerie()   -> reglage par le moniteur serie : T14:32:00 (optionnel)
// Variables lisibles : heureH, heureM, heureS

// ---------- Broches (a changer si elles servent a autre chose) ----------
const int PIN_CLK = 7;
const int PIN_DAT = 8;
const int PIN_RST = 9;   // CE du DS1302

// ---------- Registres du DS1302 ----------
const byte REG_SEC     = 0x80;
const byte REG_MIN     = 0x82;
const byte REG_HEURE   = 0x84;
const byte REG_PROTECT = 0x8E;

// ---------- Heure courante ----------
byte heureH = 0, heureM = 0, heureS = 0;
unsigned long derniereLecture = 0;
const unsigned long PERIODE_LECTURE_MS = 200;

// ---------- Utilitaires ----------
byte decVersBcd(byte d) { return ((d / 10) << 4) | (d % 10); }
byte bcdVersDec(byte b) { return ((b >> 4) * 10) + (b & 0x0F); }

// ---------- Communication 3 fils avec le DS1302 ----------
void ecrireOctet(byte b) {
  pinMode(PIN_DAT, OUTPUT);
  for (int i = 0; i < 8; i++) {
    digitalWrite(PIN_DAT, (b >> i) & 1);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_CLK, LOW);
    delayMicroseconds(2);
  }
}

byte lireOctet() {
  pinMode(PIN_DAT, INPUT);
  byte v = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(PIN_DAT)) v |= (1 << i);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_CLK, LOW);
    delayMicroseconds(2);
  }
  return v;
}

void ecrireRegistre(byte reg, byte val) {
  digitalWrite(PIN_CLK, LOW);
  digitalWrite(PIN_RST, HIGH);
  ecrireOctet(reg);
  ecrireOctet(val);
  digitalWrite(PIN_RST, LOW);
}

byte lireRegistre(byte reg) {
  digitalWrite(PIN_CLK, LOW);
  digitalWrite(PIN_RST, HIGH);
  ecrireOctet(reg | 0x01);   // bit0 = 1 -> lecture
  byte v = lireOctet();
  digitalWrite(PIN_RST, LOW);
  return v;
}

// ---------- Fonctions du module ----------
void horlogeRegler(byte h, byte mi, byte s) {
  ecrireRegistre(REG_PROTECT, 0x00);          // desactive la protection en ecriture
  ecrireRegistre(REG_SEC,   decVersBcd(s));   // bit7 (CH) = 0 -> l'horloge tourne
  ecrireRegistre(REG_MIN,   decVersBcd(mi));
  ecrireRegistre(REG_HEURE, decVersBcd(h));   // mode 24 h
  ecrireRegistre(REG_PROTECT, 0x80);
  heureH = h; heureM = mi; heureS = s;
}

void horlogeInit() {
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);
  digitalWrite(PIN_CLK, LOW);

  // Module arrete (CH = 1) : reglage avec l'heure de compilation, une seule fois.
  // Sinon on n'y touche pas : l'heure est conservee apres une coupure (ET2.3).
  if (lireRegistre(REG_SEC) & 0x80) {
    byte h  = (__TIME__[0] - '0') * 10 + (__TIME__[1] - '0');
    byte mi = (__TIME__[3] - '0') * 10 + (__TIME__[4] - '0');
    byte s  = (__TIME__[6] - '0') * 10 + (__TIME__[7] - '0');
    horlogeRegler(h, mi, s);
  }
  derniereLecture = millis() - PERIODE_LECTURE_MS;   // force une lecture immediate
}

void horlogeMettreAJour() {
  unsigned long now = millis();
  if (now - derniereLecture < PERIODE_LECTURE_MS) return;
  derniereLecture = now;

  byte s1, s2, mi, h;
  do {                                    // relit si les secondes ont change pendant la lecture
    s1 = lireRegistre(REG_SEC);
    mi = lireRegistre(REG_MIN);
    h  = lireRegistre(REG_HEURE);
    s2 = lireRegistre(REG_SEC);
  } while (s1 != s2);

  heureS = bcdVersDec(s1 & 0x7F);
  heureM = bcdVersDec(mi & 0x7F);
  heureH = bcdVersDec(h  & 0x3F);
}

void horlogeTexte(char* buf) {            // buf : au moins 9 caracteres
  buf[0] = '0' + heureH / 10; buf[1] = '0' + heureH % 10; buf[2] = ':';
  buf[3] = '0' + heureM / 10; buf[4] = '0' + heureM % 10; buf[5] = ':';
  buf[6] = '0' + heureS / 10; buf[7] = '0' + heureS % 10; buf[8] = '\0';
}

// Reglage optionnel par le moniteur serie : T14:32:00 (115200 bauds, "Nouvelle ligne")
void horlogeCommandeSerie() {
  if (!Serial.available()) return;
  String ligne = Serial.readStringUntil('\n');
  ligne.trim();
  if (ligne.length() >= 9 && ligne[0] == 'T') {
    byte h  = ligne.substring(1, 3).toInt();
    byte mi = ligne.substring(4, 6).toInt();
    byte s  = ligne.substring(7, 9).toInt();
    if (h < 24 && mi < 60 && s < 60) {
      horlogeRegler(h, mi, s);
      Serial.println("Heure reglee");
    }
  }
}


