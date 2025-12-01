package parser

import "errors"

const delim string = " \t|&<>;\n"

var ErrSyntax = errors.New("syntax error")
