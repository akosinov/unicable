package main

import (
	"fmt"
	"io"
)

type DsqWriter struct {
	w		io.Writer
	maxLen		int
	outCount	int
	prev		byte
	repCount	int
}

func (self *DsqWriter) out(data byte) (err error) {
	if self.outCount % (self.maxLen - 2) == 0 {
		_, err = fmt.Fprintf(self.w, "\n7F 10 %02X", data)
	} else {
		_, err = fmt.Fprintf(self.w, " %02X", data)
	}
	self.outCount++
	return
}

//CNTIN		0	1	>1
//SAME			CNT++	CNT++
//			OUT	
//DIFFER			CNTOUT
//		CNT=1	CNT=1	CNT=1
//		PRV=CUR	PRV=CUR	PRV=CUR
//		OUT	OUT	OUT

func (self *DsqWriter) put(data byte) error {
	switch {
		case (self.repCount > 0) && (data == self.prev) :
			if self.repCount == 1 {
				self.repCount++
				break
			}
			if self.repCount < 257 {
				self.repCount++
				return nil
			}
			fallthrough
		default:
			if self.repCount > 1 {
				if err := self.out((byte)(self.repCount - 2)); err != nil {
					return err
				}
			}
			self.repCount = 1
			self.prev = data
	}
	return self.out(data)
}

func (self *DsqWriter) Write(p []byte) (n int, err error) {
	for _, data := range p {
		if err = self.put(data); err != nil {
			break
		}
		n++
	}
	return
}

func (self *DsqWriter) Close() (err error) {
	if self.repCount > 1 {
		if err = self.out((byte)(self.repCount - 2)); err != nil {
			return
		}
	}

	_, err = fmt.Fprintf(self.w, "\n7F 20 00 %02X %02X\n", (byte)(self.outCount / 256), (byte)(self.outCount % 256))
	return
}

func NewDsqWriter(w io.Writer, maxLen int) (*DsqWriter, error) {
	if (maxLen < 3) || (maxLen > 32) {
		return nil, fmt.Errorf("Invalid dsq command length : %d", maxLen)
	}

	if _, err := fmt.Fprintf(w, "7F 04 00 00 00"); err != nil {
		return nil, err
	}

	return &DsqWriter{w: w, maxLen: maxLen}, nil
}
