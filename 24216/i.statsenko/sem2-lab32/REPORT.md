# Лаба 32 — Многопоточный кэширующий прокси

## Общая идея

Отличие от лабы 31 одно: вместо одного цикла `select` для всех соединений —
каждое соединение получает свою горутину. Горутина использует **блокирующий**
I/O (обычные `Read`/`Write` без `select`). Пока горутина ждёт данных от
источника, другие горутины продолжают работать.

В Go горутины — это зелёные потоки (не OS-потоки). Планировщик Go сам
переключается между ними. Создание горутины дёшево (~2 KB стека), можно
держать тысячи одновременно.

Кэш разделяется между всеми горутинами, поэтому там `sync.RWMutex`.

---

## proxy.go

### Тип `Proxy`

```go
type Proxy struct {
    listenFd int
    cache    *Cache
}
```

Нет поля `conns` — каждая горутина сама хранит свой `clientFd` в стеке.

### `NewProxy(port int) (*Proxy, error)`

Идентично лабе 31: `Socket` → `SetsockoptInt(SO_REUSEADDR)` → `Bind` →
`Listen`. Описание каждой функции см. в REPORT.md лабы 31.

### `Run() error`

```go
for {
    fd, _, err := unix.Accept(p.listenFd)
    if err != nil {
        if err == unix.EINTR { continue }
        return err
    }
    go p.handleConn(fd)
}
```

**`unix.Accept(p.listenFd)`** — блокирующий вызов (в отличие от лабы 31, где
мы ждём через `select`). Главная горутина здесь просто спит пока не придёт
новый клиент, потом немедленно запускает горутину и снова блокируется в
`Accept`. Это значит: принятие новых соединений никогда не ждёт завершения
обработки старых.

**`unix.EINTR`** — сигнал прервал системный вызов. Не ошибка, просто
повторяем `Accept`.

**`go p.handleConn(fd)`** — запускает горутину. `fd` копируется в аргумент,
поэтому каждая горутина работает со своим дескриптором.

---

## handler.go

### `handleConn(clientFd int)`

Точка входа каждой горутины. Обрабатывает ровно одно клиентское соединение
от начала до конца, потом горутина завершается.

```go
defer func() {
    unix.Shutdown(clientFd, unix.SHUT_RDWR)
    unix.Close(clientFd)
}()
```

`defer` гарантирует закрытие `clientFd` при любом выходе — даже при `return`
в середине функции. Так не бывает утечек дескрипторов.

**Шаг 1. Читаем запрос:**
```go
reqBuf, err := readUntilDoubleCRLF(clientFd)
```

**Шаг 2. Парсим:**
```go
host, port, path, cacheKey, err := parseRequest(string(reqBuf))
```

**Шаг 3. Кэш:**
```go
if data, ok := p.cache.Get(cacheKey); ok {
    writeAll(clientFd, data)
    return
}
```
Если URL в кэше — отдаём сразу и выходим. `Get` потокобезопасен.

**Шаг 4. Коннект к источнику:**
```go
originFd, err := dialTCP(host, port)
defer func() {
    unix.Shutdown(originFd, unix.SHUT_RDWR)
    unix.Close(originFd)
}()
```

**Шаг 5. Отправляем запрос источнику:**
```go
req := fmt.Sprintf("GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", path, host)
writeAll(originFd, []byte(req))
```

**Шаг 6. Читаем ответ и одновременно пишем клиенту:**
```go
for {
    n, err := unix.Read(originFd, tmp[:])
    if n > 0 {
        respBuf = append(respBuf, tmp[:n]...)
        writeAll(clientFd, tmp[:n])
    }
    if err != nil || n == 0 { break }
}
```

Отличие от лабы 31: здесь мы пишем клиенту **сразу по мере получения** данных
от источника, не дожидаясь полного ответа. В лабе 31 мы тоже это делаем, но
через `select`. Здесь `Read` блокирующий — горутина просто ждёт.

