/**
 * @file mappings.h
 * @brief Map value labels and message codes to human-readable information
 * @version 0.1
 * @date 2025-11-25
 *
 *
 */
#pragma once
#include <Arduino.h>
#include <map>
#include "maputils.h"


const std::map<String, int> inverterConversions{
    {"V", 1000},       // Batterispänning (växelriktare), V
    {"AC_OUT_S", 1},   // Effekt växelström, VA
    {"AC_OUT_I", 10},  // Strömstyrka växelström, A
    {"AC_OUT_V", 100}, // Spänning växelström, V
};

const CodeMap inverterCodes{
    {"Relay",
     {
         {0, "Av"},
         {1, "På"},
     }},
    {"AR",
     {
         {0, "Inget larm"},
         {1, "Låg spänning"},
         {2, "Hög spänning"},
         {32, "Låg temperatur"},
         {64, "Hög temperatur"},
         {256, "Överbelastning"},
         {512, "DC-rippel"},
         {1024, "Låg V AC ut"},
         {2048, "Hög V AC ut"},
     }},
    {"CS",
     {
         {0, "Avstängd"},
         {1, "Söker last"},
         {2, "Fel"},
         {9, "Omvandlar"},
     }},
    {"WARN",
     {
         {0, "Inget fel"},
         {2, "Batterispänning för hög"},
         {17, "Laddaren för varm"},
         {18, "Överström i laddaren"},
         {19, "Laddström omvänd"},
         {20, "Bulk-tidsgräns nådd"},
         {21, "Strömsensor defekt"},
         {26, "Terminaler överhettade"},
         {28, "Problem med omvandlaren"},
         {33, "Ingående spänning för hög"},
         {34, "Ingående ström för hög"},
         {38, "Ingående ström avstängd p.g.a. hög batterispänning"},
         {39, "Ingående ström avstängd p.g.a. ström under avstängt läge"},
         {65, "Tappat kontakten med någon enhet"},
         {66, "Problem med synkroniserade laddningsenhet"},
         {67, "Tappat kontakt med BMS"},
         {68, "Nätverk felkonfigurerat"},
         {116, "Förlorat fabrikskonfiguration"},
         {117, "Fel i den fasta programvaran"},
         {119, "Fel i instälningar"},
     }},
    {"MODE",
     {
         {2, "Omvandlar"},
         {4, "Av"},
         {5, "Eco"},
     }},
    {"OR",
     {
         {0x00000000, "Inte avstängd"},
         {0x00000001, "Ingen inkommande ström"},
         {0x00000002, "Avstängd (brytare)"},
         {0x00000004, "Avstängd (lägesinställning)"},
         {0x00000008, "Fjärrkontroll"},
         {0x00000010, "Skydd aktiverat"},
         {0x00000020, "Paygo"},
         {0x00000040, "BMS"},
         {0x00000080, "Motor avstängd"},
         {0x00000100, "Analyserar inkommande spänning"},
     }}};

const std::map<String, String> inverterDisplayNames{
    {"V", "Batterispänning (växelriktare)"},
    {"AC_OUT_S", "Effekt växelström"},
    {"AC_OUT_I", "Strömstyrka växelström"},
    {"AC_OUT_V", "Spänning växelström"},
    {"Relay", "Reläläge (växelriktare)"},
    {"CS", "Driftstillstånd (växelriktare)"},
    {"MODE", "Driftsläge (växelriktare)"},
    {"OR", "Av-orsak  (växelriktare)"},
    {"AR", "Larmorsak (växelriktare)"},
    {"WARN", "Felorsak (växelriktare)"},
    {"PID", "Produkt-ID"}
};

const std::map<String, int> mpptConversions{
    {"PPV", 1},    // Paneleffekt, W
    {"VPV", 1000}, // Panelspänning, V
    {"I", 1000},   // Laddström, A
    {"V", 1000},   // Batterispänning (laddningsregulator), V
    {"H19", 100},  // Total avkastning, kWh
    {"H20", 100},  // Avkastning idag, kWh
    {"H21", 1},    // Max. effekt idag, W
    {"H22", 100},  // Avkastning igår, kWh
    {"H23", 1},    // Max. effekt igår, W
    {"HSDS", 1},   // Dagsnummer (Måndag = 1)
};

const CodeMap mpptCodes{
    {"Relay",
     {
         {0, "Av"},
         {1, "På"},
     }},
    {"AR",
     {
         {0, "Inget larm"},
         {1, "Låg spänning"},
         {2, "Hög spänning"},
         {32, "Låg temperatur"},
         {64, "Hög temperatur"},
         {256, "Överbelastning"},
         {512, "DC-rippel"},
         {1024, "Låg V AC ut"},
         {2048, "Hög V AC ut"},
     }},
    {"MPPT",
     {
         {0, "Av"},
         {1, "Spänning eller ström begränsad"},
         {2, "MPPT-spårare aktiv"},
     }},
    {"CS",
     {
         {0, "Avstängd"},
         {2, "Fel"},
         {3, "Bulk"},
         {4, "Absorbtion"},
         {5, "Float"},
         {7, "Utjämna (manuell)"},
         {254, "Startar"},
         {247, "Auto-utjämning"},
         {252, "Extern styrning"},
     }},
    {"ERR",
     {
         {0, "Inget fel"},
         {2, "Batterispänning för hög"},
         {17, "Laddaren för varm"},
         {18, "Överström i laddaren"},
         {19, "Laddström omvänd"},
         {20, "Bulk-tidsgräns nådd"},
         {21, "Strömsensor defekt"},
         {26, "Terminaler överhettade"},
         {28, "Problem med omvandlaren"},
         {33, "Ingående spänning för hög"},
         {34, "Ingående ström för hög"},
         {38, "Ingående ström avstängd p.g.a. hög batterispänning"},
         {39, "Ingående ström avstängd p.g.a. ström under avstängt läge"},
         {65, "Tappat kontakten med någon enhet"},
         {66, "Problem med synkroniserade laddningsenhet"},
         {67, "Tappat kontakt med BMS"},
         {68, "Nätverk felkonfigurerat"},
         {116, "Förlorat fabrikskonfiguration"},
         {117, "Fel i den fasta programvaran"},
         {119, "Fel i instälningar"},
     }},
    {"OR",
     {
         {0x00000000, "Inte avstängd"},
         {0x00000001, "Ingen inkommande ström"},
         {0x00000002, "Avstängd (brytare)"},
         {0x00000004, "Avstängd (lägesinställning)"},
         {0x00000008, "Fjärrkontroll"},
         {0x00000010, "Skydd aktiverat"},
         {0x00000020, "Paygo"},
         {0x00000040, "BMS"},
         {0x00000080, "Motor avstängd"},
         {0x00000100, "Analyserar inkommande spänning"},
     }}

};

const std::map<String, String> mpptDisplayNames{
    {"PPV", "Paneleffekt"},
    {"VPV", "Panelspänning"},
    {"I", "Laddström"},
    {"V", "Batterispänning (laddningsregulator)"},
    {"H19", "Total avkastning"},
    {"H20", "Avkastning idag"},
    {"H21", "Max. effekt idag"},
    {"H22", "Avkastning igår"},
    {"H23", "Max. effekt igår"},
    {"HSDS", "Dagsnummer"},
    {"MPPT", "Spårningsläge"},
    {"Relay", "Reläläge (laddningsregulator)"},
    {"CS", "Driftsläge (laddningsregulator)"},
    {"ERR", "Felkod (laddningsregulator)"},
    {"OR", "Av-orsak (laddningsregulator)"},
    {"PID", "Produkt-ID"}};
