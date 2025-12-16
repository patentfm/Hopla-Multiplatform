# Dokumentacja BLE (GATT + Advertising) — Hopla!

Firmware udostępnia **dwa interfejsy BLE**:

- **Advertising (Beacon)**: dane XYZ lecą w **Manufacturer Specific Data** bez łączenia.
- **GATT (po połączeniu)**: **XYZ Config Service** do zmiany parametrów pracy (częstotliwości, nazwy, progu, trybu itd.).

Poniżej są wszystkie “numerki” potrzebne do implementacji klienta na Androidzie: **UUID**, **format bajtów**, **zakresy**, oraz **uwagi dot. połączenia**.

---

## Parametry BLE istotne dla Androida

- **Nazwa urządzenia (GAP)**: `Hopla!` (domyślnie, może być zmieniona z GATT).
- **Tryb advertising**: `CONNECTABLE + SCANNABLE + UNDIRECTED`.
- **Bezpieczeństwo**: firmware **nie wspiera parowania/bondingu** (żądania security są odrzucane).
- **Connection parameters (PPCP)**: min ~100 ms, max ~200 ms, timeout ~4 s.

---

## Format Advertising (skanowanie bez łączenia)

W pakiecie reklamowym jest:

- **Complete Local Name**: pełna nazwa urządzenia (GAP name).
- **Manufacturer Specific Data** (AD type `0xFF`):
    - **Company Identifier**: `0xFFFF` (testowy, do zmiany w `src/config.h` jako `MANUFACTURER_ID`)
    - **Payload**: 8 bajtów (poniżej)

### Manufacturer payload (8 bajtów)

Endianness: **LE (little-endian)** dla liczb 16-bit.

| Offset | Rozmiar | Nazwa | Typ | Opis |
|---:|---:|---|---|---|
| 0 | 1 | `data_type` | `uint8` | Typ danych: `0x01` = XYZ (`DATA_TYPE_XYZ`) |
| 1..2 | 2 | `x_mg` | `int16 LE` | Oś X w **mg** (milli‑g) |
| 3..4 | 2 | `y_mg` | `int16 LE` | Oś Y w **mg** |
| 5..6 | 2 | `z_mg` | `int16 LE` | Oś Z w **mg** |
| 7 | 1 | `seq` | `uint8` | Licznik sekwencji, inkrementowany co update |

Uwagi:

- Wartości XYZ są w **mg** i pochodzą z LIS2DH12 (lub trybu demo, jeśli akcelerometr nie działa).
- Update manufacturer data odbywa się co `log_interval_ms` (patrz GATT).

---

## GATT: XYZ Config Service (połączenie)

To jest **vendor-specific service** (custom UUID).

### Bazowy UUID

UUID base (stały): `12340000-1234-5678-ABCD-1234567890AB`

W tej bazie firmware podmienia 16-bitowy “short id” `0xNNNN` na pełny UUID:

`1234NNNN-1234-5678-ABCD-1234567890AB`

### UUID usługi i charakterystyk

**Service UUID**:

- **Short**: `0x0001`
- **Full**: `12340001-1234-5678-ABCD-1234567890AB`

Większość charakterystyk ma:

- **Properties**: Read + Write (Write Request, z odpowiedzią)
- **Security**: open (bez parowania)
- **Notifications/Indications**: brak

#### Tabela charakterystyk (pełne “numerki” + format)

Endianness dla liczb >1 bajt: **LE**.

Domyślne wartości po starcie wynikają z `src/config.h` oraz inicjalizacji usługi.

| Nazwa (User Desc) | Short UUID | Full UUID | R/W | Rozmiar | Typ | Default | Zakres / wartości | Opis |
|---|---:|---|---|---:|---|---|---|
| Sample Rate | `0x0002` | `12340002-1234-5678-ABCD-1234567890AB` | R/W | 2 | `uint16 LE` | 200 | 10..10000 | Okres próbkowania akcelerometru w ms |
| Log Interval | `0x0003` | `12340003-1234-5678-ABCD-1234567890AB` | R/W | 2 | `uint16 LE` | 200 | 100..60000 | Jak często aktualizować Manufacturer Data w reklamie (ms) |
| Adv Interval | `0x0004` | `12340004-1234-5678-ABCD-1234567890AB` | R/W | 2 | `uint16 LE` | 100 | 20..4000 | Interwał advertisingu (ms) |
| TX Power | `0x0005` | `12340005-1234-5678-ABCD-1234567890AB` | R/W | 1 | `int8` | 0 | -40..4 | Moc TX w dBm (praktycznie: -40,-20,-16,-12,-8,-4,0,3,4) |
| Device Name | `0x0006` | `12340006-1234-5678-ABCD-1234567890AB` | R/W | 1..20 | `UTF-8 bytes` | `Hopla!` | max 20 bajtów | Nazwa urządzenia (GAP). Zapis restartuje reklamę z nową nazwą. Mozna uzyc zeby ustawiac np Hopla!NazwaGry. Wtedy wchodzi w gre multiplayer |
| Accel Thresh | `0x0007` | `12340007-1234-5678-ABCD-1234567890AB` | R/W | 2 | `uint16 LE` | 500 | 50..8000 | Próg ruchu w mg (używany też w trybie ARMED) |
| Accel Range | `0x0008` | `12340008-1234-5678-ABCD-1234567890AB` | R/W | 1 | `uint8` | 2 | 2..16 | Zakres ±g; firmware normalizuje do 2/4/8/16 |
| Accel Calib | `0x0009` | `12340009-1234-5678-ABCD-1234567890AB` | R/W | 6 | `3×int16 LE` | (0,0,0) | dowolne `int16` (mg) | Offsety kalibracji w mg: X,Y,Z |
| Mode | `0x000A` | `1234000A-1234-5678-ABCD-1234567890AB` | R/W | 1 | `uint8` | 0 | 0/1/2 | Tryb: 0=Normal, 1=Eco, 2=Armed |
| Logs | `0x000B` | `1234000B-1234-5678-ABCD-1234567890AB` | R | 0..1024 | `ASCII bytes` | (puste) | — | Bufor logów w RAM (linie zakończone `\\n`), **nadpisuje najstarsze** gdy zabraknie miejsca |
| Log Ctrl | `0x000C` | `1234000C-1234-5678-ABCD-1234567890AB` | W | 1..8 | `bytes` | 0 | — | Sterowanie logami (clear/freeze) – opis niżej |
| Diag LED | `0x000D` | `1234000D-1234-5678-ABCD-1234567890AB` | R/W | 1 | `uint8` | 0 | 0/1 | LED diagnostyczna: 0=wyłączona (domyślnie), 1=włączona (miganie/statusowe “flash’e”) |