`tmp[:n]` — срез от начала до `n`. Дальше `tmp[n:]` — мусор от предыдущего
чтения, его игнорируем.

**Шаг 7. Кэшируем:**
```go
p.cache.Set(cacheKey, respBuf)
```

После выхода из цикла `respBuf` содержит полный ответ.

---

### `readUntilDoubleCRLF(fd int) ([]byte, error)`

```go
var buf []byte
tmp := make([]byte, 4096)
for {
    n, err := unix.Read(fd, tmp)
    if n > 0 {
        buf = append(buf, tmp[:n]...)
        if findDoubleCRLF(buf) >= 0 {
            return buf, nil
        }
    }
    if err != nil || n == 0 {
        return nil, fmt.Errorf("connection closed before complete request")
    }
}
```

Читает байты в цикле, накапливает в `buf`, пока не найдёт `\r\n\r\n`.
В лабе 31 это же делалось через `handleReading` + состояние `reqBuf`, но
там нельзя было блокироваться. Здесь горутина блокируется в `Read` сколько
нужно — это нормально.

---

### `dialTCP(host string, port int) (int, error)`

```go
ips, err := net.LookupIP(host)
fd, err := unix.Socket(unix.AF_INET, unix.SOCK_STREAM, 0)
addr := &unix.SockaddrInet4{Port: port}
copy(addr.Addr[:], ips[0].To4())
err = unix.Connect(fd, addr)
```

**`net.LookupIP(host)`** — DNS. Здесь блокирует только свою горутину, другие
горутины продолжают работать. В лабе 31 блокировал весь прокси.

**`unix.Connect(fd, addr)`** — на **блокирующем** сокете (SetNonblock не
вызываем!) блокируется до установки TCP-соединения. Снова — только эта
горутина ждёт, остальные работают.

**`ips[0].To4()`** — преобразует IP в 4-байтовый формат IPv4. DNS может
вернуть несколько адресов, берём первый.

**`copy(addr.Addr[:], ...)`** — копируем IP-адрес в массив `[4]byte` структуры
`SockaddrInet4`.

---

### `writeAll(fd int, data []byte) error`

```go
for len(data) > 0 {
    n, err := unix.Write(fd, data)
    if err != nil { return err }
    data = data[n:]
}
```

`unix.Write` может записать меньше байт чем просили (partial write). Это
происходит когда буфер сокета полон. Цикл повторяет запись с того места где
остановился (`data = data[n:]`), пока не запишет всё.

В лабе 31 `Write` в `handleForwarding` и `handleSending` тоже может писать
частично — там `sendOff` играет роль курсора между итерациями `select`.

---

### `parseRequest` и `findDoubleCRLF`

Идентичны лабе 31, описание там же.

---

## Жизненный цикл одного запроса

```
Клиент подключается
       ↓
Accept() → go handleConn(fd)  ← главная горутина уже ждёт следующего
       ↓
[новая горутина]
readUntilDoubleCRLF(clientFd)  ← блокирует только эту горутину
       ↓
parseRequest()
       ↓
cache.Get()
   попадание → writeAll(clientFd, cached) → Shutdown+Close → конец горутины
   промах ↓
dialTCP(host, port)            ← блокируется в Connect, остальные работают
       ↓
writeAll(originFd, "GET ...")
       ↓
цикл: Read(originFd) → append(respBuf) → writeAll(clientFd)
       ↓
cache.Set(cacheKey, respBuf)
       ↓
defer: Shutdown+Close clientFd и originFd → конец горутины
```

## Отличие от лабы 31

| | Лаба 31 | Лаба 32 |
|---|---|---|
| Потоки | 1 | N (по одной горутине на соединение) |
| I/O | Неблокирующий + select | Блокирующий |
| DNS блокирует | Весь прокси | Только свою горутину |
| Кэш | Без защиты (1 поток) | sync.RWMutex |
| Память на соединение | ~300 байт (proxyConn) | ~2KB (стек горутины) + данные |
