package main

import (
	"fmt"
	"io"
)

const (
	state_first	= 0
	state_next	= 1
	state_skip	= 2
)

type DmpReader struct {
	r	io.Reader
	state	int
}

func (self *DmpReader) Read(p []byte) (n int, err error) {
	for ; n < len(p) ; {
		var data byte
		if _, err = fmt.Fscanf(self.r, "%02X", &data); err != nil {
			if err.Error() != "unexpected newline" {
				break;
			}
			err = nil
			self.state = state_first
		} else  {
			if self.state == state_first {
				if data == 0x74 {
					self.state = state_next
				} else {
					self.state = state_skip
				}
			} else if self.state == state_next {
			        p[n] = data
				n++
			}
		}
	}
	return
}

func NewDmpReader(r io.Reader) *DmpReader {
	return &DmpReader{r: r}
}
