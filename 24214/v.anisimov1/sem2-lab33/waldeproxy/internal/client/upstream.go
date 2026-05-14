package client

type UpstreamConnection struct {
	ParentClientFd int
	Socketfd       int
	Buffer         []byte
	PayloadSize    int
	WroteBytes     int
	IsClosed       bool
}

func (uc *UpstreamConnection) NewResponseChunk(offset int) bool {
	return offset < uc.PayloadSize
}

func (uc *UpstreamConnection) GetNewChunk(offset int) []byte {
	return uc.Buffer[offset:uc.PayloadSize]
}
