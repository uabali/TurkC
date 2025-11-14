# TurkC Lexer Projesi

Bu proje, derleyici tasarım dersi için hazırlanmıştır. Türkçe anahtar kelimelerle temel bir programlama dili (TurkC) tanımlanmış ve Flex kullanarak bir lexer (scanner) yazılmıştır.

---

## English Version

# TurkC Lexer Project

This project is prepared for the compiler design course. A basic programming language (TurkC) with Turkish keywords is defined, and a lexer (scanner) is written using Flex.

## Language Specification
TurkC is a minimal version of the C programming language. Keywords are changed to Turkish. See `spesifikasyon.md` for details.

### Basic Features
- **Keywords:** eger, degilse, icin, iken, dondur, int, void
- **Identifiers:** Starting with letter or '_', containing letters/numbers/'_'.
- **Comments:** // (single line), /* */ (multi-line)
- **Operations:** +, -, *, /, %, ==, !=, <, >, <=, >=, =
- **Conditions & Loops:** eger/degilse, icin (for), iken (while)

## Scanner Description
- **Tool:** Flex (lexer generator)
- **File:** `scanner.l` – Rules and regex defined.
- **Generated Code:** `lex.yy.c` – Auto-generated C code by Flex.
- **Output:** Parses source code into tokens and prints types (e.g., "KEYWORD: eger").

## Installation and Running
1. **Requirements:** Flex and GCC (install via WSL or MSYS2).
2. **Compilation:**
   ```
   flex scanner.l
   gcc lex.yy.c -lfl -o scanner
   ```
3. **Testing:**
   ```
   ./scanner < test1.tc
   ./scanner < test2.tc
   ```

## Test Results
### test1.tc (Simple Function)
Output:
```
KEYWORD: int
IDENTIFIER: ana
PUNCTUATION: (
PUNCTUATION: )
PUNCTUATION: {
KEYWORD: int
IDENTIFIER: x
OPERATOR: =
NUMBER: 10
PUNCTUATION: ;
KEYWORD: dondur
NUMBER: 0
PUNCTUATION: ;
PUNCTUATION: }
```

### test2.tc (Condition and Loop)
Output:
```
KEYWORD: int
IDENTIFIER: ana
PUNCTUATION: (
PUNCTUATION: )
PUNCTUATION: {
KEYWORD: int
IDENTIFIER: x
OPERATOR: =
NUMBER: 5
PUNCTUATION: ;
KEYWORD: eger
PUNCTUATION: (
IDENTIFIER: x
OPERATOR: >
NUMBER: 0
PUNCTUATION: )
PUNCTUATION: {
}
KEYWORD: degilse
PUNCTUATION: {
}
KEYWORD: icin
PUNCTUATION: (
KEYWORD: int
IDENTIFIER: i
OPERATOR: =
NUMBER: 0
PUNCTUATION: ;
IDENTIFIER: i
OPERATOR: <
NUMBER: 10
PUNCTUATION: ;
IDENTIFIER: i
OPERATOR: =
IDENTIFIER: i
OPERATOR: +
NUMBER: 1
PUNCTUATION: )
PUNCTUATION: {
}
KEYWORD: dondur
NUMBER: 0
PUNCTUATION: ;
PUNCTUATION: }
```

## Files
- `spesifikasyon.md`: Language definition.
- `scanner.l`: Flex source code.
- `lex.yy.c`: Generated C code.
- `test1.tc`, `test2.tc`: Test source files.
- `scanner`: Compiled executable.

---

## Türkçe Versiyon

## Dil Spesifikasyonu
TurkC, C programlama dilinin tiny bir versiyonudur. Anahtar kelimeler Türkçe olarak değiştirilmiştir. Detaylar için `spesifikasyon.md` dosyasına bakınız.

### Temel Özellikler
- **Keywords:** eger, degilse, icin, iken, dondur, int, void
- **Identifiers:** Harf veya '_' ile başlayan, harf/rakam/'_' içeren.
- **Comments:** // (tek satır), /* */ (çok satır)
- **Operations:** +, -, *, /, %, ==, !=, <, >, <=, >=, =
- **Conditions & Loops:** eger/degilse, icin (for), iken (while)

## Scanner Açıklaması
- **Araç:** Flex (lexer generator)
- **Dosya:** `scanner.l` – Kurallar ve regex'ler tanımlanmış.
- **Üretilen Kod:** `lex.yy.c` – Flex tarafından otomatik üretilen C kodu.
- **Çıktı:** Kaynak kodu token'lara ayırır ve türlerini yazdırır (örn. "KEYWORD: eger").

## Kurulum ve Çalıştırma
1. **Gereksinimler:** Flex ve GCC (WSL veya MSYS2 ile kurulum).
2. **Derleme:**
   ```
   flex scanner.l
   gcc lex.yy.c -lfl -o scanner
   ```
3. **Test Etme:**
   ```
   ./scanner < test1.tc
   ./scanner < test2.tc
   ```

## Test Sonuçları
### test1.tc (Basit Fonksiyon)
Çıktı:
```
KEYWORD: int
IDENTIFIER: ana
PUNCTUATION: (
PUNCTUATION: )
PUNCTUATION: {
KEYWORD: int
IDENTIFIER: x
OPERATOR: =
NUMBER: 10
PUNCTUATION: ;
KEYWORD: dondur
NUMBER: 0
PUNCTUATION: ;
PUNCTUATION: }
```

### test2.tc (Koşul ve Döngü)
Çıktı:
```
KEYWORD: int
IDENTIFIER: ana
PUNCTUATION: (
PUNCTUATION: )
PUNCTUATION: {
KEYWORD: int
IDENTIFIER: x
OPERATOR: =
NUMBER: 5
PUNCTUATION: ;
KEYWORD: eger
PUNCTUATION: (
IDENTIFIER: x
OPERATOR: >
NUMBER: 0
PUNCTUATION: )
PUNCTUATION: {
}
KEYWORD: degilse
PUNCTUATION: {
}
KEYWORD: icin
PUNCTUATION: (
KEYWORD: int
IDENTIFIER: i
OPERATOR: =
NUMBER: 0
PUNCTUATION: ;
IDENTIFIER: i
OPERATOR: <
NUMBER: 10
PUNCTUATION: ;
IDENTIFIER: i
OPERATOR: =
IDENTIFIER: i
OPERATOR: +
NUMBER: 1
PUNCTUATION: )
PUNCTUATION: {
}
KEYWORD: dondur
NUMBER: 0
PUNCTUATION: ;
PUNCTUATION: }
```

## Dosyalar
- `spesifikasyon.md`: Dil tanımı.
- `scanner.l`: Flex kaynak kodu.
- `lex.yy.c`: Üretilen C kodu.
- `test1.tc`, `test2.tc`: Test kaynak dosyaları.
- `scanner`: Derlenmiş executable.
