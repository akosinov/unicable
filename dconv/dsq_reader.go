package main

import (
	"fmt"
	"io"
)

type DsqReader struct {
	r	io.Reader
	cnt	int
	prev	byte
	st	int
}

const (
	st_first	= 0
	st_second	= 1
	st_digit	= 2
	st_skip		= 3
)

func (self *DsqReader) get() (data byte, err error) {
	for {
		if _, err = fmt.Fscanf(self.r, "%02X", &data); err != nil {
			if err.Error() != "unexpected newline" {
				break;
			}
			err = nil
			self.st = st_first
		} else {
			if self.st == st_digit {
				break
			} else if self.st == st_first {
				if data == 0x7F {
					self.st = st_second
				} else {
					self.st = st_skip
				}
			} else if self.st == st_second {
				if data == 0x10 {
					self.st = st_digit
				} else {
					self.st = st_skip
				}
			}
		}
	}
	return
}

func (self *DsqReader) Read(p []byte) (n int, err error) {
	var data byte
	for ; n < len(p) ; {
		if self.cnt > 0 {
			p[n] = self.prev
			n++
			self.cnt--
		} else if data, err = self.get(); err != nil {
			break;
		} else if self.cnt == -1 {
			self.cnt = (int)(data)
		} else {
			p[n] = data
			n++
			if (self.cnt == -2) && (data == self.prev) {
				self.cnt = -1
			} else {
				self.cnt = -2
				self.prev = data
			}
		}
	}
	return
}

func NewDsqReader(r io.Reader) *DsqReader {
	return &DsqReader{r: r}
}
