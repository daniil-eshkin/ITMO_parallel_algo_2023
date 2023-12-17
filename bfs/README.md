Используется библиотека https://cmuparlay.github.io/parlaylib/

Число потоков ставится через переменную окружения ```PARLAY_NUM_THREADS```  
Для бенчмарка использовалось ```PARLAY_NUM_THREADS=4```

```
Sequential BFS started
Sequential BFS finished. Time elapsed: 32.9244
Parallel BFS 1 started
Parallel BFS 1 finished. Time elapsed: 12.5109
Parallel BFS 2 started
Parallel BFS 2 finished. Time elapsed: 12.6161
Parallel BFS 3 started
Parallel BFS 3 finished. Time elapsed: 12.6106
Parallel BFS 4 started
Parallel BFS 4 finished. Time elapsed: 12.5966
Parallel BFS 5 started
Parallel BFS 5 finished. Time elapsed: 12.5764
Parallel BFS less allocations warmup started
Parallel BFS less allocations warmup finished. Time elapsed: 13.7714
Parallel BFS less allocations 1 started
Parallel BFS less allocations 1 finished. Time elapsed: 12.8984
Parallel BFS less allocations 2 started
Parallel BFS less allocations 2 finished. Time elapsed: 12.9333
Parallel BFS less allocations 3 started
Parallel BFS less allocations 3 finished. Time elapsed: 12.8913
Parallel BFS less allocations 4 started
Parallel BFS less allocations 4 finished. Time elapsed: 13.4858
Parallel BFS less allocations 5 started
Parallel BFS less allocations 5 finished. Time elapsed: 13.2355
Average parallel BFS time: 12.5821
Seq / Par time ratio: 2.61677
Average parallel BFS less allocations time: 13.0889
Seq / ParLA time ratio: 2.51546
```

Тут рассмотрены две реализации:
- `Parallel BFS` - обычный non-deterministic BFS.
- `Parallel BFS less allocations` - тот же алгоритм, 
но почти вся память статическая (аллоцируется при первом запуске, 
а потому в бенчмарке присутсвует прогрев). Без `-O3` флага работает быстрее, 
но там в лучшем случае достигалось отношение `2.16164`.