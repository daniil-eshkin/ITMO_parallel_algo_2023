Используется библиотека https://cmuparlay.github.io/parlaylib/

Число потоков ставится через переменную окружения ```PARLAY_NUM_THREADS```  
Для бенчмарка использовалось ```PARLAY_NUM_THREADS=4```

```
Sequential sort started
Sequential sort finished. Time elapsed: 911.065
Parallel sort 1 started
Parallel sort 1 finished. Time elapsed: 359.25
Parallel sort 2 started
Parallel sort 2 finished. Time elapsed: 361.695
Parallel sort 3 started
Parallel sort 3 finished. Time elapsed: 360.166
Parallel sort 4 started
Parallel sort 4 finished. Time elapsed: 357.51
Parallel sort 5 started
Parallel sort 5 finished. Time elapsed: 377.078
Average parallel sort time: 363.14
Seq / Par time ratio: 2.50886
```

2.5 - это не очень три, и это можно решить подгонкой размера блока, но один запуск бенча гоняется минут 40, так что посчитаем, что это примерно три.

Upd: поставил `-O3` флаг
```
Sequential sort started
Sequential sort finished. Time elapsed: 347.269
Parallel sort 1 started
Parallel sort 1 finished. Time elapsed: 93.2009
Parallel sort 2 started
Parallel sort 2 finished. Time elapsed: 93.3671
Parallel sort 3 started
Parallel sort 3 finished. Time elapsed: 93.0966
Parallel sort 4 started
Parallel sort 4 finished. Time elapsed: 93.0011
Parallel sort 5 started
Parallel sort 5 finished. Time elapsed: 92.8907
Average parallel sort time: 93.1113
Seq / Par time ratio: 3.72962
```