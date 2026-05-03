// +build illumos

package proxy

// #cgo LDFLAGS: -L/usr/lib/amd64 -lxnet -lsocket -lnsl
import "C"
