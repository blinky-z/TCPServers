# TCP Серверы

## Вступление
Различные имплементации blocking и non-blocking i/o TCP серверов

## Полезные bash команды для дебага сервера

### ```ps aux | fgrep <pattern>```
Данная команда ищет все процессы по указанному паттерну. Полезна при необходимости узнать id процесса сервера

Пример использования:
Скомпилируем наш исходный файл с сервером и запустим его
```bash
g++ StartNonBlockingServer.cpp -O2 -o StartServer
./StartServer
```

Найдем id процесса сервера:
```bash
ps aux | fgrep StartServer
```

Output:
```
root    12608  0.0  0.0  13996  1832 pts/1    S+   23:45   0:00 ./StartServer
root    12610  0.0  0.0  21536  1000 pts/0    S+   23:45   0:00 grep -F --color=auto StartServer
```

Нас интересует первая строчка, откуда мы узнаем процесс нашего сервера - *12608*. Он понадобится нам далее

###  ```lsof -p <process_id>```
Данная команда выводит список всех открытых процессом файлов. В случае сервера это позволит нам увидеть открытые файловые
дескрипторы сокетов.

Пример использования:
```lsof -p 12608```

Output:
```
COMMAND     PID  USER   FD   TYPE   DEVICE SIZE/OFF    NODE NAME
StartServ 12608 root  cwd    DIR    259,6     4096  927591 /home/root/CLionProjects/SocketLearn
StartServ 12608 root  rtd    DIR    259,6     4096       2 /
StartServ 12608 root  txt    REG    259,6    19416  925481 /home/root/CLionProjects/SocketLearn/StartServer
StartServ 12608 root  mem    REG    259,6  1700792  136859 /lib/x86_64-linux-gnu/libm-2.27.so
StartServ 12608 root  mem    REG    259,6  2030544  136796 /lib/x86_64-linux-gnu/libc-2.27.so
StartServ 12608 root  mem    REG    259,6    96616  131157 /lib/x86_64-linux-gnu/libgcc_s.so.1
StartServ 12608 root  mem    REG    259,6  1594832 1179779 /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.25
StartServ 12608 root  mem    REG    259,6   170960  136768 /lib/x86_64-linux-gnu/ld-2.27.so
StartServ 12608 root    0u   CHR    136,1      0t0       4 /dev/pts/1
StartServ 12608 root    1u   CHR    136,1      0t0       4 /dev/pts/1
StartServ 12608 root    2u   CHR    136,1      0t0       4 /dev/pts/1
StartServ 12608 root    3u  IPv4 77107935      0t0     TCP *:http-alt (LISTEN) <- Наш открытый fd мастер сокета
```
### ```sudo strace -p <process_id>```
Данная команда позволяет отслеживать все системные вызовы процесса
Это позволит нам наблюдать за тем, что происходит внутри системы и какие происходят системные вызовы к ядру.

Рассмотрим несколько примеров (на примере *select* сервера):

  * Принятие нового соединения:
```
accept(3, NULL, NULL)                   = 4
fcntl(4, F_GETFL)                       = 0x2 (flags O_RDWR)
fcntl(4, F_SETFL, O_RDWR|O_NONBLOCK)    = 0
select(5, [3 4], NULL, NULL, NULL
```

Мы видим, что новое соединение было принято и сервер снова повис на select в бесконечном таймауте, ожидая новых ивентов

  * Принятие сообщения от клиента:
```
recvfrom(4, "Hello, Select Server!\r\n", 1023, MSG_NOSIGNAL, NULL, NULL) = 23
sendto(4, "Hello, Select Server!\r\n", 23, MSG_NOSIGNAL, NULL, 0) = 23
select(5, [3 4], NULL, NULL, NULL
```

Мы видим системные вызовы recvfrom и sendto на файловом дескрипторе с номером **4**, где было принято сообщение размером 23 и отображено такое же, далее сервер снова повис
в таймауте

  * Отключение клиента от сервера:
```
recvfrom(4, "", 1023, MSG_NOSIGNAL, NULL, NULL) = 0
shutdown(4, SHUT_RDWR)                  = 0
close(4)                                = 0
```

Мы видим, что на сервер пришло пустое сообщение, а значит клиент отключился, соответственно мы на стороне сервера закрываем данный сокет, что и видно по системным вызовам