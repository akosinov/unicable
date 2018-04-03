package main

import (
	"fmt"
	"io"
)

type HexReader struct {
	r	io.Reader
}

func (self *HexReader) Read(p []byte) (n int, err error) {
	for ; n < len(p) ; {
		var data byte
		if _, err = fmt.Fscanf(self.r, "%02X", &data); err != nil {
			if err.Error() != "unexpected newline" {
				break;
			}
			err = nil
		} else  {
		        p[n] = data
			n++
		}
	}
	return
}

func NewHexReader(r io.Reader) *HexReader {
	return &HexReader{r: r}
}
