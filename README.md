# TurkC Compiler - Project 3

## Proje Açıklaması

TurkC derleyicisinin üçüncü projesi, semantic analiz ve bytecode üretimini içerir. Bu proje, Project 1 (Lexer) ve Project 2 (Parser + AST) üzerine inşa edilmiştir.

### Özellikler

- **Semantic Analiz**: Tip kontrolü, değişken çözümleme, kapsam yönetimi
- **Bytecode Üretimi**: Stack-tabanlı virtual machine için bytecode üretimi
- **Virtual Machine**: Üretilen bytecode'u çalıştıran yorumlayıcı
- **Hata Raporlama**: Satır numarası ile detaylı hata mesajları

---

## Derleme ve Çalıştırma

### Gereksinimler

- GCC (C11 desteği)
- Flex (lexer generator)
- Bison (parser generator)
- Make (optional)

### Derleme

```bash
# Makefile ile
make

# Manuel olarak
bison -d parser.y
flex scanner.l
gcc -Wall -Wextra -std=c11 parser.tab.c lex.yy.c ast.c symbol.c semantic.c codegen.c vm.c main.c -o turkc
```

### Kullanım

```bash
# Programı derle ve çalıştır
./turkc program.tc

# Sadece bytecode'u göster
./turkc -c program.tc

# AST'yi göster
./turkc -a program.tc

# Sembol tablosunu göster
./turkc -s program.tc

# Debug modu (her instruction'ı göster)
./turkc -d program.tc

# Tüm seçenekleri birleştir
./turkc -a -s -c program.tc
```

---

## Dil Spesifikasyonu

### Veri Tipleri

| Tip | Açıklama |
|-----|----------|
| `int` | 32-bit tamsayı |
| `void` | Dönüş değeri olmayan fonksiyonlar için |

### Anahtar Kelimeler

| TurkC | C Karşılığı | Açıklama |
|-------|-------------|----------|
| `eger` | `if` | Koşul ifadesi |
| `degilse` | `else` | Alternatif dal |
| `icin` | `for` | For döngüsü |
| `iken` | `while` | While döngüsü |
| `dondur` | `return` | Fonksiyondan dönüş |
| `int` | `int` | Tamsayı tipi |
| `void` | `void` | Void tipi |

### Operatörler

| Kategori | Operatörler |
|----------|-------------|
| Aritmetik | `+`, `-`, `*`, `/`, `%` |
| Karşılaştırma | `==`, `!=`, `<`, `>`, `<=`, `>=` |
| Atama | `=` |

### Sözdizimi Örneği

```c
/* Faktoriyel hesaplama */
int faktoriyel(int n) {
    int sonuc = 1;
    int i;
    
    icin (i = 1; i <= n; i = i + 1) {
        sonuc = sonuc * i;
    }
    
    dondur sonuc;
}

int ana() {
    int f5 = faktoriyel(5);
    dondur f5;  /* 120 döndürür */
}
```

---

## Semantic Analiz

### Kontrol Edilen Durumlar

1. **Değişken Çözümleme**: Tüm değişkenler kullanılmadan önce tanımlanmalı
2. **Tip Kontrolü**: Atama ve ifadelerde tip uyumluluğu kontrol edilir
3. **Kapsam Yönetimi**: İç içe kapsamlar ve shadowing desteklenir
4. **Fonksiyon Doğrulama**: 
   - Parametre sayısı kontrolü
   - Dönüş tipi kontrolü
   - Fonksiyon yeniden tanımlama kontrolü

### Hata Mesajları

```
Semantik hata (satir 5): 'y' degiskeni tanimlanmamis
Semantik hata (satir 8): 'x' degiskeni ayni kapsamda zaten tanimlanmis
Semantik hata (satir 12): 'topla' fonksiyonu 2 parametre bekliyor, 1 verildi
```

---

## Bytecode Instruction Set

### Stack Operasyonları

| Opcode | Açıklama |
|--------|----------|
| `PUSH n` | n değerini stack'e pushla |
| `POP` | Stack'ten pop yap |
| `DUP` | Stack'in üstünü kopyala |
| `LOAD n` | n offset'teki local değişkeni yükle |
| `STORE n` | n offset'teki local değişkene kaydet |

### Aritmetik Operasyonlar

| Opcode | Açıklama |
|--------|----------|
| `ADD` | Toplama (a + b) |
| `SUB` | Çıkarma (a - b) |
| `MUL` | Çarpma (a * b) |
| `DIV` | Bölme (a / b) |
| `MOD` | Mod (a % b) |
| `NEG` | Negation (-a) |

### Karşılaştırma Operasyonları

| Opcode | Açıklama |
|--------|----------|
| `EQ` | Eşitlik (==) |
| `NEQ` | Eşit değil (!=) |
| `LT` | Küçük (<) |
| `GT` | Büyük (>) |
| `LEQ` | Küçük eşit (<=) |
| `GEQ` | Büyük eşit (>=) |

### Kontrol Akışı

| Opcode | Açıklama |
|--------|----------|
| `JMP addr` | Koşulsuz jump |
| `JZ addr` | Sıfırsa jump |
| `JNZ addr` | Sıfır değilse jump |

