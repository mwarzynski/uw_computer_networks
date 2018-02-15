# Treść

Należy przygotować parę programów – klient i serwer – służącą do komunikacji
pomiędzy komputerami na następujących zasadach:

1. Klient to program, który po uruchomieniu wysyła po UDP do serwera znacznik czasu oraz literę, a następnie w nieskończonej pętli oczekuje
na datagramy od serwera i wypisuje ich treść na standardowe wyjście.
2. Serwer zaś, gdy tylko otrzyma poprawny datagram, to dopisuje do jego treści zawartość wskazanego przy uruchomieniu pliku i tak uzupełniony datagram rozsyła do pozostałych klientów, którzy przesłali coś do serwera w czasie ostatnich 2 minut.


## Wywołanie programu

Serwer uruchamiamy poleceniem:

`./server port nazwa_pliku`

Parametry:

- `port`        – numer portu, na którym serwer ma odbierać dane od  klientów (liczba dziesiętna)
- `nazwa_pliku` – nazwa pliku, którego treść należy doklejać


Klienta uruchamiamy poleceniem:

`./client timestamp c host [port]`

Parametry:

- `timestamp` – znacznik czasu wysyłany w pakiecie
- `c`         – znak wysyłany w pakiecie
- `host`      – nazwa serwera, z którym należy się połączyć
- `port`      – opcjonalny numer portu serwera, z którym należy się połączyć (liczba dziesiętna); jeśli nie podano argumentu, jako numer portu powinna być przyjęta liczba 20160


## Protokół

Komunikacja pomiędzy klientem i serwerem odbywa się po UDP. Klient po uruchomieniu powinien wysłać datagram do serwera, a następnie w nieskończonej pętli czekać na datagramy zwrotne.

Komunikaty przesyłane pomiędzy serwerem i klientem powinny być następującej postaci:

1. Liczba 64-bitowa bez znaku, podana w sieciowej kolejności bajtów, interpretowana jako znacznik czasu, taki, jak zwracany przez time(), tzn. liczba sekund od godziny 0.00 1 stycznia 1970 UTC.
2. Jeden znak (1 bajt).
3. W przypadku datagramów od serwera do klienta – tekst wczytany z podanego pliku. Tekst kończy się znakiem NULL. Można założyć, że w pliku nie znajduje się znak NULL i plik nie zmienia się w czasie wykonania programu.

Jeżeli rok zawarty w znaczniku czasu jest mniejszy niż 1717 lub większy niż 4242, to taki znacznik czasu uznajemy za niepoprawny.


## Wymagania szczegółowe

Programy powinny być odporne na podanie niepoprawnych parametrów lub złej liczby parametrów. W takim przypadku należy wypisać stosowny komunikat o błędzie i zakończyć program z kodem powrotu 1. Programy nie mogą wysyłać nieprawidłowych danych.

Programy powinny kompilować się i działać zarówno na maszynach 64-bitowych, jak i 32-bitowych.

W przypadku otrzymania nieprawidłowych (złośliwych) danych przysłanych przez sieć zarówno klient, jak i serwer powinny wypisać zrozumiały dla człowieka komunikat na standardowe wyjście błędów, zawierający także adres nadawcy błędnego datagramu, i działać dalej.

W serwerze można przyjąć ograniczenie 42 na liczbę jednocześnie obsługiwanych klientów.

W kliencie na standardowe wyjście należy wypisywać tylko komunikaty otrzymane od serwera, bez żadnych dodatkowych informacji ani dodatkowych znaków końca linii.

Rozwiązanie powinno:
- działać w środowisku Linux w laboratorium komputerowym
- być napisane w języku C lub C++ z wykorzystaniem interfejsu gniazd oraz, w serwerze, mechanizmu poll lub select (nie wolno tworzyć wielu wątków ani  procesów, jak również korzystać z libevent ani boost::asio)
- kompilować się za pomocą GCC (polecenie gcc lub g++), wśród parametrów należy użyć -Wall -O2

Wysyłanie datagramów nie powinno blokować ich odbierania. Jeżeli datagramy nadsyłane są szybciej, niż udaje się wysyłać odpowiedzi, to powinny być umieszczane w buforze o rozmiarze 4096 datagramów, a jeżeli zabraknie miejsca w buforze, to należy nadpisać najstarszy datagram. Datagramy do każdego z klientów należy zawsze wysyłać w kolejności zgodnej z tą, w jakiej były odebrane przez serwer.

Ponieważ rozwiązanie będzie automatycznie testowane, należy ściśle przestrzegać podanej specyfikacji (nazwy plików, protokół komunikacji, komunikaty na standardowym wyjściu, kody powrotu).

