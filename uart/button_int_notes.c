
/*
button interrupts
syscfg = taktowanie potrzebne tylko w trakcie konfigurowania (przerwań?)

*/

/* 
dma

TCIE = przerwanie na koniec transmisji
ustawiony bit DIR_0 = z pamięci do uartu

inicjowanie wysyłania
M0AR - wrzucamy adres bufora
NDTR - ile bajtów

przy odbieraniu dostaniemy przerwanie dopiero po NDRT bajtów



*/
