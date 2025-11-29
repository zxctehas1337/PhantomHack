# Features Structure

Эта папка содержит модульную структуру для всех функций чита.

## Структура:

```
Features/
├── Offsets.h          # Статические офсеты CS2
├── ESP/               # ESP система
│   ├── ESP.h
│   └── ESP.cpp
├── Movement/          # Функции движения
│   ├── BunnyHop.h
│   └── BunnyHop.cpp
├── Visuals/           # Визуальные эффекты
│   ├── Watermark.h
│   └── Watermark.cpp
├── Aimbot/            # Система прицеливания
│   ├── Aimbot.h
│   └── Aimbot.cpp
└── Misc/              # Разные функции
    ├── Triggerbot.h
    └── Triggerbot.cpp
```

## Преимущества модульной структуры:

1. **Организация кода** - каждая функция в своем модуле
2. **Легкость разработки** - можно работать над отдельными модулями
3. **Переиспользование** - модули можно использовать в других проектах
4. **Отладка** - проще найти и исправить ошибки
5. **Расширяемость** - легко добавлять новые функции

## Как добавить новую функцию:

1. Создать папку для категории (если нужно)
2. Создать .h и .cpp файлы
3. Добавить namespace для функции
4. Подключить в dllmain.cpp
5. Добавить вызов в hkPresent или MainThread

## Пример нового модуля:

```cpp
// Features/Misc/NoFlash.h
#pragma once

namespace NoFlash {
    void RunNoFlash();
}

// Features/Misc/NoFlash.cpp
#include "NoFlash.h"
#include "../Offsets.h"

namespace NoFlash {
    extern uintptr_t g_ClientBase;
    extern namespace cfg {
        extern bool misc_noflash;
        extern float misc_flash_alpha;
    }
    
    void RunNoFlash() {
        if (!cfg::misc_noflash || !g_ClientBase) return;
        
        // Реализация функции
    }
}
```