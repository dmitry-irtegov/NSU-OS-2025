package client

type ClientState int

const (
	Read ClientState = iota
	SendRequestToUpstream
	SendResponseFromUpstream
	SendResponseFromCache
)
