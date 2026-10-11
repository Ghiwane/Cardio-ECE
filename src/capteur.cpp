#include "capteur.h"
#include "config.h"

#include <Arduino.h>

const float PPG_TAU_MOY_MS  = 1000.0;   // constante de temps de la moyenne (tendance)
const float PPG_TAU_AMP_MS  = 5000.0;   // "oubli" de l'amplitude max
const float PPG_AMP_MIN     = 10.0;     // evite d'amplifier le bruit sans doigt
const float PPG_SEUIL_HAUT  = 0.5;      // detection d'un battement
const float PPG_SEUIL_BAS   = 0.2;      // rearmement (hysteresis)
const unsigned long PPG_REFRACT_MS = 300;    // pas deux battements a moins de 300 ms
const unsigned long PPG_TIMEOUT_MS = 3000;   // aucun battement depuis 3 s -> invalide
const unsigned long PPG_STABIL_MS  = 2000;   // on attend 2 s avant de detecter
const int   PPG_NB_INTERV   = 5;        // moyenne sur 5 intervalles
const int   PPG_ECRAN_CENTRE = 512;     // norm = 0  -> milieu de l'ecran
const int   PPG_ECRAN_AMPL   = 400;     // norm = 1  -> +400 sur l'echelle 0..1023
const float PPG_AMP_DOIGT = 25.0; 

// Variables lisibles par le reste du projet 
float ppgBpm = 0;            // 0 si la mesure n'est pas fiable (-> tiret a l'ecran)
bool  ppgBpmValide = false;
float ppgNorm = 0;           // signal recentre et normalise (environ -1..1)

// Variables internes 
bool  ppgPret = false, ppgArme = true;
float ppgMoy = 0, ppgAmp = PPG_AMP_MIN, ppgNormPrec = 0;
unsigned long ppgPrecMs = 0, ppgDebutMs = 0, ppgDernierBat = 0;
unsigned long ppgIntervalles[PPG_NB_INTERV];
int   ppgNbInterv = 0, ppgIdxInterv = 0, ppgRejets = 0;

float ppgMoyenneIntervalles() {
  unsigned long s = 0;
  for (int i = 0; i < ppgNbInterv; i++) s += ppgIntervalles[i];
  return (float)s / ppgNbInterv;
}

// Lit le capteur, traite le signal, renvoie 0..1023 pour l'ecran
int ppgTraiter() {
  unsigned long now = millis();
  long sommeBrut = 0;
  for (int i = 0; i < 8; i++) sommeBrut += analogRead(PIN_CAPTEUR);   // 8 lectures moyennees (~1 ms)
  int brut = sommeBrut / 8;

  if (!ppgPret) {  // premiere lecture : initialisation
    ppgMoy = brut;
    ppgPrecMs = now;
    ppgDebutMs = now;
    ppgPret = true;
    return PPG_ECRAN_CENTRE;
  }

  unsigned long dtL = now - ppgPrecMs;
  if (dtL < 1) dtL = 1;
  float dt = (float)dtL;
  unsigned long tPrec = ppgPrecMs;
  ppgPrecMs = now;

  // 1) Moyenne exponentielle = tendance (derive + lumiere ambiante)
  ppgMoy += (dt / (PPG_TAU_MOY_MS + dt)) * (brut - ppgMoy);

  // 2) Signal recentre
  float centre = brut - ppgMoy;

  // 3) Normalisation par l'amplitude recente (robustesse a la luminosite, ET1.3)
  float a = fabs(centre);
  if (a > ppgAmp) ppgAmp = a;
  else ppgAmp -= ppgAmp * (dt / PPG_TAU_AMP_MS);
  if (ppgAmp < PPG_AMP_MIN) ppgAmp = PPG_AMP_MIN;
  ppgNorm = centre / ppgAmp;

  // 4) Detection : passage du seuil vers le haut, avec hysteresis
  if (now - ppgDebutMs > PPG_STABIL_MS) {
    if (ppgArme && ppgAmp >= PPG_AMP_DOIGT && ppgNormPrec <= PPG_SEUIL_HAUT && ppgNorm > PPG_SEUIL_HAUT) {
      // instant exact du passage, interpole entre les deux echantillons
      float frac = (PPG_SEUIL_HAUT - ppgNormPrec) / (ppgNorm - ppgNormPrec);
      unsigned long tBat = tPrec + (unsigned long)(frac * dt);

      if (ppgDernierBat == 0 || (tBat - ppgDernierBat) > PPG_REFRACT_MS) {
        ppgArme = false;
        if (ppgDernierBat != 0) {
          unsigned long iv = tBat - ppgDernierBat;
          bool ok = (iv <= PPG_TIMEOUT_MS);
          if (ok && ppgNbInterv >= 3) {            // rejet des intervalles incoherents
            float m = ppgMoyenneIntervalles();
            if (iv < 0.6 * m || iv > 1.6 * m) ok = false;
          }
          if (ok) {
            ppgIntervalles[ppgIdxInterv] = iv;
            ppgIdxInterv = (ppgIdxInterv + 1) % PPG_NB_INTERV;
            if (ppgNbInterv < PPG_NB_INTERV) ppgNbInterv++;
            ppgRejets = 0;
          } else if (++ppgRejets >= 3) {           // rythme vraiment change : on repart de zero
            ppgNbInterv = 0;
            ppgRejets = 0;
          }
        }
        ppgDernierBat = tBat;
      }
    }
    if (ppgNorm < PPG_SEUIL_BAS) ppgArme = true;

    // 5) bpm = 60000 / intervalle moyen (ms)
    float b = 0;
    if (ppgNbInterv >= 3) b = 60000.0 / ppgMoyenneIntervalles();

    // Plus de battement : doigt retire ou signal perdu
    if (ppgDernierBat != 0 && (now - ppgDernierBat) > PPG_TIMEOUT_MS) {
      ppgNbInterv = 0;
      b = 0;
    }

    ppgBpmValide = (ppgNbInterv >= 3) && (b >= BPM_MIN) && (b <= BPM_MAX) && (ppgAmp >= PPG_AMP_DOIGT);
    ppgBpm = ppgBpmValide ? b : 0;
  }
  ppgNormPrec = ppgNorm;

  // 6) Mise a l'echelle 0..1023 pour afficherEcran2()
  int v = PPG_ECRAN_CENTRE + (int)(ppgNorm * PPG_ECRAN_AMPL);
  return constrain(v, 0, 1023);
}