### Kodowanie wartości (przykłady)

- `uint16 LE`: najmłodszy bajt pierwszy. Przykład: 1000 ms = `0x03E8` → zapisujesz bajty: `E8 03`.
- `Accel Calib` (6 bajtów): `x` (2 bajty LE), potem `y`, potem `z`.
- `Device Name`: dokładnie N bajtów stringa, **bez** `\0`.

---

## Semantyka trybów (Mode)

Firmware przechowuje wartości “bazowe” z GATT, ale **faktycznie używa wartości efektywnych zależnych od trybu**:

- **NORMAL (0)**: efektywne = bazowe.
- **ECO (1)**: tryb oszczedny
    - `sample_rate_ms`, `log_interval_ms`, `adv_interval_ms` są mnożone ×2 (z clampem do zakresów)
    - `tx_power_dbm` jest ograniczane do maks. **-8 dBm** (jeśli bazowo było wyżej)
- **ARMED (2)**: tryb wyczulony
    - `sample_rate_ms`, `log_interval_ms` są dzielone ÷2 (z clampem)
    - `adv_interval_ms` jest dzielone ÷2 (jeśli > minimum)
    - dodatkowo urządzenie “miga na czerwono” gdy \(\max(|x|,|y|,|z|)\) ≥ `accel_threshold_mg`

---

## Logi po GATT (Logs + Log Ctrl)

### Logs (`0x000B`)

- **Odczyt**: użyj **Read long / Read blob** (w nRF Connect: “Read long”). SoftDevice będzie pytał o kolejne offsety aż urządzenie zwróci 0 bajtów.
- **Chunking**: w jednym kawałku urządzenie zwraca maks. `min(244, 1024 - offset)` bajtów.
- **Format**: surowe bajty ASCII; kolejne wpisy są rozdzielone `\\n`.
- **Pamięć**: to jest **ring buffer w RAM** o rozmiarze `GATT_LOG_BUFFER_SIZE` (domyślnie 1024 B). Po zapełnieniu **najstarsze dane są automatycznie usuwane**.

#### Jak czytać w nRF Connect (praktycznie)

1) (Opcjonalnie) Write do **Log Ctrl**: `02 01` (freeze), żeby log nie zmieniał się w trakcie odczytu.
2) Na **Logs** użyj **Read long** (apka będzie czytać kolejne offsety aż dostanie 0 bajtów).
4) (Opcjonalnie) Write do **Log Ctrl**: `02 00` (unfreeze).

**Uwaga:** jeśli nRF Connect pokazuje “puste pole”, to często znaczy, że urządzenie zwróciło **0 bajtów** (czyli “koniec logów”). Zrestartuj urządzenie albo doprowadź do wygenerowania nowych logów i spróbuj ponownie.

#### Jak rozkodować `(0x)` HEX

nRF Connect często pokazuje bajty jako `(0x) ...`. To jest tekst ASCII w hexie (`0a` = newline `\\n`), np.:

`6c-6f-67-73-3a-72-65-61-64-79-0a-62-6f-6f-74-0a`

= `logs:ready\\nboot\\n`

### Log Ctrl (`0x000C`)

Komendy (Write):

- `01` — **clear** (wyczyść logi)
- `02 00` — **freeze off** (wznów dopisywanie)
- `02 01` — **freeze on** (zatrzymaj dopisywanie, ułatwia stabilny odczyt)

### Diag LED (`0x000D`)

- **Odczyt**: zwykły Read.
- **Zapis**: Write (1 bajt).
- **Wartości**: `0` = OFF (domyślnie), `1` = ON (miganie/statusowe “flashe”).


## Uwaga o “handle” vs UUID

Wartości **handle** (np. `value_handle`) są przydzielane dynamicznie przez SoftDevice i mogą się różnić między buildami/urządzeniami.  
Na Androidzie należy robić **discoverServices()** i wyszukiwać po **UUID**.

---

## Minimalny schemat w Androidzie (co robić)

- Skanuj i filtruj po:
    - nazwie `Hopla!.*` **lub**
    - Manufacturer Data z Company ID `0xFFFF` i `data_type=0x01`
- Jeśli chcesz zmieniać ustawienia:
    - połącz się (GATT connect)
    - wykonaj `discoverServices()`
    - znajdź service UUID `12340001-1234-5678-ABCD-1234567890AB`
    - zapisuj charakterystyki przez **Write (z odpowiedzią)** w formacie opisanym wyżej


