package worker

func (w *Worker) Run() {
	w.normalLogger.Printf("worker [%d] start working\n", w.id)

	for {
		w.step()
	}
}
