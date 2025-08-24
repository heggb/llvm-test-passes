# MCST llvm-test-passes

Этот репозиторий содержит набор реализаций и тестов функциональных LLVM-пассов. Проект ориентирован на исследование промежуточного представления (LLVM IR), а также написание и проверку собственных анализирующих проходов компилятора.

## Состав проекта

- src/ — основной исходный код, содержащий реализации LLVM-пассов и вспомогательные модули.
- include/ — заголовочные файлы для подключения собственных компонентов.
- tests_src/ — исходники тестов, демонстрирующие применение и корректность работы написанных пассов.
- scripts/ — вспомогательные скрипты для сборки, запуска и анализа результатов.
- results/ — директория для сохранения результатов выполнения тестов и оптимизаций.
- build/ — каталог для сборки.

## Требования

Для работы с проектом необходимы:

- [`clang`](https://clang.llvm.org/) — компилятор Clang
- [`opt`](https://llvm.org/docs/CommandGuide/opt.html) — инструмент оптимизации из набора LLVM
- [`cmake`](https://cmake.org/)) — система сборки
- [`ninja`](https://ninja-build.org/) или [`make`] — генератор сборки

### Установка (Linux mint)

```bash
sudo apt update
sudo apt install llvm clang cmake ninja-build
```

## Быстрый старт

1. **Склонируйте репозиторий:**

```bash
cd llvm-project
git clone https://github.com/heggb/llvm-test-passes.git
```

2. **Создайте переменную окружения:**

```bash
export LLVM_DIR=<path to llvm-project>
```

3. **Сгенерируйте файл сборки и соберите:**

```bash
cd llvm-test-passes/build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR .
make
```

4. **Перейдите в llvm-project и получите LLVM IR (например, для test_input.c):**

```bash
cd llvm-project
./build/bin/clang-22 -O0 -S -emit-llvm ./llvm-test-passes/tests_src/test_input.c -o test_input.ll
```


5. **Запустите проход (например, RPOPass для test_input.c):**

```bash
cd llvm-project
./build/bin/opt -load-pass-plugin ./llvm-test-passes/build/libTestPasses.so -passes=RPOPass -disable-output test_input.ll
```

## Тестирование

1. **Перейдите в каталог со скриптами:**

```bash
cd llvm-test-passes/scripts
```

2. **Запустите скрипт теста нужного прохода для нужного файла (например, для RPOPass test_input.c):**

```bash
python3 test_passes.py RPOPass test_input.c
```

3. **Для теста всех проходов на всех файлах запустите скрипт без дополнительных параметров:**

```bash
python3 test_passes.py
```

