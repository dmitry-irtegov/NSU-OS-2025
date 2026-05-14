package request

func (r *RequestMeta) GetMethod() string {
	return r.method
}

func (r *RequestMeta) GetResource() string {
	return r.resource
}

func (r *RequestMeta) GetHost() string {
	return r.host
}

func (r *RequestMeta) GetPort() int {
	return r.port
}
