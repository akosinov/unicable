package main

import (
	"flag"
	"fmt"
	"os"
	"io"
)

func showerr(err error) {
	fmt.Fprintf(os.Stderr, "Error: %v\n", err)
}

func dclose(c io.Closer) {
	if err := c.Close(); err != nil {
		showerr(err)
	}
}

func run() (n int64, err error) {
	var (
		r	io.Reader
		w	io.Writer
	)

	inTypePtr := flag.String("it", "dmp", "Input file type.  dsq | bin | dmp | hex")
	outTypePtr := flag.String("ot", "dsq", "Output file name. dsq | bin | hex")
	inNamePtr := flag.String("i", "", "Input file name.  <stdin> if omitted")
	outNamePtr := flag.String("o", "", "Output file name. <stdout> if omitted")
	dsqLenPtr := flag.Int("l", 16, "Maximun length of diseqc command for dsq out")
	bufLenPtr := flag.Int("b", 1, "Copy buffer size")
	flag.Parse()

	inFile := os.Stdin
	if *inNamePtr != "" {
		if inFile, err = os.OpenFile(*inNamePtr, os.O_RDONLY, 0); err != nil {
			return
		}
		defer dclose(inFile)
	}

	outFile := os.Stdout
	if *outNamePtr != "" {
		if outFile, err = os.OpenFile(*outNamePtr, os.O_WRONLY | os.O_CREATE | os.O_TRUNC, 664); err != nil {
			return
		}
		defer dclose(outFile)
	}

	switch(*inTypePtr) {
		case "dsq":
			r = NewDsqReader(inFile)
		case "bin":
			r = inFile
		case "dmp":
			r = NewDmpReader(inFile)
		case "hex":
			r = NewHexReader(inFile)
		default:
			err = fmt.Errorf("Invalid input file type")
	}
	if err != nil {
		return
	}

	switch(*outTypePtr) {
		case "dsq":
			var writer *DsqWriter
			if writer, err = NewDsqWriter(outFile, *dsqLenPtr); err == nil {
				w = writer
				defer dclose(writer)
			}
		case "bin":
			w = outFile
		case "hex":
			var writer *HexWriter
			if writer, err = NewHexWriter(outFile, *dsqLenPtr); err == nil {
				w = writer
				defer dclose(writer)
			}
		default:
			err = fmt.Errorf("Invalid output file type")
	}
	if err != nil {
		return
	}

	if n, err = io.CopyBuffer(w, r, make([]byte, *bufLenPtr)); err != nil {
		return
	}

	return
}

func main() {
	if n, err := run(); err != nil {
		showerr(err)
		os.Exit(1)
	} else if n != 0x1c0 {
		fmt.Fprintf(os.Stderr, "Warning: we nedded 0x01C0 bytes, but has %04X\n", n)
	}
}
