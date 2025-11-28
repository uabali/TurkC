# TurkC Parser ve AST Projesi

TurkC, C dilinin küçük ölçekli bir yorumudur. Bu depo, Proje 1'de yazılan sözlük analizcisinin (lexer) üzerine bir sözdizim analizcisi (parser) ve Soyut Sözdizim Ağacı (AST) üretimini ekler. Artık kaynak kod, Türkçe anahtar kelimelerle yazılmış tam bir programı kabul eder, sözdizimini doğrular ve hiyerarşik AST çıktısı üretir.

## Depo Yapısı
- `ast.h`, `ast.c` – AST düğüm tanımları, yardımcı işlevler, yazdırma ve bellek temizleme.
- `parser.y` – Bison tabanlı sözdizim analizcisi ve AST üretim kuralları.
- `scanner.l` – Flex tabanlı lexer; parser ile entegre olacak şekilde güncellendi.
- `main.c` – Komut satırı arabirimi, `yyparse` çağrısı ve AST çıktısı.
- `Makefile` – Derleme otomasyonu.
- `spesifikasyon.md` – Dilin ayrıntılı tanımı.
- `test1.tc`, `test2.tc`, `test3.tc` – Geçerli örnek programlar.
- `test_invalid_missing_semicolon.tc` – Sözdizim hatasını tetikleyen örnek.

## Derleme ve Çalıştırma
### Gereksinimler
- Flex
- Bison
- GCC
- Make (opsiyonel; yoksa komutlar tek tek çalıştırılabilir)

### Makefile ile
```
make
```
Bu komut `parser.tab.c`, `lex.yy.c` dosyalarını üretir ve `turkc` yürütülebilirini derler.

### Make olmadan (PowerShell örneği)
```
bison -d parser.y
flex scanner.l
gcc -Wall -Wextra -std=c11 parser.tab.c lex.yy.c ast.c main.c -o turkc
```
Flex/Bison çıktı dosyaları varsayılan adlarıyla (`parser.tab.*`, `lex.yy.c`) oluşturulmalıdır.

### Parser'ı çalıştırma
```
./turkc test1.tc
```
veya standart girdiden:
```
./turkc < test2.tc
```
Başarılı parse sonrasında AST, girintili bir metin ağacı olarak yazdırılır. Hata durumunda `Sozdizimi hatasi` mesajı üretilir ve program sıfırdan farklı bir kodla sonlanır.

## Dil Sözdizimi Özeti
TurkC dili aşağıdaki temel bileşenleri destekler:
- Fonksiyon tanımları (`int` veya `void` dönüş tipleri).
- Bloklar ve iç içe `eger`/`degilse`, `iken`, `icin` yapıları.
- Değişken bildirimleri (isteğe bağlı başlangıç değeriyle).
- Atama, karşılaştırma ve aritmetik ifadeler.
- String ve tam sayı sabitleri, fonksiyon çağrıları ve `dondur` deyimi.

### Uygulanan EBNF
```
program                ::= (declaration | function_definition)+ ;
function_definition    ::= type IDENTIFIER "(" parameter_list? ")" block ;
declaration            ::= type init_declarator ("," init_declarator)* ";" ;
init_declarator        ::= IDENTIFIER ("=" assignment_expression)? ;
block                  ::= "{" statement* "}" ;
statement              ::= block
                         | declaration
                         | expression_statement
                         | selection_statement
                         | iteration_statement
                         | jump_statement ;
selection_statement    ::= "eger" "(" expression ")" statement
                         ( "degilse" statement )? ;
iteration_statement    ::= "iken" "(" expression ")" statement
                         | "icin" "(" (declaration | expression_statement)? 
                                   expression_statement? expression? ")"
                                   statement ;
jump_statement         ::= "dondur" expression? ";" ;
expression             ::= assignment_expression ;
assignment_expression  ::= unary_expression "=" assignment_expression
                         | binary_expression ;
binary_expression      ::= ...  // +, -, *, /, %, karşılaştırma operatörleri
```
Tam ayrıntılar için `parser.y` ve `spesifikasyon.md` dosyalarına bakabilirsiniz.

## AST Düğümleri
- `PROGRAM` – Programın kök düğümü.
- `FUNCTION` – Fonksiyon tanımı (`text`: fonksiyon adı, `data_type`: dönüş tipi).
- `PARAM_LIST` / `PARAM` – Parametre listesi ve tekil parametreler.
- `BLOCK` – Süslü parantez içindeki deyim listesi.
- `VAR_DECL` – Değişken bildirimi (`data_type`: tür, çocuk: başlangıç ifadesi).
- `EXPR_STATEMENT` – Ifade deyimi (`;` ile biten).
- `IF`, `IF_ELSE`, `WHILE`, `FOR` – Kontrol yapıları.
- `RETURN` – `dondur` deyimi.
- `ASSIGNMENT`, `BINARY_EXPR`, `UNARY_EXPR` – İfade düğümleri.
- `FUNCTION_CALL` ve `ARGUMENT_LIST` – Fonksiyon çağrıları.
- `IDENTIFIER`, `NUMBER_LITERAL`, `STRING_LITERAL` – Birincil ifadeler.
- `EMPTY` – `for` başlıklarındaki isteğe bağlı kısımlar için yer tutucu.
`ast_print` fonksiyonu bu düğümlerin isimlerini ve varsa ek bilgilerini girintili biçimde gösterir; `ast_free` tüm ağacı serbest bırakır.

## Test Dosyaları
| Dosya                            | Tür     | Açıklama                                       |
|----------------------------------|---------|------------------------------------------------|
| `test1.tc`                       | Geçerli | Basit fonksiyon ve `dondur` deyimi.            |
| `test2.tc`                       | Geçerli | Koşul, `for` döngüsü ve yorum örnekleri.       |
| `test3.tc`                       | Geçerli | Fonksiyon çağrısı, toplama/karşılaştırma.      |
| `test_invalid_missing_semicolon.tc` | Hatalı | Satır sonu `;` eksik; parser hata üretir.      |

Her test dosyasını `turkc` aracı ile çalıştırarak AST çıktısını görebilir veya hatalı örnekle hata mesajını doğrulayabilirsiniz.

---

## English Summary

### Overview
TurkC is a tiny C-like language with Turkish keywords. This project combines a Flex lexer with a Bison parser that builds an Abstract Syntax Tree. Valid programs produce an indented AST dump; invalid syntax is rejected with a clear error.

### Build & Run
```
make                 # or run bison/flex/gcc manually, see above
./turkc test3.tc     # prints the AST to stdout
```
Dependencies: Flex, Bison, GCC, optionally Make.

### Grammar & AST
- Grammar covers function definitions, declarations, control flow (`eger/degilse`, `iken`, `icin`), arithmetic and comparison expressions, and return statements.
- AST nodes mirror language constructs (`FUNCTION`, `VAR_DECL`, `FOR`, `ASSIGNMENT`, `BINARY_EXPR`, etc.). Helper routines in `ast.c` manage allocation, printing, and cleanup.

### Tests
- `test1.tc`, `test2.tc`, `test3.tc` exercise declarations, control flow, and calls.
- `test_invalid_missing_semicolon.tc` demonstrates parser error handling.

For detailed specification, refer to `spesifikasyon.md`, `scanner.l`, and `parser.y`.
## Dosyalar
- `spesifikasyon.md`: Dil tanımı.
- `scanner.l`: Flex kaynak kodu.
- `lex.yy.c`: Üretilen C kodu.
- `test1.tc`, `test2.tc`: Test kaynak dosyaları.
- `scanner`: Derlenmiş executable.
