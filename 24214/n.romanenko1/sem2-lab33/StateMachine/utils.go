package statemachine

func deleteFd(fd int, allFds *[]int) {
	for i, cfd := range *allFds {
		if cfd == fd {
			*allFds = append((*allFds)[:i], (*allFds)[i+1:]...)
			break
		}
	}
}
