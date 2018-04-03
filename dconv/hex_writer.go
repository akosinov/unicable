package main

import (
	"fmt"
	"io"
)

type HexWriter struct {
	w		io.Writer
	maxLen		int
	outCount	int
}

func (self *HexWriter) Write(p []byte) (n int, err error) {
	for _, data := range p {
		if self.outCount == 0  {
			_, err = fmt.Fprintf(self.w, "%02X", data)
			self.outCount++
		} else if self.outCount < self.maxLen - 1 {
			_, err = fmt.Fprintf(self.w, " %02X", data)
			self.outCount++
		} else {
			_, err = fmt.Fprintf(self.w, " %02X\n", data)
			self.outCount = 0
		}
		if err != nil {
			break
		}
		n++
	}
	return
}

func (self *HexWriter) Close() (err error) {
	if (self.outCount > 0) && (self.outCount < self.maxLen)  {
		_, err = fmt.Fprintf(self.w, "\n")
	}
	return
}

func NewHexWriter(w io.Writer, maxLen int) (*HexWriter, error) {
	if (maxLen < 3) || (maxLen > 32) {
		return nil, fmt.Errorf("Invalid hex line length : %d", maxLen)
	}
	return &HexWriter{w: w, maxLen: maxLen}, nil
}
