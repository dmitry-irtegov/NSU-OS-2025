package request

func NewRequestMeta(
	method,
	resourse,
	host string,
	port int,
) *RequestMeta {
	return &RequestMeta{
		method:   method,
		resource: resourse,
		host:     host,
		port:     port,
	}
}
