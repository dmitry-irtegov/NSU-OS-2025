package workerpool

import cache "proxy/Cache"

var MAX_CLIENTS_ONE_WORKER = 1024

type WorkerPool struct {
	tasks   chan Task
	max     int
	workers []Task
	cache   *cache.Cache
}

func NewWorkerPool(tasks chan Task, max int, cache *cache.Cache) *WorkerPool {
	return &WorkerPool{
		tasks: tasks,
		max:   max,
		cache: cache,
	}
}

func (wp *WorkerPool) Start() {
	for i := 0; i < wp.max; i++ {
		go NewWorker(i, MAX_CLIENTS_ONE_WORKER, wp.cache).Start(wp.tasks)
	}
}
