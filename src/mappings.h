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

/*
// Map value labels to human-readable display name names and units
// Label, human-readable name, unit, conversion factor (0 for non-numerical values)
const NameUnitMap mapLabelDisplaynameUnit{
    {"TIMESTAMP", {"Unix-tid", "s", 1}},
    {"ESP_UPTIME", {"Drifttid, kontroller", "s", 1}},
    {"ESP_MEM_FREE", {"Tillgängligt minne", "B", 1}},
    {"ESP_MEM_LOWEST", {"Lägsta tillgängliga minne", "B", 1}},
    {"ESP_PSRAM_FREE", {"Tillgängligt PSRAM", "B", 1}},
    {"ESP_PSRAM_LOWEST", {"Lägsta tillgängliga PSRAM", "B", 1}},
    {"ENV_REFRIG_TEMP", {"Temperatur, kyl", "°C", 1}},
    {"ENV_ROOM_TEMP", {"Temperatur rum", "°C", 1}},
    {"ENV_ROOM_HUMID", {"Luftfuktighet rum", "%RH", 1}},
    {"CTRL_INV_ON", {"Inverter påslagen", "", 0}},
    {"VE_MPPT_PPV", {"Paneleffekt", "W", 1}},
    {"VE_MPPT_VPV", {"Panelspänning", "V", 1000}},
    {"VE_MPPT_I", {"Laddström", "A", 1000}},
    {"VE_MPPT_V", {"Batterispänning (laddningsregulator)", "V", 1000}},
    {"VE_MPPT_H19", {"Total avkastning", "kWh", 100}},
    {"VE_MPPT_H20", {"Avkastning idag", "kWh", 100}},
    {"VE_MPPT_H21", {"Max. effekt idag", "W", 1}},
    {"VE_MPPT_H22", {"Avkastning igår", "kWh", 100}},
    {"VE_MPPT_H23", {"Max. effekt igår", "W", 1}},
    {"VE_MPPT_HSDS", {"Dagsnummer", "", 1}},
    {"VE_INV_V", {"Batterispänning (växelriktare)", "V", 1000}},
    {"VE_INV_AC_OUT_S", {"Effekt växelström", "VA", 1}},
    {"VE_INV_AC_OUT_I", {"Strömstyrka växelström", "A", 10}},
    {"VE_INV_AC_OUT_V", {"Spänning växelström", "V", 100}},
    {"VE_MPPT_MPPT", {"Spårningsläge", "", 0}},
    {"VE_INV_Relay", {"Reläläge (växelriktare)", "", 0}},
    {"VE_MPPT_Relay", {"Reläläge (laddningsregulator)", "", 0}},
    {"VE_MPPT_CS", {"Driftsläge (laddningsregulator)", "", 0}},
    {"VE_INV_CS", {"Driftstillstånd (växelriktare)", "", 0}},
    {"VE_MPPT_ERR", {"Felkod (laddningsregulator)", "", 0}},
    {"VE_INV_MODE", {"Driftsläge (växelriktare)", "", 0}},
    {"VE_INV_OR", {"Av-orsak  (växelriktare)", "", 0}},
    {"VE_MPPT_OR", {"Av-orsak  (laddningsregulator)", "", 0}},
    {"VE_INV_AR", {"Larmorsak (växelriktare)", "", 0}},
    {"VE_INV_WARN", {"Felorsak (växelriktare)", "", 0}},
};

// Map message codes to messages, for various labels
const CodeMap mapLabelCodeText{
    {"CTRL_INV_ON",
     {
         {0, "Av"},
         {1, "På"},
     }},
    {"VE_MPPT_Relay",
     {
         {0, "Av"},
         {1, "På"},
     }},
    {"VE_INV_Relay",
     {
         {0, "Av"},
         {1, "På"},
     }},
    {"VE_INV_AR",
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
    {"VE_MPPT_MPPT",
     {
         {0, "Av"},
         {1, "Spänning eller ström begränsad"},
         {2, "MPPT-spårare aktiv"},
     }},
    {"VE_MPPT_CS",
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
    {"VE_INV_CS",
     {
         {0, "Avstängd"},
         {1, "Söker last"},
         {2, "Fel"},
         {9, "Omvandlar"},
     }},
    {"VE_MPPT_ERR",
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
    {"VE_INV_WARN",
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
    {"VE_INV_MODE",
     {
         {2, "Omvandlar"},
         {4, "Av"},
         {5, "Eco"},
     }},
    {"VE_INV_OR",
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
     }},
    {"VE_MPPT_OR",
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
     }},
};

*/



// Refactor
// using CodeMap = std::map<String, std::map<int, String>>;

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
    {"OR", "Av-orsak  (laddningsregulator)"},
};
