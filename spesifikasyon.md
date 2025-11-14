# Türkçe Temelli Programlama Dili Spesifikasyonu (Temel C Benzeri)

Bu dil, C programlama dilinin tiny(minimal) bir versiyonudur, bundan dolayi anahtar kelimeler Türkçe olarak değiştirilmiştir. Dil, Compiler Design dersi için gelistirildi ve lexer ile tarama islemi yapildi.

## Genel Özellikler
- **Dil Adı:** TurkC (Türkçe C)
- **Amaç:** Temel programlama yapıları kullanarak basit programlar yazmak.
- **Karakter Seti:** ASCII
- **Case Sensitivity:** Büyük/küçük harf duyarlı (örn. `eger` != `Eger`).

## Sözcük Yapısı (Lexical Structure)
- **Identifiers (Değişken İsimleri):** Harf veya '_' ile başlar, harf, rakam veya '_' içerebilir. Örn: `x`, `sayi1`, `_temp`.
- **Keywords (Anahtar Kelimeler):** Aşağıdaki Türkçe kelimeler ayrılmıştır:
  - `eger` (if)
  - `degilse` (else)
  - `icin` (for)
  - `iken` (while)
  - `dondur` (return)
  - `int` (veri türü, değişmeden bırakıldı)
  - `void` (veri türü, değişmeden bırakıldı)
- **Comments (Yorumlar):**
  - Tek satırlık: `//` ile başlar, satır sonuna kadar.
  - Çok satırlık: `/*` ile başlar, `*/` ile biter.
- **Literals (Sabitler):**
  - Tamsayı: `[0-9]+` (örn: 42)
  - String: `"..."` (örn: "merhaba")
- **Operators (Operatörler):**
  - Aritmetik: `+`, `-`, `*`, `/`, `%`
  - Karşılaştırma: `==`, `!=`, `<`, `>`, `<=`, `>=`
  - Atama: `=`
  - Diğer: `(`, `)`, `{`, `}`, `;`, `,`
- **Whitespace:** Boşluk, tab, yeni satır – token'lar arasında yok sayılır.

## Sözdizimi (Syntax)
- **Değişken Tanımlama:** `int x = 5;`
- **Koşullar (Conditions):** 
  ```
  eger (x > 0) {
      // kod
  } degilse {
      // kod
  }
  ```
- **Döngüler (Loops):**
  - For: `icin (int i = 0; i < 10; i = i + 1) { ... }`
  - While: `iken (x > 0) { ... }`
- **Fonksiyonlar:** `void fonksiyon() { dondur; }`
- **İşlemler:** Standart C gibi.

## Örnek Program
```
int ana() {
    int x = 10;
    eger (x > 5) {
        // Büyük
    } degilse {
        // Küçük
    }
    icin (int i = 0; i < x; i = i + 1) {
        // Döngü
    }
    dondur 0;
}
```