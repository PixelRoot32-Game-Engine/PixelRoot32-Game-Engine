# LogLevel

<Badge type="info" text="Enum" />

**Source:** `Log.h`

## Description

Enumeration of log levels.

## Methods

### `const char* levelToString(LogLevel level)`

**Description:**

Converts a log level to its string representation.

**Parameters:**

- `level`: The log level to convert.

**Returns:** A string representation of the log level.

### `void platformPrint(const char* text)`

**Description:**

Prints text to the platform-specific output.

**Parameters:**

- `text`: The text to print.

### `void logInternal(LogLevel level, const char* fmt, va_list args)`

**Description:**

Internal logging function that handles formatted message output.

**Parameters:**

- `level`: The log level for the message.
- `fmt`: The format string (printf-style).
- `args`: The va_list of arguments for the format string.

### `inline void log(LogLevel level, const char* fmt, ...)`

**Description:**

Logs a message with the specified level.

**Parameters:**

- `level`: The log level.
- `fmt`: The format string (printf-style).

### `inline void log(const char* fmt, ...)`

**Description:**

Logs a message with the default Info level.

**Parameters:**

- `fmt`: The format string (printf-style).