### Fonksiyon Operasyonları

| Opcode | Açıklama |
|--------|----------|
| `CALL idx` | idx numaralı fonksiyonu çağır |
| `RET` | Fonksiyondan dön |
| `RETVAL` | Değerle fonksiyondan dön |
| `ENTER n` | n local değişken için yer ayır |

### Diğer

| Opcode | Açıklama |
|--------|----------|
| `HALT` | Programı durdur |
| `PRINT` | Stack'in üstünü yazdır |

---

## Örnek Çıktı

### Basit Program

```c
int ana() {
    int x = 5 + 3;
    dondur x;
}
```

### Üretilen Bytecode

```
=== Fonksiyon Tablosu ===
[0] ana: entry=0, params=0, locals=16

=== Uretilen Bytecode ===

; ana:
0000: ENTER      16
0001: PUSH       5
0002: PUSH       3
0003: ADD       
0004: STORE      0
0005: LOAD       0
0006: RETVAL    
0007: PUSH       0
0008: RETVAL    
0009: HALT      
```

### Program Çıktısı

```
=== TurkC Derleyici v3.0 ===
Dosya: test.tc

[1/4] Parsing (sozdizimi analizi)...
      Parsing basarili.

[2/4] Semantik analiz (tip kontrolu)...
      Semantik analiz basarili.

[3/4] Kod uretimi (bytecode)...
      Bytecode uretildi: 10 instruction.

[4/4] Program calistiriliyor...

--- Program Ciktisi ---
--- Program Sonu ---

Program cikis kodu: 8

=== Derleme Tamamlandi ===
```

---

## Test Suite

### Geçerli Testler (Semantic Analiz)

| Dosya | Açıklama |
|-------|----------|
| `test_var_decl.tc` | Değişken tanımları |
| `test_scopes.tc` | İç içe kapsamlar |
| `test_functions.tc` | Fonksiyon tanımları |
| `test_expressions.tc` | Aritmetik ifadeler |

### Hatalı Testler (Hata Kontrolü)

| Dosya | Beklenen Hata |
|-------|---------------|
| `test_undeclared_var.tc` | Tanımlanmamış değişken |
| `test_redeclare.tc` | Aynı kapsamda yeniden tanımlama |
| `test_func_redeclare.tc` | Fonksiyon yeniden tanımlama |
| `test_invalid_call.tc` | Yanlış parametre sayısı |

### Code Generation Testleri

| Dosya | Beklenen Sonuç |
|-------|----------------|
| `test_arithmetic.tc` | 7 |
| `test_if_else.tc` | 1 |
| `test_while.tc` | 15 |
| `test_for.tc` | 55 |
| `test_function_call.tc` | 17 |
| `test_nested_control.tc` | 30 |
| `test_complete.tc` | 90 |

### Test Çalıştırma

```bash
# Tüm testleri çalıştır
make test

# Tek test çalıştır
./turkc tests/test_complete.tc
```

---

## Dosya Yapısı

```
project3/
├── ast.h              # AST düğüm tanımları
├── ast.c              # AST işlemleri
├── parser.y           # Bison grammar
├── scanner.l          # Flex lexer
├── symbol.h           # Sembol tablosu tanımları
├── symbol.c           # Sembol tablosu implementasyonu
├── semantic.h         # Semantic analiz arayüzü
├── semantic.c         # Semantic analiz implementasyonu
├── codegen.h          # Bytecode tanımları
├── codegen.c          # Kod üretimi
├── vm.h               # Virtual machine arayüzü
├── vm.c               # VM implementasyonu
├── main.c             # Ana program
├── Makefile           # Build sistemi
├── README.md          # Bu dosya
└── tests/             # Test dosyaları
    ├── test_var_decl.tc
    ├── test_scopes.tc
    ├── test_functions.tc
    ├── test_expressions.tc
    ├── test_undeclared_var.tc
    ├── test_redeclare.tc
    ├── test_func_redeclare.tc
    ├── test_invalid_call.tc
    ├── test_arithmetic.tc
    ├── test_if_else.tc
    ├── test_while.tc
    ├── test_for.tc
    ├── test_function_call.tc
    ├── test_nested_control.tc
    └── test_complete.tc
```

---

## Bilinen Sınırlamalar

1. **Tip Sistemi**: Sadece `int` ve `void` desteklenir
2. **String**: String değişkenler tam desteklenmez
3. **Global Değişkenler**: Şu an sadece fonksiyon içi değişkenler desteklenir
4. **Özyineleme**: Fonksiyon özyinelemesi sınırlı stack derinliğine bağlı

---

## English Summary

TurkC is a C-like language with Turkish keywords. This project (Project 3) implements:

- **Semantic Analysis**: Type checking, variable resolution, scope management
- **Code Generation**: Stack-based bytecode generation
- **Virtual Machine**: Bytecode interpreter

Build with `make`, run with `./turkc program.tc`.

---

## Lisans

Bu proje derleyici tasarım dersi kapsamında hazırlanmıştır.
