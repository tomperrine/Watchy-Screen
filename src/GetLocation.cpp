#include "GetLocation.h"

#include <Arduino_JSON.h>
#include <IPAddress.h>

#include "Watchy.h"  // for connectWiFi
#include "WatchyErrors.h"

namespace Watchy_GetLocation {

constexpr const float DEFAULT_LOCATION_LATITUDE = -37.8136;
constexpr const float DEFAULT_LOCATION_LONGDITUDE = 144.9631;
const int LOCATION_UPDATE_INTERVAL = 5 * 60 * 1000;  // 5 minutes in millis

const char *TZDB_UDP_HOST = "timezoned.rop.nl";
const int TZDB_UDP_PORT = 2342;

RTC_DATA_ATTR time_t lastGetLocationTS = 0;  // can use this to throttle
RTC_DATA_ATTR location currentLocation = {
    DEFAULT_LOCATION_LATITUDE,       // lat
    DEFAULT_LOCATION_LONGDITUDE,     // lon
    DEFAULT_TIMEZONE,  // timezone
    "Melbourne"                      // default location is in Melbourne
};

// built from tzdb version 2021a

#include <string.h>

const char *posix[] = {
    /*   0 */ "GMT0",
    /*   1 */ "EAT-3",
    /*   2 */ "CET-1",
    /*   3 */ "WAT-1",
    /*   4 */ "CAT-2",
    /*   5 */ "EET-2",
    /*   6 */ "<+01>-1",
    /*   7 */ "CET-1CEST,M3.5.0,M10.5.0/3",
    /*   8 */ "SAST-2",
    /*   9 */ "HST10HDT,M3.2.0,M11.1.0",
    /*  10 */ "AKST9AKDT,M3.2.0,M11.1.0",
    /*  11 */ "AST4",
    /*  12 */ "<-03>3",
    /*  13 */ "<-04>4<-03>,M10.1.0/0,M3.4.0/0",
    /*  14 */ "EST5",
    /*  15 */ "CST6CDT,M4.1.0,M10.5.0",
    /*  16 */ "CST6",
    /*  17 */ "<-04>4",
    /*  18 */ "<-05>5",
    /*  19 */ "MST7MDT,M3.2.0,M11.1.0",
    /*  20 */ "CST6CDT,M3.2.0,M11.1.0",
    /*  21 */ "MST7MDT,M4.1.0,M10.5.0",
    /*  22 */ "MST7",
    /*  23 */ "EST5EDT,M3.2.0,M11.1.0",
    /*  24 */ "PST8PDT,M3.2.0,M11.1.0",
    /*  25 */ "AST4ADT,M3.2.0,M11.1.0",
    /*  26 */ "<-03>3<-02>,M3.5.0/-2,M10.5.0/-1",
    /*  27 */ "CST5CDT,M3.2.0/0,M11.1.0/1",
    /*  28 */ "<-03>3<-02>,M3.2.0,M11.1.0",
    /*  29 */ "<-02>2",
    /*  30 */ "<-04>4<-03>,M9.1.6/24,M4.1.6/24",
    /*  31 */ "<-01>1<+00>,M3.5.0/0,M10.5.0/1",
    /*  32 */ "NST3:30NDT,M3.2.0,M11.1.0",
    /*  33 */ "<+11>-11",
    /*  34 */ "<+07>-7",
    /*  35 */ "<+10>-10",
    /*  36 */ "AEST-10AEDT,M10.1.0,M4.1.0/3",
    /*  37 */ "<+05>-5",
    /*  38 */ "NZST-12NZDT,M9.5.0,M4.1.0/3",
    /*  39 */ "<+03>-3",
    /*  40 */ "<+00>0<+02>-2,M3.5.0/1,M10.5.0/3",
    /*  41 */ "<+06>-6",
    /*  42 */ "EET-2EEST,M3.5.4/24,M10.5.5/1",
    /*  43 */ "<+12>-12",
    /*  44 */ "<+04>-4",
    /*  45 */ "EET-2EEST,M3.5.0/0,M10.5.0/0",
    /*  46 */ "<+08>-8",
    /*  47 */ "IST-5:30",
    /*  48 */ "<+09>-9",
    /*  49 */ "CST-8",
    /*  50 */ "<+0530>-5:30",
    /*  51 */ "EET-2EEST,M3.5.5/0,M10.5.5/0",
    /*  52 */ "EET-2EEST,M3.5.0/3,M10.5.0/4",
    /*  53 */ "EET-2EEST,M3.4.4/48,M10.4.4/49",
    /*  54 */ "HKT-8",
    /*  55 */ "WIB-7",
    /*  56 */ "WIT-9",
    /*  57 */ "IST-2IDT,M3.4.4/26,M10.5.0",
    /*  58 */ "<+0430>-4:30",
    /*  59 */ "PKT-5",
    /*  60 */ "<+0545>-5:45",
    /*  61 */ "WITA-8",
    /*  62 */ "PST-8",
    /*  63 */ "KST-9",
    /*  64 */ "<+0630>-6:30",
    /*  65 */ "<+0330>-3:30<+0430>,J79/24,J263/24",
    /*  66 */ "JST-9",
    /*  67 */ "WET0WEST,M3.5.0/1,M10.5.0",
    /*  68 */ "<-01>1",
    /*  69 */ "ACST-9:30ACDT,M10.1.0,M4.1.0/3",
    /*  70 */ "AEST-10",
    /*  71 */ "ACST-9:30",
    /*  72 */ "<+0845>-8:45",
    /*  73 */ "<+1030>-10:30<+11>-11,M10.1.0,M4.1.0",
    /*  74 */ "AWST-8",
    /*  75 */ "<-06>6<-05>,M9.1.6/22,M4.1.6/22",
    /*  76 */ "IST-1GMT0,M10.5.0,M3.5.0/1",
    /*  77 */ "<-10>10",
    /*  78 */ "<-11>11",
    /*  79 */ "<-12>12",
    /*  80 */ "<-06>6",
    /*  81 */ "<-07>7",
    /*  82 */ "<-08>8",
    /*  83 */ "<-09>9",
    /*  84 */ "<+13>-13",
    /*  85 */ "<+14>-14",
    /*  86 */ "<+02>-2",
    /*  87 */ "UTC0",
    /*  88 */ "GMT0BST,M3.5.0/1,M10.5.0",
    /*  89 */ "EET-2EEST,M3.5.0,M10.5.0/3",
    /*  90 */ "MSK-3",
    /*  91 */ "<-00>0",
    /*  92 */ "HST10",
    /*  93 */ "MET-1MEST,M3.5.0,M10.5.0/3",
    /*  94 */ "<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45",
    /*  95 */ "<+13>-13<+14>,M9.5.0/3,M4.1.0/4",
    /*  96 */ "<+12>-12<+13>,M11.2.0,M1.2.3/99",
    /*  97 */ "ChST-10",
    /*  98 */ "<-0930>9:30",
    /*  99 */ "SST11",
    /* 100 */ "<+11>-11<+12>,M10.1.0,M4.1.0/3",
};

const uint32_t mask = 0x1fffff;

const struct {
  uint32_t hash : 24;
  uint8_t posix : 8;
} zones[] = {
    {3052, 0},       // Etc/localtime
    {3177, 12},      // America/Argentina/Buenos_Aires
    {5424, 77},      // Pacific/Rarotonga
    {6596, 29},      // Brazil/DeNoronha
    {8404, 99},      // Pacific/Midway
    {16206, 88},     // Europe/London
    {23764, 7},      // Europe/Berlin
    {25742, 70},     // Australia/Lindeman
    {28550, 52},     // Europe/Nicosia
    {33699, 41},     // Asia/Urumqi
    {34704, 88},     // Europe/Jersey
    {39895, 97},     // Pacific/Saipan
    {43289, 19},     // US/Mountain
    {43535, 43},     // Pacific/Kwajalein
    {45558, 17},     // America/Campo_Grande
    {46140, 12},     // America/Cordoba
    {47414, 54},     // Hongkong
    {47637, 9},      // America/Atka
    {56581, 16},     // America/Guatemala
    {57398, 78},     // Pacific/Niue
    {61482, 53},     // Asia/Hebron
    {63824, 22},     // America/Hermosillo
    {75452, 23},     // America/Fort_Wayne
    {75610, 11},     // America/Curacao
    {77251, 11},     // America/Marigot
    {77667, 35},     // Asia/Ust-Nera
    {80762, 18},     // America/Guayaquil
    {83533, 34},     // Asia/Saigon
    {91050, 11},     // America/Blanc-Sablon
    {92809, 12},     // America/Sao_Paulo
    {93028, 23},     // Canada/Eastern
    {97827, 52},     // Europe/Bucharest
    {98146, 35},     // Antarctica/DumontDUrville
    {98189, 24},     // Mexico/BajaNorte
    {100383, 45},    // Asia/Beirut
    {102841, 14},    // America/Coral_Harbour
    {104536, 48},    // Asia/Dili
    {109229, 43},    // Asia/Kamchatka
    {110344, 5},     // Egypt
    {121092, 12},    // Atlantic/Stanley
    {124286, 0},     // Africa/Freetown
    {136540, 90},    // Europe/Moscow
    {138335, 11},    // America/Montserrat
    {153557, 55},    // Asia/Pontianak
    {156843, 52},    // Europe/Kiev
    {160344, 37},    // Asia/Yekaterinburg
    {161461, 65},    // Iran
    {165199, 20},    // America/Resolute
    {166924, 33},    // Pacific/Guadalcanal
    {172026, 3},     // Africa/Malabo
    {186333, 7},     // Europe/Rome
    {190474, 87},    // Universal
    {191305, 93},    // MET
    {193306, 7},     // Atlantic/Jan_Mayen
    {195711, 97},    // Pacific/Guam
    {196047, 92},    // US/Hawaii
    {200499, 49},    // Asia/Macao
    {201322, 0},     // Africa/Ouagadougou
    {201945, 52},    // Europe/Athens
    {208700, 48},    // Asia/Yakutsk
    {210977, 49},    // Asia/Macau
    {221293, 46},    // Asia/Brunei
    {222990, 38},    // Antarctica/South_Pole
    {226052, 16},    // America/Costa_Rica
    {232294, 33},    // Pacific/Bougainville
    {241713, 40},    // Antarctica/Troll
    {245936, 7},     // Europe/Podgorica
    {250998, 36},    // Australia/NSW
    {251033, 23},    // EST5EDT
    {262983, 1},     // Africa/Addis_Ababa
    {267738, 34},    // Asia/Tomsk
    {270075, 23},    // America/Iqaluit
    {274113, 29},    // America/Noronha
    {276475, 7},     // Europe/Tirane
    {277285, 84},    // Pacific/Enderbury
    {279399, 61},    // Asia/Makassar
    {281938, 19},    // America/Boise
    {285314, 88},    // Europe/Guernsey
    {288178, 94},    // NZ-CHAT
    {288555, 73},    // Australia/Lord_Howe
    {289636, 25},    // America/Moncton
    {290785, 12},    // America/Punta_Arenas
    {296680, 62},    // Asia/Manila
    {300125, 20},    // Canada/Central
    {303007, 19},    // America/Shiprock
    {303455, 41},    // Asia/Bishkek
    {307480, 37},    // Asia/Aqtau
    {309993, 44},    // Asia/Muscat
    {310155, 12},    // America/Argentina/San_Juan
    {318936, 85},    // Etc/GMT-14
    {320548, 35},    // Etc/GMT-10
    {320951, 33},    // Etc/GMT-11
    {321354, 43},    // Etc/GMT-12
    {321757, 84},    // Etc/GMT-13
    {322216, 36},    // Australia/Currie
    {325064, 14},    // America/Atikokan
    {329953, 15},    // Mexico/General
    {335595, 21},    // Mexico/BajaSur
    {337525, 23},    // America/Thunder_Bay
    {344612, 20},    // America/Rainy_River
    {346512, 49},    // Asia/Taipei
    {349954, 0},     // Africa/Nouakchott
    {350456, 1},     // Africa/Dar_es_Salaam
    {362265, 90},    // W-SU
    {367282, 12},    // America/Argentina/Ushuaia
    {375886, 12},    // America/Argentina/Mendoza
    {379069, 34},    // Asia/Bangkok
    {384126, 23},    // America/Montreal
    {385136, 3},     // Africa/Kinshasa
    {392475, 12},    // America/Argentina/Tucuman
    {395160, 67},    // Atlantic/Faeroe
    {398176, 36},    // Australia/Tasmania
    {402710, 43},    // Pacific/Nauru
    {407955, 44},    // Europe/Samara
    {419965, 16},    // America/Swift_Current
    {421329, 38},    // NZ
    {427478, 10},    // America/Juneau
    {429402, 49},    // Asia/Chongqing
    {431351, 41},    // Asia/Kashgar
    {431659, 0},     // Africa/Lome
    {431737, 19},    // America/Cambridge_Bay
    {431765, 7},     // Europe/Madrid
    {445829, 0},     // Iceland
    {448725, 6},     // Africa/Casablanca
    {456379, 60},    // Asia/Kathmandu
    {457803, 24},    // America/Vancouver
    {459497, 23},    // America/Nassau
    {461661, 61},    // Asia/Ujung_Pandang
    {461663, 15},    // America/Monterrey
    {464955, 4},     // Africa/Harare
    {467586, 67},    // Atlantic/Canary
    {492113, 88},    // Europe/Belfast
    {492850, 88},    // Europe/Isle_of_Man
    {498759, 22},    // America/Dawson_Creek
    {501621, 12},    // America/Cayenne
    {512398, 0},     // Etc/GMT0
    {513033, 12},    // Antarctica/Palmer
    {513738, 37},    // Asia/Samarkand
    {516616, 99},    // Pacific/Samoa
    {516962, 49},    // Asia/Chungking
    {519199, 12},    // America/Rosario
    {520109, 91},    // Factory
    {525358, 41},    // Asia/Omsk
    {526318, 89},    // Europe/Chisinau
    {529695, 63},    // ROK
    {529783, 15},    // America/Mexico_City
    {532919, 49},    // ROC
    {537346, 21},    // America/Chihuahua
    {540603, 1},     // Africa/Kampala
    {556011, 2},     // Africa/Tunis
    {562127, 11},    // America/Guadeloupe
    {569631, 0},     // Africa/Dakar
    {569859, 11},    // America/Kralendijk
    {570340, 10},    // America/Anchorage
    {573129, 44},    // Indian/Reunion
    {583325, 35},    // Pacific/Yap
    {586628, 11},    // America/Lower_Princes
    {587591, 10},    // America/Yakutat
    {595614, 20},    // America/Rankin_Inlet
    {601710, 1},     // Indian/Comoro
    {604958, 36},    // Australia/ACT
    {609925, 57},    // Israel
    {611845, 7},     // Poland
    {614930, 0},     // GMT-0
    {621209, 11},    // America/Virgin
    {626509, 59},    // Asia/Karachi
    {627717, 98},    // Pacific/Marquesas
    {639134, 11},    // America/Barbados
    {639738, 33},    // Pacific/Noumea
    {645690, 64},    // Asia/Rangoon
    {648563, 0},     // Africa/Abidjan
    {653866, 77},    // Pacific/Tahiti
    {657702, 8},     // Africa/Mbabane
    {657734, 1},     // Africa/Nairobi
    {661560, 46},    // Asia/Ulaanbaatar
    {663785, 33},    // Asia/Sakhalin
    {671411, 12},    // America/Jujuy
    {675419, 39},    // Europe/Volgograd
    {679096, 7},     // Europe/Copenhagen
    {681504, 14},    // America/Panama
    {683109, 35},    // Pacific/Chuuk
    {685444, 41},    // Antarctica/Vostok
    {685809, 23},    // America/Pangnirtung
    {688338, 19},    // America/Edmonton
    {690567, 11},    // America/Antigua
    {692618, 30},    // America/Santiago
    {692891, 7},     // Arctic/Longyearbyen
    {695903, 46},    // Asia/Irkutsk
    {696273, 7},     // Europe/Brussels
    {697408, 46},    // Asia/Choibalsan
    {702292, 21},    // America/Mazatlan
    {703920, 37},    // Asia/Oral
    {707873, 33},    // Antarctica/Casey
    {711703, 23},    // America/Detroit
    {711788, 41},    // Asia/Almaty
    {711799, 38},    // Antarctica/McMurdo
    {712423, 4},     // Africa/Khartoum
    {712988, 7},     // Europe/Vaduz
    {716645, 1},     // Indian/Antananarivo
    {717463, 12},    // America/Argentina/Rio_Gallegos
    {730803, 43},    // Pacific/Tarawa
    {732255, 37},    // Asia/Qyzylorda
    {736937, 41},    // Asia/Thimphu
    {737384, 7},     // Europe/Vatican
    {737794, 39},    // Asia/Istanbul
    {745820, 19},    // America/Denver
    {746366, 66},    // Asia/Tokyo
    {746788, 48},    // Pacific/Palau
    {749066, 5},     // Libya
    {751039, 37},    // Asia/Ashgabat
    {765182, 41},    // Indian/Chagos
    {770053, 48},    // Asia/Chita
    {778195, 0},     // Atlantic/St_Helena
    {779608, 14},    // America/Cancun
    {790313, 74},    // Australia/West
    {790984, 44},    // Europe/Ulyanovsk
    {794645, 52},    // Europe/Riga
    {796867, 48},    // Asia/Khandyga
    {803253, 0},     // Africa/Bamako
    {809639, 1},     // Africa/Mogadishu
    {819359, 20},    // US/Indiana-Starke
    {820461, 17},    // America/Porto_Velho
    {820802, 3},     // Africa/Libreville
    {820968, 92},    // HST
    {825000, 5},     // Europe/Kaliningrad
    {825021, 71},    // Australia/North
    {827251, 12},    // America/Buenos_Aires
    {827271, 67},    // Portugal
    {827378, 33},    // Pacific/Ponape
    {828715, 20},    // US/Central
    {833473, 23},    // America/Indiana/Winamac
    {836835, 11},    // America/St_Vincent
    {845698, 35},    // Pacific/Port_Moresby
    {845934, 32},    // America/St_Johns
    {853628, 5},     // Africa/Cairo
    {853804, 19},    // America/Inuvik
    {857365, 39},    // Asia/Kuwait
    {857780, 23},    // America/Indiana/Marengo
    {861210, 20},    // America/Indiana/Tell_City
    {874038, 33},    // Pacific/Efate
    {876028, 0},     // Atlantic/Reykjavik
    {876382, 87},    // Etc/UCT
    {877492, 25},    // America/Thule
    {881218, 95},    // Pacific/Apia
    {883583, 12},    // America/Argentina/Catamarca
    {884797, 43},    // Pacific/Wake
    {889468, 79},    // Etc/GMT+12
    {890274, 77},    // Etc/GMT+10
    {890677, 78},    // Etc/GMT+11
    {891082, 0},     // Etc/GMT
    {893463, 67},    // Atlantic/Madeira
    {901474, 57},    // Asia/Tel_Aviv
    {904171, 16},    // Canada/Saskatchewan
    {906508, 3},     // Africa/Bangui
    {911377, 52},    // EET
    {913956, 0},     // GMT+0
    {920547, 10},    // US/Alaska
    {934775, 7},     // Europe/Bratislava
    {938682, 12},    // America/Argentina/San_Luis
    {939645, 87},    // UTC
    {943222, 44},    // Asia/Tbilisi
    {949217, 18},    // Brazil/Acre
    {951063, 17},    // America/Guyana
    {965414, 85},    // Pacific/Kiritimati
    {971533, 39},    // Asia/Bahrain
    {973205, 23},    // America/Kentucky/Monticello
    {975038, 23},    // America/Port-au-Prince
    {980806, 18},    // America/Bogota
    {981204, 90},    // Europe/Simferopol
    {981929, 14},    // Jamaica
    {985401, 24},    // America/Ensenada
    {987190, 44},    // Europe/Saratov
    {992722, 39},    // Asia/Aden
    {994235, 47},    // Asia/Calcutta
    {997374, 11},    // America/Dominica
    {999887, 7},     // Europe/Sarajevo
    {1000699, 66},   // Japan
    {1003911, 58},   // Asia/Kabul
    {1005304, 63},   // Asia/Seoul
    {1007821, 44},   // Asia/Dubai
    {1014376, 10},   // America/Sitka
    {1020394, 22},   // US/Arizona
    {1021091, 67},   // Atlantic/Faroe
    {1027658, 52},   // Europe/Mariehamn
    {1028352, 16},   // America/Regina
    {1029516, 6},    // Africa/El_Aaiun
    {1039495, 36},   // Australia/Victoria
    {1043921, 34},   // Asia/Barnaul
    {1046837, 33},   // Asia/Magadan
    {1051310, 18},   // America/Rio_Branco
    {1052350, 70},   // Australia/Brisbane
    {1056374, 12},   // America/Argentina/Cordoba
    {1060994, 11},   // America/St_Lucia
    {1061054, 52},   // Europe/Tallinn
    {1062928, 11},   // America/St_Thomas
    {1063980, 49},   // PRC
    {1064998, 37},   // Asia/Dushanbe
    {1068205, 12},   // America/Catamarca
    {1072445, 4},    // Africa/Gaborone
    {1075218, 89},   // Europe/Tiraspol
    {1084768, 3},    // Africa/Ndjamena
    {1085336, 0},    // Africa/Banjul
    {1086799, 69},   // Australia/Yancowinna
    {1089024, 12},   // Etc/GMT+3
    {1089427, 29},   // Etc/GMT+2
    {1089830, 68},   // Etc/GMT+1
    {1090111, 4},    // Africa/Blantyre
    {1090233, 0},    // Etc/GMT+0
    {1090636, 81},   // Etc/GMT+7
    {1091039, 80},   // Etc/GMT+6
    {1091442, 18},   // Etc/GMT+5
    {1091845, 17},   // Etc/GMT+4
    {1093054, 83},   // Etc/GMT+9
    {1093457, 82},   // Etc/GMT+8
    {1094261, 0},    // Africa/Sao_Tome
    {1097463, 69},   // Australia/Adelaide
    {1104071, 14},   // EST
    {1109219, 0},    // Africa/Timbuktu
    {1109996, 88},   // GB
    {1110416, 29},   // Atlantic/South_Georgia
    {1112624, 44},   // Asia/Yerevan
    {1112853, 9},    // US/Aleutian
    {1114572, 23},   // America/Louisville
    {1115282, 4},    // Africa/Lubumbashi
    {1118334, 42},   // Asia/Amman
    {1129135, 4},    // Africa/Windhoek
    {1130380, 7},    // Europe/Stockholm
    {1132822, 52},   // Europe/Vilnius
    {1138217, 7},    // Europe/Malta
    {1138914, 55},   // Asia/Jakarta
    {1147945, 12},   // America/Argentina/ComodRivadavia
    {1153584, 3},    // Africa/Douala
    {1153793, 37},   // Asia/Ashkhabad
    {1153929, 0},    // Africa/Bissau
    {1162475, 52},   // Asia/Famagusta
    {1164709, 26},   // America/Godthab
    {1166382, 41},   // Asia/Qostanay
    {1171420, 0},    // America/Danmarkshavn
    {1173424, 24},   // PST8PDT
    {1174363, 0},    // Greenwich
    {1175981, 12},   // America/Belem
    {1180279, 17},   // America/Cuiaba
    {1181500, 23},   // US/Michigan
    {1185412, 15},   // America/Merida
    {1186379, 7},    // Europe/Zurich
    {1194669, 11},   // America/Anguilla
    {1196657, 36},   // Australia/Melbourne
    {1198798, 12},   // America/Maceio
    {1199087, 39},   // Turkey
    {1200829, 71},   // Australia/Darwin
    {1203125, 52},   // Europe/Helsinki
    {1204987, 11},   // America/Martinique
    {1208470, 54},   // Asia/Hong_Kong
    {1209491, 41},   // Asia/Thimbu
    {1210986, 72},   // Australia/Eucla
    {1213061, 12},   // Antarctica/Rothera
    {1215578, 17},   // America/Caracas
    {1217188, 7},    // Europe/Luxembourg
    {1217254, 25},   // Atlantic/Bermuda
    {1220402, 7},    // Europe/Belgrade
    {1221000, 24},   // America/Santa_Isabel
    {1226645, 12},   // Brazil/East
    {1228331, 27},   // America/Havana
    {1229346, 20},   // America/North_Dakota/Center
    {1229493, 96},   // Pacific/Fiji
    {1233986, 22},   // America/Whitehorse
    {1235835, 39},   // Antarctica/Syowa
    {1236962, 23},   // America/Nipigon
    {1238271, 92},   // Pacific/Honolulu
    {1241695, 1},    // Africa/Asmera
    {1241995, 4},    // Africa/Kigali
    {1247746, 19},   // Navajo
    {1248438, 23},   // America/Indiana/Petersburg
    {1252962, 63},   // Asia/Pyongyang
    {1254429, 0},    // GMT0
    {1255107, 4},    // Africa/Bujumbura
    {1256032, 52},   // Europe/Uzhgorod
    {1258171, 7},    // Europe/Ljubljana
    {1263939, 7},    // Europe/San_Marino
    {1270740, 20},   // America/Chicago
    {1271679, 19},   // MST7MDT
    {1279015, 3},    // Africa/Porto-Novo
    {1283805, 20},   // America/Menominee
    {1292061, 19},   // America/Yellowknife
    {1296096, 25},   // America/Goose_Bay
    {1296855, 2},    // Africa/Algiers
    {1303250, 37},   // Asia/Aqtobe
    {1304112, 23},   // America/Indiana/Vincennes
    {1306975, 43},   // Kwajalein
    {1308115, 7},    // Europe/Warsaw
    {1316301, 3},    // Africa/Niamey
    {1319533, 18},   // America/Eirunepe
    {1337063, 87},   // UCT
    {1337990, 83},   // Pacific/Gambier
    {1338312, 1},    // Indian/Mayotte
    {1342523, 23},   // America/Grand_Turk
    {1344460, 12},   // America/Mendoza
    {1351228, 7},    // Europe/Gibraltar
    {1361407, 34},   // Asia/Novokuznetsk
    {1365325, 7},    // Europe/Andorra
    {1368647, 19},   // Canada/Mountain
    {1369907, 7},    // Europe/Vienna
    {1371911, 69},   // Australia/Broken_Hill
    {1372423, 67},   // WET
    {1377039, 12},   // America/Santarem
    {1377848, 30},   // Chile/Continental
    {1379920, 68},   // Atlantic/Cape_Verde
    {1395088, 23},   // US/East-Indiana
    {1405309, 12},   // America/Araguaina
    {1408594, 8},    // Africa/Johannesburg
    {1411942, 100},  // Pacific/Norfolk
    {1416215, 8},    // Africa/Maseru
    {1417058, 15},   // America/Bahia_Banderas
    {1417591, 22},   // America/Fort_Nelson
    {1419250, 36},   // Australia/Sydney
    {1419657, 23},   // America/Toronto
    {1425345, 99},   // US/Samoa
    {1429596, 7},    // Europe/Prague
    {1429753, 82},   // Pacific/Pitcairn
    {1433103, 94},   // Pacific/Chatham
    {1436375, 20},   // America/Matamoros
    {1438897, 0},    // Africa/Conakry
    {1446971, 35},   // Pacific/Truk
    {1448104, 31},   // America/Scoresbysund
    {1453728, 4},    // Africa/Maputo
    {1454891, 16},   // America/Belize
    {1456669, 56},   // Asia/Jayapura
    {1458268, 23},   // America/New_York
    {1459791, 34},   // Asia/Hovd
    {1462267, 75},   // Pacific/Easter
    {1475245, 11},   // America/St_Kitts
    {1475507, 0},    // Africa/Monrovia
    {1485189, 20},   // America/Winnipeg
    {1495084, 28},   // America/Miquelon
    {1504159, 22},   // MST
    {1505373, 34},   // Indian/Christmas
    {1510870, 92},   // Pacific/Johnston
    {1520618, 39},   // Europe/Minsk
    {1525516, 37},   // Asia/Atyrau
    {1530094, 11},   // America/Grenada
    {1532373, 11},   // America/Puerto_Rico
    {1535924, 7},    // Africa/Ceuta
    {1540528, 12},   // America/Montevideo
    {1540992, 25},   // Canada/Atlantic
    {1544434, 13},   // America/Asuncion
    {1551375, 7},    // Europe/Oslo
    {1556415, 22},   // America/Phoenix
    {1557856, 4},    // Africa/Juba
    {1559064, 31},   // Atlantic/Azores
    {1568759, 24},   // Canada/Pacific
    {1570889, 9},    // America/Adak
    {1571112, 18},   // America/Porto_Acre
    {1578023, 41},   // Asia/Dhaka
    {1582037, 24},   // US/Pacific
    {1598908, 80},   // Pacific/Galapagos
    {1600593, 50},   // Asia/Colombo
    {1601819, 60},   // Asia/Katmandu
    {1603047, 11},   // America/Port_of_Spain
    {1604820, 24},   // America/Los_Angeles
    {1606619, 67},   // Europe/Lisbon
    {1607248, 33},   // Pacific/Pohnpei
    {1607370, 22},   // America/Creston
    {1608844, 27},   // Cuba
    {1609737, 18},   // America/Lima
    {1610087, 17},   // America/La_Paz
    {1610980, 75},   // Chile/EasterIsland
    {1612262, 3},    // Africa/Brazzaville
    {1614075, 7},    // Europe/Monaco
    {1617001, 25},   // America/Glace_Bay
    {1617113, 34},   // Asia/Ho_Chi_Minh
    {1618683, 87},   // Etc/Universal
    {1628048, 36},   // Australia/Canberra
    {1628566, 84},   // Pacific/Tongatapu
    {1628682, 22},   // Canada/Yukon
    {1632083, 7},    // CET
    {1632914, 36},   // Antarctica/Macquarie
    {1651585, 49},   // Asia/Shanghai
    {1651622, 7},    // Europe/Budapest
    {1654743, 39},   // Europe/Kirov
    {1658833, 43},   // Asia/Anadyr
    {1660911, 46},   // Asia/Ulan_Bator
    {1664872, 20},   // America/Knox_IN
    {1672213, 74},   // Australia/Perth
    {1674610, 16},   // America/Tegucigalpa
    {1675410, 84},   // Pacific/Fakaofo
    {1676415, 17},   // Brazil/West
    {1685232, 34},   // Asia/Krasnoyarsk
    {1692325, 11},   // America/Santo_Domingo
    {1694433, 12},   // America/Argentina/Jujuy
    {1698695, 3},    // Africa/Luanda
    {1701688, 12},   // America/Recife
    {1708083, 4},    // Africa/Lusaka
    {1710030, 20},   // CST6CDT
    {1713796, 46},   // Asia/Kuala_Lumpur
    {1716755, 23},   // America/Kentucky/Louisville
    {1719069, 43},   // Pacific/Majuro
    {1721181, 44},   // Europe/Astrakhan
    {1722440, 0},    // Africa/Accra
    {1722853, 20},   // America/Indiana/Knox
    {1726451, 5},    // Africa/Tripoli
    {1733838, 23},   // America/Indiana/Vevay
    {1734054, 20},   // America/North_Dakota/Beulah
    {1736591, 7},    // Europe/Paris
    {1738905, 19},   // America/Ojinaga
    {1741247, 0},    // GMT
    {1742182, 16},   // America/Managua
    {1746506, 0},    // Etc/Greenwich
    {1749527, 69},   // Australia/South
    {1749682, 46},   // Asia/Singapore
    {1756562, 20},   // America/North_Dakota/New_Salem
    {1769420, 70},   // Australia/Queensland
    {1774546, 23},   // America/Indiana/Indianapolis
    {1776375, 52},   // Europe/Zaporozhye
    {1778258, 39},   // Europe/Istanbul
    {1781122, 44},   // Indian/Mauritius
    {1784482, 44},   // Indian/Mahe
    {1786941, 39},   // Asia/Qatar
    {1788281, 7},    // Europe/Busingen
    {1798337, 46},   // Asia/Kuching
    {1798422, 76},   // Europe/Dublin
    {1802468, 24},   // America/Tijuana
    {1803694, 14},   // America/Jamaica
    {1832626, 12},   // America/Paramaribo
    {1841914, 76},   // Eire
    {1847810, 37},   // Asia/Tashkent
    {1850225, 23},   // America/Indianapolis
    {1853077, 43},   // Pacific/Wallis
    {1857751, 32},   // Canada/Newfoundland
    {1861199, 34},   // Asia/Novosibirsk
    {1862715, 39},   // Asia/Riyadh
    {1863529, 34},   // Asia/Vientiane
    {1863787, 1},    // Africa/Asmara
    {1871852, 87},   // Etc/UTC
    {1884018, 52},   // Europe/Sofia
    {1884747, 17},   // America/Manaus
    {1894052, 7},    // Europe/Skopje
    {1894752, 35},   // Asia/Vladivostok
    {1901000, 87},   // Etc/Zulu
    {1902986, 34},   // Asia/Phnom_Penh
    {1904373, 10},   // America/Nome
    {1905218, 23},   // US/Eastern
    {1906241, 46},   // Singapore
    {1917776, 49},   // Asia/Harbin
    {1921672, 16},   // America/El_Salvador
    {1938433, 11},   // America/St_Barthelemy
    {1942188, 10},   // America/Metlakatla
    {1946378, 12},   // America/Fortaleza
    {1951927, 39},   // Asia/Baghdad
    {1958859, 11},   // America/Tortola
    {1959517, 33},   // Asia/Srednekolymsk
    {1961627, 25},   // America/Halifax
    {1962619, 87},   // Zulu
    {1967285, 37},   // Indian/Kerguelen
    {1968744, 38},   // Pacific/Auckland
    {1968987, 7},    // Europe/Zagreb
    {1970831, 11},   // America/Aruba
    {1981568, 22},   // America/Dawson
    {1987977, 14},   // America/Cayman
    {2000737, 34},   // Antarctica/Davis
    {2006054, 1},    // Africa/Djibouti
    {2006408, 99},   // Pacific/Pago_Pago
    {2009827, 43},   // Pacific/Funafuti
    {2013536, 17},   // America/Boa_Vista
    {2015747, 51},   // Asia/Damascus
    {2016109, 26},   // America/Nuuk
    {2017028, 33},   // Pacific/Kosrae
    {2019445, 12},   // America/Argentina/La_Rioja
    {2019448, 36},   // Australia/Hobart
    {2023156, 88},   // GB-Eire
    {2026282, 3},    // Africa/Lagos
    {2028182, 52},   // Asia/Nicosia
    {2033048, 64},   // Asia/Yangon
    {2062776, 7},    // Europe/Amsterdam
    {2068042, 37},   // Indian/Maldives
    {2070769, 12},   // America/Argentina/Salta
    {2071245, 73},   // Australia/LHI
    {2071733, 12},   // America/Bahia
    {2072709, 47},   // Asia/Kolkata
    {2074130, 65},   // Asia/Tehran
    {2075568, 37},   // Etc/GMT-5
    {2075971, 44},   // Etc/GMT-4
    {2076374, 34},   // Etc/GMT-7
    {2076777, 41},   // Etc/GMT-6
    {2077180, 6},    // Etc/GMT-1
    {2077265, 53},   // Asia/Gaza
    {2077583, 0},    // Etc/GMT-0
    {2077986, 39},   // Etc/GMT-3
    {2078389, 86},   // Etc/GMT-2
    {2080404, 48},   // Etc/GMT-9
    {2080807, 46},   // Etc/GMT-8
    {2087116, 64},   // Indian/Cocos
    {2088323, 37},   // Antarctica/Mawson
    {2089285, 44},   // Asia/Baku
    {2090334, 57},   // Asia/Jerusalem
    {2094940, 41},   // Asia/Dacca
};

const uint NumZones = sizeof(zones) / sizeof(zones[0]);

const uint32_t FNV_PRIME = 16777619u;
const uint32_t OFFSET_BASIS = 2166136261u;

// this function computes the "fnv" hash of a string, ignoring the nul
// termination
// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
uint32_t fnvHash(const char *str) {
  uint32_t hash = OFFSET_BASIS;
  while (*str) {
    hash ^= uint32_t(*str++);
    hash *= FNV_PRIME;
  }
  return hash;
}

// this function, given a tz name in "Olson" format (like "Australia/Melbourne")
// returns the tz in "Posix" format (like "AEST-10AEDT,M10.1.0,M4.1.0/3"), which
// is what setenv("TZ",...) and settz wants. It does a binary search on the
// hashed values. It assumes that the "olson" string is a valid timezone name
// from the same version of the tzdb as it was compiled for. If passed an
// invalid string the behaviour is undefined.
const char *getPosixTZforOlson(const char *olson) {
  static_assert(NumZones > 0, "zones should not be empty");
  auto olsonHash = fnvHash(olson) & mask;
  auto i = &zones[0];
  auto x = &zones[NumZones];
  while (x - i > 1) {
    auto m = i + ((x - i) / 2);
    if (m->hash > olsonHash) {
      x = m;
    } else {
      i = m;
    }
  }
  if (i->hash == olsonHash) {
    return posix[i->posix];
  }
  return "UTC0"; // couldn't find it, use default
}

void getLocation() {
  // http://ip-api.com/json?fields=57792
  // {"status":"success","lat":-27.4649,"lon":153.028,"timezone":"Australia/Brisbane","query":"202.144.174.72"}
  if (now() - lastGetLocationTS < LOCATION_UPDATE_INTERVAL) {  // too soon
    log_i("%ld-%ld=%ld", now(), lastGetLocationTS, now() - lastGetLocationTS);
    Watchy::err = Watchy::RATE_LIMITED;
    return;
  }
  if (!Watchy::getWiFi()) {
    log_e("getWiFi failed");
    Watchy::err = Watchy::WIFI_FAILED;
    return;
  }

  auto start = millis();
  // WiFi is connected Use IP-API.com API to map geo-located IP to lat/lon/etc
  HTTPClient http;
  http.setConnectTimeout(5000);  // 5 second connect timeout
  // fields is a pseudo-bitmap indicating which fields should be returned
  // ex. 57792 - query, status, lat, lon, timezone
  // ex. 57808 - query, status, lat, lon, timezone, city
  const char *locationQueryURL = "http://ip-api.com/json?fields=57808";
  if (!http.begin(locationQueryURL)) {
    log_e("http.begin failed");
    Watchy::err = Watchy::REQUEST_FAILED;
  } else {
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String payload = http.getString();
      JSONVar responseObject = JSON.parse(payload);
      location loc;
      loc.lat = double(responseObject["lat"]);
      loc.lon = double(responseObject["lon"]);
      strncpy(loc.city, responseObject["city"], sizeof(loc.city));

      const char* olsonTZ = static_cast<const char *>(responseObject["timezone"]);
      loc.timezone = getPosixTZforOlson(olsonTZ);
      if ( loc.timezone ) {
        Watchy_Event::Event{
            .id = Watchy_Event::LOCATION_UPDATE,
            .micros = micros(),
            {.loc = loc},
        }.send();
        lastGetLocationTS = now();
        Watchy::err = Watchy::OK;
      } else {
        log_e("getPosixTZForOlson failed");
        Watchy::err = Watchy::REQUEST_FAILED;
      }
    } else {
      log_e("http error %d", httpResponseCode);
      Watchy::err = Watchy::REQUEST_FAILED;
      // http error
    }
    http.end();
  }
  log_i("getLocation took %ldms", millis() - start);
  Watchy::releaseWiFi();
}
}  // namespace Watchy_GetLocation
