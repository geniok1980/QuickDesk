# 🐍 Python Executor Skill

**QuickDesk Skill** для выполнения Python кода на удалённой машине.

## Обзор

Этот skill позволяет AI-агенту выполнять Python код непосредственно на машине пользователя через MCP (Model Context Protocol).

## Возможности

- ✅ Выполнение произвольного Python кода
- ✅ Запуск Python скриптов с аргументами
- ✅ Информация о системе Python
- ✅ Обработка ошибок
- ✅ Таймауты для безопасности

## Установка

### 1. Rust версия (рекомендуется)

```bash
# Установить Rust (если ещё не установлен)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Собрать
cd quickdesk-skill-host
cargo build -p python-executor

# Или для release (оптимизированная версия)
cargo build -p python-executor --release
```

### 2. Альтернатива: Python версия

Если Rust недоступен, используйте Python версию:

```bash
cd /root/QuickDesk
python3 python-executor-test.py
```

## Использование

### Как MCP Tool

Skill загружается автоматически через QuickDesk Skill Host:

```markdown
# SKILL.md
---
name: python-executor
description: Execute Python code on remote machine
---

Когда пользователь просит выполнить Python код, используй python_executor.
```

### Direct использование

```bash
# Запуск MCP сервера
./target/debug/python-executor

# Server ожидает MCP JSON-RPC через stdin/stdout
```

## API

### Инструменты

| Инструмент | Описание |
|------------|----------|
| `get_python_info` | Информация о Python (версия, платформа) |
| `run_python` | Выполнить Python код |
| `run_script` | Запустить .py файл |

### Примеры

```python
# get_python_info
{"tool": "get_python_info", "args": {}}
# Returns: {"python_version": "3.12.3", "platform": "Linux", ...}

# run_python
{"tool": "run_python", "args": {"code": "print(2+2)"}}
# Returns: {"stdout": "4\n", "exit_code": 0}

# run_script
{"tool": "run_script", "args": {"path": "script.py", "args": ["--verbose"]}}
```

## Безопасность

⚠️ **ВНИМАНИЕ**: Выполнение произвольного кода — это риск!

### Рекомендации:

1. **Используйте в изолированных окружениях**
2. **Ограничьте доступ к sensitive файлам**
3. **Таймауты** предотвращают бесконечные циклы
4. **Не выполняйте код от имени root** без необходимости

### Ограничения по умолчанию:

- Таймаут: 30 секунд
- Максимальный вывод: 1MB
- Запрещённые операции: системные файлы, network вредоносного кода

## Разработка

### Структура проекта

```
python-executor/
├── Cargo.toml          # Rust dependencies
├── src/
│   └── main.rs         # MCP сервер
├── SKILL.md           # Skill description
└── README.md          # Этот файл
```

### Тестирование

```bash
# Unit тесты
cargo test -p python-executor

# Интеграционные тесты
python3 ../python-executor-test.py
```

## Лицензия

MIT License (как и QuickDesk)

## Автор

Forked from barry-ran/QuickDesk

Modified by geniok1980
