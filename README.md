# Проектирование вычислительных систем

Лабораторные работы, вариант 3. Стенд SDK-1.1М, микроконтроллер STM32F427VIT6, среда STM32CubeIDE 1.9.0.

## Лабораторные

| № | Работа | Описание | Код |
|---|---|---|---|
| 1 | GPIO, азбука Морзе | [Описание](https://github.com/tsostanov/design_of_computing_systems/blob/e7a9e4813b846f6feda4ed85eb60c893eca59def/README.md) | [Версия ЛР №1](https://github.com/tsostanov/design_of_computing_systems/tree/e7a9e4813b846f6feda4ed85eb60c893eca59def) |
| 2 | UART, азбука Морзе | [Запуск](docs/LAB2.md), [проверки](docs/TESTING.md), [защита](docs/DEFENSE.md) | [Версия ЛР №2](https://github.com/tsostanov/design_of_computing_systems/tree/605aa4c788a8af6bcc645205de5498fe3e30cacf) |
| 3 | I2C, электронный уровень | [Файлы и материалы для отчёта](docs/LAB3.md) | [Прошивка](Core/Src/level_app.c), [клиент](client) |

В `main` находится ЛР №3. Она считывает ускорение по трём осям, рисует горизонталь или стрелку на OLED и передаёт данные программе на ПК.

## Запуск ЛР №3

1. Откройте проект `test_f427` в STM32CubeIDE.
2. Выполните Refresh, затем Build Project.
3. Загрузите прошивку через конфигурацию `test_f427 Debug` и нажмите Resume, если отладчик остановился.
4. Запустите клиент из папки проекта:

```powershell
python -m pip install -r client/requirements.txt
python -m client COM5
```

Вместо `COM5` укажите порт стенда. Нужен Python с Tkinter. Для просмотра интерфейса без стенда: `python -m client --demo`.

По умолчанию в `Core/Inc/lab_select.h` выбран `LAB_NUMBER=3`. Для сборки ЛР №2 задайте значение `2`.

## Проверки

```powershell
python tests/run_level_tests.py
python tests/run_tests.py
```

Для C-тестов нужен GCC. Что проверяется и какие результаты добавить в отчёт, описано в [документации ЛР №3](docs/LAB3.md).
